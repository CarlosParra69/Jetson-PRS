#ifndef PLATE_VALIDATOR_H
#define PLATE_VALIDATOR_H

#include <string>
#include <vector>
#include <regex>
#include <map>

namespace jetson_lpr {

/**
 * Validador de placas de vehículos colombianos
 * 
 * Formato estándar: ABC123 (3 letras + 3 números)
 * Formato diplomático: CD1234 (2 letras + 4 números)
 * Formato moto: ABC12 (3 letras + 2 números)
 */
class PlateValidator {
public:
    /**
     * Normalizar texto OCR a formato de placa colombiana válido
     * 
     * @param raw_text Texto crudo del OCR
     * @return Placa normalizada de 6 caracteres o string vacío si no es válida
     */
    static std::string normalizeColombianPlate(const std::string& raw_text);
    
    /**
     * Validar que la placa tenga formato colombiano válido
     * 
     * @param plate_text Texto de la placa
     * @return true si es formato válido
     */
    static bool isValidColombianFormat(const std::string& plate_text);
    
    /**
     * Extraer múltiples candidatos posibles de una cadena más larga
     * 
     * @param raw_text Texto crudo del OCR
     * @return Lista de candidatos ordenados por probabilidad
     */
    static std::vector<std::string> extractBestPlateCandidates(const std::string& raw_text);
    
    /**
     * Calcular un score basado en formatos de placas colombianas
     * 
     * @param text Texto de la placa
     * @return Score entre 0.0 y 1.0
     */
    static double calculateFormatScore(const std::string& text);
    
    /**
     * Limpiar texto: solo letras y números, mayúsculas
     * 
     * @param text Texto a limpiar
     * @return Texto limpio
     */
    static std::string cleanText(const std::string& text);

private:
    // Patrones de placas colombianas
    static const std::regex PATTERN_STANDARD;      // ABC123
    static const std::regex PATTERN_DIPLOMATIC;    // CD1234
    static const std::regex PATTERN_MOTO;          // ABC12
    
    // Diccionarios de corrección
    static const std::map<char, char> CHAR_TO_INT;
    static const std::map<char, char> INT_TO_CHAR;
};

} // namespace jetson_lpr

#endif // PLATE_VALIDATOR_H

