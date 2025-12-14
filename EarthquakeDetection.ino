// Earthquake Detection System - ESP32 with Blynk IoT

// ============ BLYNK CREDENTIALS (MUST BE FIRST!) ============
#define BLYNK_TEMPLATE_ID "tempid"
#define BLYNK_TEMPLATE_NAME "templatename"
#define BLYNK_AUTH_TOKEN "authtoken"

// ============ NOW INCLUDE LIBRARIES ============
#include <Wire.h>
#include <Adafruit_ADXL345_U.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// ============ WiFi CREDENTIALS - CHANGE THESE ============
char ssid[] = "name";          // Change this to your WiFi name
char pass[] = "pass";      // Change this to your WiFi password

// ============ OLED DISPLAY SETUP ============
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============ ADXL345 ACCELEROMETER SETUP ============
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// ============ PIN DEFINITIONS ============
const int BUZZER_PIN = 2;
const int LED_PIN = 4;

// ============ EARTHQUAKE DETECTION SETTINGS ============
const float THRESHOLD = 15.0;     // m/s² threshold for earthquake detection
const int ALARM_DURATION = 5000;  // 5 seconds alarm duration
const int BUZZ_PATTERN = 100;     // 100ms buzz on/off pattern

// ============ GLOBAL VARIABLES ============
float maxAcceleration = 0;
unsigned long alarmStartTime = 0;
bool alarmActive = false;
unsigned long lastBlynkSend = 0;
const int BLYNK_SEND_INTERVAL = 500;  // Send data to Blynk every 500ms

// ============ SETUP ============
void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Initialize GPIO pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED initialization failed");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();
  
  // Initialize ADXL345 accelerometer
  if (!accel.begin()) {
    Serial.println("ADXL345 not detected. Check wiring!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("ADXL345 Error");
    display.display();
    while (1);
  }
  
  // Set sensor range to ±16g
  accel.setRange(ADXL345_RANGE_16_G);
  
  // Connect to WiFi and Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  // Display setup complete
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("HAPPY BIRTHDAY");
  display.println("Blynk Enabled");
  display.println("Connecting WiFi...");
  display.display();
  
  Serial.println("System Ready - Connecting to Blynk...");
  delay(2000);
}

// ============ MAIN LOOP ============
void loop() {
  if (Blynk.connected()) {
    Blynk.run();  // Run Blynk connection
  }
  
  // Get sensor data
  sensors_event_t event;
  accel.getEvent(&event);
  
  // Calculate total acceleration magnitude (√(x² + y² + z²))
  float magnitude = sqrt(event.acceleration.x * event.acceleration.x +
                         event.acceleration.y * event.acceleration.y +
                         event.acceleration.z * event.acceleration.z);
  
  // Update maximum acceleration value
  if (magnitude > maxAcceleration) {
    maxAcceleration = magnitude;
  }
  
  // Earthquake detection logic
  if (magnitude > THRESHOLD && !alarmActive) {
    alarmActive = true;
    alarmStartTime = millis();
    Serial.println("⚠️ EARTHQUAKE DETECTED!");
    Blynk.logEvent("earthquake_alert", "Earthquake detected!");  // Send Blynk alert
  }
  
  // Handle alarm duration and buzzer/LED pattern
  if (alarmActive) {
    if (millis() - alarmStartTime < ALARM_DURATION) {
      // Buzzer and LED on/off pattern
      if (((millis() - alarmStartTime) / BUZZ_PATTERN) % 2 == 0) {
        digitalWrite(BUZZER_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
      } else {
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
      }
    } else {
      // Alarm duration finished
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
      alarmActive = false;
      maxAcceleration = 0;
    }
  }
  
  // Send data to Blynk every BLYNK_SEND_INTERVAL milliseconds
  if (millis() - lastBlynkSend >= BLYNK_SEND_INTERVAL && Blynk.connected()) {
    Blynk.virtualWrite(V1, magnitude);  // Send magnitude to V1
    Blynk.virtualWrite(V0, alarmActive ? 1 : 0);  // Send alert status to V0
    lastBlynkSend = millis();
  }
  
  // Update OLED display
  updateDisplay(event.acceleration.x, event.acceleration.y, 
                event.acceleration.z, magnitude);
  
  delay(100);
}

// ============ DISPLAY UPDATE FUNCTION ============
void updateDisplay(float x, float y, float z, float mag) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  // Display title
  display.println("EARTHQUAKE DETECTOR");
  display.println("---Blynk Enabled---");
  
  // Display acceleration values
  display.print("X:"); display.print(x, 1); display.println(" m/s2");
  display.print("Y:"); display.print(y, 1); display.println(" m/s2");
  display.print("Z:"); display.print(z, 1); display.println(" m/s2");
  display.println("");
  
  // Display magnitude and maximum
  display.print("Magnitude: "); display.println(mag, 1);
  display.print("Max: "); display.println(maxAcceleration, 1);
  
  // Display alarm status
  if (alarmActive) {
    display.println("");
    display.println("!!! ALERT !!!");
  } else {
    display.println("");
    if (Blynk.connected()) {
      display.println("ONLINE");
    } else {
      display.println("Connecting...");
    }
  }
  
  display.display();
  
  // Serial output for debugging
  Serial.print("X:"); Serial.print(x); Serial.print(" Y:"); Serial.print(y);
  Serial.print(" Z:"); Serial.print(z); Serial.print(" | Mag:"); 
  Serial.print(mag); Serial.print(" | Status:");
  Serial.println(Blynk.connected() ? "ONLINE" : "OFFLINE");
}
