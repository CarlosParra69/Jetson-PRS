#ifndef LPR_SYSTEM_H
#define LPR_SYSTEM_H

#include "config_manager.h"
#include "video_capture.h"
#include "detector.h"
#include "ocr_processor.h"
#include "plate_validator.h"
#include "database_manager.h"

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <opencv2/opencv.hpp>

namespace jetson_lpr {

/**
 * Estructura para resultado de detección completa
 */
struct DetectionResult {
    std::string plate_text;           // Texto de la placa normalizada
    float yolo_confidence;             // Confianza de YOLO
    float ocr_confidence;              // Confianza de OCR
    cv::Rect plate_bbox;               // Bounding box de la placa
    cv::Rect vehicle_bbox;             // Bounding box del vehículo (opcional)
    std::chrono::system_clock::time_point timestamp;  // Timestamp de detección
    
    bool valid;                         // Si la placa es válida (formato colombiano)
    bool authorized;                    // Si el vehículo está autorizado
    
    DetectionResult() 
        : yolo_confidence(0.0f)
        , ocr_confidence(0.0f)
        , valid(false)
        , authorized(false)
    {}
};

/**
 * Sistema principal de reconocimiento de placas en tiempo real
 * Integra todos los componentes: captura, detección, OCR, validación y base de datos
 */
class LPRSystem {
public:
    /**
     * Constructor
     * 
     * @param config_path Ruta al archivo de configuración
     */
    explicit LPRSystem(const std::string& config_path = "config/default_config.json");
    
    /**
     * Destructor
     */
    ~LPRSystem();
    
    /**
     * Inicializar sistema
     * 
     * @return true si se inicializó correctamente
     */
    bool initialize();
    
    /**
     * Iniciar procesamiento en tiempo real
     */
    void start();
    
    /**
     * Detener procesamiento
     */
    void stop();
    
    /**
     * Verificar si está corriendo
     * 
     * @return true si está corriendo
     */
    bool isRunning() const { return running_; }
    
    /**
     * Estructura de estadísticas del sistema
     */
    struct Stats {
        uint64_t total_frames;          // Frames procesados totales
        uint64_t ai_frames;             // Frames procesados con IA
        uint64_t detections_count;      // Número de detecciones
        double capture_fps;             // FPS de captura
        double ai_fps;                  // FPS de procesamiento IA
        double average_latency_ms;      // Latencia promedio (ms)
    };
    
    /**
     * Obtener estadísticas del sistema
     */
    Stats getStats() const;

private:
    ConfigManager config_;
    
    std::unique_ptr<VideoCapture> video_capture_;
    std::unique_ptr<PlateDetector> detector_;
    std::unique_ptr<OCRProcessor> ocr_processor_;
    std::unique_ptr<DatabaseManager> db_manager_;
    
    std::atomic<bool> running_;
    std::atomic<bool> initialized_;
    
    // Threading
    std::thread capture_thread_;
    std::thread processing_thread_;
    
    // Colas para procesamiento
    std::queue<cv::Mat> frame_queue_;
    std::mutex frame_queue_mutex_;
    size_t max_queue_size_;
    
    // Cola de resultados
    std::queue<DetectionResult> result_queue_;
    std::mutex result_queue_mutex_;
    
    // Estadísticas
    mutable std::mutex stats_mutex_;
    Stats stats_;
    
    // Cooldown de detecciones (evitar duplicados)
    std::unordered_map<std::string, std::chrono::system_clock::time_point> detection_cooldown_;
    std::mutex cooldown_mutex_;
    double cooldown_seconds_;
    
    // Contadores
    uint64_t frame_counter_;
    uint64_t ai_frame_counter_;
    uint64_t detection_counter_;
    
    // Tiempo de inicio
    std::chrono::steady_clock::time_point start_time_;
    
    // Visualización
    bool display_enabled_;
    double display_scale_;
    std::string window_name_;
    
    /**
     * Hilo de captura de frames
     */
    void captureThread();
    
    /**
     * Hilo de procesamiento con IA
     */
    void processingThread();
    
    /**
     * Procesar un frame completo
     * 
     * @param frame Frame de entrada
     * @return Resultados de detección
     */
    std::vector<DetectionResult> processFrame(const cv::Mat& frame);
    
    /**
     * Guardar detección en base de datos
     * 
     * @param result Resultado de detección
     */
    void saveDetection(const DetectionResult& result);
    
    /**
     * Verificar cooldown de detección
     * 
     * @param plate_text Texto de la placa
     * @return true si puede procesar (no está en cooldown)
     */
    bool checkCooldown(const std::string& plate_text);
    
    /**
     * Actualizar estadísticas
     */
    void updateStats();
    
    /**
     * Mostrar frame en ventana (si está habilitado)
     * 
     * @param frame Frame a mostrar
     * @param results Resultados de detección para dibujar
     */
    void displayFrame(const cv::Mat& frame, const std::vector<DetectionResult>& results);
};

} // namespace jetson_lpr

#endif // LPR_SYSTEM_H

