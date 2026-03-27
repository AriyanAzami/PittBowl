#include <Arduino.h>
#include <U8x8lib.h>

// On the Grove Beginner Kit for Arduino, the onboard LED is connected to Pin 4
const int LED_PIN = 4;

// Onboard OLED is an SSD1315 (compatible with SSD1306) on the I2C bus
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("System Initialization Started...");

  // Initialize the onboard LED
  pinMode(LED_PIN, OUTPUT);
  
  // Initialize the OLED display
  u8x8.begin();
  u8x8.setFlipMode(1);  // Flip the display 180 degrees
  u8x8.setPowerSave(0); // Turn off power save to enable display
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  
  // Display initial setup text
  u8x8.clearDisplay();
  u8x8.drawString(0, 0, "Grove Kit Init");
  u8x8.drawString(0, 2, "Checking setup:");
  u8x8.drawString(0, 4, "- LED (D4)");
  u8x8.drawString(0, 5, "- OLED (I2C)");
  
  Serial.println("System Initialization Complete.");
}

void loop() {
  // Blink the LED and update the OLED to verify things are running
  digitalWrite(LED_PIN, HIGH);
  u8x8.drawString(0, 7, "LED State: ON ");
  Serial.println("LED ON");
  delay(1000);
  
  digitalWrite(LED_PIN, LOW);
  u8x8.drawString(0, 7, "LED State: OFF");
  Serial.println("LED OFF");
  delay(1000);
}
