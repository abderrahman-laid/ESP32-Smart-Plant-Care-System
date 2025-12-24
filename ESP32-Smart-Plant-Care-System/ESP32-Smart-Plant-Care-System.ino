/**
 * Smart Plant Care System with ESP32 and Blynk
 * Created by: Abderrahmane Laid
 * Date: 2025
 *
 * Description: Monitors soil moisture, temperature, humidity, and light.
 * Automatically activates a water pump when moisture is low.
 * Displays data on an OLED screen and syncs with Blynk IoT app.
 */

// Library Includes
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ------------------- CONFIGURATION ------------------- //

// BLYNK CREDENTIALS (Get these from the Blynk Dashboard)
#define BLYNK_TEMPLATE_ID "TMPLxxxxxx"
#define BLYNK_DEVICE_NAME "Smart Plant Care"
#define BLYNK_AUTH_TOKEN "Your_Blynk_Auth_Token_Here"

// WIFI CREDENTIALS
char ssid[] = "Your_WiFi_SSID";
char pass[] = "Your_WiFi_Password";

// PIN DEFINITIONS
#define DHTPIN 4           // DHT11/22 Data pin
#define SOIL_PIN 34        // Capacitive Soil Moisture Sensor (Analog)
#define LDR_PIN 35         // Light Dependent Resistor (Analog)
#define RELAY_PIN 5        // Relay module for Water Pump
#define OLED_SDA 21        // OLED SDA pin
#define OLED_SCL 22        // OLED SCL pin

// SENSOR SETTINGS
#define DHTTYPE DHT11      // DHT 11 (Change to DHT22 if needed)
#define DRY_THRESHOLD 60   // Soil moisture percentage to trigger watering
#define WET_THRESHOLD 70   // Soil moisture percentage to stop watering

// ------------------- OBJECTS & VARIABLES ------------------- //

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
BlynkTimer timer;

// Variables to store sensor readings
float temperature = 0;
float humidity = 0;
int soilMoistureRaw = 0;
int soilMoisturePercent = 0;
int lightLevelRaw = 0;
int lightLevelPercent = 0;
bool isPumpOn = false;

// ------------------- SETUP ------------------- //

void setup() {
  // Debug console
  Serial.begin(115200);

  // Initialize Pins
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Assuming Active LOW Relay (HIGH = OFF)

  // Initialize Sensors
  dht.begin();

  // Initialize OLED Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("Connecting to WiFi...");
  display.display();

  // Initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Setup a timer to run the 'sendSensorData' function every 2 seconds
  timer.setInterval(2000L, sendSensorData);
}

// ------------------- MAIN LOOP ------------------- //

void loop() {
  Blynk.run();
  timer.run();
}

// ------------------- FUNCTIONS ------------------- //

void sendSensorData() {
  // 1. READ SENSORS
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  soilMoistureRaw = analogRead(SOIL_PIN);
  lightLevelRaw = analogRead(LDR_PIN);

  // Check if DHT read failed
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // 2. DATA PROCESSING & MAPPING
  // Map Soil Moisture:
  // IMPORTANT: You must calibrate these values.
  // 4095 is usually completely dry (air), 0-1500 is usually wet (water) on ESP32 12-bit ADC
  // Adjust 'AirValue' and 'WaterValue' based on your sensor calibration
  const int AirValue = 3500;
  const int WaterValue = 1500;
  soilMoisturePercent = map(soilMoistureRaw, AirValue, WaterValue, 0, 100);
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

  // Map Light Level (0-4095) to Percentage
  lightLevelPercent = map(lightLevelRaw, 0, 4095, 0, 100);

  // 3. AUTOMATION LOGIC (AUTO-WATERING)
  // If soil is dry, turn pump ON
  if (soilMoisturePercent < DRY_THRESHOLD) {
    digitalWrite(RELAY_PIN, LOW); // Activate Relay (Active LOW)
    isPumpOn = true;
    Blynk.logEvent("plant_thirsty", "Watering started! Soil is dry.");
  }
  // If soil is wet enough, turn pump OFF
  else if (soilMoisturePercent > WET_THRESHOLD) {
    digitalWrite(RELAY_PIN, HIGH); // Deactivate Relay
    isPumpOn = false;
  }

  // 4. UPDATE OLED DISPLAY
  updateDisplay();

  // 5. SEND DATA TO BLYNK
  // Virtual Pins setup in Blynk Dashboard
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
  Blynk.virtualWrite(V2, soilMoisturePercent);
  Blynk.virtualWrite(V3, lightLevelPercent);
  Blynk.virtualWrite(V4, isPumpOn ? 1 : 0); // Pump Status LED

  // Debug Print
  Serial.print("Temp: "); Serial.print(temperature);
  Serial.print(" | Hum: "); Serial.print(humidity);
  Serial.print(" | Soil: "); Serial.print(soilMoisturePercent);
  Serial.print("% | Light: "); Serial.print(lightLevelPercent);
  Serial.println("%");
}

void updateDisplay() {
  display.clearDisplay();

  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SMART PLANT CARE");
  display.drawLine(0, 10, 128, 10, WHITE);

  // Temperature & Humidity
  display.setCursor(0, 15);
  display.print("Temp: "); display.print(temperature, 1); display.print("C");
  display.setCursor(0, 25);
  display.print("Hum:  "); display.print(humidity, 1); display.print("%");

  // Soil & Light
  display.setCursor(0, 35);
  display.print("Soil: "); display.print(soilMoisturePercent); display.print("%");
  display.setCursor(0, 45);
  display.print("Light:"); display.print(lightLevelPercent); display.print("%");

  // Pump Status
  display.setCursor(0, 55);
  display.print("Pump: ");
  if(isPumpOn) display.print("ON");
  else display.print("OFF");

  display.display();
}