#include "video_capture.h"
#include <iostream>
#include <chrono>
#include <stdexcept>

namespace jetson_lpr {

VideoCapture::VideoCapture(const std::string& rtsp_url, size_t buffer_size)
    : rtsp_url_(rtsp_url)
    , buffer_size_(buffer_size)
    , running_(false)
    , started_(false)
    , frame_count_(0)
    , fps_(0.0)
    , last_fps_time_(0.0)
    , last_fps_frame_count_(0)
{
}

VideoCapture::~VideoCapture() {
    stop();
}

bool VideoCapture::start() {
    if (started_) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(cap_mutex_);
    
    // Configurar OpenCV para RTSP
    cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);  // Buffer mínimo para reducir latencia
    
    // Abrir captura RTSP
    cap_.open(rtsp_url_, cv::CAP_FFMPEG);
    
    if (!cap_.isOpened()) {
        std::cerr << "Error: No se pudo abrir la cámara RTSP: " << rtsp_url_ << std::endl;
        return false;
    }
    
    // Configurar FPS objetivo (si es posible)
    cap_.set(cv::CAP_PROP_FPS, 25.0);
    
    // Verificar que la captura funcione
    cv::Mat test_frame;
    if (!cap_.read(test_frame)) {
        std::cerr << "Error: No se pudo leer frames de la cámara" << std::endl;
        cap_.release();
        return false;
    }
    
    std::cout << "✅ Cámara RTSP conectada: " << rtsp_url_ << std::endl;
    
    // Iniciar hilo de captura
    running_ = true;
    started_ = true;
    capture_thread_ = std::thread(&VideoCapture::captureWorker, this);
    
    // Inicializar estadísticas FPS
    auto now = std::chrono::steady_clock::now();
    last_fps_time_ = std::chrono::duration<double>(now.time_since_epoch()).count();
    last_fps_frame_count_ = 0;
    
    return true;
}

void VideoCapture::stop() {
    if (!started_) {
        return;
    }
    
    running_ = false;
    
    // Esperar a que termine el hilo de captura
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
    
    // Liberar captura
    std::lock_guard<std::mutex> lock(cap_mutex_);
    if (cap_.isOpened()) {
        cap_.release();
    }
    
    // Limpiar cola de frames
    std::lock_guard<std::mutex> queue_lock(queue_mutex_);
    while (!frame_queue_.empty()) {
        frame_queue_.pop();
    }
    
    started_ = false;
    std::cout << "📹 Captura de video detenida" << std::endl;
}

bool VideoCapture::getFrame(cv::Mat& frame) {
    if (!started_ || !running_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (frame_queue_.empty()) {
        return false;
    }
    
    frame = frame_queue_.front();
    frame_queue_.pop();
    
    return true;
}

double VideoCapture::getFPS() const {
    return fps_.load();
}

void VideoCapture::setBufferSize(size_t size) {
    buffer_size_ = size;
}

bool VideoCapture::getCameraInfo(int& width, int& height, double& fps) {
    std::lock_guard<std::mutex> lock(cap_mutex_);
    
    if (!cap_.isOpened()) {
        return false;
    }
    
    width = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_WIDTH));
    height = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_HEIGHT));
    fps = cap_.get(cv::CAP_PROP_FPS);
    
    return true;
}

void VideoCapture::captureWorker() {
    std::cout << "📹 Iniciando hilo de captura de video..." << std::endl;
    
    while (running_) {
        cv::Mat frame;
        bool frame_read = false;
        
        {
            std::lock_guard<std::mutex> lock(cap_mutex_);
            if (cap_.isOpened()) {
                frame_read = cap_.read(frame);
            }
        }
        
        if (!frame_read || frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // Actualizar estadísticas
        frame_count_++;
        updateFPS();
        
        // Agregar frame a la cola
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            
            // Limitar tamaño del buffer
            while (frame_queue_.size() >= buffer_size_) {
                frame_queue_.pop();
            }
            
            frame_queue_.push(frame.clone());
        }
        
        // Pequeña pausa para no saturar CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::cout << "📹 Hilo de captura terminado" << std::endl;
}

void VideoCapture::updateFPS() {
    auto now = std::chrono::steady_clock::now();
    double current_time = std::chrono::duration<double>(now.time_since_epoch()).count();
    
    // Actualizar FPS cada segundo
    if (current_time - last_fps_time_ >= 1.0) {
        uint64_t current_frame_count = frame_count_.load();
        uint64_t frames_diff = current_frame_count - last_fps_frame_count_;
        double time_diff = current_time - last_fps_time_;
        
        if (time_diff > 0) {
            double calculated_fps = frames_diff / time_diff;
            fps_.store(calculated_fps);
        }
        
        last_fps_time_ = current_time;
        last_fps_frame_count_ = current_frame_count;
    }
}

} // namespace jetson_lpr

