#ifndef DETECTOR_H
#define DETECTOR_H

#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>

namespace jetson_lpr {

/**
 * Estructura para representar una detección de placa
 */
struct PlateDetection {
    cv::Rect bbox;              // Bounding box de la placa
    float confidence;           // Confianza de la detección (0.0 - 1.0)
    int class_id;               // ID de clase (placa)
    
    PlateDetection() : confidence(0.0), class_id(0) {}
    
    PlateDetection(const cv::Rect& box, float conf, int id)
        : bbox(box), confidence(conf), class_id(id) {}
};

/**
 * Detector de placas de vehículos usando YOLO
 * Soporta TensorRT (Jetson) y ONNX Runtime
 */
class PlateDetector {
public:
    /**
     * Constructor
     * 
     * @param model_path Ruta al modelo (ONNX, TensorRT engine, o similar)
     * @param confidence_threshold Umbral de confianza mínimo (default: 0.3)
     */
    explicit PlateDetector(const std::string& model_path, 
                          float confidence_threshold = 0.3f);
    
    /**
     * Destructor
     */
    ~PlateDetector();
    
    /**
     * Inicializar detector
     * 
     * @return true si se inicializó correctamente
     */
    bool initialize();
    
    /**
     * Detectar placas en un frame
     * 
     * @param frame Frame de entrada
     * @return Vector de detecciones
     */
    std::vector<PlateDetection> detect(const cv::Mat& frame);
    
    /**
     * Configurar umbral de confianza
     * 
     * @param threshold Nuevo umbral (0.0 - 1.0)
     */
    void setConfidenceThreshold(float threshold) {
        confidence_threshold_ = threshold;
    }
    
    /**
     * Obtener umbral de confianza actual
     * 
     * @return Umbral de confianza
     */
    float getConfidenceThreshold() const {
        return confidence_threshold_;
    }

private:
    std::string model_path_;
    float confidence_threshold_;
    
    bool initialized_;
    
    // Para OpenCV DNN (ONNX)
    cv::dnn::Net net_;
    
    // Configuración del modelo
    cv::Size input_size_;
    float scale_factor_;
    cv::Scalar mean_;
    bool swap_rb_;
    
    /**
     * Preprocesar frame para inferencia
     * 
     * @param frame Frame de entrada
     * @param blob Blob de salida
     */
    void preprocess(const cv::Mat& frame, cv::Mat& blob);
    
    /**
     * Postprocesar resultados de inferencia
     * 
     * @param outputs Outputs del modelo
     * @param frame_size Tamaño original del frame
     * @return Vector de detecciones
     */
    std::vector<PlateDetection> postprocess(
        const std::vector<cv::Mat>& outputs,
        const cv::Size& frame_size
    );
    
    /**
     * Aplicar NMS (Non-Maximum Suppression)
     * 
     * @param detections Detecciones de entrada
     * @param nms_threshold Umbral NMS (default: 0.5)
     * @return Detecciones después de NMS
     */
    std::vector<PlateDetection> applyNMS(
        const std::vector<PlateDetection>& detections,
        float nms_threshold = 0.5f
    );
};

} // namespace jetson_lpr

#endif // DETECTOR_H

