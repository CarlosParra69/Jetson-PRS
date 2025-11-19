cmake_minimum_required(VERSION 3.18)
project(JetsonLPR CXX)

# C++17 estándar
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configuración de compilación
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Directorios
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Opciones
option(USE_CUDA "Usar CUDA para inferencia" ON)
option(USE_TENSORRT "Usar TensorRT para inferencia" ON)
option(USE_ONNXRUNTIME "Usar ONNX Runtime como alternativa" OFF)

# Incluir directorios
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Encontrar dependencias
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Tesseract OCR
pkg_check_modules(TESSERACT REQUIRED tesseract)

# MySQL Connector
find_path(MYSQL_INCLUDE_DIR mysql/mysql.h
    PATHS
    /usr/include/mysql
    /usr/local/include/mysql
    /opt/homebrew/include/mysql
)

find_library(MYSQL_LIBRARY
    NAMES mysqlclient libmysqlclient
    PATHS
    /usr/lib
    /usr/local/lib
    /opt/homebrew/lib
    /usr/lib/x86_64-linux-gnu
)

# JSON (nlohmann/json header-only)
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/third_party/nlohmann/json.hpp)
    message(STATUS "Descargando nlohmann/json...")
    file(DOWNLOAD 
        https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
        ${CMAKE_SOURCE_DIR}/third_party/nlohmann/json.hpp
    )
endif()
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# CUDA/TensorRT (opcional, para Jetson)
if(USE_CUDA)
    find_package(CUDA QUIET)
    if(CUDA_FOUND)
        message(STATUS "CUDA encontrado: ${CUDA_VERSION}")
        
        # Buscar el compilador nvcc en ubicaciones comunes de Jetson
        find_program(CUDA_NVCC_EXECUTABLE
            NAMES nvcc
            PATHS
            ${CUDA_TOOLKIT_ROOT_DIR}/bin
            /usr/local/cuda/bin
            /usr/bin
            ENV PATH
            NO_DEFAULT_PATH
        )
        
        # Si no se encuentra en ubicaciones específicas, buscar en PATH
        if(NOT CUDA_NVCC_EXECUTABLE)
            find_program(CUDA_NVCC_EXECUTABLE NAMES nvcc)
        endif()
        
        if(CUDA_NVCC_EXECUTABLE)
            message(STATUS "Compilador CUDA encontrado: ${CUDA_NVCC_EXECUTABLE}")
            # Configurar CMAKE_CUDA_COMPILER antes de enable_language
            set(CMAKE_CUDA_COMPILER ${CUDA_NVCC_EXECUTABLE})
            enable_language(CUDA)
            set(CMAKE_CUDA_STANDARD 17)
            set(CMAKE_CUDA_STANDARD_REQUIRED ON)
        else()
            message(WARNING "CUDA encontrado pero nvcc no está disponible. Desactivando CUDA.")
            message(WARNING "Para habilitar CUDA, asegúrate de que nvcc esté en el PATH:")
            message(WARNING "  export PATH=/usr/local/cuda/bin:$PATH")
            set(USE_CUDA OFF)
            set(CUDA_FOUND FALSE)
        endif()
    else()
        message(STATUS "CUDA no encontrado - compilando sin soporte CUDA")
        set(USE_CUDA OFF)
    endif()
endif()

# TensorRT (para Jetson Orin Nano)
if(USE_TENSORRT)
    find_path(TENSORRT_INCLUDE_DIR NvInfer.h
        PATHS
        /usr/include/aarch64-linux-gnu
        /usr/local/cuda/include
        ${CUDA_TOOLKIT_ROOT_DIR}/include
    )
    
    find_library(TENSORRT_LIBRARY nvinfer
        PATHS
        /usr/lib/aarch64-linux-gnu
        /usr/local/cuda/lib64
        ${CUDA_TOOLKIT_ROOT_DIR}/lib64
    )
    
    if(TENSORRT_INCLUDE_DIR AND TENSORRT_LIBRARY)
        message(STATUS "TensorRT encontrado")
        add_definitions(-DUSE_TENSORRT)
        include_directories(${TENSORRT_INCLUDE_DIR})
    else()
        message(WARNING "TensorRT no encontrado, se usará CPU")
        set(USE_TENSORRT OFF)
    endif()
endif()

# ONNX Runtime (alternativa a TensorRT)
if(USE_ONNXRUNTIME)
    find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h
        PATHS
        /usr/local/include
        /opt/onnxruntime/include
    )
    
    find_library(ONNXRUNTIME_LIBRARY onnxruntime
        PATHS
        /usr/local/lib
        /opt/onnxruntime/lib
    )
    
    if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_LIBRARY)
        message(STATUS "ONNX Runtime encontrado")
        add_definitions(-DUSE_ONNXRUNTIME)
        include_directories(${ONNXRUNTIME_INCLUDE_DIR})
    else()
        message(WARNING "ONNX Runtime no encontrado")
        set(USE_ONNXRUNTIME OFF)
    endif()
endif()

# Fuentes
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
)

file(GLOB_RECURSE HEADERS
    "include/*.h"
    "include/*.hpp"
)

# Ejecutable principal
add_executable(jetson_lpr
    ${SOURCES}
)

# Enlaces
target_link_libraries(jetson_lpr
    ${OpenCV_LIBS}
    ${TESSERACT_LIBRARIES}
    ${MYSQL_LIBRARY}
    Threads::Threads
)

# Incluir directorios para el ejecutable
target_include_directories(jetson_lpr PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${TESSERACT_INCLUDE_DIRS}
    ${MYSQL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
)

# Compilar flags
target_compile_options(jetson_lpr PRIVATE
    ${TESSERACT_CFLAGS_OTHER}
    -Wall
    -Wextra
    -O3
    -march=native
)

# Mensajes informativos
message(STATUS "=== Configuración Jetson LPR ===")
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
if(USE_CUDA AND CUDA_FOUND)
    message(STATUS "CUDA: ${CUDA_VERSION} (${CUDA_NVCC_EXECUTABLE})")
else()
    message(STATUS "CUDA: Desactivado")
endif()
message(STATUS "TensorRT: ${USE_TENSORRT}")
message(STATUS "ONNX Runtime: ${USE_ONNXRUNTIME}")
message(STATUS "Tesseract: ${TESSERACT_VERSION}")
message(STATUS "MySQL: ${MYSQL_LIBRARY}")
message(STATUS "=================================")

