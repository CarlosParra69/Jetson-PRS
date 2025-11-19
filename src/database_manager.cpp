#include "database_manager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace jetson_lpr {

DatabaseManager::DatabaseManager()
    : connection_(nullptr)
    , connected_(false)
{
}

DatabaseManager::~DatabaseManager() {
    disconnect();
}

bool DatabaseManager::connect(const std::string& host,
                              int port,
                              const std::string& database,
                              const std::string& user,
                              const std::string& password) {
    if (connected_) {
        disconnect();
    }
    
    // Inicializar MySQL
    connection_ = mysql_init(nullptr);
    if (!connection_) {
        std::cerr << "Error: No se pudo inicializar MySQL" << std::endl;
        return false;
    }
    
    // Configurar timeout de conexión
    unsigned int timeout = 5;
    mysql_options(connection_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    
    // Conectar a la base de datos
    my_bool reconnect = 1;
    mysql_options(connection_, MYSQL_OPT_RECONNECT, &reconnect);
    
    MYSQL* result = mysql_real_connect(
        connection_,
        host.c_str(),
        user.c_str(),
        password.c_str(),
        database.c_str(),
        port,
        nullptr,
        CLIENT_FOUND_ROWS
    );
    
    if (!result) {
        std::cerr << "Error: No se pudo conectar a MySQL: " 
                  << mysql_error(connection_) << std::endl;
        mysql_close(connection_);
        connection_ = nullptr;
        return false;
    }
    
    // Configurar charset UTF-8
    mysql_set_character_set(connection_, "utf8mb4");
    
    connected_ = true;
    std::cout << "✅ Conectado a MySQL: " << host << ":" << port 
              << "/" << database << std::endl;
    
    // Crear tablas si no existen
    createTablesIfNotExist();
    
    return true;
}

void DatabaseManager::disconnect() {
    if (connection_) {
        mysql_close(connection_);
        connection_ = nullptr;
    }
    connected_ = false;
}

bool DatabaseManager::isConnected() const {
    return connected_ && connection_ != nullptr;
}

bool DatabaseManager::insertDetection(const DetectionData& detection) {
    if (!isConnected()) {
        std::cerr << "Error: No hay conexión a la base de datos" << std::endl;
        return false;
    }
    
    try {
        std::ostringstream query;
        query << "INSERT INTO lpr_detections "
              << "(timestamp, plate_text, confidence, plate_score, "
              << "vehicle_bbox, plate_bbox, camera_location) "
              << "VALUES (";
        
        // Timestamp
        std::string timestamp = detection.timestamp.empty() ? 
                               getCurrentTimestamp() : detection.timestamp;
        query << "'" << escapeString(timestamp) << "', ";
        
        // Plate text
        query << "'" << escapeString(detection.plate_text) << "', ";
        
        // Confidence (YOLO)
        query << detection.yolo_confidence << ", ";
        
        // Plate score (OCR)
        query << detection.ocr_confidence << ", ";
        
        // Vehicle bbox (JSON)
        query << "'["
              << detection.vehicle_bbox[0] << ","
              << detection.vehicle_bbox[1] << ","
              << detection.vehicle_bbox[2] << ","
              << detection.vehicle_bbox[3]
              << "]', ";
        
        // Plate bbox (JSON)
        query << "'["
              << detection.plate_bbox[0] << ","
              << detection.plate_bbox[1] << ","
              << detection.plate_bbox[2] << ","
              << detection.plate_bbox[3]
              << "]', ";
        
        // Camera location
        query << "'" << escapeString(detection.camera_location) << "'";
        
        query << ")";
        
        if (executeQuery(query.str())) {
            std::cout << "✅ Detección insertada: " << detection.plate_text << std::endl;
            return true;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "Error insertando detección: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::isAuthorized(const std::string& plate_text) {
    if (!isConnected()) {
        return false;
    }
    
    try {
        std::ostringstream query;
        query << "SELECT authorized FROM registered_vehicles "
              << "WHERE plate_number = '" << escapeString(plate_text) << "' "
              << "AND (authorization_start IS NULL OR authorization_start <= CURDATE()) "
              << "AND (authorization_end IS NULL OR authorization_end >= CURDATE()) "
              << "LIMIT 1";
        
        if (mysql_query(connection_, query.str().c_str()) != 0) {
            std::cerr << "Error en consulta: " << mysql_error(connection_) << std::endl;
            return false;
        }
        
        MYSQL_RES* result = mysql_store_result(connection_);
        if (!result) {
            return false;
        }
        
        MYSQL_ROW row = mysql_fetch_row(result);
        bool authorized = false;
        
        if (row && row[0]) {
            authorized = (std::string(row[0]) == "1");
        }
        
        mysql_free_result(result);
        return authorized;
        
    } catch (const std::exception& e) {
        std::cerr << "Error verificando autorización: " << e.what() << std::endl;
        return false;
    }
}

std::vector<DetectionData> DatabaseManager::getRecentDetections(int hours) {
    std::vector<DetectionData> detections;
    
    if (!isConnected()) {
        return detections;
    }
    
    try {
        std::ostringstream query;
        query << "SELECT timestamp, plate_text, confidence, plate_score, "
              << "vehicle_bbox, plate_bbox, camera_location "
              << "FROM lpr_detections "
              << "WHERE timestamp >= DATE_SUB(NOW(), INTERVAL " << hours << " HOUR) "
              << "ORDER BY timestamp DESC "
              << "LIMIT 1000";
        
        if (mysql_query(connection_, query.str().c_str()) != 0) {
            std::cerr << "Error en consulta: " << mysql_error(connection_) << std::endl;
            return detections;
        }
        
        MYSQL_RES* result = mysql_store_result(connection_);
        if (!result) {
            return detections;
        }
        
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            DetectionData detection;
            
            if (row[0]) detection.timestamp = row[0];
            if (row[1]) detection.plate_text = row[1];
            if (row[2]) detection.yolo_confidence = std::stof(row[2]);
            if (row[3]) detection.ocr_confidence = std::stof(row[3]);
            if (row[4]) {
                // Parsear vehicle_bbox JSON [x,y,w,h]
                std::string bbox_str = row[4];
                // Simplificación: buscar números
                std::istringstream iss(bbox_str);
                char c;
                iss >> c; // '['
                iss >> detection.vehicle_bbox[0] >> c 
                    >> detection.vehicle_bbox[1] >> c
                    >> detection.vehicle_bbox[2] >> c
                    >> detection.vehicle_bbox[3];
            }
            if (row[5]) {
                // Parsear plate_bbox JSON
                std::string bbox_str = row[5];
                std::istringstream iss(bbox_str);
                char c;
                iss >> c; // '['
                iss >> detection.plate_bbox[0] >> c 
                    >> detection.plate_bbox[1] >> c
                    >> detection.plate_bbox[2] >> c
                    >> detection.plate_bbox[3];
            }
            if (row[6]) detection.camera_location = row[6];
            
            detections.push_back(detection);
        }
        
        mysql_free_result(result);
        
    } catch (const std::exception& e) {
        std::cerr << "Error obteniendo detecciones: " << e.what() << std::endl;
    }
    
    return detections;
}

bool DatabaseManager::createTablesIfNotExist() {
    if (!isConnected()) {
        return false;
    }
    
    // Crear tabla de detecciones
    std::string create_detections = R"(
        CREATE TABLE IF NOT EXISTS lpr_detections (
            id INT AUTO_INCREMENT PRIMARY KEY,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            plate_text VARCHAR(10) NOT NULL,
            confidence FLOAT,
            plate_score FLOAT,
            vehicle_bbox TEXT,
            plate_bbox TEXT,
            camera_location VARCHAR(100) DEFAULT 'entrada_principal',
            processed BOOLEAN DEFAULT FALSE,
            entry_type ENUM('entrada', 'salida') DEFAULT 'entrada',
            
            INDEX idx_timestamp (timestamp),
            INDEX idx_plate (plate_text),
            INDEX idx_location (camera_location)
        )
    )";
    
    if (!executeQuery(create_detections)) {
        return false;
    }
    
    // Crear tabla de vehículos registrados
    std::string create_vehicles = R"(
        CREATE TABLE IF NOT EXISTS registered_vehicles (
            id INT AUTO_INCREMENT PRIMARY KEY,
            plate_number VARCHAR(10) UNIQUE NOT NULL,
            owner_name VARCHAR(100),
            owner_phone VARCHAR(20),
            vehicle_type ENUM('particular', 'moto', 'diplomatico', 'comercial') DEFAULT 'particular',
            vehicle_brand VARCHAR(50),
            vehicle_color VARCHAR(30),
            authorized BOOLEAN DEFAULT TRUE,
            authorization_start DATE,
            authorization_end DATE,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
            notes TEXT,
            
            INDEX idx_plate (plate_number),
            INDEX idx_authorized (authorized)
        )
    )";
    
    if (!executeQuery(create_vehicles)) {
        return false;
    }
    
    // Crear tabla de log de acceso
    std::string create_access_log = R"(
        CREATE TABLE IF NOT EXISTS access_log (
            id INT AUTO_INCREMENT PRIMARY KEY,
            detection_id INT,
            plate_number VARCHAR(10) NOT NULL,
            access_granted BOOLEAN DEFAULT FALSE,
            access_reason VARCHAR(100),
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            camera_location VARCHAR(100),
            
            FOREIGN KEY (detection_id) REFERENCES lpr_detections(id),
            INDEX idx_plate (plate_number),
            INDEX idx_timestamp (timestamp)
        )
    )";
    
    if (!executeQuery(create_access_log)) {
        return false;
    }
    
    std::cout << "✅ Tablas de base de datos verificadas/creadas" << std::endl;
    return true;
}

bool DatabaseManager::executeQuery(const std::string& query) {
    if (!isConnected()) {
        return false;
    }
    
    if (mysql_query(connection_, query.c_str()) != 0) {
        std::cerr << "Error ejecutando query: " << mysql_error(connection_) << std::endl;
        std::cerr << "Query: " << query << std::endl;
        return false;
    }
    
    return true;
}

std::string DatabaseManager::getCurrentTimestamp() const {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string DatabaseManager::escapeString(const std::string& str) const {
    if (!connection_) {
        return str;
    }
    
    char* escaped = new char[str.length() * 2 + 1];
    unsigned long length = mysql_real_escape_string(connection_, escaped, str.c_str(), str.length());
    
    std::string result(escaped, length);
    delete[] escaped;
    
    return result;
}

} // namespace jetson_lpr

