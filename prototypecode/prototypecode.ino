#include <Arduino.h>
#include <U8x8lib.h>

// ==========================================
// CENTRALIZED PIN CONFIGURATION
// ==========================================
// Actuators
const int PUMP_PIN   = 4;  // PWM - Water pump (Moved to D3)
const int LED_PIN    = 3;  // PWM/Dig - Status LED
const int BUZZER_PIN = 5;  // PWM/Dig - Startup Buzzer

// Sensors & Inputs (Grove Beginner Kit: pot on A0; moisture on A2 via Grove cable)
const int BUTTON_PIN = 6;       // Digital - Startup Button
const int POT_PIN = A0;         // Analog - Threshold adjustment
const int MOISTURE_SENSOR_PIN = A2; // Analog - Water level / moisture

// Long-press: release after >= this many ms toggles pause (drain / resume).
// Button is polled every BUTTON_POLL_MS; a 500ms-only loop under-counted hold time and broke resume.
const unsigned long PAUSE_HOLD_MS = 4000;

// Hysteresis around threshold (ADC counts) stops REFILL/WATER OK chatter and OLED flicker
const int MOISTURE_HYST = 45;

// Sensor/control loop period (ms). Lower value = faster response.
const unsigned long SENSOR_PERIOD_MS = 200;

// Fast poll for button edges (ms)
const unsigned long BUTTON_POLL_MS = 15;

// ==========================================
// GLOBAL OBJECTS
// ==========================================
// Onboard OLED is an SSD1315 (compatible with SSD1306) on the I2C bus
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

bool systemPaused = false;
bool prevButtonDown = false;
unsigned long buttonPressStartMs = 0;
bool longPressHandled = false;

static void beepBuzzer(unsigned int freqHz, unsigned int durationMs) {
  tone(BUZZER_PIN, freqHz, durationMs);
  delay(durationMs + 20);
  noTone(BUZZER_PIN);
}

static void pollPauseButton() {
  bool down = digitalRead(BUTTON_PIN) == HIGH;
  if (down && !prevButtonDown) {
    buttonPressStartMs = millis();
    longPressHandled = false;
  }
  if (down && !longPressHandled) {
    unsigned long held = millis() - buttonPressStartMs;
    if (held >= PAUSE_HOLD_MS) {
      systemPaused = !systemPaused;
      longPressHandled = true;
      // Longer, audible beep for pause/resume toggle feedback.
      beepBuzzer(2200, 180);
    }
  }
  if (!down && prevButtonDown) {
    // Reset gate on release so the next long-press can toggle again.
    longPressHandled = false;
  }
  prevButtonDown = down;
}

void setup() {
  Serial.begin(115200);

  // Initialize Pins
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  // Ensure actuators are OFF initially
  analogWrite(PUMP_PIN, 0);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize OLED
  u8x8.begin();
  u8x8.setFlipMode(1);  // Flip display 180 degrees as requested
  u8x8.setPowerSave(0);
  u8x8.setFont(u8x8_font_chroma48medium8_r);

  // ==========================================
  // STARTUP SEQUENCE
  // ==========================================
  u8x8.clearDisplay();
  u8x8.drawString(0, 1, "Ready!");
  u8x8.drawString(0, 3, "Press Button");
  u8x8.drawString(0, 5, "to Start...");
  Serial.println("Waiting for button press to start...");

  // Wait indefinitely until the button is pressed.
  // The Grove button module goes HIGH when pressed.
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(50); // Small delay to prevent busy-waiting
  }

  // Button was pressed!
  Serial.println("Button pressed. Starting system...");

  // Startup confirmation beep (audible on Grove buzzer).
  beepBuzzer(1800, 300);

  // Show Welcome Screen
  u8x8.clearDisplay();
  u8x8.drawString(0, 2, "   PittBowl   ");
  u8x8.drawString(0, 4, " Group: NT5T4 ");

  delay(3000); // Leave welcome message up for 3 seconds
  u8x8.clearDisplay();
}

void loop() {
  static unsigned long lastSensorMs = 0;
  static bool lowWater = false; // Schmitt state: need refill
  static bool firstSample = true;
  static int moistureFiltered = 0;

  unsigned long now = millis();
  pollPauseButton();

  if (now - lastSensorMs >= SENSOR_PERIOD_MS) {
    lastSensorMs = now;

    int moistureLevel = analogRead(MOISTURE_SENSOR_PIN);
    int threshold = analogRead(POT_PIN);

    // Simple low-pass filter to smooth out fast spikes from the sensor.
    if (firstSample) {
      moistureFiltered = moistureLevel;
      firstSample = false;
    } else {
      // 1/2 previous value, 1/2 new reading (faster response than 3/4 + 1/4)
      moistureFiltered = (moistureFiltered + moistureLevel) / 2;
    }

    // Schmitt trigger vs. pot threshold — stable pump/display, no flicker.
    // NOTE: Lower moistureFiltered than threshold means sensor is IN water.
    //       Higher moistureFiltered than threshold means sensor is OUT of water.
    if (lowWater) {
      // Currently in "need refill" state; only clear it once sensor is clearly back in water.
      if (moistureFiltered < threshold - MOISTURE_HYST) {
        lowWater = false;
      }
    } else {
      // Currently "water OK"; only enter refill state once sensor is clearly out of water.
      if (moistureFiltered > threshold + MOISTURE_HYST) {
        lowWater = true;
      }
    }

    Serial.print("Moisture raw: ");
    Serial.print(moistureLevel);
    Serial.print(" | filt: ");
    Serial.print(moistureFiltered);
    Serial.print(" | Thresh: ");
    Serial.print(threshold);
    Serial.print(" | Paused: ");
    Serial.println(systemPaused ? "Y" : "N");

    char line[17];

    if (systemPaused) {
      analogWrite(PUMP_PIN, 0);
      digitalWrite(LED_PIN, LOW);
      snprintf(line, sizeof(line), "%-16s", "Status: PAUSED");
      u8x8.drawString(0, 2, line);
    } else if (lowWater) {
      analogWrite(PUMP_PIN, 100);
      digitalWrite(LED_PIN, HIGH);
      snprintf(line, sizeof(line), "%-16s", "Status: REFILL");
      u8x8.drawString(0, 2, line);
    } else {
      analogWrite(PUMP_PIN, 0);
      digitalWrite(LED_PIN, LOW);
      snprintf(line, sizeof(line), "%-16s", "Status: WATER OK");
      u8x8.drawString(0, 2, line);
    }

    snprintf(line, sizeof(line), "Moist:%4d      ", moistureLevel);
    u8x8.drawString(0, 0, line);
    snprintf(line, sizeof(line), "Thresh:%4d     ", threshold);
    u8x8.drawString(0, 1, line);
  }

  delay(BUTTON_POLL_MS);
}
