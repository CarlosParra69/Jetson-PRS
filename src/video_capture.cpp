#include "video_capture.h"
#include <iostream>
#include <chrono>
#include <thread>
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
    
    std::cout << "🔍 Intentando conectar a: " << rtsp_url_ << std::endl;
    
    // Intentar múltiples métodos de conexión RTSP
    bool opened = false;
    
    // Método 1: Intentar con CAP_FFMPEG explícitamente
    std::cout << "📡 Método 1: Intentando con CAP_FFMPEG..." << std::endl;
    cap_.open(rtsp_url_, cv::CAP_FFMPEG);
    
    if (cap_.isOpened()) {
        opened = true;
        std::cout << "✅ Conexión exitosa con CAP_FFMPEG" << std::endl;
    } else {
        // Método 2: Intentar sin especificar backend (auto-detección)
        std::cout << "📡 Método 2: Intentando auto-detección de backend..." << std::endl;
        cap_.release();
        cap_.open(rtsp_url_);
        
        if (cap_.isOpened()) {
            opened = true;
            std::cout << "✅ Conexión exitosa con auto-detección" << std::endl;
        } else {
            // Método 3: Intentar con GStreamer si está disponible
            std::cout << "📡 Método 3: Intentando con GStreamer..." << std::endl;
            cap_.release();
            std::string gstreamer_pipeline = "rtspsrc location=" + rtsp_url_ + 
                " latency=0 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! appsink";
            cap_.open(gstreamer_pipeline, cv::CAP_GSTREAMER);
            
            if (cap_.isOpened()) {
                opened = true;
                std::cout << "✅ Conexión exitosa con GStreamer" << std::endl;
            }
        }
    }
    
    if (!opened) {
        std::cerr << "❌ Error: No se pudo abrir la cámara RTSP: " << rtsp_url_ << std::endl;
        std::cerr << "💡 Verificaciones:" << std::endl;
        std::cerr << "   1. Verifica que la URL RTSP es correcta" << std::endl;
        std::cerr << "   2. Verifica que la cámara está encendida y accesible en la red" << std::endl;
        std::cerr << "   3. Prueba la conexión con: ffmpeg -i \"" << rtsp_url_ << "\" -t 5 -f null -" << std::endl;
        std::cerr << "   4. Verifica credenciales (usuario/contraseña) en la URL" << std::endl;
        return false;
    }
    
    // Configurar propiedades de captura
    cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);  // Buffer mínimo para reducir latencia
    
    // No forzar FPS - dejar que la cámara use su FPS nativo
    // cap_.set(cv::CAP_PROP_FPS, 30.0);  // Comentado - puede causar problemas
    
    // NO reducir resolución automáticamente - puede romper la conexión RTSP
    // La resolución se manejará en el procesamiento si es necesario
    
    // Configurar timeout para RTSP (importante para conexiones lentas)
    // Nota: OpenCV puede no soportar todas estas propiedades dependiendo del backend
    try {
        cap_.set(cv::CAP_PROP_OPEN_TIMEOUT_MSEC, 10000);  // 10 segundos timeout
    } catch (...) {
        // Ignorar si no está soportado
    }
    
    // Esperar un momento para que la conexión se estabilice
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Verificar que la captura funcione leyendo un frame de prueba
    std::cout << "🔍 Verificando lectura de frames..." << std::endl;
    cv::Mat test_frame;
    int attempts = 0;
    const int max_attempts = 10;
    
    while (attempts < max_attempts && !cap_.read(test_frame)) {
        attempts++;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        if (attempts % 3 == 0) {
            std::cout << "   Intento " << attempts << "/" << max_attempts << "..." << std::endl;
        }
    }
    
    if (test_frame.empty()) {
        std::cerr << "❌ Error: No se pudo leer frames de la cámara después de " << max_attempts << " intentos" << std::endl;
        std::cerr << "💡 La conexión se estableció pero no se pueden leer frames" << std::endl;
        cap_.release();
        return false;
    }
    
    // Obtener información de la cámara
    int width = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = cap_.get(cv::CAP_PROP_FPS);
    
    std::cout << "✅ Cámara RTSP conectada exitosamente!" << std::endl;
    std::cout << "   Resolución: " << width << "x" << height << std::endl;
    std::cout << "   FPS: " << fps << std::endl;
    std::cout << "   URL: " << rtsp_url_ << std::endl;
    
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
            
            // Limitar tamaño del buffer - descartar frames antiguos si está lleno
            if (frame_queue_.size() >= buffer_size_) {
                // Descartar el frame más antiguo
                frame_queue_.pop();
            }
            
            frame_queue_.push(frame.clone());
        }
        
        // Pequeña pausa para no saturar CPU y permitir que la cámara capture
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

