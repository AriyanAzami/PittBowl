#include <Arduino.h>
#include <U8x8lib.h>

// ==========================================
// CENTRALIZED PIN CONFIGURATION
// ==========================================
// Actuators
const int PUMP_PIN   = 3;  // PWM - Water pump (Moved to D3)
const int LED_PIN    = 4;  // PWM/Dig - Status LED
const int BUZZER_PIN = 5;  // PWM/Dig - Startup Buzzer

// Sensors & Inputs
const int BUTTON_PIN = 6;  // Digital - Startup Button
const int MOISTURE_SENSOR_PIN = A0; // Analog - Water level detection
const int LIGHT_SENSOR_PIN    = A6; // Analog - Ambient light / Debris shield detection

// Thresholds for logic (Tune these as needed when testing)
const int MOISTURE_THRESHOLD = 400; // Below this = Low Water (0 = dry, 1023 = fully submerged)
const int LIGHT_THRESHOLD    = 100; // Below this = Dark

// ==========================================
// GLOBAL OBJECTS
// ==========================================
// Onboard OLED is an SSD1315 (compatible with SSD1306) on the I2C bus
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

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
  // The Grove button modular goes HIGH when pressed.
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(50); // Small delay to prevent busy-waiting
  }

  // Button was pressed! 
  Serial.println("Button pressed. Starting system...");
  
  // Sound the buzzer (short beep)
  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);
  digitalWrite(BUZZER_PIN, LOW);

  // Show Welcome Screen
  u8x8.clearDisplay();
  u8x8.drawString(0, 2, "   PittBowl   ");
  u8x8.drawString(0, 4, " Group: NT5T4 ");
  
  delay(3000); // Leave welcome message up for 3 seconds
  u8x8.clearDisplay();
}

void loop() {
  // Read Sensors
  int moistureLevel = analogRead(MOISTURE_SENSOR_PIN);
  int lightLevel    = analogRead(LIGHT_SENSOR_PIN);

  // Output to Serial for debugging monitor
  Serial.print("Moisture: ");
  Serial.print(moistureLevel);
  Serial.print(" | Light: ");
  Serial.println(lightLevel);

  // Logic: Is the water low?
  bool isWaterLow = (moistureLevel < MOISTURE_THRESHOLD);

  if (isWaterLow) {
    // Actuate: LOW WATER
    analogWrite(PUMP_PIN, 100); // 100/255 speed, as requested in history
    digitalWrite(LED_PIN, HIGH);
    
    // Update display
    u8x8.drawString(0, 0, " Status:      ");
    u8x8.drawString(0, 1, " REFILLING... ");
  } else {
    // Actuate: ADEQUATE WATER
    analogWrite(PUMP_PIN, 0); // Turn pump off
    digitalWrite(LED_PIN, LOW);
    
    // Update display
    u8x8.drawString(0, 0, " Status:      ");
    u8x8.drawString(0, 1, " WATER OK     ");
  }

  // Display sensor readings on OLED for live tuning
  u8x8.setCursor(0, 4);
  u8x8.print("Moist: ");
  u8x8.print(moistureLevel);
  u8x8.print("   "); // Clear trailing chars

  u8x8.setCursor(0, 6);
  u8x8.print("Light: ");
  u8x8.print(lightLevel);
  u8x8.print("   ");

  delay(500); // Run loop every half second
}
