#include <Wire.h>
#include <Adafruit_VEML7700.h>
#include <Adafruit_AHTX0.h>

#define TOUCH_PIN   17
#define LED_PWM_PIN 18
#define PIR_PIN     39

Adafruit_VEML7700 veml;
Adafruit_AHTX0 aht;

// ==================================================
// BRIGHTNESS LEVELS
// ==================================================

int brightnessLevels[] = {
  255,   // 100%
  204,   // 80%
  153,   // 60%
  102,   // 40%
  51,    // 20%
  0      // OFF
};

int currentLevel = 0;

// ==================================================
// TOUCH CONTROL
// ==================================================

int lastTouchState = LOW;

bool autoMode = true;

unsigned long lastTouchTime = 0;

const unsigned long MANUAL_OVERRIDE_TIME = 10000;

// ==================================================
// PIR OCCUPANCY
// ==================================================

unsigned long lastMotionTime = 0;

const unsigned long NO_MOTION_TIMEOUT = 10000;

bool occupied = true;

// ==================================================
// SERIAL / SENSOR TIMING
// ==================================================

unsigned long lastSensorPrint = 0;

const unsigned long SENSOR_PRINT_INTERVAL = 1000;

// ==================================================
// DEMO CLOCK
// RTC NOT CONNECTED YET
// ==================================================

int demoHour = 8;
int demoMinute = 30;
int demoSecond = 0;

unsigned long lastClockUpdate = 0;

// ==================================================
// SETUP
// ==================================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  pinMode(TOUCH_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  // LED PWM
  ledcAttach(LED_PWM_PIN, 5000, 8);

  // Start at full brightness
  ledcWrite(LED_PWM_PIN, 255);

  // I2C
  Wire.begin(21, 22);

  Serial.println();
  Serial.println("========================================");
  Serial.println("            DESKBEAM STARTUP");
  Serial.println("========================================");

  // VEML7700
  if (!veml.begin()) {
    Serial.println("[ERROR] VEML7700 not detected!");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("[OK] VEML7700 detected");

  // AHT20/AHT21
  if (!aht.begin()) {
    Serial.println("[ERROR] AHT20/AHT21 not detected!");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("[OK] AHT20/AHT21 detected");

  Serial.println("[OK] PIR initialized");
  Serial.println("[OK] Capacitive touch initialized");
  Serial.println("[OK] LED PWM initialized");

  Serial.println("[DEMO] RTC not connected");
  Serial.println("[DEMO] Simulated clock enabled");

  Serial.println("========================================");
  Serial.println("DeskBeam ready");
  Serial.println("========================================");
  Serial.println();

  lastMotionTime = millis();
  lastClockUpdate = millis();
}

// ==================================================
// MAIN LOOP
// ==================================================

void loop() {

  // ==================================================
  // DEMO CLOCK
  // ==================================================

  if (millis() - lastClockUpdate >= 1000) {

    lastClockUpdate += 1000;

    demoSecond++;

    if (demoSecond >= 60) {
      demoSecond = 0;
      demoMinute++;
    }

    if (demoMinute >= 60) {
      demoMinute = 0;
      demoHour++;
    }

    if (demoHour >= 24) {
      demoHour = 0;
    }
  }

  // ==================================================
  // PIR OCCUPANCY
  // ==================================================

  int motionState = digitalRead(PIR_PIN);

  if (motionState == HIGH) {

    lastMotionTime = millis();

    if (!occupied) {
      Serial.println();
      Serial.println(">>> MOTION DETECTED - LAMP ACTIVE");
    }

    occupied = true;
  }

  if (millis() - lastMotionTime >= NO_MOTION_TIMEOUT) {

    if (occupied) {
      Serial.println();
      Serial.println(">>> NO MOTION FOR 10 SECONDS");
      Serial.println(">>> LAMP OFF");
    }

    occupied = false;
  }

  // ==================================================
  // TOUCH CONTROL
  // ==================================================

  int touchState = digitalRead(TOUCH_PIN);

  if (touchState == HIGH && lastTouchState == LOW) {

    currentLevel++;

    if (currentLevel > 5) {
      currentLevel = 0;
    }

    autoMode = false;
    lastTouchTime = millis();

    Serial.println();
    Serial.println(">>> MANUAL TOUCH CONTROL");

    Serial.print("Brightness set to: ");
    Serial.print(brightnessLevels[currentLevel] * 100 / 255);
    Serial.println("%");
  }

  lastTouchState = touchState;

  // Return to AUTO after 10 seconds
  if (!autoMode &&
      millis() - lastTouchTime >= MANUAL_OVERRIDE_TIME) {

    autoMode = true;

    Serial.println();
    Serial.println(">>> RETURNING TO AUTO MODE");
  }

  // ==================================================
  // READ SENSORS
  // ==================================================

  float lux = veml.readLux();

  sensors_event_t humidity;
  sensors_event_t temperature;

  aht.getEvent(&humidity, &temperature);

  // ==================================================
  // AUTO BRIGHTNESS
  // ==================================================

  int autoBrightness = 0;
  int finalBrightness = 0;

  if (autoMode) {

    if (lux < 50) {
      autoBrightness = 255;       // 100%
    }
    else if (lux < 150) {
      autoBrightness = 204;       // 80%
    }
    else if (lux < 300) {
      autoBrightness = 153;       // 60%
    }
    else if (lux < 500) {
      autoBrightness = 102;       // 40%
    }
    else if (lux < 800) {
      autoBrightness = 51;        // 20%
    }
    else {
      autoBrightness = 0;         // OFF
    }

    finalBrightness = autoBrightness;
  }

  else {
    finalBrightness = brightnessLevels[currentLevel];
  }

  // PIR always has final authority
  if (!occupied) {
    finalBrightness = 0;
  }

  ledcWrite(LED_PWM_PIN, finalBrightness);

  // ==================================================
  // SERIAL MONITOR DISPLAY
  // ==================================================

  if (millis() - lastSensorPrint >= SENSOR_PRINT_INTERVAL) {

    lastSensorPrint = millis();

    Serial.println();
    Serial.println("========================================");
    Serial.println("             DESKBEAM DISPLAY");
    Serial.println("========================================");

    // TIME
    Serial.print("Time          : ");

    if (demoHour < 10) Serial.print("0");
    Serial.print(demoHour);

    Serial.print(":");

    if (demoMinute < 10) Serial.print("0");
    Serial.print(demoMinute);

    Serial.print(":");

    if (demoSecond < 10) Serial.print("0");
    Serial.println(demoSecond);

    // TEMPERATURE
    Serial.print("Temperature   : ");
    Serial.print(temperature.temperature, 1);
    Serial.println(" C");

    // HUMIDITY
    Serial.print("Humidity      : ");
    Serial.print(humidity.relative_humidity, 1);
    Serial.println(" %");

    // AMBIENT LIGHT
    Serial.print("Ambient Light : ");
    Serial.print(lux, 1);
    Serial.println(" lux");

    // OCCUPANCY
    Serial.print("Occupancy     : ");

    if (occupied) {
      Serial.println("PRESENT");
    }
    else {
      Serial.println("NOT PRESENT");
    }

    // MODE
    Serial.print("Light Mode    : ");

    if (autoMode) {
      Serial.println("AUTO");
    }
    else {
      Serial.println("MANUAL");
    }

    // BRIGHTNESS
    Serial.print("Brightness    : ");
    Serial.print(finalBrightness * 100 / 255);
    Serial.println("%");

    // RTC STATUS
    Serial.println("----------------------------------------");
    Serial.println("RTC Status    : DEMO / NOT CONNECTED");

    Serial.println("========================================");
  }

  delay(50);
}

#include <Wire.h>
#include <Adafruit_VEML7700.h>
#include <Adafruit_AHTX0.h>

#define TOUCH_PIN   17
#define LED_PWM_PIN 18
#define PIR_PIN     39

Adafruit_VEML7700 veml;
Adafruit_AHTX0 aht;

// ==================================================
// BRIGHTNESS LEVELS
// ==================================================

int brightnessLevels[] = {
  255,   // 100%
  204,   // 80%
  153,   // 60%
  102,   // 40%
  51,    // 20%
  0      // OFF
};

int currentLevel = 0;

// ==================================================
// TOUCH CONTROL
// ==================================================

int lastTouchState = LOW;

bool autoMode = true;

unsigned long lastTouchTime = 0;

const unsigned long MANUAL_OVERRIDE_TIME = 10000;

// ==================================================
// PIR OCCUPANCY
// ==================================================

unsigned long lastMotionTime = 0;

const unsigned long NO_MOTION_TIMEOUT = 10000;

bool occupied = true;

// ==================================================
// SERIAL / SENSOR TIMING
// ==================================================

unsigned long lastSensorPrint = 0;

const unsigned long SENSOR_PRINT_INTERVAL = 1000;

// ==================================================
// DEMO CLOCK
// RTC NOT CONNECTED YET
// ==================================================

int demoHour = 8;
int demoMinute = 30;
int demoSecond = 0;

unsigned long lastClockUpdate = 0;

// ==================================================
// SETUP
// ==================================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  pinMode(TOUCH_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  // LED PWM
  ledcAttach(LED_PWM_PIN, 5000, 8);

  // Start at full brightness
  ledcWrite(LED_PWM_PIN, 255);

  // I2C
  Wire.begin(21, 22);

  Serial.println();
  Serial.println("========================================");
  Serial.println("            DESKBEAM STARTUP");
  Serial.println("========================================");

  // VEML7700
  if (!veml.begin()) {
    Serial.println("[ERROR] VEML7700 not detected!");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("[OK] VEML7700 detected");

  // AHT20/AHT21
  if (!aht.begin()) {
    Serial.println("[ERROR] AHT20/AHT21 not detected!");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("[OK] AHT20/AHT21 detected");

  Serial.println("[OK] PIR initialized");
  Serial.println("[OK] Capacitive touch initialized");
  Serial.println("[OK] LED PWM initialized");

  Serial.println("[DEMO] RTC not connected");
  Serial.println("[DEMO] Simulated clock enabled");

  Serial.println("========================================");
  Serial.println("DeskBeam ready");
  Serial.println("========================================");
  Serial.println();

  lastMotionTime = millis();
  lastClockUpdate = millis();
}

// ==================================================
// MAIN LOOP
// ==================================================

void loop() {

  // ==================================================
  // DEMO CLOCK
  // ==================================================

  if (millis() - lastClockUpdate >= 1000) {

    lastClockUpdate += 1000;

    demoSecond++;

    if (demoSecond >= 60) {
      demoSecond = 0;
      demoMinute++;
    }

    if (demoMinute >= 60) {
      demoMinute = 0;
      demoHour++;
    }

    if (demoHour >= 24) {
      demoHour = 0;
    }
  }

  // ==================================================
  // PIR OCCUPANCY
  // ==================================================

  int motionState = digitalRead(PIR_PIN);

  if (motionState == HIGH) {

    lastMotionTime = millis();

    if (!occupied) {
      Serial.println();
      Serial.println(">>> MOTION DETECTED - LAMP ACTIVE");
    }

    occupied = true;
  }

  if (millis() - lastMotionTime >= NO_MOTION_TIMEOUT) {

    if (occupied) {
      Serial.println();
      Serial.println(">>> NO MOTION FOR 10 SECONDS");
      Serial.println(">>> LAMP OFF");
    }

    occupied = false;
  }

  // ==================================================
  // TOUCH CONTROL
  // ==================================================

  int touchState = digitalRead(TOUCH_PIN);

  if (touchState == HIGH && lastTouchState == LOW) {

    currentLevel++;

    if (currentLevel > 5) {
      currentLevel = 0;
    }

    autoMode = false;
    lastTouchTime = millis();

    Serial.println();
    Serial.println(">>> MANUAL TOUCH CONTROL");

    Serial.print("Brightness set to: ");
    Serial.print(brightnessLevels[currentLevel] * 100 / 255);
    Serial.println("%");
  }

  lastTouchState = touchState;

  // Return to AUTO after 10 seconds
  if (!autoMode &&
      millis() - lastTouchTime >= MANUAL_OVERRIDE_TIME) {

    autoMode = true;

    Serial.println();
    Serial.println(">>> RETURNING TO AUTO MODE");
  }

  // ==================================================
  // READ SENSORS
  // ==================================================

  float lux = veml.readLux();

  sensors_event_t humidity;
  sensors_event_t temperature;

  aht.getEvent(&humidity, &temperature);

  // ==================================================
  // AUTO BRIGHTNESS
  // ==================================================

  int autoBrightness = 0;
  int finalBrightness = 0;

  if (autoMode) {

    if (lux < 50) {
      autoBrightness = 255;       // 100%
    }
    else if (lux < 150) {
      autoBrightness = 204;       // 80%
    }
    else if (lux < 300) {
      autoBrightness = 153;       // 60%
    }
    else if (lux < 500) {
      autoBrightness = 102;       // 40%
    }
    else if (lux < 800) {
      autoBrightness = 51;        // 20%
    }
    else {
      autoBrightness = 0;         // OFF
    }

    finalBrightness = autoBrightness;
  }

  else {
    finalBrightness = brightnessLevels[currentLevel];
  }

  // PIR always has final authority
  if (!occupied) {
    finalBrightness = 0;
  }

  ledcWrite(LED_PWM_PIN, finalBrightness);

  // ==================================================
  // SERIAL MONITOR DISPLAY
  // ==================================================

  if (millis() - lastSensorPrint >= SENSOR_PRINT_INTERVAL) {

    lastSensorPrint = millis();

    Serial.println();
    Serial.println("========================================");
    Serial.println("             DESKBEAM DISPLAY");
    Serial.println("========================================");

    // TIME
    Serial.print("Time          : ");

    if (demoHour < 10) Serial.print("0");
    Serial.print(demoHour);

    Serial.print(":");

    if (demoMinute < 10) Serial.print("0");
    Serial.print(demoMinute);

    Serial.print(":");

    if (demoSecond < 10) Serial.print("0");
    Serial.println(demoSecond);

    // TEMPERATURE
    Serial.print("Temperature   : ");
    Serial.print(temperature.temperature, 1);
    Serial.println(" C");

    // HUMIDITY
    Serial.print("Humidity      : ");
    Serial.print(humidity.relative_humidity, 1);
    Serial.println(" %");

    // AMBIENT LIGHT
    Serial.print("Ambient Light : ");
    Serial.print(lux, 1);
    Serial.println(" lux");

    // OCCUPANCY
    Serial.print("Occupancy     : ");

    if (occupied) {
      Serial.println("PRESENT");
    }
    else {
      Serial.println("NOT PRESENT");
    }

    // MODE
    Serial.print("Light Mode    : ");

    if (autoMode) {
      Serial.println("AUTO");
    }
    else {
      Serial.println("MANUAL");
    }

    // BRIGHTNESS
    Serial.print("Brightness    : ");
    Serial.print(finalBrightness * 100 / 255);
    Serial.println("%");

    // RTC STATUS
    Serial.println("----------------------------------------");
    Serial.println("RTC Status    : DEMO / NOT CONNECTED");

    Serial.println("========================================");
  }

  delay(50);
}
