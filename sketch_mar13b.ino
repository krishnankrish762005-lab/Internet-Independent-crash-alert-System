#include <Arduino.h>
#include <TinyGPS++.h>

// --- PIN CONFIGURATION ---
const int CRASH_SENSOR_PIN = 13; 
const int STOP_BUTTON_PIN = 3;   
const int WARNING_LED_PIN = 16;  

TinyGPSPlus gps;

// --- DATA ---
String vehicleID = "TN 36 BG 4586";
String emergencyContact1 = "+91XXXXXXXXXX"; // Update with real number
double currentLat = 0.0;
double currentLong = 0.0;

// --- STATE VARIABLES ---
bool crashDetected = false;
unsigned long crashTime = 0;
const unsigned long countdownDuration = 10000; 

void sendSOS();

void setup() {
  Serial.begin(115200); 
  Serial1.begin(9600);  // SIM800L
  Serial2.begin(9600);  // GPS
  
  pinMode(CRASH_SENSOR_PIN, INPUT_PULLUP); 
  pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(WARNING_LED_PIN, OUTPUT);

  // IMPORTANT: Startup delay to let power stabilize
  Serial.println("\n--- SYSTEM STARTING: STABILIZING POWER ---");
  for(int i=0; i<5; i++) {
    digitalWrite(WARNING_LED_PIN, HIGH); delay(100);
    digitalWrite(WARNING_LED_PIN, LOW); delay(100);
  }
  
  Serial1.write("AT\r\n"); 
  delay(1000);
  Serial1.write("AT+CMGF=1\r\n"); 
  Serial.println("READY. Waiting for actual crash signal...");
}

void loop() {
  // 1. GPS BACKGROUND
  while (Serial2.available() > 0) {
    if (gps.encode(Serial2.read())) {
      if (gps.location.isValid()) {
        currentLat = gps.location.lat();
        currentLong = gps.location.lng();
      }
    }
  }

  // 2. CRASH DETECTION (With 500ms safety debounce)
  // Only trigger if pin 13 is LOW for more than half a second
  if (digitalRead(CRASH_SENSOR_PIN) == LOW && !crashDetected) {
    delay(500); 
    if (digitalRead(CRASH_SENSOR_PIN) == LOW) { // Verify it's still LOW
        Serial.println("\n!!! CRASH CONFIRMED !!!");
        crashDetected = true;
        crashTime = millis();
    }
  }

  // 3. SOS SEQUENCE
  if (crashDetected) {
    unsigned long elapsed = millis() - crashTime;
    digitalWrite(WARNING_LED_PIN, (millis() / 200) % 2); // Blink LED

    if (digitalRead(STOP_BUTTON_PIN) == LOW) {
      crashDetected = false;
      digitalWrite(WARNING_LED_PIN, LOW); 
      Serial.println("SOS CANCELLED BY USER.");
      delay(2000);
    }
    else if (elapsed >= countdownDuration) {
      sendSOS();
      crashDetected = false;
    }
  }

  // 4. PASSTHROUGH
  while (Serial.available()) { Serial1.write(Serial.read()); }
  while (Serial1.available()) { Serial.write(Serial1.read()); }
}

void sendSOS() {
  Serial.println("\n>>> DISPATCHING SOS...");
  Serial1.write("AT+CMGF=1\r\n");
  delay(500);
  Serial1.print("AT+CMGS=\"");
  Serial1.print(emergencyContact1);
  Serial1.write("\"\r\n");
  
  delay(3000); // Wait for '>' prompt
  
  String mapLink = "http://google.com/maps?q=" + String(currentLat, 6) + "," + String(currentLong, 6);
  Serial1.print("SOS ALERT: " + vehicleID + " Loc: " + mapLink);
  
  delay(1000);
  Serial1.write(26); // Ctrl+Z
  Serial.println("COMMANDS SENT.");
}
