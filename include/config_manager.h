#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <memory>

namespace jetson_lpr {

// Forward declaration
struct JsonHolder;

/**
 * Gestor de configuración del sistema LPR
 * Lee configuración desde archivo JSON
 */
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    
    /**
     * Cargar configuración desde archivo JSON
     * 
     * @param config_path Ruta al archivo de configuración
     * @return true si se cargó correctamente
     */
    bool loadFromFile(const std::string& config_path);
    
    /**
     * Obtener string de configuración
     */
    std::string getString(const std::string& key, const std::string& default_value = "") const;
    
    /**
     * Obtener entero de configuración
     */
    int getInt(const std::string& key, int default_value = 0) const;
    
    /**
     * Obtener flotante de configuración
     */
    double getDouble(const std::string& key, double default_value = 0.0) const;
    
    /**
     * Obtener booleano de configuración
     */
    bool getBool(const std::string& key, bool default_value = false) const;
    
    /**
     * Verificar si existe una clave
     */
    bool has(const std::string& key) const;
    
    // Estructuras de configuración
    struct CameraConfig {
        std::string ip;
        std::string user;
        std::string password;
        std::string rtsp_url;
    };
    
    struct JetsonConfig {
        std::string ip;
        std::string interface;
    };
    
    struct ProcessingConfig {
        double confidence_threshold;
        double plate_confidence_min;
        double detection_cooldown_sec;
        bool ocr_cache_enabled;
    };
    
    struct DatabaseConfig {
        std::string host;
        int port;
        std::string database;
        std::string user;
        std::string password;
    };
    
    /**
     * Obtener configuración de cámara
     */
    CameraConfig getCameraConfig() const;
    
    /**
     * Obtener configuración de Jetson/Red
     */
    JetsonConfig getJetsonConfig() const;
    
    /**
     * Obtener configuración de procesamiento
     */
    ProcessingConfig getProcessingConfig() const;
    
    /**
     * Obtener configuración de base de datos
     */
    DatabaseConfig getDatabaseConfig() const;

private:
    std::unique_ptr<JsonHolder> json_data_;
    
    /**
     * Obtener valor JSON anidado usando clave con "."
     */
    void* getNestedValue(const std::string& key) const;
    
    /**
     * Crear configuración por defecto
     */
    void createDefaultConfig();
};

} // namespace jetson_lpr

#endif // CONFIG_MANAGER_H
