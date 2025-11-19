#!/bin/bash
# Script completo de configuración para Sistema LPR C++
# Instala dependencias, configura MySQL y convierte modelo YOLO a ONNX

set -e  # Salir si hay error

echo "🚗 ==========================================================="
echo "   Sistema LPR C++ - Configuración Completa"
echo "==========================================================="
echo ""

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Función para imprimir mensajes
print_step() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

# 1. INSTALAR DEPENDENCIAS DEL SISTEMA
echo "📦 PASO 1: Instalando dependencias del sistema..."
echo ""

if [ -f /etc/debian_version ]; then
    # Debian/Ubuntu
    print_step "Detectado sistema Debian/Ubuntu"
    
    sudo apt update
    sudo apt install -y \
        build-essential \
        cmake \
        git \
        pkg-config \
        libopencv-dev \
        libopencv-contrib-dev \
        libtesseract-dev \
        libleptonica-dev \
        libmysqlclient-dev \
        mysql-client \
        mysql-server \
        python3-pip \
        python3-dev
    
    print_step "Dependencias del sistema instaladas"
    
elif [ -f /etc/redhat-release ]; then
    # RedHat/CentOS/Fedora
    print_warning "Sistema RedHat/CentOS detectado"
    print_warning "Por favor instala manualmente: opencv-devel, tesseract-devel, mysql-devel"
    
else
    print_error "Sistema operativo no reconocido"
    print_error "Por favor instala manualmente las dependencias"
    exit 1
fi

# Verificar instalaciones
echo ""
echo "🔍 Verificando instalaciones..."
echo ""

if command -v cmake &> /dev/null; then
    print_step "CMake instalado: $(cmake --version | head -n1)"
else
    print_error "CMake no encontrado"
    exit 1
fi

if pkg-config --exists opencv4; then
    OPENCV_VERSION=$(pkg-config --modversion opencv4)
    print_step "OpenCV instalado: $OPENCV_VERSION"
else
    print_error "OpenCV no encontrado"
    exit 1
fi

if command -v tesseract &> /dev/null; then
    print_step "Tesseract instalado: $(tesseract --version | head -n1)"
else
    print_error "Tesseract no encontrado"
    exit 1
fi

if mysql --version &> /dev/null; then
    print_step "MySQL instalado: $(mysql --version | head -n1)"
else
    print_error "MySQL no encontrado"
    exit 1
fi

# 2. CONFIGURAR BASE DE DATOS MySQL
echo ""
echo "💾 PASO 2: Configurando base de datos MySQL..."
echo ""

# Iniciar MySQL si no está corriendo
if ! sudo systemctl is-active --quiet mysql; then
    print_warning "Iniciando MySQL..."
    sudo systemctl start mysql
    sudo systemctl enable mysql
fi

# Crear base de datos y usuario
print_step "Creando base de datos y usuario..."

# Pedir contraseña root si es necesario
read -sp "Ingresa contraseña root de MySQL (Enter si no hay): " MYSQL_ROOT_PASS
echo ""

MYSQL_CMD="mysql -u root"
if [ ! -z "$MYSQL_ROOT_PASS" ]; then
    MYSQL_CMD="mysql -u root -p$MYSQL_ROOT_PASS"
fi

# SQL para crear base de datos y usuario
cat << EOF | sudo $MYSQL_CMD
CREATE DATABASE IF NOT EXISTS parqueadero_jetson;
CREATE USER IF NOT EXISTS 'lpr_user'@'localhost' IDENTIFIED BY 'lpr_password';
GRANT ALL PRIVILEGES ON parqueadero_jetson.* TO 'lpr_user'@'localhost';
FLUSH PRIVILEGES;
EOF

if [ $? -eq 0 ]; then
    print_step "Base de datos creada exitosamente"
    print_step "  Base de datos: parqueadero_jetson"
    print_step "  Usuario: lpr_user"
    print_step "  Contraseña: lpr_password"
else
    print_error "Error creando base de datos"
    print_warning "Puedes crearla manualmente después"
fi

# 3. CONVERTIR MODELO YOLO A ONNX
echo ""
echo "🔄 PASO 3: Convirtiendo modelo YOLO a ONNX..."
echo ""

# Buscar modelos .pt
cd "$(dirname "$0")/.."  # Ir al directorio raíz del proyecto

MODEL_DIR="."
MODEL_FILE=""

# Buscar modelo preferido
if [ -f "$MODEL_DIR/license_plate_detector.pt" ]; then
    MODEL_FILE="$MODEL_DIR/license_plate_detector.pt"
elif [ -f "../license_plate_detector.pt" ]; then
    MODEL_FILE="../license_plate_detector.pt"
fi

if [ -z "$MODEL_FILE" ]; then
    print_warning "No se encontró license_plate_detector.pt"
    print_warning "Buscando otros modelos..."
    
    if [ -f "$MODEL_DIR/yolo11n.pt" ]; then
        MODEL_FILE="$MODEL_DIR/yolo11n.pt"
    elif [ -f "../yolo11n.pt" ]; then
        MODEL_FILE="../yolo11n.pt"
    fi
fi

if [ -z "$MODEL_FILE" ]; then
    print_error "No se encontró ningún modelo .pt"
    print_warning "Por favor convierte manualmente con:"
    print_warning "  python scripts/convert_model_to_onnx.py <modelo.pt>"
    exit 1
fi

print_step "Modelo encontrado: $MODEL_FILE"

# Crear directorio models si no existe
mkdir -p models

# Instalar ultralytics si no está instalado
if ! python3 -c "import ultralytics" 2>/dev/null; then
    print_warning "Instalando Ultralytics para conversión..."
    pip3 install ultralytics --user
fi

# Convertir modelo
OUTPUT_MODEL="models/$(basename "$MODEL_FILE" .pt).onnx"
print_step "Convirtiendo a ONNX..."

python3 scripts/convert_model_to_onnx.py "$MODEL_FILE" "$OUTPUT_MODEL"

if [ $? -eq 0 ] && [ -f "$OUTPUT_MODEL" ]; then
    print_step "Modelo convertido exitosamente: $OUTPUT_MODEL"
    MODEL_SIZE=$(du -h "$OUTPUT_MODEL" | cut -f1)
    print_step "Tamaño: $MODEL_SIZE"
else
    print_error "Error convirtiendo modelo"
    exit 1
fi

# 4. RESUMEN FINAL
echo ""
echo "==========================================================="
echo "✅ CONFIGURACIÓN COMPLETA"
echo "==========================================================="
echo ""
print_step "Dependencias instaladas"
print_step "Base de datos MySQL configurada"
print_step "Modelo YOLO convertido a ONNX"
echo ""
echo "📋 Próximos pasos:"
echo "  1. Compilar proyecto:"
echo "     cd New-Lpr"
echo "     mkdir build && cd build"
echo "     cmake .."
echo "     make -j4"
echo ""
echo "  2. Ejecutar sistema:"
echo "     ./bin/jetson_lpr --config ../config/default_config.json"
echo ""
echo "🎉 ¡Todo listo para detectar placas!"
echo ""

