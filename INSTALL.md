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

### Error: Dependencias rotas de OpenCV (unmet dependencies / held broken packages)

Este error es común en Jetson cuando hay conflictos de versiones. Soluciones:

#### Solución 1: Usar script de corrección automática

```bash
bash scripts/fix_opencv_dependencies.sh
```

#### Solución 2: Instalación manual paso a paso

```bash
# 1. Corregir dependencias rotas
sudo apt-get install -f -y

# 2. Actualizar lista de paquetes
sudo apt update

# 3. Instalar solo versión base (más seguro)
sudo apt install -y libopencv-dev python3-opencv

# 4. Verificar versión instalada
apt-cache policy libopencv-dev | grep Installed

# 5. Si necesitas contrib, instalar con versión específica
# Reemplaza VERSION con la versión obtenida en el paso 4
sudo apt install -y libopencv-contrib-dev=VERSION
```

#### Solución 3: Instalar solo lo esencial (sin contrib)

Si `libopencv-contrib-dev` no es estrictamente necesario:

```bash
sudo apt install -y libopencv-dev python3-opencv
```

Luego verifica que OpenCV funciona:

```bash
pkg-config --modversion opencv4
# o
pkg-config --modversion opencv
```

#### Solución 4: En Jetson - Verificar instalación de JetPack

Los Jetson suelen tener OpenCV preinstalado con JetPack:

```bash
# Verificar si OpenCV ya está instalado
pkg-config --modversion opencv4

# Si está instalado, puede que no necesites instalar nada más
```

### Error: OpenCV no encontrado

```bash
sudo apt install libopencv-dev
# Si contrib no es necesario, evita libopencv-contrib-dev
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

### Error: No CMAKE_CUDA_COMPILER could be found

Este error ocurre cuando CUDA está instalado pero el compilador `nvcc` no está en el PATH.

#### Solución 1: Agregar nvcc al PATH (recomendado para Jetson)

```bash
# Agregar CUDA al PATH (temporal para esta sesión)
export PATH=/usr/local/cuda/bin:$PATH

# O agregar permanentemente a ~/.bashrc
echo 'export PATH=/usr/local/cuda/bin:$PATH' >> ~/.bashrc
source ~/.bashrc

# Verificar que nvcc está disponible
which nvcc
nvcc --version
```

Luego ejecuta cmake nuevamente:

```bash
cd build
cmake ..
```

#### Solución 2: Compilar sin CUDA (si no lo necesitas)

Si no necesitas aceleración CUDA, puedes compilar sin ella:

```bash
cd build
rm -rf *  # Limpiar configuración anterior
cmake .. -DUSE_CUDA=OFF
make -j4
```

#### Solución 3: Especificar ruta del compilador manualmente

```bash
cd build
rm -rf *
cmake .. -DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc
make -j4
```

### Error: CUDA no disponible

- En Jetson, verifica con `nvidia-smi`
- En otros sistemas, compila sin CUDA: `cmake .. -DUSE_CUDA=OFF`

### Error: No se puede conectar a la cámara RTSP

Este es uno de los errores más comunes. Sigue estos pasos:

#### 1. Probar la conexión RTSP manualmente

```bash
# Usar el script de prueba
chmod +x scripts/test_rtsp.sh
bash scripts/test_rtsp.sh

# O probar con ffmpeg directamente (reemplaza con tu URL)
ffmpeg -i "rtsp://admin:admin@192.168.1.101/cam/realmonitor?channel=1&subtype=1" -t 5 -f null -
```

#### 2. Verificar la URL RTSP

La URL RTSP debe tener el formato:
```
rtsp://usuario:contraseña@IP:puerto/ruta
```

Ejemplos comunes:
- **Hikvision**: `rtsp://admin:password@192.168.1.101/Streaming/Channels/101`
- **Dahua**: `rtsp://admin:password@192.168.1.101/cam/realmonitor?channel=1&subtype=0`
- **Generic**: `rtsp://admin:password@192.168.1.101:554/stream1`

#### 3. Verificar conectividad de red

```bash
# Ping a la cámara (reemplaza con la IP de tu cámara)
ping 192.168.1.101

# Verificar puerto RTSP (554 por defecto)
nc -zv 192.168.1.101 554
```

#### 4. Verificar credenciales

Prueba acceder a la interfaz web de la cámara:
```bash
# Abrir en navegador (reemplaza con la IP de tu cámara)
# http://192.168.1.101
```

#### 5. Soluciones comunes

**Problema: "backend is generally available but can't be used"**
- Instalar ffmpeg: `sudo apt install ffmpeg`
- El código mejorado ahora intenta múltiples métodos de conexión automáticamente

**Problema: Timeout al conectar**
- El código mejorado ahora tiene timeout de 10 segundos y reintentos
- Verificar firewall: `sudo ufw allow 554/udp`

**Problema: Cámara en red diferente**
- Verificar que el Jetson y la cámara están en la misma red
- Verificar configuración de red: `ip addr show`

#### 6. Usar cámara USB como alternativa

Si tienes problemas con RTSP, puedes usar una cámara USB:

1. Edita `config/default_config.json`
2. Cambia `rtsp_url` a un número de dispositivo:
   ```json
   "rtsp_url": "0"  // Para /dev/video0
   ```
3. El código detectará automáticamente que es un dispositivo USB

## Próximos Pasos

1. ✅ Convertir modelo YOLO a ONNX
2. ✅ Configurar base de datos MySQL
3. ✅ Configurar URL RTSP de cámara
4. ✅ Compilar proyecto
5. ✅ Ejecutar sistema
6. ✅ Verificar detecciones en base de datos

¡El sistema está listo para detectar placas vehiculares colombianas en tiempo real!
