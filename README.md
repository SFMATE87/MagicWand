# Magic Wand Project (ESP32 + MPU6050)

This is my project where I built a "magic wand" using an ESP32 and an MPU6050 IMU. The goal was to train a simple 1D CNN (Convolutional Neural Network) to recognize hand gestures in real-time. It can currently detect the letters 'I', 'Z', 'O', and an 'Idle' state.

It does the inference directly on the microcontroller using TensorFlow Lite for Microcontrollers (TinyML).

## Hardware Used
* ESP32 DevKit (38 pins)
* MPU-6050 sensor module
* A big foek stick to mount everything on
* Lots of tape and jumper wires

**I2C Wiring:**
* VCC -> 3V3
* GND -> GND
* SDA -> GPIO 21
* SCL -> GPIO 22

**⚠️ Important hardware note:** When you swing the wand, the centrifugal force will literally pull the female jumper wires out of the pins or cause random I2C errors. You have to crimp them tight with pliers or just solder the whole thing. I learned this the hard way because the code kept freezing (`while(1)` trap) during data collection.

## How it works (Software)
I migrated the project to PlatformIO because the Arduino IDE got too messy for TFLite dependencies.

The C++ code uses a circular buffer (sliding window). This way, I don't have to use heavy `memmove()` operations every time a new sensor reading comes in. It just overwrites the oldest data using modulo math.

**A huge trap I fell into (Timing issue):** The Python dataset was recorded at 50 Hz (100 samples = 2 seconds). In the C++ code, I initially had a `delay(10)` which ran the loop at 100 Hz. The neural network got totally confused because the live movements looked twice as fast as the training data. **Make sure your inference loop runs at exactly the same speed as your training data** (I changed the loop delay to 20ms to match the 50 Hz requirement).

## Folder Structure
* `src/` - `main.cpp` is here. This handles the sensor reading, INT8 quantization, and TFLite inference.
* `include/` - `model.h` (the exported TFLite model as a C byte array).
* `dataset/` - My raw CSV files from the 1200 gestures I manually recorded.
* `python_tools/` - Python scripts I used to record serial data, train the Keras model, and convert it to `.tflite`.
* `esp32_firmware/` - The old `.ino` file I used to collect the raw data before I moved to PlatformIO. Kept it as an archive.

## How to run it
1. Open the folder in VS Code with the PlatformIO extension installed.
2. It should download all libraries automatically based on the `platformio.ini` file (like `Adafruit_MPU6050` and the TFLite port).
3. Connect the ESP32, build, and upload.
4. Open the Serial Monitor at `115200` baud and start waving the wand.

*(Note: If you want to train your own model, you'll need Python installed with `tensorflow`, `pandas`, `numpy`, and `pyserial`. Run the scripts in the `python_tools` folder).*