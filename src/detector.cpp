#include "detector.h"
#include <iostream>
#include <algorithm>
#include <numeric>

namespace jetson_lpr {

PlateDetector::PlateDetector(const std::string& model_path, float confidence_threshold)
    : model_path_(model_path)
    , confidence_threshold_(confidence_threshold)
    , initialized_(false)
    , input_size_(640, 640)  // Tamaño estándar YOLO
    , scale_factor_(1.0 / 255.0)  // Normalización 0-255 -> 0-1
    , mean_(0.0, 0.0, 0.0)   // Sin media (ya está normalizado)
    , swap_rb_(true)         // BGR -> RGB
{
}

PlateDetector::~PlateDetector() {
    // La red se libera automáticamente
}

bool PlateDetector::initialize() {
    if (initialized_) {
        return true;
    }
    
    try {
        // Intentar cargar modelo ONNX
        std::string extension = model_path_.substr(model_path_.find_last_of(".") + 1);
        
        if (extension == "onnx") {
            net_ = cv::dnn::readNetFromONNX(model_path_);
        } else if (extension == "xml" || extension == "bin") {
            // OpenVINO format
            net_ = cv::dnn::readNet(model_path_);
        } else {
            std::cerr << "Error: Formato de modelo no soportado: " << extension << std::endl;
            std::cerr << "Soporta: .onnx, .xml/.bin (OpenVINO)" << std::endl;
            return false;
        }
        
        if (net_.empty()) {
            std::cerr << "Error: No se pudo cargar el modelo: " << model_path_ << std::endl;
            return false;
        }
        
        // Configurar backend preferido
        // En Jetson, intentar CUDA primero, luego CPU
        #ifdef USE_CUDA
        try {
            net_.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            net_.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
            std::cout << "✅ Modelo cargado con backend CUDA" << std::endl;
        } catch (const cv::Exception&) {
            std::cout << "⚠️ CUDA no disponible, usando CPU" << std::endl;
            net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }
        #else
        net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        std::cout << "✅ Modelo cargado con backend CPU" << std::endl;
        #endif
        
        initialized_ = true;
        std::cout << "✅ Detector inicializado: " << model_path_ << std::endl;
        
        return true;
        
    } catch (const cv::Exception& e) {
        std::cerr << "Error OpenCV al inicializar detector: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error al inicializar detector: " << e.what() << std::endl;
        return false;
    }
}

std::vector<PlateDetection> PlateDetector::detect(const cv::Mat& frame) {
    if (!initialized_ || frame.empty()) {
        return {};
    }
    
    try {
        // Preprocesar frame
        cv::Mat blob;
        preprocess(frame, blob);
        
        // Inferencia
        std::vector<std::string> output_names = net_.getUnconnectedOutLayersNames();
        std::vector<cv::Mat> outputs;
        
        net_.setInput(blob);
        net_.forward(outputs, output_names);
        
        // Postprocesar resultados
        std::vector<PlateDetection> detections = postprocess(outputs, frame.size());
        
        // Aplicar NMS
        std::vector<PlateDetection> filtered_detections = applyNMS(detections);
        
        return filtered_detections;
        
    } catch (const cv::Exception& e) {
        std::cerr << "Error en detección: " << e.what() << std::endl;
        return {};
    }
}

void PlateDetector::preprocess(const cv::Mat& frame, cv::Mat& blob) {
    // Crear blob desde frame
    cv::dnn::blobFromImage(
        frame,
        blob,
        scale_factor_,
        input_size_,
        mean_,
        swap_rb_,
        false,
        CV_32F
    );
}

std::vector<PlateDetection> PlateDetector::postprocess(
    const std::vector<cv::Mat>& outputs,
    const cv::Size& frame_size
) {
    std::vector<PlateDetection> detections;
    
    // YOLO v5/v8 genera outputs en formato [batch, num_detections, 5+num_classes]
    // Para YOLO v8: [batch, num_boxes, 4+num_classes] donde 4 son las coordenadas
    
    for (const auto& output : outputs) {
        // output es [batch, num_boxes, features]
        // Para YOLO v8: features = [x_center, y_center, width, height, class_scores...]
        
        const int num_detections = output.size[1];
        const int num_features = output.size[2];
        
        if (num_features < 4) {
            continue;
        }
        
        // Calcular ratios de escalado
        float x_scale = static_cast<float>(frame_size.width) / input_size_.width;
        float y_scale = static_cast<float>(frame_size.height) / input_size_.height;
        
        // Iterar sobre detecciones
        for (int i = 0; i < num_detections; ++i) {
            float* data = (float*)output.data + i * num_features;
            
            // Obtener coordenadas (YOLO formato: center_x, center_y, width, height)
            float center_x = data[0];
            float center_y = data[1];
            float width = data[2];
            float height = data[3];
            
            // Convertir a coordenadas de bbox (x, y, width, height)
            float x = (center_x - width / 2.0f) * x_scale;
            float y = (center_y - height / 2.0f) * y_scale;
            float w = width * x_scale;
            float h = height * y_scale;
            
            // Obtener confianza (max score de clases)
            float max_confidence = 0.0f;
            int class_id = 0;
            
            if (num_features > 4) {
                // Buscar la clase con mayor confianza
                for (int j = 4; j < num_features; ++j) {
                    if (data[j] > max_confidence) {
                        max_confidence = data[j];
                        class_id = j - 4;
                    }
                }
            } else {
                // Si no hay scores de clase, usar un valor por defecto
                max_confidence = 0.5f;
            }
            
            // Filtrar por umbral de confianza
            if (max_confidence >= confidence_threshold_) {
                // Crear bbox
                cv::Rect bbox(
                    static_cast<int>(std::max(0.0f, x)),
                    static_cast<int>(std::max(0.0f, y)),
                    static_cast<int>(std::min(w, static_cast<float>(frame_size.width))),
                    static_cast<int>(std::min(h, static_cast<float>(frame_size.height)))
                );
                
                detections.emplace_back(bbox, max_confidence, class_id);
            }
        }
    }
    
    return detections;
}

std::vector<PlateDetection> PlateDetector::applyNMS(
    const std::vector<PlateDetection>& detections,
    float nms_threshold
) {
    if (detections.empty()) {
        return {};
    }
    
    // Extraer bboxes y confidencias
    std::vector<cv::Rect> bboxes;
    std::vector<float> confidences;
    std::vector<int> indices;
    
    for (const auto& det : detections) {
        bboxes.push_back(det.bbox);
        confidences.push_back(det.confidence);
    }
    
    // Aplicar NMS usando OpenCV
    cv::dnn::NMSBoxes(
        bboxes,
        confidences,
        confidence_threshold_,
        nms_threshold,
        indices
    );
    
    // Crear vector de detecciones filtradas
    std::vector<PlateDetection> filtered_detections;
    for (int idx : indices) {
        filtered_detections.push_back(detections[idx]);
    }
    
    return filtered_detections;
}

} // namespace jetson_lpr

