#ifndef OCR_PROCESSOR_H
#define OCR_PROCESSOR_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <opencv2/opencv.hpp>

// Forward declaration de Tesseract
namespace tesseract {
    class TessBaseAPI;
}

namespace jetson_lpr {

/**
 * Estructura para resultado de OCR
 */
struct OCRResult {
    std::string text;          // Texto reconocido
    float confidence;          // Confianza (0.0 - 1.0)
    
    OCRResult() : confidence(0.0f) {}
    OCRResult(const std::string& t, float c) : text(t), confidence(c) {}
};

/**
 * Procesador OCR usando Tesseract
 * Optimizado para reconocimiento de placas colombianas
 */
class OCRProcessor {
public:
    /**
     * Constructor
     * 
     * @param language Idioma para OCR (default: "eng")
     * @param data_path Ruta al directorio de datos de Tesseract (opcional)
     */
    explicit OCRProcessor(const std::string& language = "eng", 
                         const std::string& data_path = "");
    
    /**
     * Destructor
     */
    ~OCRProcessor();
    
    /**
     * Inicializar procesador OCR
     * 
     * @return true si se inicializó correctamente
     */
    bool initialize();
    
    /**
     * Reconocer texto en una imagen de placa
     * 
     * @param plate_image Imagen de la placa (ROI)
     * @param use_cache Si usar cache de resultados (default: true)
     * @return Resultado de OCR
     */
    OCRResult recognize(const cv::Mat& plate_image, bool use_cache = true);
    
    /**
     * Reconocer texto con múltiples intentos (diferentes preprocesamientos)
     * 
     * @param plate_image Imagen de la placa
     * @return Mejor resultado de OCR
     */
    OCRResult recognizeMultipleAttempts(const cv::Mat& plate_image);
    
    /**
     * Preprocesar imagen de placa para mejor OCR
     * 
     * @param image Imagen de entrada
     * @return Imagen preprocesada
     */
    static cv::Mat preprocessPlateImage(const cv::Mat& image);
    
    /**
     * Limpiar cache de OCR
     */
    void clearCache();
    
    /**
     * Configurar umbral de confianza mínimo
     * 
     * @param threshold Umbral mínimo (0.0 - 1.0)
     */
    void setConfidenceThreshold(float threshold) {
        confidence_threshold_ = threshold;
    }

private:
    std::string language_;
    std::string data_path_;
    float confidence_threshold_;
    
    std::unique_ptr<tesseract::TessBaseAPI> tesseract_api_;
    bool initialized_;
    
    // Cache de resultados OCR
    std::unordered_map<std::string, OCRResult> ocr_cache_;
    std::mutex cache_mutex_;
    size_t max_cache_size_;
    
    /**
     * Calcular hash simple de imagen para cache
     * 
     * @param image Imagen
     * @return Hash de la imagen
     */
    std::string calculateImageHash(const cv::Mat& image);
    
    /**
     * Procesar imagen con Tesseract directamente
     * 
     * @param processed_image Imagen preprocesada
     * @return Resultado de OCR
     */
    OCRResult recognizeInternal(const cv::Mat& processed_image);
    
    /**
     * Aplicar múltiples técnicas de binarización
     * 
     * @param gray Imagen en escala de grises
     * @return Vector de imágenes binarizadas
     */
    std::vector<cv::Mat> applyMultipleThresholds(const cv::Mat& gray);
};

} // namespace jetson_lpr

#endif // OCR_PROCESSOR_H

