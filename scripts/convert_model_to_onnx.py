#!/usr/bin/env python3
"""
Script para convertir modelo YOLO (.pt) a formato ONNX
Compatible con Ultralytics YOLOv8/YOLOv11
"""

import sys
import os
from pathlib import Path

def convert_yolo_to_onnx(model_path, output_path=None, imgsz=640):
    """
    Convertir modelo YOLO a ONNX
    
    Args:
        model_path: Ruta al modelo .pt
        output_path: Ruta de salida (opcional, por defecto mismo nombre con .onnx)
        imgsz: Tamaño de imagen para exportación (default: 640)
    """
    try:
        from ultralytics import YOLO
    except ImportError:
        print("❌ Error: Ultralytics no está instalado")
        print("Instala con: pip install ultralytics")
        return False
    
    model_path = Path(model_path)
    
    if not model_path.exists():
        print(f"❌ Error: Modelo no encontrado: {model_path}")
        return False
    
    if output_path is None:
        output_path = model_path.parent / f"{model_path.stem}.onnx"
    else:
        output_path = Path(output_path)
    
    print(f"🔄 Convirtiendo modelo YOLO a ONNX...")
    print(f"   Entrada: {model_path}")
    print(f"   Salida: {output_path}")
    
    try:
        # Cargar modelo
        print("📦 Cargando modelo...")
        model = YOLO(str(model_path))
        
        # Exportar a ONNX
        print("🔄 Exportando a ONNX...")
        success = model.export(
            format='onnx',
            imgsz=imgsz,
            simplify=True,  # Simplificar modelo
            opset=12,        # Versión ONNX opset
            dynamic=False,   # Tamaño fijo para mejor rendimiento
            half=False       # FP32 (usar FP16 solo si soportado)
        )
        
        if success:
            # El modelo se exporta en el mismo directorio que el .pt
            exported_path = model_path.parent / f"{model_path.stem}.onnx"
            
            # Si especificamos una ruta diferente, moverlo
            if exported_path != output_path:
                if output_path.parent.exists():
                    import shutil
                    shutil.move(str(exported_path), str(output_path))
                    print(f"✅ Modelo movido a: {output_path}")
                else:
                    output_path.parent.mkdir(parents=True, exist_ok=True)
                    import shutil
                    shutil.move(str(exported_path), str(output_path))
                    print(f"✅ Modelo movido a: {output_path}")
            else:
                print(f"✅ Modelo exportado: {exported_path}")
            
            print(f"\n✅ Conversión exitosa!")
            print(f"   Modelo ONNX: {output_path}")
            print(f"   Tamaño: {output_path.stat().st_size / (1024*1024):.2f} MB")
            
            return True
        else:
            print("❌ Error: No se pudo exportar el modelo")
            return False
            
    except Exception as e:
        print(f"❌ Error durante la conversión: {e}")
        import traceback
        traceback.print_exc()
        return False

def main():
    if len(sys.argv) < 2:
        print("Uso: python convert_model_to_onnx.py <modelo.pt> [salida.onnx]")
        print("\nEjemplo:")
        print("  python convert_model_to_onnx.py ../license_plate_detector.pt")
        print("  python convert_model_to_onnx.py ../yolo11n.pt models/yolo11n.onnx")
        sys.exit(1)
    
    model_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else None
    
    # Si no se especifica salida, poner en models/
    if output_path is None:
        model_file = Path(model_path)
        output_path = Path("models") / f"{model_file.stem}.onnx"
        Path("models").mkdir(exist_ok=True)
    
    success = convert_yolo_to_onnx(model_path, output_path)
    
    if not success:
        sys.exit(1)

if __name__ == "__main__":
    main()

