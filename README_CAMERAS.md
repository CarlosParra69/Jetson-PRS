# Configuración de Cámaras - Sistema LPR

Este proyecto soporta múltiples configuraciones de cámaras. Puedes cambiar entre ellas fácilmente.

## 📹 Configuraciones Disponibles

### Cámara Actual (Nueva) - `config/default_config.json`
- **IP**: `192.168.0.100`
- **Usuario**: `admin`
- **Contraseña**: `tlJwpbo6`
- **URL RTSP**: `rtsp://192.168.0.100:554/user=admin_password=tlJwpbo6_channel=1_stream=0.sdp?real_stream`
- **Estado**: Activa (configuración por defecto)

### Cámara Legacy (Anterior) - `config/camera_legacy.json`
- **IP**: `192.168.1.101`
- **Usuario**: `admin`
- **Contraseña**: `admin`
- **URLs RTSP**:
  - Principal: `rtsp://admin:admin@192.168.1.101/cam/realmonitor?channel=1&subtype=1`
  - Alternativa: `rtsp://admin:admin@192.168.1.101/video2`
- **Fallback**: `./videos/video2.mp4`
- **Estado**: Consolidada (disponible para uso)

## 🔄 Cambiar entre Cámaras

### Opción 1: Usar argumento de línea de comandos

```bash
# Usar cámara nueva (por defecto)
./build/bin/jetson_lpr --config config/default_config.json

# Usar cámara legacy (anterior)
./build/bin/jetson_lpr --config config/camera_legacy.json
```

### Opción 2: Usar script de ejecución

```bash
# Cámara nueva
bash scripts/run_lpr.sh

# Cámara legacy
bash scripts/run_lpr.sh --config config/camera_legacy.json
```

### Opción 3: Cambiar el archivo por defecto

Si quieres que la cámara legacy sea la predeterminada:

```bash
# Hacer backup de la configuración actual
cp config/default_config.json config/default_config_new.json

# Usar la configuración legacy como predeterminada
cp config/camera_legacy.json config/default_config.json
```

## 📋 Estructura de Configuración

Ambas configuraciones incluyen:

```json
{
    "camera": {
        "ip": "...",
        "user": "...",
        "password": "...",
        "rtsp_url": "..."
    },
    "jetson": {
        "ip": "192.168.1.100",
        "interface": "enP8p1s0"
    },
    "processing": {
        "confidence_threshold": 0.30,
        "plate_confidence_min": 0.25,
        "detection_cooldown_sec": 0.5,
        "ocr_cache_enabled": true
    },
    "database": {
        "host": "localhost",
        "port": 3306,
        "database": "parqueadero_jetson",
        "user": "lpr_user",
        "password": "lpr_password"
    },
    "realtime_optimization": {
        "ai_process_every": 2,
        "motion_activation": true,
        "display_scale": 0.25,
        "headless_mode": true
    }
}
```

## 🔍 Probar Conexión de Cámaras

### Probar cámara nueva
```bash
bash scripts/test_rtsp.sh "rtsp://192.168.0.100:554/user=admin_password=tlJwpbo6_channel=1_stream=0.sdp?real_stream"
```

### Probar cámara legacy
```bash
bash scripts/test_rtsp.sh "rtsp://admin:admin@192.168.1.101/cam/realmonitor?channel=1&subtype=1"
```

## 📝 Notas

- La configuración legacy incluye URLs alternativas y fallback a video local
- Ambas configuraciones usan la misma base de datos MySQL
- Los parámetros de procesamiento son idénticos en ambas configuraciones
- Puedes crear más configuraciones copiando y modificando estos archivos

## 🆕 Agregar Nueva Cámara

Para agregar una nueva configuración de cámara:

1. Copia una configuración existente:
   ```bash
   cp config/default_config.json config/camera_nueva.json
   ```

2. Edita `config/camera_nueva.json` con los datos de tu nueva cámara

3. Ejecuta con la nueva configuración:
   ```bash
   ./build/bin/jetson_lpr --config config/camera_nueva.json
   ```

