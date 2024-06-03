#include <dummy.h>
#include <MPU6050.h>
#include "time.h"
#include <stdio.h>
#include <WiFi.h>
#include <WebSocketsServer_Generic.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include <AsyncTCP.h>

// #include <chrono>
#include <esp_timer.h>
#include <stdio.h>
#define FREQUENCY_HZ 50
#define INTERVAL_MS (1000 / (FREQUENCY_HZ + 1))
#define INTERVAL_US INTERVAL_MS * 1000

WebSocketsServer webSocket = WebSocketsServer(80, "");

float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;
float roll, pitch, yaw;
float AccErrorX, AccErrorY, GyroErrorX, GyroErrorY, GyroErrorZ;
float elapsedTime, currentTime, previousTime;
int c = 0;
static unsigned long last_interval_ms = 0;
const char *ntpServer = "pool.ntp.org"; // a cluster of timeservers that anyone can use to request the time
const long gmtOffset_sec = 10800;       // defines the offset in seconds between your time zone and GMT. 3600*3=10800 (UTC+3) Москва
const int daylightOffset_sec = 0;       // variable defines the offset in seconds for daylight saving time

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 accelgyro;
// MPU6050 accelgyro(0x69); // <-- use for AD0 high
int16_t ax, ay, az;
int16_t gx, gy, gz;

// uncomment "OUTPUT_READABLE_ACCELGYRO" if you want to see a tab-separated
// list of the accel X/Y/Z and then gyro X/Y/Z values in decimal. Easy to read,
// not so easy to parse, and slow(er) over UART.
#define OUTPUT_READABLE_ACCELGYRO

const char *ssid = "Tssss!";
const char *password = "SvinkaPePa37";

void initWiFi()
{
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void setup()
{
// join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    bool result = Wire.begin(12, 13, 0); //!!!
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
    Fastwire::setup(400, true);
#endif

    Serial.begin(115200);
    initWiFi();

    // initialize device
    Serial.println("Initializing I2C devices...");
    accelgyro.initialize();

    // verify connection
    Serial.println("Testing device connections...");
    Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

    // Init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printLocalTime();

    webSocket.begin();
}

void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }

    Serial.print(&timeinfo, "%H:%M:%S");
}

void loop()
{
  if (millis() > last_interval_ms + INTERVAL_MS) {
        last_interval_ms = millis();
    webSocket.loop();
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    AccX = ax / 16384.0 * 9.81; // X-axis value
    AccY = ay / 16384.0 * 9.81; // Y-axis value
    AccZ = az / 16384.0 * 9.81; // Z-axis value

    Serial.print(AccX);
    Serial.print(",");
    Serial.print(AccY);
    Serial.print(",");
    Serial.print(AccZ);
    Serial.println(",");

    String str = String(AccX) + "," + String(AccY) + "," + String(AccZ);
    // events.send(, "data", millis());
    webSocket.broadcastTXT(str.c_str());
  }
}
