#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

// Időzítés beállításai (50 Hz = 20 ms)
unsigned long lastSampleTime = 0;
const int sampleInterval = 20; 

bool streaming = false;
int lastButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  
  // Szenzor inicializálása
  if (!mpu.begin()) {
    while (1) {
      yield(); 
    }
  }

  // Mérési tartományok beállítása (adatgyűjtéshez optimális)
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  pinMode(0, INPUT_PULLUP); // BOOT gomb beállítása
}

void loop() {
  int currentButtonState = digitalRead(0);

  // Gombnyomás detektálása (Edge detection)
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    streaming = !streaming;
    delay(200); // Debounce (prellmentesítés)
  }
  lastButtonState = currentButtonState;

  if (streaming) {
    unsigned long currentTime = millis();
    if (currentTime - lastSampleTime >= sampleInterval) {
      lastSampleTime = currentTime;

      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);

      // Formátum: ax,ay,az,gx,gy,gz
      Serial.print(a.acceleration.x); Serial.print(",");
      Serial.print(a.acceleration.y); Serial.print(",");
      Serial.print(a.acceleration.z); Serial.print(",");
      Serial.print(g.gyro.x); Serial.print(",");
      Serial.print(g.gyro.y); Serial.print(",");
      Serial.println(g.gyro.z);
    }
  }
}