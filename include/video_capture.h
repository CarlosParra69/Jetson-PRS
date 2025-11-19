#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <queue>
#include <opencv2/opencv.hpp>

namespace jetson_lpr {

/**
 * Captura de video RTSP optimizada para tiempo real
 * Soporta cámaras IP/RTSP y procesamiento multihilo
 */
class VideoCapture {
public:
    /**
     * Constructor
     * 
     * @param rtsp_url URL RTSP de la cámara
     * @param buffer_size Tamaño del buffer de frames (default: 2)
     */
    explicit VideoCapture(const std::string& rtsp_url, size_t buffer_size = 2);
    
    /**
     * Destructor
     */
    ~VideoCapture();
    
    /**
     * Iniciar captura de video
     * 
     * @return true si se inició correctamente
     */
    bool start();
    
    /**
     * Detener captura de video
     */
    void stop();
    
    /**
     * Obtener siguiente frame
     * 
     * @param frame Frame de salida
     * @return true si se obtuvo un frame válido
     */
    bool getFrame(cv::Mat& frame);
    
    /**
     * Verificar si la captura está activa
     * 
     * @return true si está activa
     */
    bool isActive() const { return running_; }
    
    /**
     * Obtener FPS de captura
     * 
     * @return FPS actual
     */
    double getFPS() const;
    
    /**
     * Obtener contador de frames
     * 
     * @return Número de frames capturados
     */
    uint64_t getFrameCount() const { return frame_count_; }
    
    /**
     * Configurar tamaño de buffer
     * 
     * @param size Tamaño del buffer
     */
    void setBufferSize(size_t size);
    
    /**
     * Obtener información de la cámara
     * 
     * @return true si se obtuvo información
     */
    bool getCameraInfo(int& width, int& height, double& fps);

private:
    std::string rtsp_url_;
    size_t buffer_size_;
    
    std::atomic<bool> running_;
    std::atomic<bool> started_;
    
    cv::VideoCapture cap_;
    std::mutex cap_mutex_;
    
    std::queue<cv::Mat> frame_queue_;
    std::mutex queue_mutex_;
    
    std::thread capture_thread_;
    
    std::atomic<uint64_t> frame_count_;
    std::atomic<double> fps_;
    
    double last_fps_time_;
    uint64_t last_fps_frame_count_;
    
    /**
     * Hilo de captura
     */
    void captureWorker();
    
    /**
     * Actualizar estadísticas FPS
     */
    void updateFPS();
};

} // namespace jetson_lpr

#endif // VIDEO_CAPTURE_H

