#include "ocr_processor.h"
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <functional>

namespace jetson_lpr {

OCRProcessor::OCRProcessor(const std::string& language, const std::string& data_path)
    : language_(language)
    , data_path_(data_path)
    , confidence_threshold_(0.2f)
    , tesseract_api_(nullptr)
    , initialized_(false)
    , max_cache_size_(100)
{
}

OCRProcessor::~OCRProcessor() {
    if (tesseract_api_) {
        tesseract_api_->End();
    }
}

bool OCRProcessor::initialize() {
    if (initialized_) {
        return true;
    }
    
    try {
        tesseract_api_ = std::make_unique<tesseract::TessBaseAPI>();
        
        // Inicializar Tesseract
        int init_result = 0;
        if (!data_path_.empty()) {
            init_result = tesseract_api_->Init(data_path_.c_str(), language_.c_str());
        } else {
            init_result = tesseract_api_->Init(nullptr, language_.c_str());
        }
        
        if (init_result != 0) {
            std::cerr << "Error: No se pudo inicializar Tesseract OCR" << std::endl;
            std::cerr << "Error code: " << init_result << std::endl;
            return false;
        }
        
        // Configurar parámetros para placas colombianas
        tesseract_api_->SetPageSegMode(tesseract::PSM_SINGLE_LINE);  // Una línea de texto
        tesseract_api_->SetVariable("tessedit_char_whitelist", 
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");  // Solo letras y números
        
        // Configuraciones adicionales para mejor reconocimiento
        tesseract_api_->SetVariable("classify_bln_numeric_mode", "0");
        tesseract_api_->SetVariable("textord_min_linesize", "2.5");
        
        initialized_ = true;
        std::cout << "✅ OCR Processor inicializado (idioma: " << language_ << ")" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error al inicializar OCR: " << e.what() << std::endl;
        return false;
    }
}

OCRResult OCRProcessor::recognize(const cv::Mat& plate_image, bool use_cache) {
    if (!initialized_ || plate_image.empty()) {
        return OCRResult();
    }
    
    // Verificar cache
    if (use_cache) {
        std::string image_hash = calculateImageHash(plate_image);
        
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = ocr_cache_.find(image_hash);
        if (it != ocr_cache_.end()) {
            return it->second;
        }
    }
    
    // Preprocesar imagen
    cv::Mat processed = preprocessPlateImage(plate_image);
    
    // Reconocer texto
    OCRResult result = recognizeInternal(processed);
    
    // Guardar en cache
    if (use_cache && !result.text.empty()) {
        std::string image_hash = calculateImageHash(plate_image);
        
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        // Limpiar cache si está muy lleno
        if (ocr_cache_.size() >= max_cache_size_) {
            // Eliminar 20% más antiguos
            size_t to_remove = max_cache_size_ / 5;
            auto it = ocr_cache_.begin();
            for (size_t i = 0; i < to_remove && it != ocr_cache_.end(); ++i) {
                it = ocr_cache_.erase(it);
            }
        }
        
        ocr_cache_[image_hash] = result;
    }
    
    return result;
}

OCRResult OCRProcessor::recognizeMultipleAttempts(const cv::Mat& plate_image) {
    if (!initialized_ || plate_image.empty()) {
        return OCRResult();
    }
    
    OCRResult best_result;
    float best_confidence = 0.0f;
    
    // Convertir a escala de grises si es necesario
    cv::Mat gray;
    if (plate_image.channels() == 3) {
        cv::cvtColor(plate_image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = plate_image.clone();
    }
    
    // Intentar múltiples técnicas de binarización
    std::vector<cv::Mat> binary_images = applyMultipleThresholds(gray);
    
    // Probar cada imagen binarizada
    for (const auto& binary_img : binary_images) {
        OCRResult result = recognizeInternal(binary_img);
        
        if (result.confidence > best_confidence) {
            best_confidence = result.confidence;
            best_result = result;
        }
        
        // Si encontramos una confianza muy alta, usar este resultado
        if (best_confidence > 0.9f) {
            break;
        }
    }
    
    // Si ningún resultado es bueno, intentar con la imagen original preprocesada
    if (best_confidence < 0.5f) {
        cv::Mat processed = preprocessPlateImage(plate_image);
        OCRResult result = recognizeInternal(processed);
        
        if (result.confidence > best_confidence) {
            best_result = result;
        }
    }
    
    return best_result;
}

cv::Mat OCRProcessor::preprocessPlateImage(const cv::Mat& image) {
    cv::Mat processed;
    
    // Convertir a escala de grises si es necesario
    if (image.channels() == 3) {
        cv::cvtColor(image, processed, cv::COLOR_BGR2GRAY);
    } else {
        processed = image.clone();
    }
    
    // Redimensionar si es muy pequeña (mínimo 60px de ancho)
    int min_width = 60;
    int min_height = 20;
    
    if (processed.cols < min_width || processed.rows < min_height) {
        float scale = std::max(
            static_cast<float>(min_width) / processed.cols,
            static_cast<float>(min_height) / processed.rows
        );
        scale = std::min(scale, 4.0f);  // Limitar escala máxima
        
        int new_width = static_cast<int>(processed.cols * scale);
        int new_height = static_cast<int>(processed.rows * scale);
        
        cv::resize(processed, processed, cv::Size(new_width, new_height), 
                  0, 0, cv::INTER_CUBIC);
    }
    
    // Aplicar filtro bilateral para reducir ruido
    cv::bilateralFilter(processed, processed, 5, 30, 30);
    
    // Aplicar umbral adaptativo
    cv::Mat binary;
    cv::adaptiveThreshold(processed, binary, 255, 
                         cv::ADAPTIVE_THRESH_GAUSSIAN_C, 
                         cv::THRESH_BINARY, 11, 2);
    
    return binary;
}

void OCRProcessor::clearCache() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    ocr_cache_.clear();
}

std::string OCRProcessor::calculateImageHash(const cv::Mat& image) {
    // Hash simple basado en tamaño y algunos píxeles
    std::ostringstream oss;
    oss << image.rows << "x" << image.cols << "_";
    
    // Muestrear algunos píxeles
    int sample_size = std::min(20, image.rows * image.cols / 100);
    for (int i = 0; i < sample_size; ++i) {
        int x = (i * 17) % image.cols;
        int y = (i * 23) % image.rows;
        oss << std::hex << static_cast<int>(image.at<uchar>(y, x)) << "_";
    }
    
    return oss.str();
}

OCRResult OCRProcessor::recognizeInternal(const cv::Mat& processed_image) {
    if (!tesseract_api_ || processed_image.empty()) {
        return OCRResult();
    }
    
    try {
        // Configurar imagen en Tesseract
        tesseract_api_->SetImage(processed_image.data, 
                                processed_image.cols,
                                processed_image.rows,
                                1,  // bytes per pixel
                                processed_image.step);
        
        // Obtener texto y confianza
        char* text = tesseract_api_->GetUTF8Text();
        int* confidences = tesseract_api_->AllWordConfidences();
        
        std::string result_text = text ? std::string(text) : "";
        
        // Calcular confianza promedio
        float avg_confidence = 0.0f;
        if (confidences) {
            int sum = 0;
            int count = 0;
            for (int i = 0; confidences[i] != -1; ++i) {
                sum += confidences[i];
                count++;
            }
            if (count > 0) {
                avg_confidence = static_cast<float>(sum) / count / 100.0f;  // Convertir 0-100 a 0-1
            }
            delete[] confidences;
        }
        
        // Limpiar texto
        if (text) {
            delete[] text;
        }
        
        // Filtrar por confianza mínima
        if (avg_confidence < confidence_threshold_) {
            return OCRResult();
        }
        
        // Limpiar y normalizar texto
        std::string cleaned_text;
        for (char c : result_text) {
            if (std::isalnum(c)) {
                cleaned_text += std::toupper(c);
            }
        }
        
        return OCRResult(cleaned_text, avg_confidence);
        
    } catch (const std::exception& e) {
        std::cerr << "Error en reconocimiento OCR: " << e.what() << std::endl;
        return OCRResult();
    }
}

std::vector<cv::Mat> OCRProcessor::applyMultipleThresholds(const cv::Mat& gray) {
    std::vector<cv::Mat> results;
    
    // 1. Threshold de Otsu
    cv::Mat otsu;
    cv::threshold(gray, otsu, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
    results.push_back(otsu);
    
    // 2. Threshold de Otsu inverso
    cv::Mat otsu_inv;
    cv::threshold(gray, otsu_inv, 0, 255, cv::THRESH_BINARY_INV + cv::THRESH_OTSU);
    results.push_back(otsu_inv);
    
    // 3. Threshold adaptativo (media)
    cv::Mat adaptive_mean;
    cv::adaptiveThreshold(gray, adaptive_mean, 255,
                         cv::ADAPTIVE_THRESH_MEAN_C,
                         cv::THRESH_BINARY, 11, 2);
    results.push_back(adaptive_mean);
    
    // 4. Threshold adaptativo (Gaussiano)
    cv::Mat adaptive_gauss;
    cv::adaptiveThreshold(gray, adaptive_gauss, 255,
                         cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                         cv::THRESH_BINARY, 11, 2);
    results.push_back(adaptive_gauss);
    
    // 5. Thresholds manuales (varios valores)
    std::vector<int> thresholds = {80, 100, 120, 140};
    for (int thresh : thresholds) {
        cv::Mat manual;
        cv::threshold(gray, manual, thresh, 255, cv::THRESH_BINARY);
        results.push_back(manual);
        
        cv::Mat manual_inv;
        cv::threshold(gray, manual_inv, thresh, 255, cv::THRESH_BINARY_INV);
        results.push_back(manual_inv);
    }
    
    return results;
}

} // namespace jetson_lpr

