#!/bin/bash
# Script para instalar dependencias del sistema para Sistema LPR C++
# Compatible con Ubuntu/Debian y Jetson Orin Nano

set -e  # Salir si hay error

echo "🚀 Instalando dependencias para Sistema LPR C++"
echo "================================================"
echo ""

# Detectar distribución
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
else
    echo "⚠️  No se pudo detectar la distribución, asumiendo Ubuntu"
    DISTRO="ubuntu"
fi

echo "📦 Distribución detectada: $DISTRO"
echo ""

# Actualizar sistema
echo "🔄 Actualizando sistema..."
sudo apt update

# Dependencias base
echo ""
echo "📦 Instalando dependencias base..."
sudo apt install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    wget \
    curl \
    unzip

# OpenCV
echo ""
echo "📦 Instalando OpenCV..."

# Detectar si es Jetson
IS_JETSON=false
if [ -f /etc/nv_tegra_release ] || [ -n "$(uname -a | grep -i jetson)" ] || [ -n "$(uname -a | grep -i tegra)" ]; then
    IS_JETSON=true
    echo "🔍 Jetson detectado - usando método de instalación especial"
fi

# Intentar corregir dependencias rotas primero
echo "🔄 Corrigiendo dependencias rotas..."
sudo apt-get install -f -y || true

# Método 1: Intentar instalar OpenCV completo (incluyendo contrib)
echo "📦 Intentando instalar OpenCV completo..."
if sudo apt install -y libopencv-dev libopencv-contrib-dev python3-opencv 2>&1 | grep -q "unmet dependencies\|held broken packages"; then
    echo "⚠️  Dependencias rotas detectadas, intentando método alternativo..."
    
    # Método 2: Instalar solo paquetes base primero
    echo "📦 Instalando paquetes base de OpenCV..."
    sudo apt install -y libopencv-dev python3-opencv || true
    
    # Método 3: Intentar instalar versiones específicas que coincidan
    echo "📦 Intentando instalar versiones compatibles..."
    OPENCV_BASE_VERSION=$(apt-cache policy libopencv-dev | grep "Installed\|Candidate" | head -1 | awk '{print $2}' | cut -d: -f2 || echo "")
    
    if [ -n "$OPENCV_BASE_VERSION" ]; then
        echo "🔍 Versión base encontrada: $OPENCV_BASE_VERSION"
        # Intentar instalar contrib con la misma versión
        sudo apt install -y \
            libopencv-contrib-dev=${OPENCV_BASE_VERSION} \
            libopencv-calib3d-dev=${OPENCV_BASE_VERSION} \
            libopencv-core-dev=${OPENCV_BASE_VERSION} \
            libopencv-dnn-dev=${OPENCV_BASE_VERSION} \
            libopencv-features2d-dev=${OPENCV_BASE_VERSION} \
            libopencv-flann-dev=${OPENCV_BASE_VERSION} \
            libopencv-highgui-dev=${OPENCV_BASE_VERSION} \
            libopencv-imgcodecs-dev=${OPENCV_BASE_VERSION} \
            libopencv-imgproc-dev=${OPENCV_BASE_VERSION} \
            libopencv-ml-dev=${OPENCV_BASE_VERSION} \
            libopencv-objdetect-dev=${OPENCV_BASE_VERSION} \
            libopencv-photo-dev=${OPENCV_BASE_VERSION} \
            libopencv-stitching-dev=${OPENCV_BASE_VERSION} \
            libopencv-video-dev=${OPENCV_BASE_VERSION} \
            libopencv-videoio-dev=${OPENCV_BASE_VERSION} || {
            echo "⚠️  No se pudo instalar contrib, continuando con versión base..."
        }
    else
        echo "⚠️  No se pudo determinar versión, instalando solo paquetes base..."
    fi
    
    # Método 4: Si es Jetson, verificar si OpenCV ya está instalado por JetPack
    if [ "$IS_JETSON" = true ]; then
        echo "🔍 Verificando instalación de OpenCV en Jetson..."
        if pkg-config --exists opencv4 || pkg-config --exists opencv; then
            echo "✅ OpenCV ya está disponible en el sistema (probablemente instalado por JetPack)"
        fi
    fi
else
    echo "✅ OpenCV completo instalado correctamente"
fi

# Verificar versión OpenCV
OPENCV_VERSION=$(pkg-config --modversion opencv4 2>/dev/null || pkg-config --modversion opencv 2>/dev/null || echo "desconocida")
echo "✅ OpenCV version: $OPENCV_VERSION"

# Tesseract OCR
echo ""
echo "📦 Instalando Tesseract OCR..."
sudo apt install -y \
    tesseract-ocr \
    libtesseract-dev \
    libleptonica-dev

# Verificar Tesseract
TESSERACT_VERSION=$(tesseract --version 2>/dev/null | head -n1 || echo "desconocida")
echo "✅ Tesseract: $TESSERACT_VERSION"

# MySQL
echo ""
echo "📦 Instalando MySQL..."
sudo apt install -y \
    mysql-server \
    mysql-client \
    libmysqlclient-dev

# Verificar MySQL
if systemctl is-active --quiet mysql; then
    echo "✅ MySQL está corriendo"
else
    echo "🔄 Iniciando MySQL..."
    sudo systemctl start mysql
    sudo systemctl enable mysql
fi

# Python y dependencias para conversión de modelos
echo ""
echo "📦 Instalando dependencias Python (para conversión de modelos)..."
sudo apt install -y \
    python3 \
    python3-pip \
    python3-dev

# Instalar Ultralytics para conversión de modelos
echo ""
echo "📦 Instalando Ultralytics (para conversión de modelos YOLO)..."
pip3 install --user ultralytics onnx

# nlohmann/json (descargar si no existe)
echo ""
echo "📦 Descargando nlohmann/json..."
mkdir -p third_party/nlohmann
if [ ! -f third_party/nlohmann/json.hpp ]; then
    wget -q -O third_party/nlohmann/json.hpp \
        https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
    echo "✅ nlohmann/json descargado"
else
    echo "✅ nlohmann/json ya existe"
fi

# Verificaciones finales
echo ""
echo "🔍 Verificando instalación..."
echo ""

# Verificar CMake
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1)
    echo "✅ $CMAKE_VERSION"
else
    echo "❌ CMake no encontrado"
fi

# Verificar compilador C++
if command -v g++ &> /dev/null; then
    GCC_VERSION=$(g++ --version | head -n1)
    echo "✅ $GCC_VERSION"
else
    echo "❌ g++ no encontrado"
fi

# Verificar OpenCV
if pkg-config --exists opencv4 || pkg-config --exists opencv; then
    echo "✅ OpenCV configurado correctamente"
else
    echo "⚠️  OpenCV no encontrado en pkg-config"
fi

# Verificar Tesseract
if command -v tesseract &> /dev/null; then
    echo "✅ Tesseract instalado"
else
    echo "❌ Tesseract no encontrado"
fi

# Verificar MySQL
if mysql --version &> /dev/null; then
    MYSQL_VERSION=$(mysql --version)
    echo "✅ $MYSQL_VERSION"
else
    echo "❌ MySQL no encontrado"
fi

echo ""
echo "✅ Instalación de dependencias completada!"
echo ""
echo "📝 Próximos pasos:"
echo "   1. Convertir modelo YOLO a ONNX:"
echo "      python3 scripts/convert_model_to_onnx.py ../license_plate_detector.pt"
echo ""
echo "   2. Configurar base de datos MySQL:"
echo "      bash scripts/setup_mysql.sh"
echo ""
echo "   3. Compilar proyecto:"
echo "      mkdir build && cd build"
echo "      cmake .."
echo "      make -j4"
echo ""

