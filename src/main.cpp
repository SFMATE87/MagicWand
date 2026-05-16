#include <Arduino.h>
#include "model.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

float ring_bf[100][6];
int sample_count = 0;
int head = 0;

uint8_t tensor_arena[1024*30];

TfLiteTensor* input;
TfLiteTensor* output;
const tflite::Model* model;
tflite::MicroInterpreter* interpreter;

float input_scale = 0.0f;
int input_zero_point = 0;

char get_gesture(int8_t* predictions);

void setup() {
  Serial.begin(115200);

  pinMode(2, OUTPUT);

  if(!mpu.begin()){
    Serial.println("Critical Error: MPU6050 Not Found");
    while(1);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  model = tflite::GetModel(magic_wand_model_tflite);
  static tflite::AllOpsResolver resolver;

  static tflite::MicroErrorReporter micro_error_reporter;
  static tflite::MicroInterpreter static_interpreter(
    model, resolver, tensor_arena, 30 * 1024, &micro_error_reporter, nullptr, nullptr
  );
  interpreter = &static_interpreter;

  //Parcelling
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
      Serial.println("Error: Not enough memory in the tensor arena");
      while(1);
  }

  input = interpreter->input(0);
  output = interpreter->output(0);

  input_scale = input->params.scale;
  input_zero_point = input->params.zero_point;
}

void loop() {
  if (sample_count == 0) {
      digitalWrite(2, HIGH); 
    }

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  //Limits: -128...127
  ring_bf[head][0] = a.acceleration.x;
  ring_bf[head][1] = a.acceleration.y;
  ring_bf[head][2] = a.acceleration.z;
  ring_bf[head][3] = g.gyro.x;
  ring_bf[head][4] = g.gyro.y;
  ring_bf[head][5] = g.gyro.z;

  head = (head + 1) % 100;
  sample_count++;

  //Only if there are 100 data
  if(sample_count >= 100){
    digitalWrite(2, LOW);

    float means[6] = {0, 0, 0, 0, 0, 0};
    for (int i = 0; i < 100; i++) {
      for (int j = 0; j < 6; j++) {
        means[j] += ring_bf[i][j]; 
      }
    }
    for (int j = 0; j < 6; j++) {
        means[j] /= 100.0f;
    }

    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 6; j++) {
          float centered_val = ring_bf[(head + i) % 100][j] - means[j]; //minus 1G
          
          //Quantization
          input->data.int8[6 * i + j] = constrain(round(centered_val / input_scale) + input_zero_point, -128, 127);
      }
    }

    interpreter->Invoke();
    char detected_gesture = get_gesture(output->data.int8);

    Serial.println("-------------");
      if (detected_gesture != '-') {
        Serial.print("RECOGNISED GESTURE: ");
        Serial.println(detected_gesture); 
      } else {
        Serial.println("IDLE / NOISE");
      }
      Serial.println("-------------");

    delay(1000);
    sample_count = 0;                 
  }
  if (sample_count > 0) {
      delay(20); 
    }
}

char get_gesture(int8_t* predictions) {
  //debug telemtria
  Serial.print(" | I: "); Serial.print(predictions[0]);
  Serial.print(" | z: "); Serial.print(predictions[1]);
  Serial.print(" | O: "); Serial.print(predictions[2]);
  Serial.print("Idle: "); Serial.println(predictions[3]);

  int8_t max_val = -128; 
  int max_index = 0;

  for(int i = 0; i < 4; i++){
    if (predictions[i] > max_val){
      max_index = i;
      max_val = predictions[i];
    }
  }
  if(max_val <= 80){
    return '-'; //Not sure enough 
  }

  if (max_index == 0) return 'I';
  if (max_index == 1) return 'Z';
  if (max_index == 2) return 'O';
  if (max_index == 3) return '-';

  return '?';
}