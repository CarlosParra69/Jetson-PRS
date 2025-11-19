#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>
#include <iomanip>
#include <memory>
#include "config_manager.h"
#include "plate_validator.h"
#include "lpr_system.h"

using namespace jetson_lpr;

// Variable global para manejar señales
static std::unique_ptr<LPRSystem> g_lpr_system = nullptr;

void signalHandler(int /*signal*/) {
    std::cout << "\n🛑 Señal recibida, deteniendo sistema..." << std::endl;
    if (g_lpr_system) {
        g_lpr_system->stop();
    }
    exit(0);
}

void printUsage(const char* program_name) {
    std::cout << "Uso: " << program_name << " [OPCIONES]\n"
              << "\n"
              << "OPCIONES:\n"
              << "  -h, --help                  Mostrar esta ayuda\n"
              << "  --config CONFIG             Archivo de configuración (default: config/default_config.json)\n"
              << "  --ai-every AI_EVERY         Procesar IA cada N frames (default: 2)\n"
              << "  --cooldown COOLDOWN         Cooldown en segundos (default: 0.5)\n"
              << "  --confidence CONFIDENCE     Umbral confianza detección (default: 0.30)\n"
              << "  --headless                  Modo sin GUI (recomendado para Jetson)\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "🚗 Sistema LPR (License Plate Recognition) - Versión C++\n"
              << "===========================================================\n"
              << std::endl;
    
    // Registrar manejador de señales
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Parsear argumentos de línea de comandos
    std::string config_path = "config/default_config.json";
    int ai_every = 2;
    double cooldown = 0.5;
    double confidence = 0.30;
    bool headless = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        } else if (arg == "--ai-every" && i + 1 < argc) {
            ai_every = std::stoi(argv[++i]);
        } else if (arg == "--cooldown" && i + 1 < argc) {
            cooldown = std::stod(argv[++i]);
        } else if (arg == "--confidence" && i + 1 < argc) {
            confidence = std::stod(argv[++i]);
        } else if (arg == "--headless") {
            headless = true;
        } else {
            std::cerr << "Opción desconocida: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Crear e inicializar sistema LPR
    g_lpr_system = std::make_unique<LPRSystem>(config_path);
    
    // Configurar parámetros desde línea de comandos
    // (Esto requeriría métodos adicionales en ConfigManager, por ahora se usan los valores del archivo)
    
    // Inicializar sistema
    std::cout << "🔧 Inicializando sistema..." << std::endl;
    if (!g_lpr_system->initialize()) {
        std::cerr << "❌ Error: No se pudo inicializar el sistema LPR" << std::endl;
        return 1;
    }
    
    // Iniciar sistema
    std::cout << "🚀 Iniciando procesamiento en tiempo real..." << std::endl;
    g_lpr_system->start();
    
    // Bucle principal
    std::cout << "\n📊 Sistema LPR en ejecución. Presiona Ctrl+C para detener.\n" << std::endl;
    
    while (g_lpr_system->isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // Mostrar estadísticas cada 5 segundos
        auto stats = g_lpr_system->getStats();
        
        std::cout << "📈 Estadísticas:" << std::endl;
        std::cout << "   Frames capturados: " << stats.total_frames << std::endl;
        std::cout << "   Frames procesados (IA): " << stats.ai_frames << std::endl;
        std::cout << "   Detecciones: " << stats.detections_count << std::endl;
        std::cout << "   FPS captura: " << std::fixed << std::setprecision(1) << stats.capture_fps << std::endl;
        std::cout << "   FPS IA: " << std::fixed << std::setprecision(1) << stats.ai_fps << std::endl;
        std::cout << std::endl;
    }
    
    // Detener sistema
    g_lpr_system->stop();
    
    // Mostrar estadísticas finales
    auto final_stats = g_lpr_system->getStats();
    std::cout << "\n📊 Estadísticas finales:" << std::endl;
    std::cout << "   Total frames: " << final_stats.total_frames << std::endl;
    std::cout << "   Frames IA: " << final_stats.ai_frames << std::endl;
    std::cout << "   Detecciones totales: " << final_stats.detections_count << std::endl;
    std::cout << "   FPS promedio captura: " << std::fixed << std::setprecision(1) << final_stats.capture_fps << std::endl;
    std::cout << "   FPS promedio IA: " << std::fixed << std::setprecision(1) << final_stats.ai_fps << std::endl;
    
    std::cout << "\n✅ Sistema LPR finalizado correctamente" << std::endl;
    
    return 0;
}
