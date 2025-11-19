#include "lpr_system.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace jetson_lpr {

LPRSystem::LPRSystem(const std::string& config_path)
    : config_()
    , running_(false)
    , initialized_(false)
    , max_queue_size_(5)
    , cooldown_seconds_(0.5)
    , frame_counter_(0)
    , ai_frame_counter_(0)
    , detection_counter_(0)
{
    // Cargar configuración
    config_.loadFromFile(config_path);
    
    // Inicializar estadísticas
    stats_ = Stats{};
}

LPRSystem::~LPRSystem() {
    stop();
}

bool LPRSystem::initialize() {
    if (initialized_) {
        return true;
    }
    
    std::cout << "🚀 Inicializando Sistema LPR..." << std::endl;
    
    // Obtener configuración
    auto camera_config = config_.getCameraConfig();
    auto processing_config = config_.getProcessingConfig();
    auto database_config = config_.getDatabaseConfig();
    
    // Inicializar captura de video
    std::cout << "📹 Inicializando captura de video..." << std::endl;
    video_capture_ = std::make_unique<VideoCapture>(camera_config.rtsp_url, 2);
    if (!video_capture_->start()) {
        std::cerr << "Error: No se pudo iniciar la captura de video" << std::endl;
        return false;
    }
    
    // Inicializar detector (necesita modelo YOLO convertido a ONNX)
    std::cout << "🔍 Inicializando detector de placas..." << std::endl;
    // TODO: Obtener ruta del modelo desde configuración o usar path por defecto
    std::string model_path = "models/license_plate_detector.onnx";  // Cambiar según tu modelo
    detector_ = std::make_unique<PlateDetector>(
        model_path, 
        static_cast<float>(processing_config.confidence_threshold)
    );
    if (!detector_->initialize()) {
        std::cerr << "Error: No se pudo inicializar el detector" << std::endl;
        std::cerr << "Nota: Necesitas convertir el modelo YOLO (.pt) a formato ONNX" << std::endl;
        // No retornar false, permitir continuar sin detector para pruebas
    }
    
    // Inicializar OCR
    std::cout << "📝 Inicializando OCR..." << std::endl;
    ocr_processor_ = std::make_unique<OCRProcessor>("eng");
    if (!ocr_processor_->initialize()) {
        std::cerr << "Error: No se pudo inicializar el OCR" << std::endl;
        return false;
    }
    ocr_processor_->setConfidenceThreshold(
        static_cast<float>(processing_config.plate_confidence_min)
    );
    
    // Inicializar base de datos
    std::cout << "💾 Inicializando base de datos..." << std::endl;
    db_manager_ = std::make_unique<DatabaseManager>();
    if (!db_manager_->connect(
            database_config.host,
            database_config.port,
            database_config.database,
            database_config.user,
            database_config.password
        )) {
        std::cerr << "Error: No se pudo conectar a la base de datos" << std::endl;
        std::cerr << "El sistema continuará sin guardar en BD" << std::endl;
        // No retornar false, permitir continuar sin BD
    }
    
    // Configurar cooldown
    cooldown_seconds_ = processing_config.detection_cooldown_sec;
    
    initialized_ = true;
    std::cout << "✅ Sistema LPR inicializado correctamente" << std::endl;
    
    return true;
}

void LPRSystem::start() {
    if (!initialized_) {
        std::cerr << "Error: Sistema no inicializado. Llama a initialize() primero." << std::endl;
        return;
    }
    
    if (running_) {
        std::cerr << "Advertencia: Sistema ya está corriendo" << std::endl;
        return;
    }
    
    running_ = true;
    start_time_ = std::chrono::steady_clock::now();
    
    // Iniciar hilos
    capture_thread_ = std::thread(&LPRSystem::captureThread, this);
    processing_thread_ = std::thread(&LPRSystem::processingThread, this);
    
    std::cout << "🚀 Sistema LPR iniciado" << std::endl;
}

void LPRSystem::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // Esperar a que terminen los hilos
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
    
    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }
    
    // Detener captura
    if (video_capture_) {
        video_capture_->stop();
    }
    
    // Desconectar base de datos
    if (db_manager_) {
        db_manager_->disconnect();
    }
    
    std::cout << "🛑 Sistema LPR detenido" << std::endl;
}

LPRSystem::Stats LPRSystem::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void LPRSystem::captureThread() {
    std::cout << "📹 Hilo de captura iniciado" << std::endl;
    
    while (running_) {
        cv::Mat frame;
        
        if (!video_capture_->getFrame(frame)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        frame_counter_++;
        
        // Agregar frame a la cola de procesamiento
        {
            std::lock_guard<std::mutex> lock(frame_queue_mutex_);
            
            // Limitar tamaño de cola
            while (frame_queue_.size() >= max_queue_size_) {
                frame_queue_.pop();
            }
            
            frame_queue_.push(frame.clone());
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::cout << "📹 Hilo de captura terminado" << std::endl;
}

void LPRSystem::processingThread() {
    std::cout << "🧠 Hilo de procesamiento iniciado" << std::endl;
    
    int ai_every = config_.getInt("realtime_optimization.ai_process_every", 2);
    int frame_skip_counter = 0;
    
    while (running_) {
        cv::Mat frame;
        
        // Obtener frame de la cola
        {
            std::lock_guard<std::mutex> lock(frame_queue_mutex_);
            if (frame_queue_.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            frame = frame_queue_.front();
            frame_queue_.pop();
        }
        
        // Procesar cada N frames (optimización)
        frame_skip_counter++;
        if (frame_skip_counter % ai_every == 0) {
            auto start_time = std::chrono::steady_clock::now();
            
            // Procesar frame
            std::vector<DetectionResult> results = processFrame(frame);
            
            ai_frame_counter_++;
            
            // Procesar resultados
            for (const auto& result : results) {
                if (result.valid) {
                    detection_counter_++;
                    
                    std::cout << "🎯 PLACA DETECTADA: " << result.plate_text 
                              << " (YOLO: " << result.yolo_confidence 
                              << ", OCR: " << result.ocr_confidence << ")" << std::endl;
                    
                    // Guardar en base de datos
                    saveDetection(result);
                }
            }
            
            // Actualizar estadísticas
            updateStats();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::cout << "🧠 Hilo de procesamiento terminado" << std::endl;
}

std::vector<DetectionResult> LPRSystem::processFrame(const cv::Mat& frame) {
    std::vector<DetectionResult> results;
    
    if (!detector_) {
        return results;
    }
    
    // Detectar placas con YOLO
    std::vector<PlateDetection> detections = detector_->detect(frame);
    
    for (const auto& detection : detections) {
        DetectionResult result;
        result.plate_bbox = detection.bbox;
        result.yolo_confidence = detection.confidence;
        result.timestamp = std::chrono::system_clock::now();
        
        // Extraer ROI de la placa
        cv::Rect roi = detection.bbox;
        
        // Asegurar que el ROI está dentro de los límites del frame
        roi.x = std::max(0, roi.x);
        roi.y = std::max(0, roi.y);
        roi.width = std::min(roi.width, frame.cols - roi.x);
        roi.height = std::min(roi.height, frame.rows - roi.y);
        
        if (roi.width <= 0 || roi.height <= 0) {
            continue;
        }
        
        cv::Mat plate_roi = frame(roi);
        
        // Reconocer texto con OCR
        OCRResult ocr_result = ocr_processor_->recognizeMultipleAttempts(plate_roi);
        
        if (ocr_result.text.empty()) {
            continue;
        }
        
        result.ocr_confidence = ocr_result.confidence;
        
        // Normalizar y validar placa
        std::string normalized = PlateValidator::normalizeColombianPlate(ocr_result.text);
        
        if (normalized.empty()) {
            continue;
        }
        
        result.plate_text = normalized;
        result.valid = PlateValidator::isValidColombianFormat(normalized);
        
        // Verificar cooldown
        if (!checkCooldown(normalized)) {
            continue;
        }
        
        // Verificar autorización (si hay BD)
        if (db_manager_ && db_manager_->isConnected()) {
            result.authorized = db_manager_->isAuthorized(normalized);
        }
        
        results.push_back(result);
    }
    
    return results;
}

void LPRSystem::saveDetection(const DetectionResult& result) {
    if (!db_manager_ || !db_manager_->isConnected()) {
        return;
    }
    
    DetectionData detection;
    detection.plate_text = result.plate_text;
    detection.yolo_confidence = result.yolo_confidence;
    detection.ocr_confidence = result.ocr_confidence;
    
    detection.plate_bbox[0] = result.plate_bbox.x;
    detection.plate_bbox[1] = result.plate_bbox.y;
    detection.plate_bbox[2] = result.plate_bbox.width;
    detection.plate_bbox[3] = result.plate_bbox.height;
    
    // Vehicle bbox (mismo que plate si no hay detección separada)
    detection.vehicle_bbox[0] = result.vehicle_bbox.x > 0 ? result.vehicle_bbox.x : result.plate_bbox.x;
    detection.vehicle_bbox[1] = result.vehicle_bbox.y > 0 ? result.vehicle_bbox.y : result.plate_bbox.y;
    detection.vehicle_bbox[2] = result.vehicle_bbox.width > 0 ? result.vehicle_bbox.width : result.plate_bbox.width;
    detection.vehicle_bbox[3] = result.vehicle_bbox.height > 0 ? result.vehicle_bbox.height : result.plate_bbox.height;
    
    auto camera_config = config_.getCameraConfig();
    detection.camera_location = "entrada_principal";
    
    // Convertir timestamp
    auto time_t = std::chrono::system_clock::to_time_t(result.timestamp);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    detection.timestamp = oss.str();
    
    db_manager_->insertDetection(detection);
}

bool LPRSystem::checkCooldown(const std::string& plate_text) {
    std::lock_guard<std::mutex> lock(cooldown_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto it = detection_cooldown_.find(plate_text);
    
    if (it != detection_cooldown_.end()) {
        auto elapsed = std::chrono::duration<double>(
            now - it->second
        ).count();
        
        if (elapsed < cooldown_seconds_) {
            return false;  // Está en cooldown
        }
    }
    
    // Actualizar cooldown
    detection_cooldown_[plate_text] = now;
    
    // Limpiar entradas antiguas
    for (auto it = detection_cooldown_.begin(); it != detection_cooldown_.end();) {
        auto elapsed = std::chrono::duration<double>(now - it->second).count();
        if (elapsed > cooldown_seconds_ * 10) {  // Mantener 10x cooldown
            it = detection_cooldown_.erase(it);
        } else {
            ++it;
        }
    }
    
    return true;
}

void LPRSystem::updateStats() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(now - start_time_).count();
    
    if (elapsed <= 0) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_frames = frame_counter_;
    stats_.ai_frames = ai_frame_counter_;
    stats_.detections_count = detection_counter_;
    stats_.capture_fps = frame_counter_ / elapsed;
    stats_.ai_fps = ai_frame_counter_ / elapsed;
}

} // namespace jetson_lpr

