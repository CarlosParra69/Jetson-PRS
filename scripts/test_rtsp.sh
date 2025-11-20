#!/bin/bash
# Script para probar conexión RTSP a una cámara
# Uso: bash scripts/test_rtsp.sh [RTSP_URL]

set -e

# URL RTSP por defecto (desde config)
RTSP_URL="${1:-rtsp://192.168.0.100:554/user=admin_password=tlJwpbo6_channel=1_stream=0.sdp?real_stream}"

echo "🔍 Probando conexión RTSP"
echo "=========================="
echo ""
echo "URL: $RTSP_URL"
echo ""

# Verificar si ffmpeg está instalado
if ! command -v ffmpeg &> /dev/null; then
    echo "⚠️  ffmpeg no está instalado"
    echo "💡 Instálalo con: sudo apt install ffmpeg"
    echo ""
    echo "Intentando con OpenCV Python como alternativa..."
    
    # Intentar con Python/OpenCV
    python3 << EOF
import cv2
import sys

url = "$RTSP_URL"
print(f"Intentando conectar a: {url}")

cap = cv2.VideoCapture(url, cv2.CAP_FFMPEG)
if cap.isOpened():
    print("✅ Conexión exitosa!")
    ret, frame = cap.read()
    if ret and not frame.empty():
        print(f"✅ Frame leído: {frame.shape}")
    else:
        print("⚠️  Conexión OK pero no se pueden leer frames")
    cap.release()
else:
    print("❌ No se pudo conectar")
    sys.exit(1)
EOF
    exit $?
fi

# Probar con ffmpeg
echo "📡 Probando con ffmpeg..."
echo ""

# Intentar capturar 5 segundos de video
if ffmpeg -i "$RTSP_URL" -t 5 -f null - 2>&1 | grep -q "Stream\|Duration\|frame"; then
    echo "✅ Conexión RTSP exitosa!"
    echo ""
    echo "Información del stream:"
    ffmpeg -i "$RTSP_URL" -t 1 2>&1 | grep -E "Stream|Duration|Video|Audio" | head -5
else
    echo "❌ No se pudo conectar a la cámara RTSP"
    echo ""
    echo "💡 Posibles soluciones:"
    echo "   1. Verifica que la IP de la cámara es correcta: $(echo $RTSP_URL | grep -oP 'rtsp://[^@]+@\K[^/]+')"
    echo "   2. Verifica las credenciales (usuario/contraseña)"
    echo "   3. Verifica que la cámara está encendida y en la misma red"
    echo "   4. Prueba acceder desde un navegador: http://$(echo $RTSP_URL | grep -oP 'rtsp://[^@]+@\K[^/]+')"
    exit 1
fi

echo ""
echo "✅ Prueba completada"


