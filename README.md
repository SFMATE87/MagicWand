# Magic Wand: ESP32 Gesture Recognition Dataset & Collector

## Project Objective
This repository contains a 50 Hz data collection system based on an ESP32 and an MPU-6050 (accelerometer and gyroscope). The goal of the project was to create a labeled dataset of 1,200 samples (letters 'I', 'Z', 'O', and an 'Idle' state) suitable for training a 1D Convolutional Neural Network (Conv1D) to recognize magic wand-like gestures.

## Hardware Requirements
* ESP32 Dev Module (38-pin)
* MPU-6050 6-axis IMU sensor
* Jumper wires and physical mounting (e.g., metal fork/wand)

### I2C Wiring
| MPU-6050 | ESP32 Pin |
| :--- | :--- |
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 (D21) |
| SCL | GPIO 22 (D22) |

*Critical Engineering Note:* Due to the centrifugal forces and sudden jerks during data collection, physically tightening the female jumper wire connectors (using pliers) is mandatory to prevent contact failures and disruptions on the I2C bus.

## Directory Structure
* `/esp32_firmware`: Contains the C++ code that streams data over the serial port timed strictly at 50 Hz, triggered by a debounced BOOT button press.
* `/python_tools`: Contains the `collect.py` Python script to automatically capture and format the data stream into 100-timestep (2-second) CSV files.
* `/dataset`: The 1,200 cleaned, manually recorded gesture samples (categorized into 'I', 'Z', 'O', and 'Idle' classes).

## Software Dependencies (Python)
To run the Python data collection script, the following package is required:
```bash
pip install pyserial