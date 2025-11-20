#!/bin/bash
# Script para ayudar a encontrar la URL RTSP correcta de una cámara
# Uso: bash scripts/find_rtsp_url.sh [IP] [USER] [PASSWORD]

set -e

CAMERA_IP="${1:-192.168.1.101}"
CAMERA_USER="${2:-admin}"
CAMERA_PASSWORD="${3:-admin}"

echo "🔍 Buscando URL RTSP para la cámara"
echo "===================================="
echo ""
echo "IP: $CAMERA_IP"
echo "Usuario: $CAMERA_USER"
echo "Contraseña: [oculta]"
echo ""

# Verificar conectividad
echo "1️⃣ Verificando conectividad..."
if ping -c 2 -W 2 "$CAMERA_IP" &> /dev/null; then
    echo "✅ La cámara responde al ping"
else
    echo "❌ No se puede hacer ping a la cámara"
    echo "💡 Verifica que estás en la misma red"
    exit 1
fi
echo ""

# Verificar puerto RTSP
echo "2️⃣ Verificando puerto RTSP (554)..."
if timeout 2 bash -c "echo > /dev/tcp/$CAMERA_IP/554" 2>/dev/null; then
    echo "✅ Puerto 554 está abierto"
else
    echo "⚠️  Puerto 554 no responde (puede estar bloqueado por firewall)"
fi
echo ""

# URLs RTSP comunes a probar
echo "3️⃣ URLs RTSP comunes para probar:"
echo ""

# Formato Hikvision
echo "📹 Hikvision:"
echo "   rtsp://$CAMERA_USER:$CAMERA_PASSWORD@$CAMERA_IP:554/Streaming/Channels/101"
echo "   rtsp://$CAMERA_USER:$CAMERA_PASSWORD@$CAMERA_IP:554/Streaming/Channels/102"
echo ""

# Formato Dahua
echo "📹 Dahua:"
echo "   rtsp://$CAMERA_USER:$CAMERA_PASSWORD@$CAMERA_IP:554/cam/realmonitor?channel=1&subtype=0"
echo "   rtsp://$CAMERA_USER:$CAMERA_PASSWORD@$CAMERA_IP:554/cam/realmonitor?channel=1&subtype=1"
echo ""

# Formato genérico
echo "📹 Genérico:"
echo "   rtsp://$CAMERA_USER:$CAMERA_PASSWORD@$CAMERA_IP:554/stream1"
echo "   rtsp://$CAMERA_USER:$CAMERA_PASSWORD@$CAMERA_IP:554/live"
echo "   rtsp://$CAMERA_USER:$CAMERA_PASSWORD@$CAMERA_IP:554/h264"
echo ""

# Intentar probar algunas URLs comunes
echo "4️⃣ Probando URLs comunes (esto puede tardar un momento)..."
echo ""

RTSP_URLS=(
    "rtsp://$CAMERA_USER:$CAMERA_PASSWORD@$CAMERA_IP:554/Streaming/Channels/101"
    "rtsp://$CAMERA_USER:$CAMERA_PASSWORD@$CAMERA_IP:554/Streaming/Channels/102"
    "rtsp://$CAMERA_USER:$CAMERA_PASSWORD@$CAMERA_IP:554/cam/realmonitor?channel=1&subtype=0"
    "rtsp://$CAMERA_USER:$CAMERA_PASSWORD@$CAMERA_IP:554/cam/realmonitor?channel=1&subtype=1"
    "rtsp://$CAMERA_USER:$CAMERA_PASSWORD@$CAMERA_IP:554/stream1"
    "rtsp://$CAMERA_USER:$CAMERA_PASSWORD@$CAMERA_IP:554/live"
)

WORKING_URL=""

for url in "${RTSP_URLS[@]}"; do
    echo -n "   Probando: $url ... "
    
    if command -v ffmpeg &> /dev/null; then
        if timeout 5 ffmpeg -i "$url" -t 1 -f null - 2>&1 | grep -q "Stream\|Video" &> /dev/null; then
            echo "✅ FUNCIONA!"
            WORKING_URL="$url"
            break
        else
            echo "❌"
        fi
    else
        # Intentar con Python/OpenCV si ffmpeg no está disponible
        python3 << EOF 2>/dev/null
import cv2
import sys
url = "$url"
cap = cv2.VideoCapture(url, cv2.CAP_FFMPEG)
if cap.isOpened():
    ret, frame = cap.read()
    if ret and not frame.empty():
        print("✅ FUNCIONA!")
        sys.exit(0)
    cap.release()
sys.exit(1)
EOF
        if [ $? -eq 0 ]; then
            WORKING_URL="$url"
            break
        else
            echo "❌"
        fi
    fi
done

echo ""

if [ -n "$WORKING_URL" ]; then
    echo "✅ ¡URL RTSP encontrada!"
    echo ""
    echo "📋 Copia esta URL a tu archivo de configuración:"
    echo "   $WORKING_URL"
    echo ""
    echo "O actualiza config/default_config.json con:"
    echo "   \"rtsp_url\": \"$WORKING_URL\""
else
    echo "⚠️  No se pudo encontrar una URL RTSP funcionando automáticamente"
    echo ""
    echo "💡 Próximos pasos:"
    echo "   1. Consulta el manual de tu cámara para la URL RTSP correcta"
    echo "   2. Accede a la interfaz web: http://$CAMERA_IP"
    echo "   3. Busca en la configuración de la cámara la sección 'RTSP' o 'Streaming'"
    echo "   4. Prueba manualmente con:"
    echo "      ffmpeg -i \"rtsp://...\" -t 5 -f null -"
fi

echo ""

