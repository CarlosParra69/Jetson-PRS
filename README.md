# Sistema de Reconocimiento de Placas Con Inteligencia Articifial (Plate Recognition System) - Versión C++

Sistema de reconocimiento automático de placas de vehículos colombianos en tiempo real, reescrito en C++ para mejorar el rendimiento y la detección.

## 📋 Características

* ✅ Detección automática de placas de vehículos en tiempo real
* ✅ Reconocimiento óptico de caracteres (OCR) para extraer texto
* ✅ Validación y verificación de placas autorizadas
* ✅ Integración con base de datos MySQL para gestión de vehículos
* ✅ Optimizado para cámaras PTZ y sistemas de estacionamiento
* ✅ Optimizado para Jetson Orin Nano con soporte CUDA/TensorRT

## 🎯 Ventajas de la versión C++

* **Mejor rendimiento**: Procesamiento más rápido que Python
* **Menor uso de memoria**: Gestión de memoria más eficiente
* **Detección mejorada**: Menos fallos de detección gracias a optimizaciones de bajo nivel
* **Latencia reducida**: Procesamiento en tiempo real más eficiente

## 📦 Requisitos del Sistema

### Dependencias del Sistema

#### Ubuntu/Jetson

```bash
sudo apt update
sudo apt install -y \\
    build-essential cmake git \\
    libopencv-dev libopencv-contrib-dev \\
    libtesseract-dev \\
    libmysqlclient-dev \\
    mysql-client mysql-server
```

### Librerías C++ Requeridas

* **OpenCV** 4.8.0+: Procesamiento de imágenes y video
* **Tesseract OCR** 5.0+: Reconocimiento óptico de caracteres
* **MySQL Connector/C++**: Conexión a base de datos MySQL
* **nlohmann/json**: Parsing de archivos JSON (header-only, incluido)
* **CUDA/TensorRT** (opcional): Aceleración GPU en Jetson

## 🔧 Compilación

### Compilación Estándar

```bash
cd New-Lpr
mkdir build \&\& cd build
cmake ..
make -j4
```

### Compilación para Jetson Orin Nano

```bash
cd New-Lpr
mkdir build \&\& cd build
cmake .. -DUSE\_TENSORRT=ON -DUSE\_CUDA=ON
make -j4
```

El ejecutable se generará en `build/bin/jetson\_lpr`

## 📝 Configuración

Edita el archivo `config/default\_config.json` o crea uno nuevo:

```json
{
  "camera": {
    "ip": "192.168.1.101",
    "user": "admin",
    "password": "admin",
    "rtsp\_url": "rtsp://admin:admin@192.168.1.101/cam/realmonitor?channel=1\&subtype=1"
  },
  "jetson": {
    "ip": "192.168.1.100",
    "interface": "enP8p1s0"
  },
  "processing": {
    "confidence\_threshold": 0.3,
    "plate\_confidence\_min": 0.25,
    "detection\_cooldown\_sec": 0.5,
    "ocr\_cache\_enabled": true
  },
  "database": {
    "host": "localhost",
    "port": 3306,
    "database": "parqueadero\_jetson",
    "user": "lpr\_user",
    "password": "lpr\_password"
  }
}
```

## 🚀 Uso

### Ejecución Básica

```bash
./jetson\_lpr --config config/default\_config.json
```

### Opciones de Línea de Comandos

```bash
./jetson\_lpr \[OPCIONES]

OPCIONES:
  -h, --help                  Mostrar ayuda
  --config CONFIG             Archivo de configuración (default: config/default\_config.json)
  --ai-every AI\_EVERY         Procesar IA cada N frames (default: 2)
  --cooldown COOLDOWN         Cooldown en segundos (default: 0.5)
  --confidence CONFIDENCE     Umbral confianza detección (default: 0.30)
  --headless                  Modo sin GUI (recomendado para Jetson)
```

### Ejemplo de Uso

```bash
./jetson\_lpr \\
    --config config/default\_config.json \\
    --ai-every 2 \\
    --cooldown 0.5 \\
    --confidence 0.30 \\
    --headless
```

## 📊 Estructura del Proyecto

```
New-Lpr/
├── CMakeLists.txt           # Sistema de compilación
├── README.md                # Este archivo
├── include/                 # Headers
│   ├── config\_manager.h     # Gestor de configuración
│   ├── plate\_validator.h    # Validador de placas colombianas
│   ├── detector.h           # Detector de placas (YOLO)
│   ├── ocr\_processor.h      # Procesador OCR
│   ├── database\_manager.h   # Gestor de base de datos
│   ├── video\_capture.h      # Captura de video RTSP
│   └── lpr\_system.h         # Sistema principal
├── src/                     # Código fuente
│   ├── main.cpp             # Punto de entrada
│   ├── config\_manager.cpp
│   ├── plate\_validator.cpp
│   ├── detector.cpp
│   ├── ocr\_processor.cpp
│   ├── database\_manager.cpp
│   ├── video\_capture.cpp
│   └── lpr\_system.cpp
├── config/                  # Archivos de configuración
│   └── default\_config.json
├── models/                  # Modelos de IA (YOLO)
│   └── license\_plate\_detector.pt  # Convertir a ONNX/TensorRT
└── third\_party/            # Dependencias header-only
    └── nlohmann/
        └── json.hpp
```

## 🔄 Migración desde Python

Este proyecto reemplaza la versión Python (`realtime\_lpr\_fixed.py`) con una implementación C++ más eficiente.

### Funcionalidades Migradas

* ✅ Detección de placas con YOLO
* ✅ OCR con Tesseract (reemplaza EasyOCR)
* ✅ Validación de placas colombianas
* ✅ Integración MySQL
* ✅ Captura RTSP
* ✅ Procesamiento en tiempo real con threading

### Diferencias Principales

* **OCR**: Tesseract en lugar de EasyOCR (mejor rendimiento en C++)
* **Detección**: TensorRT/ONNX Runtime en lugar de Ultralytics
* **Base de datos**: MySQL Connector/C++ en lugar de mysql-connector-python

## 📝 Notas

* Los modelos YOLO (.pt) deben convertirse a formato ONNX o TensorRT para uso en C++
* La configuración es compatible con la versión Python
* El formato de placas colombianas es el mismo: ABC123 (3 letras + 3 números)

## 🐛 Solución de Problemas

### Error de compilación: OpenCV no encontrado

```bash
sudo apt install libopencv-dev libopencv-contrib-dev
```

### Error de compilación: Tesseract no encontrado

```bash
sudo apt install libtesseract-dev
```

### Error de compilación: MySQL no encontrado

```bash
sudo apt install libmysqlclient-dev
```

### Error de ejecución: No se puede conectar a la cámara

* Verificar URL RTSP en la configuración
* Verificar conectividad de red
* Verificar credenciales de la cámara

## 📄 Licencia

Carlos Parra Systems Open Source MIT.

