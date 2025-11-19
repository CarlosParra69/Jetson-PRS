#!/bin/bash
# Script para ejecutar el sistema LPR
# Uso: bash scripts/run_lpr.sh [--config CONFIG_FILE]

set -e

# Obtener directorio del script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Cambiar al directorio raíz del proyecto
cd "$PROJECT_ROOT"

# Verificar que el ejecutable existe
EXECUTABLE="build/bin/jetson_lpr"
if [ ! -f "$EXECUTABLE" ]; then
    echo "❌ Error: El ejecutable no existe: $EXECUTABLE"
    echo "💡 Compila el proyecto primero:"
    echo "   cd build && cmake .. && make -j4"
    exit 1
fi

# Verificar que el archivo de configuración existe
CONFIG_FILE="${1:-config/default_config.json}"
if [ ! -f "$CONFIG_FILE" ]; then
    echo "⚠️  Advertencia: Archivo de configuración no encontrado: $CONFIG_FILE"
    echo "💡 Creando archivo de configuración por defecto..."
    mkdir -p config
    # El ConfigManager creará uno por defecto si no existe
fi

# Verificar que el modelo ONNX existe
if [ ! -f "models/license_plate_detector.onnx" ]; then
    echo "⚠️  Advertencia: Modelo ONNX no encontrado: models/license_plate_detector.onnx"
    echo "💡 Necesitas convertir tu modelo YOLO (.pt) a formato ONNX"
    echo "   Ver: scripts/convert_model_to_onnx.py"
fi

# Verificar MySQL
if ! systemctl is-active --quiet mysql 2>/dev/null; then
    echo "⚠️  Advertencia: MySQL no está corriendo"
    echo "💡 Inicia MySQL: sudo systemctl start mysql"
fi

echo "🚀 Ejecutando Sistema LPR..."
echo "📁 Directorio: $PROJECT_ROOT"
echo "⚙️  Configuración: $CONFIG_FILE"
echo ""

# Ejecutar el programa
if [ "$1" == "--config" ] && [ -n "$2" ]; then
    "$EXECUTABLE" --config "$2"
else
    "$EXECUTABLE" --config "$CONFIG_FILE"
fi

