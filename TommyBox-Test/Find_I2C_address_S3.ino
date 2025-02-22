/**********************************

      ESP32-S3 I2C Scanner
     OLED          ESP32-S3
 +----------+    +----------+
 |       GND|----|GND       |
 |       VCC|----|3V3       |
 |       SCL|----|GPIO9     |
 |       SDA|----|GPIO8     |
 +----------+    +----------+

Posted by Rui Santos: https://randomnerdtutorials.com/esp32-s3-devkitc-pinout-guide/
I2C: When using the ESP32-S3 with the Arduino IDE, these are the ESP32 I2C default pins:

GPIO 8 (SDA)

GPIO 9 (SCL)

Select these two pins like this in your sketch:
//Wire.begin(I2C_SDA, I2C_SCL);
  Wire.begin(8, 9);

Serial Monitor says:
Scanning...
I2C device found at address 0x3C 

Hardware:
ESP32-S3-DevKitC-1: https://www.amazon.com/dp/B0D9W4Y3F3
0.96" White SSD1306 I2C OLED Display: https://www.amazon.com/dp/B09T6SJBV5

Problem: The onboard addresable RGB LED on GPIO38 is always
on bright white. I cannot find a way to simply disable it.

**********************************/

#include <Wire.h>

#define rgbledPin 38

void setup() {
  //Wire.begin(I2C_SDA, I2C_SCL);
  Wire.begin(8, 9);
  //Wire.begin(5, 4);
  //Wire.begin(33, 32);
  pinMode(rgbledPin, OUTPUT);
  //pinMode(rgbledPin, INPUT);
  Serial.begin(115200);
  Serial.println("\nI2C Scanner Started...");
}

void loop() {
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  digitalWrite(rgbledPin, LOW);
  //digitalWrite(rgbledPin, HIGH);
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }
  //digitalWrite(ledPin, LOW);
  delay(2000);
}