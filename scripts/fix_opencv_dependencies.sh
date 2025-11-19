#!/bin/bash
# Script para resolver dependencias rotas de OpenCV en Jetson/Ubuntu
# Ejecutar este script si tienes errores de "unmet dependencies" o "held broken packages"

# No usar set -e porque algunos comandos pueden fallar intencionalmente

echo "🔧 Solucionando dependencias rotas de OpenCV"
echo "============================================="
echo ""

# Actualizar lista de paquetes
echo "🔄 Actualizando lista de paquetes..."
sudo apt update

# Intentar corregir dependencias automáticamente
echo ""
echo "🔄 Intentando corregir dependencias automáticamente..."
sudo apt-get install -f -y || true

# Obtener versión de libopencv-dev instalada o disponible
echo ""
echo "🔍 Detectando versión de OpenCV disponible..."
OPENCV_VERSION=$(apt-cache policy libopencv-dev | grep "Candidate" | awk '{print $2}' | cut -d: -f2)

if [ -z "$OPENCV_VERSION" ]; then
    OPENCV_VERSION=$(apt-cache policy libopencv-dev | grep "Installed" | awk '{print $2}' | cut -d: -f2)
fi

if [ -z "$OPENCV_VERSION" ]; then
    echo "❌ No se pudo detectar versión de OpenCV"
    echo "📦 Instalando versión base de OpenCV..."
    sudo apt install -y libopencv-dev python3-opencv
    OPENCV_VERSION=$(apt-cache policy libopencv-dev | grep "Installed" | awk '{print $2}' | cut -d: -f2)
fi

echo "✅ Versión detectada: $OPENCV_VERSION"
echo ""

# Método 1: Instalar solo paquetes base (más seguro)
echo "📦 Método 1: Instalando solo paquetes base de OpenCV..."
sudo apt install -y \
    libopencv-dev \
    python3-opencv || {
    echo "⚠️  Método 1 falló"
}

# Verificar si contrib es necesario
echo ""
echo "🔍 Verificando si libopencv-contrib-dev es necesario..."
if pkg-config --exists opencv4 || pkg-config --exists opencv; then
    echo "✅ OpenCV base está funcionando"
    
    # Intentar instalar contrib con versión específica
    echo ""
    echo "📦 Intentando instalar libopencv-contrib-dev con versión específica..."
    if sudo apt install -y libopencv-contrib-dev=${OPENCV_VERSION} 2>&1 | grep -q "unmet dependencies"; then
        echo "⚠️  No se puede instalar contrib con esta versión"
        echo "💡 Continuando sin contrib (puede que no sea necesario para tu proyecto)"
    else
        echo "✅ libopencv-contrib-dev instalado correctamente"
    fi
else
    echo "❌ OpenCV base no está funcionando correctamente"
fi

# Método alternativo: Si el método anterior falla, intentar instalar todas las dependencias específicas
echo ""
echo "📦 Método 2: Instalando dependencias específicas con versión ${OPENCV_VERSION}..."
sudo apt install -y \
    libopencv-calib3d-dev=${OPENCV_VERSION} \
    libopencv-core-dev=${OPENCV_VERSION} \
    libopencv-dnn-dev=${OPENCV_VERSION} \
    libopencv-features2d-dev=${OPENCV_VERSION} \
    libopencv-flann-dev=${OPENCV_VERSION} \
    libopencv-highgui-dev=${OPENCV_VERSION} \
    libopencv-imgcodecs-dev=${OPENCV_VERSION} \
    libopencv-imgproc-dev=${OPENCV_VERSION} \
    libopencv-ml-dev=${OPENCV_VERSION} \
    libopencv-objdetect-dev=${OPENCV_VERSION} \
    libopencv-photo-dev=${OPENCV_VERSION} \
    libopencv-stitching-dev=${OPENCV_VERSION} \
    libopencv-video-dev=${OPENCV_VERSION} \
    libopencv-videoio-dev=${OPENCV_VERSION} \
    libopencv-contrib-dev=${OPENCV_VERSION} 2>&1 | grep -v "already the newest version" || {
    echo "⚠️  Algunas dependencias no pudieron instalarse con versión específica"
}

# Verificación final
echo ""
echo "🔍 Verificando instalación de OpenCV..."
if pkg-config --exists opencv4 || pkg-config --exists opencv; then
    OPENCV_VER=$(pkg-config --modversion opencv4 2>/dev/null || pkg-config --modversion opencv 2>/dev/null)
    echo "✅ OpenCV está instalado y funcionando: versión $OPENCV_VER"
    
    # Verificar módulos disponibles
    echo ""
    echo "📋 Módulos de OpenCV disponibles:"
    pkg-config --list-all | grep opencv || true
else
    echo "❌ OpenCV no está correctamente configurado"
    echo ""
    echo "💡 Soluciones alternativas:"
    echo "   1. Verificar si OpenCV está instalado por JetPack (en Jetson):"
    echo "      pkg-config --modversion opencv4"
    echo ""
    echo "   2. Instalar solo lo esencial:"
    echo "      sudo apt install -y libopencv-dev python3-opencv"
    echo ""
    echo "   3. Si necesitas contrib, considera compilar OpenCV desde el código fuente"
fi

echo ""
echo "✅ Proceso completado!"

