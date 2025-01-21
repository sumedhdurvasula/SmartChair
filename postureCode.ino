#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

const uint8_t MPU6050_ADDR   = 0x68;
const uint8_t PWR_MGMT_1     = 0x6B;
const uint8_t SMPLRT_DIV     = 0x19;
const uint8_t CONFIG         = 0x1A;
const uint8_t GYRO_CONFIG    = 0x1B;
const uint8_t ACCEL_CONFIG   = 0x1C;
const uint8_t INT_ENABLE     = 0x38;
const uint8_t WHO_AM_I       = 0x75;

const int upperBackPin = 9;
const int lowerBackPin = 6;
const int seatPin      = 5;

const char* ssid       = "WIFINAME";
const char* password   = "PASSWORD";
const char* serverURL  = "http://172.20.10.5:8000/posture";

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 5000;

void MPU6050_Init();
void MPU6050_Read_Accel(int16_t &Ax, int16_t &Ay, int16_t &Az);
void writeRegister(uint8_t reg, uint8_t data);
uint8_t readRegister(uint8_t reg);
void readRegisters(uint8_t reg, uint8_t count, uint8_t *dest);

const int16_t accelXThreshold = -1000;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    delay(10);
  }
  Wire.begin();
  pinMode(upperBackPin, INPUT_PULLUP);
  pinMode(lowerBackPin, INPUT_PULLUP);
  pinMode(seatPin, INPUT_PULLUP);

  MPU6050_Init();

  uint8_t whoAmI = readRegister(WHO_AM_I);
  if (whoAmI != 0x68) {
    while (1) {
      delay(1000);
    }
  }

  WiFi.begin(ssid, password);
  uint8_t wifi_retry = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    wifi_retry++;
    if (wifi_retry > 20) {
      ESP.restart();
    }
  }
}

void loop() {
  int16_t ax, ay, az;

  MPU6050_Read_Accel(ax, ay, az);

  float accel_x = ax / 16384.0;
  float accel_y = ay / 16384.0;
  float accel_z = az / 16384.0;

  float pitch = atan2(accel_x, sqrt(accel_y * accel_y + accel_z * accel_z)) * 180.0 / PI;
  float roll  = atan2(accel_y, sqrt(accel_x * accel_x + accel_z * accel_z)) * 180.0 / PI;

  bool upperBackPressed = (digitalRead(upperBackPin) == LOW);
  bool lowerBackPressed = (digitalRead(lowerBackPin) == LOW);
  bool seatPressed      = (digitalRead(seatPin) == LOW);

  float seatPressure      = seatPressed      ? 100.0 : 0.0;
  float lowerBackPressure = lowerBackPressed ? 100.0 : 0.0;
  float upperBackPressure = upperBackPressed ? 100.0 : 0.0;

  bool isLeaning = (ax < accelXThreshold);
  bool goodPosture = seatPressed && lowerBackPressed && !isLeaning && !upperBackPressed;

  int postureState;
  if (!seatPressed) {
    postureState = 0;
  } else if (goodPosture) {
    postureState = 2;
  } else {
    postureState = 1;
  }

  Serial.print(pitch, 2);
  Serial.print(",");
  Serial.print(roll, 2);
  Serial.print(",");
  Serial.print(seatPressure, 0);
  Serial.print(",");
  Serial.print(lowerBackPressure, 0);
  Serial.print(",");
  Serial.print(upperBackPressure, 0);
  Serial.print(",");
  Serial.println(postureState);

  if (millis() - lastSendTime > sendInterval) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverURL);
      http.addHeader("Content-Type", "application/json");
      String jsonData = "{";
      jsonData += "\"postureState\": " + String(postureState);
      jsonData += "}";
      int httpResponseCode = http.POST(jsonData);
      http.end();
    } else {
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      uint8_t wifi_retry = 0;
      while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        wifi_retry++;
        if (wifi_retry > 20) {
          break;
        }
      }
    }
    lastSendTime = millis();
  }
  delay(10);
}

void MPU6050_Init() {
  writeRegister(PWR_MGMT_1, 0x00);
  delay(100);
  writeRegister(SMPLRT_DIV, 0x07);
  writeRegister(CONFIG, 0x06);
  writeRegister(GYRO_CONFIG, 0x00);
  writeRegister(ACCEL_CONFIG, 0x00);
  writeRegister(INT_ENABLE, 0x01);
}

void MPU6050_Read_Accel(int16_t &Ax, int16_t &Ay, int16_t &Az) {
  uint8_t accelData[6];
  readRegisters(0x3B, 6, accelData);
  Ax = (int16_t)(accelData[0] << 8 | accelData[1]);
  Ay = (int16_t)(accelData[2] << 8 | accelData[3]);
  Az = (int16_t)(accelData[4] << 8 | accelData[5]);
}

void writeRegister(uint8_t reg, uint8_t data) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

uint8_t readRegister(uint8_t reg) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, (uint8_t)1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0;
}

void readRegisters(uint8_t reg, uint8_t count, uint8_t *dest) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, count);
  uint8_t i = 0;
  while (Wire.available() && i < count) {
    dest[i++] = Wire.read();
  }
}
