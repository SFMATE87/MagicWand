import tensorflow as tf
import numpy as np
import pandas as pd
import os

# 1. Configuration
MODEL_NAME = 'magic_wand_model'
H5_FILE = f'{MODEL_NAME}.h5'
TFLITE_FILE = f'{MODEL_NAME}.tflite'
CPP_HEADER_FILE = 'model.h'
DATASET_PATH = 'C:/Users/Asus/Documents/Projects/MagicWand_Project/dataset'
TIME_STEPS = 100
FEATURES = 6

print(f"Loading {H5_FILE}...")
model = tf.keras.models.load_model(H5_FILE, compile=False)

#Representative Dataset Generator
#Real data fo 8 bit scaling.
def representative_data_gen():
    samples_collected = 0
    # Dig into the dataset folder
    for class_name in os.listdir(DATASET_PATH):
        class_dir = os.path.join(DATASET_PATH, class_name)
        if not os.path.isdir(class_dir): continue
            
        for filename in os.listdir(class_dir):
            if filename.endswith('.csv'):
                filepath = os.path.join(class_dir, filename)
                try:
                    df = pd.read_csv(filepath)
                    df = df.apply(pd.to_numeric, errors='coerce').dropna()
                    
                    if df.shape[0] == TIME_STEPS and df.shape[1] == FEATURES:
                        #batch of 1: (1, 100, 6)
                        data = np.array(df.values, dtype=np.float32)
                        data = data - np.mean(data, axis=0, keepdims=True)
                        data = np.expand_dims(data, axis=0)
                        yield [data]
                        
                        samples_collected += 1
                        # We only need about 100 samples to figure out the math scale
                        if samples_collected >= 100:
                            return
                except:
                    pass

# 3. The Flattener & Quantizer
print("Converting and applying INT8 Quantization...")
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]

# Force the math to be strictly 8-bit integers (INT8)
converter.representative_dataset = representative_data_gen
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8

tflite_model = converter.convert()

with open(TFLITE_FILE, 'wb') as f:
    f.write(tflite_model)
print(f"Successfully saved {TFLITE_FILE} ({len(tflite_model)} bytes)")

# 4. The Hex Dump (Bridging to C++)
print(f"Converting binary to C++ header ({CPP_HEADER_FILE})...")
with open(CPP_HEADER_FILE, 'w') as f:
    f.write(f"// Automatically generated TensorFlow Lite model\n")
    f.write(f"// Size: {len(tflite_model)} bytes\n\n")
    f.write(f"const unsigned char {MODEL_NAME}_tflite[] = {{\n")
    
    hex_array = [f"0x{b:02x}" for b in tflite_model]
    for i in range(0, len(hex_array), 12):
        f.write("  " + ", ".join(hex_array[i:i+12]) + ",\n")
    
    f.write("};\n\n")
    f.write(f"const int {MODEL_NAME}_tflite_len = {len(tflite_model)};\n")

print(f"\nSuccess! Your ESP32 brain is ready in {CPP_HEADER_FILE}")