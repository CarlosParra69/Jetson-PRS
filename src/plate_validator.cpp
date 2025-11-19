#include "plate_validator.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <regex>
#include <iostream>

namespace jetson_lpr {

// Patrones de placas colombianas
const std::regex PlateValidator::PATTERN_STANDARD(R"([A-Z]{3}[0-9]{3})");
const std::regex PlateValidator::PATTERN_DIPLOMATIC(R"(CD[0-9]{4})");
const std::regex PlateValidator::PATTERN_MOTO(R"([A-Z]{3}[0-9]{2})");

// Diccionarios de corrección de caracteres confusos
const std::map<char, char> PlateValidator::CHAR_TO_INT = {
    {'O', '0'}, {'I', '1'}, {'J', '3'}, {'A', '4'}, {'G', '6'}, {'S', '5'}
};

const std::map<char, char> PlateValidator::INT_TO_CHAR = {
    {'0', 'O'}, {'1', 'I'}, {'3', 'J'}, {'4', 'A'}, {'6', 'G'}, {'5', 'S'}
};

std::string PlateValidator::cleanText(const std::string& text) {
    std::string result;
    result.reserve(text.length());
    
    for (char c : text) {
        if (std::isalnum(c)) {
            result += std::toupper(c);
        }
    }
    
    return result;
}

std::string PlateValidator::normalizeColombianPlate(const std::string& raw_text) {
    if (raw_text.empty()) {
        return "";
    }
    
    std::string clean_text = cleanText(raw_text);
    
    // Si es menor a 6 caracteres, no es válida
    if (clean_text.length() < 6) {
        return "";
    }
    
    // Si es mayor a 6, intentar extraer los 6 más probables
    if (clean_text.length() > 6) {
        // Buscar patrón estándar: ABC123
        std::smatch match;
        if (std::regex_search(clean_text, match, PATTERN_STANDARD)) {
            return match.str().substr(0, 6);
        }
        
        // Buscar patrón diplomático: CD1234
        if (std::regex_search(clean_text, match, PATTERN_DIPLOMATIC)) {
            return match.str().substr(0, 6);
        }
        
        // Si no encuentra patrón, tomar los primeros 6
        clean_text = clean_text.substr(0, 6);
    }
    
    // Validar que sea formato colombiano válido
    if (isValidColombianFormat(clean_text)) {
        return clean_text;
    }
    
    return "";
}

bool PlateValidator::isValidColombianFormat(const std::string& plate_text) {
    if (plate_text.length() != 6) {
        return false;
    }
    
    // Patrón estándar: 3 letras + 3 números (ABC123)
    if (std::regex_match(plate_text, PATTERN_STANDARD)) {
        return true;
    }
    
    // Patrón diplomático: CD + 4 números (CD1234)
    if (std::regex_match(plate_text, PATTERN_DIPLOMATIC)) {
        return true;
    }
    
    return false;
}

std::vector<std::string> PlateValidator::extractBestPlateCandidates(const std::string& raw_text) {
    std::vector<std::string> candidates;
    
    if (raw_text.empty()) {
        return candidates;
    }
    
    std::string clean_text = cleanText(raw_text);
    
    // Extraer todas las subcadenas de 6 caracteres válidas
    for (size_t i = 0; i <= clean_text.length() - 6; ++i) {
        std::string candidate = clean_text.substr(i, 6);
        if (isValidColombianFormat(candidate)) {
            if (std::find(candidates.begin(), candidates.end(), candidate) == candidates.end()) {
                candidates.push_back(candidate);
            }
        }
    }
    
    // Buscar patrones específicos con regex
    std::smatch match;
    std::string search_text = clean_text;
    
    // Patrón estándar
    while (std::regex_search(search_text, match, PATTERN_STANDARD)) {
        std::string candidate = match.str().substr(0, 6);
        if (std::find(candidates.begin(), candidates.end(), candidate) == candidates.end()) {
            candidates.push_back(candidate);
        }
        search_text = match.suffix().str();
    }
    
    // Patrón diplomático
    search_text = clean_text;
    while (std::regex_search(search_text, match, PATTERN_DIPLOMATIC)) {
        std::string candidate = match.str().substr(0, 6);
        if (std::find(candidates.begin(), candidates.end(), candidate) == candidates.end()) {
            candidates.push_back(candidate);
        }
        search_text = match.suffix().str();
    }
    
    // Ordenar por probabilidad (estándar primero)
    std::sort(candidates.begin(), candidates.end(), [](const std::string& a, const std::string& b) {
        double score_a = calculateFormatScore(a);
        double score_b = calculateFormatScore(b);
        return score_a > score_b;
    });
    
    return candidates;
}

double PlateValidator::calculateFormatScore(const std::string& text) {
    double score = 0.0;
    
    if (text.length() == 6) {
        // Formato estándar: ABC123 (3 letras + 3 números)
        if (std::isalpha(text[0]) && std::isalpha(text[1]) && std::isalpha(text[2]) &&
            std::isdigit(text[3]) && std::isdigit(text[4]) && std::isdigit(text[5])) {
            score += 0.9;
        }
        // Formato diplomático: CD1234 (2 letras + 4 números)
        else if (text.substr(0, 2) == "CD" &&
                 std::isdigit(text[2]) && std::isdigit(text[3]) &&
                 std::isdigit(text[4]) && std::isdigit(text[5])) {
            score += 0.95;
        }
    }
    
    return std::min(score, 1.0);
}

} // namespace jetson_lpr

