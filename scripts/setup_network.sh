#!/bin/bash
# Script para configurar la red antes de ejecutar el sistema LPR
# Basado en ptz_startup.sh - Configura la red para conectar con la cámara PTZ
# Uso: bash scripts/setup_network.sh

set -e

echo "==============================================="
echo "🌐 CONFIGURACIÓN DE RED - JETSON ORIN NANO"
echo "==============================================="
echo ""

# Configuración de variables (desde ptz_startup.sh)
INTERFACE="enP8p1s0"
JETSON_IP="192.168.1.100"
CAMERA_IP="192.168.1.101"

# Función para verificar si el comando fue exitoso
check_command() {
    if [ $? -eq 0 ]; then
        echo "✅ $1"
    else
        echo "⚠️  Advertencia: $1 (continuando...)"
    fi
}

# Función para mostrar progreso
show_progress() {
    echo "🔄 $1..."
}

echo "📋 Configuración:"
echo "   - Interfaz: $INTERFACE"
echo "   - IP Jetson: $JETSON_IP"
echo "   - IP Cámara: $CAMERA_IP"
echo ""

# Verificar que la interfaz existe
if ! ip link show $INTERFACE &> /dev/null; then
    echo "❌ Error: La interfaz $INTERFACE no existe"
    echo "💡 Interfaces disponibles:"
    ip link show | grep -E "^[0-9]+:" | awk '{print "   - " $2}'
    exit 1
fi

# ==========================================
# 1. CONFIGURACIÓN DE RED
# ==========================================
show_progress "Configurando interfaz de red"

# Verificar si ya está configurada
CURRENT_IP=$(ip addr show $INTERFACE | grep "inet " | awk '{print $2}' | cut -d/ -f1)

if [ "$CURRENT_IP" == "$JETSON_IP" ]; then
    echo "✅ La IP ya está configurada: $JETSON_IP"
else
    # Limpiar configuración previa (si existe)
    if [ -n "$CURRENT_IP" ]; then
        echo "🔄 Limpiando configuración anterior: $CURRENT_IP"
        sudo ip addr flush dev $INTERFACE 2>/dev/null || true
    fi
    
    # Asignar IP a la Jetson
    sudo ip addr add ${JETSON_IP}/24 dev $INTERFACE
    check_command "Asignación de IP ${JETSON_IP}"
    
    # Activar interfaz
    sudo ip link set $INTERFACE up
    check_command "Activación de interfaz"
fi

# Configurar parámetros del enlace Ethernet (opcional)
if command -v ethtool &> /dev/null; then
    sudo ethtool -s $INTERFACE speed 100 duplex full autoneg off 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "✅ Configuración Ethernet optimizada"
    fi
fi

# Verificar que la interfaz esté activa
if ip addr show $INTERFACE | grep -q $JETSON_IP; then
    echo "✅ Configuración de red verificada"
else
    echo "❌ Error: No se pudo verificar la configuración de red"
    exit 1
fi

echo ""

# ==========================================
# 2. VERIFICACIÓN DE CONECTIVIDAD
# ==========================================
show_progress "Verificando conectividad con la cámara"

# Esperar un momento para que la red se estabilice
sleep 2

# Verificar conectividad básica
echo "🏓 Probando conectividad..."
if ping -c 2 -W 3 $CAMERA_IP > /dev/null 2>&1; then
    echo "✅ Ping exitoso a $CAMERA_IP"
else
    echo "⚠️  Sin respuesta al ping (puede ser normal si la cámara bloquea ICMP)"
    echo "💡 Continuando de todas formas..."
fi

echo ""
echo "✅ Configuración de red completada"
echo ""

