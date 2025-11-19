#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <mysql/mysql.h>

namespace jetson_lpr {

/**
 * Estructura para datos de detección
 */
struct DetectionData {
    std::string plate_text;           // Texto de la placa
    float yolo_confidence;            // Confianza de YOLO
    float ocr_confidence;             // Confianza de OCR
    int vehicle_bbox[4];              // Bbox del vehículo [x, y, w, h]
    int plate_bbox[4];                // Bbox de la placa [x, y, w, h]
    std::string camera_location;      // Ubicación de la cámara
    std::string timestamp;            // Timestamp (ISO 8601)
    
    DetectionData() 
        : yolo_confidence(0.0f)
        , ocr_confidence(0.0f)
        , camera_location("entrada_principal")
    {
        vehicle_bbox[0] = vehicle_bbox[1] = vehicle_bbox[2] = vehicle_bbox[3] = 0;
        plate_bbox[0] = plate_bbox[1] = plate_bbox[2] = plate_bbox[3] = 0;
    }
};

/**
 * Gestor de base de datos MySQL
 * Maneja conexión, inserción y consultas
 */
class DatabaseManager {
public:
    /**
     * Constructor
     */
    DatabaseManager();
    
    /**
     * Destructor
     */
    ~DatabaseManager();
    
    /**
     * Conectar a la base de datos
     * 
     * @param host Host de MySQL
     * @param port Puerto (default: 3306)
     * @param database Nombre de la base de datos
     * @param user Usuario
     * @param password Contraseña
     * @return true si se conectó correctamente
     */
    bool connect(const std::string& host,
                int port,
                const std::string& database,
                const std::string& user,
                const std::string& password);
    
    /**
     * Desconectar de la base de datos
     */
    void disconnect();
    
    /**
     * Verificar si está conectado
     * 
     * @return true si está conectado
     */
    bool isConnected() const;
    
    /**
     * Insertar detección en la base de datos
     * 
     * @param detection Datos de la detección
     * @return true si se insertó correctamente
     */
    bool insertDetection(const DetectionData& detection);
    
    /**
     * Verificar si un vehículo está autorizado
     * 
     * @param plate_text Texto de la placa
     * @return true si está autorizado
     */
    bool isAuthorized(const std::string& plate_text);
    
    /**
     * Obtener detecciones recientes
     * 
     * @param hours Número de horas hacia atrás (default: 24)
     * @return Vector de detecciones
     */
    std::vector<DetectionData> getRecentDetections(int hours = 24);
    
    /**
     * Crear tablas si no existen
     * 
     * @return true si se crearon correctamente
     */
    bool createTablesIfNotExist();

private:
    MYSQL* connection_;
    bool connected_;
    
    /**
     * Ejecutar query SQL
     * 
     * @param query Query SQL
     * @return true si se ejecutó correctamente
     */
    bool executeQuery(const std::string& query);
    
    /**
     * Obtener timestamp actual en formato MySQL
     * 
     * @return Timestamp formateado
     */
    std::string getCurrentTimestamp() const;
    
    /**
     * Escapar string para SQL
     * 
     * @param str String a escapar
     * @return String escapado
     */
    std::string escapeString(const std::string& str) const;
};

} // namespace jetson_lpr

#endif // DATABASE_MANAGER_H

