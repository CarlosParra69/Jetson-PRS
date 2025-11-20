#!/bin/bash
# Script para verificar conectividad de red con la cámara
# Uso: bash scripts/check_network.sh [CAMERA_IP]

set -e

CAMERA_IP="${1:-192.168.0.100}"

echo "🔍 Verificando conectividad de red"
echo "==================================="
echo ""
echo "IP de la cámara: $CAMERA_IP"
echo ""

# Obtener IP del Jetson
JETSON_IP=$(hostname -I | awk '{print $1}')
echo "IP del Jetson: $JETSON_IP"
echo ""

# Verificar si están en la misma red
JETSON_NETWORK=$(echo $JETSON_IP | cut -d. -f1-3)
CAMERA_NETWORK=$(echo $CAMERA_IP | cut -d. -f1-3)

if [ "$JETSON_NETWORK" != "$CAMERA_NETWORK" ]; then
    echo "⚠️  ADVERTENCIA: El Jetson y la cámara están en redes diferentes!"
    echo "   Jetson: $JETSON_NETWORK.x"
    echo "   Cámara: $CAMERA_NETWORK.x"
    echo ""
    echo "💡 Esto puede causar problemas de conectividad"
    echo ""
fi

# Verificar interfaces de red
echo "📡 Interfaces de red disponibles:"
ip addr show | grep -E "^[0-9]+:|inet " | grep -v "127.0.0.1"
echo ""

# Ping a la cámara
echo "1️⃣ Probando ping a la cámara..."
if ping -c 3 -W 2 "$CAMERA_IP" &> /dev/null; then
    echo "✅ Ping exitoso a $CAMERA_IP"
    ping -c 3 "$CAMERA_IP" | tail -1
else
    echo "❌ No se puede hacer ping a $CAMERA_IP"
    echo ""
    echo "💡 Posibles causas:"
    echo "   - La cámara está apagada"
    echo "   - Están en redes diferentes"
    echo "   - Firewall bloqueando ICMP"
    echo "   - IP incorrecta"
    echo ""
    echo "🔧 Soluciones:"
    echo "   1. Verifica que la cámara está encendida"
    echo "   2. Verifica la IP de la cámara en su interfaz web"
    echo "   3. Verifica la configuración de red del Jetson:"
    echo "      ip addr show"
    echo "   4. Prueba acceder a la interfaz web: http://$CAMERA_IP"
fi
echo ""

# Verificar puerto RTSP
echo "2️⃣ Verificando puerto RTSP (554)..."
if timeout 3 bash -c "echo > /dev/tcp/$CAMERA_IP/554" 2>/dev/null; then
    echo "✅ Puerto 554 está abierto y accesible"
else
    echo "⚠️  Puerto 554 no responde"
    echo "   Esto puede ser normal si la cámara requiere autenticación RTSP"
fi
echo ""

# Verificar puerto HTTP (80)
echo "3️⃣ Verificando puerto HTTP (80)..."
if timeout 3 bash -c "echo > /dev/tcp/$CAMERA_IP/80" 2>/dev/null; then
    echo "✅ Puerto 80 está abierto"
    echo "   Puedes acceder a la interfaz web: http://$CAMERA_IP"
else
    echo "⚠️  Puerto 80 no responde"
fi
echo ""

# Resumen
echo "📊 Resumen:"
if ping -c 1 -W 1 "$CAMERA_IP" &> /dev/null; then
    echo "✅ Conectividad básica: OK"
else
    echo "❌ Conectividad básica: FALLO"
    echo ""
    echo "🔧 Próximos pasos:"
    echo "   1. Verifica que la cámara está encendida"
    echo "   2. Verifica la configuración de red"
    echo "   3. Prueba conectar la cámara directamente al Jetson o al mismo switch"
    exit 1
fi

echo ""
echo "✅ Verificación completada"

