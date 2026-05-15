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

int8_t ring_bf[100][6];
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
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  //Limits: -128...127
  ring_bf[head][0] = constrain(round(a.acceleration.x / input_scale) + input_zero_point, -128, 127);
  ring_bf[head][1] = constrain(round(a.acceleration.y / input_scale) + input_zero_point, -128, 127);
  ring_bf[head][2] = constrain(round(a.acceleration.z / input_scale) + input_zero_point, -128, 127);
  ring_bf[head][3] = constrain(round(g.gyro.x / input_scale) + input_zero_point, -128, 127);
  ring_bf[head][4] = constrain(round(g.gyro.y / input_scale) + input_zero_point, -128, 127);
  ring_bf[head][5] = constrain(round(g.gyro.z / input_scale) + input_zero_point, -128, 127);

  head = (head + 1) % 100;
  if (sample_count < 100) {
      sample_count++;
  }

  //Only if there are 100 data
  if(sample_count >= 100){
    for (int i = 0; i < 100; i++) {
      for (int j = 0; j < 6; j++) {
        input->data.int8[6 * i + j] = ring_bf[(head + i) % 100][j]; 
      }
    }

    interpreter->Invoke();

    char detected_gesture = get_gesture(output->data.int8);

    if (detected_gesture != ' ') {
      Serial.println(detected_gesture); 
      sample_count = 0;                 
    }
  }

  delay(10);
}

char get_gesture(int8_t* predictions) {
  int8_t max_val = -128; 
  int max_index = 0;

  for(int i = 0; i < 4; i++){
    if (predictions[i] > max_val){
      max_index = i;
      max_val = predictions[i];
    }
  }
  if(max_val <= 50){
    return ' '; //Not sure enough 
  }

  if (max_index == 0) return ' '; //Idle
  if (max_index == 1) return 'I';
  if (max_index == 2) return 'O';
  if (max_index == 3) return 'Z';

  return '?';
}