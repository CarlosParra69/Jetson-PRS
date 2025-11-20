#include "config_manager.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Incluir nlohmann/json de forma pimpl para evitar dependencias públicas
#include "nlohmann/json.hpp"

namespace jetson_lpr {

// Estructura interna para almacenar JSON
struct JsonHolder {
    nlohmann::json data;
};

ConfigManager::CameraConfig ConfigManager::getCameraConfig() const {
    CameraConfig config;
    config.ip = getString("camera.ip", "192.168.0.100");
    config.user = getString("camera.user", "admin");
    config.password = getString("camera.password", "tlJwpbo6");
    config.rtsp_url = getString("camera.rtsp_url", 
        "rtsp://192.168.0.100:554/user=admin_password=tlJwpbo6_channel=1_stream=0.sdp?real_stream");
    return config;
}

ConfigManager::JetsonConfig ConfigManager::getJetsonConfig() const {
    JetsonConfig config;
    config.ip = getString("jetson.ip", "192.168.1.100");
    config.interface = getString("jetson.interface", "enP8p1s0");
    return config;
}

ConfigManager::ProcessingConfig ConfigManager::getProcessingConfig() const {
    ProcessingConfig config;
    config.confidence_threshold = getDouble("processing.confidence_threshold", 0.30);
    config.plate_confidence_min = getDouble("processing.plate_confidence_min", 0.25);
    config.detection_cooldown_sec = getDouble("processing.detection_cooldown_sec", 0.5);
    config.ocr_cache_enabled = getBool("processing.ocr_cache_enabled", true);
    return config;
}

ConfigManager::DatabaseConfig ConfigManager::getDatabaseConfig() const {
    DatabaseConfig config;
    config.host = getString("database.host", "localhost");
    config.port = getInt("database.port", 3306);
    config.database = getString("database.database", "parqueadero_jetson");
    config.user = getString("database.user", "lpr_user");
    config.password = getString("database.password", "lpr_password");
    return config;
}

ConfigManager::ConfigManager() : json_data_(nullptr) {
    createDefaultConfig();
}

ConfigManager::~ConfigManager() = default;

bool ConfigManager::loadFromFile(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de configuración: " << config_path << std::endl;
        createDefaultConfig();
        return false;
    }
    
    try {
        json_data_ = std::make_unique<JsonHolder>();
        file >> json_data_->data;
        file.close();
        
        std::cout << "Configuración cargada desde: " << config_path << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parseando JSON: " << e.what() << std::endl;
        createDefaultConfig();
        return false;
    }
}

void ConfigManager::createDefaultConfig() {
    json_data_ = std::make_unique<JsonHolder>();
    
    // Configuración por defecto
    json_data_->data = nlohmann::json::object({
        {"camera", {
            {"ip", "192.168.0.100"},
            {"user", "admin"},
            {"password", "tlJwpbo6"},
            {"rtsp_url", "rtsp://192.168.0.100:554/user=admin_password=tlJwpbo6_channel=1_stream=0.sdp?real_stream"}
        }},
        {"jetson", {
            {"ip", "192.168.1.100"},
            {"interface", "enP8p1s0"}
        }},
        {"processing", {
            {"confidence_threshold", 0.30},
            {"plate_confidence_min", 0.25},
            {"detection_cooldown_sec", 0.5},
            {"ocr_cache_enabled", true}
        }},
        {"database", {
            {"host", "localhost"},
            {"port", 3306},
            {"database", "parqueadero_jetson"},
            {"user", "lpr_user"},
            {"password", "lpr_password"}
        }},
        {"realtime_optimization", {
            {"ai_process_every", 2},
            {"motion_activation", true},
            {"display_scale", 0.25},
            {"headless_mode", true}
        }}
    });
    
    std::cout << "Configuración por defecto creada" << std::endl;
}

void* ConfigManager::getNestedValue(const std::string& key) const {
    if (!json_data_) {
        return nullptr;
    }
    
    nlohmann::json* current = &json_data_->data;
    
    std::istringstream iss(key);
    std::string segment;
    
    while (std::getline(iss, segment, '.')) {
        if (current->is_object() && current->contains(segment)) {
            current = &((*current)[segment]);
        } else {
            return nullptr;
        }
    }
    
    return static_cast<void*>(current);
}

std::string ConfigManager::getString(const std::string& key, const std::string& default_value) const {
    nlohmann::json* json_ptr = static_cast<nlohmann::json*>(getNestedValue(key));
    if (!json_ptr || json_ptr->is_null()) {
        return default_value;
    }
    
    try {
        return json_ptr->get<std::string>();
    } catch (const std::exception&) {
        return default_value;
    }
}

int ConfigManager::getInt(const std::string& key, int default_value) const {
    nlohmann::json* json_ptr = static_cast<nlohmann::json*>(getNestedValue(key));
    if (!json_ptr || json_ptr->is_null()) {
        return default_value;
    }
    
    try {
        return json_ptr->get<int>();
    } catch (const std::exception&) {
        return default_value;
    }
}

double ConfigManager::getDouble(const std::string& key, double default_value) const {
    nlohmann::json* json_ptr = static_cast<nlohmann::json*>(getNestedValue(key));
    if (!json_ptr || json_ptr->is_null()) {
        return default_value;
    }
    
    try {
        return json_ptr->get<double>();
    } catch (const std::exception&) {
        return default_value;
    }
}

bool ConfigManager::getBool(const std::string& key, bool default_value) const {
    nlohmann::json* json_ptr = static_cast<nlohmann::json*>(getNestedValue(key));
    if (!json_ptr || json_ptr->is_null()) {
        return default_value;
    }
    
    try {
        return json_ptr->get<bool>();
    } catch (const std::exception&) {
        return default_value;
    }
}

bool ConfigManager::has(const std::string& key) const {
    return getNestedValue(key) != nullptr;
}

} // namespace jetson_lpr

