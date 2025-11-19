# Guía de Instalación - Sistema LPR C++

## Requisitos Previos

### Sistema Operativo

- Ubuntu 20.04+ / Debian 11+
- Jetson Orin Nano (Linux for Tegra)

### Dependencias del Sistema

#### Instalación en Ubuntu/Debian

```bash
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
    mysql-server
```

### Dependencias de Desarrollo

#### nlohmann/json (header-only)

El CMakeLists.txt descargará automáticamente el header si no existe.

Manual:

```bash
mkdir -p third_party/nlohmann
wget -O third_party/nlohmann/json.hpp \
    https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
```

#### OpenCV con soporte CUDA (para Jetson)

Si usas Jetson Orin Nano, OpenCV ya debería incluir soporte CUDA. Si no:

```bash
# Verificar soporte CUDA
pkg-config --modversion opencv4
```

#### Tesseract OCR

```bash
sudo apt install tesseract-ocr libtesseract-dev
```

#### MySQL

```bash
sudo apt install mysql-server mysql-client libmysqlclient-dev
sudo systemctl start mysql
sudo systemctl enable mysql
```

## Compilación

### Compilación Estándar (CPU)

```bash
cd New-Lpr
mkdir build && cd build
cmake ..
make -j4
```

### Compilación con CUDA (Jetson)

```bash
cd New-Lpr
mkdir build && cd build
cmake .. -DUSE_CUDA=ON -DUSE_TENSORRT=ON
make -j4
```

### Compilación con ONNX Runtime (Alternativa)

```bash
cd New-Lpr
mkdir build && cd build
cmake .. -DUSE_ONNXRUNTIME=ON
make -j4
```

## Configuración

### 1. Base de Datos MySQL

Configura la base de datos usando el script Python del proyecto original:

```bash
cd ..  # Volver al directorio raíz del proyecto
python stream/database/setup_mysql_jetson.py
```

O manualmente:

```sql
CREATE DATABASE IF NOT EXISTS parqueadero_jetson;
CREATE USER IF NOT EXISTS 'lpr_user'@'localhost' IDENTIFIED BY 'lpr_password';
GRANT ALL PRIVILEGES ON parqueadero_jetson.* TO 'lpr_user'@'localhost';
FLUSH PRIVILEGES;
```

### 2. Modelo YOLO

**IMPORTANTE**: Necesitas convertir tu modelo YOLO (.pt) a formato ONNX:

#### Opción 1: Usando Python (Ultralytics)

```python
from ultralytics import YOLO

# Cargar modelo
model = YOLO('license_plate_detector.pt')

# Exportar a ONNX
model.export(format='onnx')
```

#### Opción 2: Usando onnxruntime

```python
import torch
import onnx

# Cargar modelo PyTorch
model = torch.load('license_plate_detector.pt')

# Exportar a ONNX
torch.onnx.export(
    model,
    torch.randn(1, 3, 640, 640),
    'license_plate_detector.onnx',
    export_params=True
)
```

Coloca el modelo ONNX en:

```
New-Lpr/models/license_plate_detector.onnx
```

### 3. Archivo de Configuración

Edita `config/default_config.json` con tus configuraciones:

```json
{
  "camera": {
    "ip": "192.168.1.101",
    "rtsp_url": "rtsp://admin:admin@192.168.1.101/cam/realmonitor?channel=1&subtype=1"
  },
  "database": {
    "host": "localhost",
    "database": "parqueadero_jetson",
    "user": "lpr_user",
    "password": "lpr_password"
  }
}
```

## Ejecución

### Ejecución Básica

```bash
cd build
./bin/jetson_lpr --config ../config/default_config.json
```

### Opciones

```bash
./bin/jetson_lpr \
    --config ../config/default_config.json \
    --ai-every 2 \
    --cooldown 0.5 \
    --confidence 0.30 \
    --headless
```

## Verificación

### Verificar Captura de Video

```bash
# Probar conexión RTSP
ffmpeg -i rtsp://admin:admin@192.168.1.101/cam/realmonitor?channel=1&subtype=1 -t 5 -f null -
```

### Verificar Base de Datos

```bash
mysql -u lpr_user -p parqueadero_jetson -e "SHOW TABLES;"
```

### Verificar Detecciones

```bash
mysql -u lpr_user -p parqueadero_jetson -e "SELECT * FROM lpr_detections ORDER BY timestamp DESC LIMIT 10;"
```

## Solución de Problemas

### Error: OpenCV no encontrado

```bash
sudo apt install libopencv-dev libopencv-contrib-dev
```

### Error: Tesseract no encontrado

```bash
sudo apt install libtesseract-dev tesseract-ocr
```

### Error: MySQL no encontrado

```bash
sudo apt install libmysqlclient-dev
```

### Error: Modelo no encontrado

- Verifica que el modelo ONNX esté en `models/license_plate_detector.onnx`
- O actualiza la ruta en `src/lpr_system.cpp` línea ~63

### Error: CUDA no disponible

- En Jetson, verifica con `nvidia-smi`
- En otros sistemas, compila sin CUDA: `cmake ..`

## Próximos Pasos

1. ✅ Convertir modelo YOLO a ONNX
2. ✅ Configurar base de datos MySQL
3. ✅ Configurar URL RTSP de cámara
4. ✅ Compilar proyecto
5. ✅ Ejecutar sistema
6. ✅ Verificar detecciones en base de datos

¡El sistema está listo para detectar placas vehiculares colombianas en tiempo real!
