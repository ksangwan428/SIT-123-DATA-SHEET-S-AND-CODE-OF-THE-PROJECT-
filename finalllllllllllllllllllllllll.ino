#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>

#define RST_PIN 9
#define SS_PIN 10
#define FLOW_SENSOR_PIN 5  // Change this to the appropriate pin connected to the flow sensor

// MFRC522 RFID module
MFRC522 rfid(SS_PIN, RST_PIN);
#define ULTRASONIC_TRIG_PIN 6
#define ULTRASONIC_ECHO_PIN 7
bool isUltrasonicHigh = false;
bool wasUltrasonicHigh = false;

// Relay pins
const int solenoidRelayPin = 4;
const int pumpRelayPin = 3;

// Flow sensor
volatile int flowPulseCount = 0;
float flowRate = 0.0;
unsigned int flowMilliLitres = 0;
unsigned long totalMilliLitres = 0;
unsigned long oldTime = 0;

// LCD Display
LiquidCrystal_I2C lcd(0x27, 16, 2);    // Adjust the I2C address if necessary

// Variables
const String preDefinedRFID = "D4A17B29";  // Pre-defined RFID tag UID as a string
const unsigned long pouringDuration = 3000;  // Pouring duration in milliseconds
bool isPouring = false;  // Flag to track pouring state
unsigned long pouringStartTime = 0;  // Timestamp of pouring start

void setup() {
  // Initialize LCD display
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.print("Milk Dispenser");

  // Initialize RFID reader
  SPI.begin();
  rfid.PCD_Init();

  // Initialize relay pins
  pinMode(solenoidRelayPin, OUTPUT);
  pinMode(pumpRelayPin, OUTPUT);

  // Set relay pins to initial state (OFF)
  digitalWrite(solenoidRelayPin, HIGH);
  digitalWrite(pumpRelayPin, HIGH);

  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);

  // Initialize flow sensor
  pinMode(FLOW_SENSOR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);
}

void loop() {
  checkUltrasonicSensor();
  // Check for RFID tag
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String rfidTag = getRFIDTagAsString();

    // Compare RFID tag with pre-defined tag
    if (rfidTag.equalsIgnoreCase(preDefinedRFID)) {
      lcd.clear();
      lcd.print("Access Granted");
      delay(1000);

      lcd.clear();
      lcd.print("Pouring Milk...");

      // Start pouring
      startPouring();

      // Wait for pouring duration
      delay(pouringDuration);

      // Stop pouring
      stopPouring();

      lcd.clear();
      lcd.print("Milk Poured");
      lcd.setCursor(0, 1);
      lcd.print("Volume: ");
      lcd.print(totalMilliLitres);
      lcd.print(" ml");
      delay(3000);
    } else {
      lcd.clear();
      lcd.print("Access Denied");
      delay(3000);
    }

    rfid.PICC_HaltA();  // Halt PICC
    rfid.PCD_StopCrypto1();  // Stop encryption on PCD
  }
}

// Function to convert the RFID tag UID to a string
String getRFIDTagAsString() {
  String tagString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      tagString += "0";
    }
    tagString += String(rfid.uid.uidByte[i], HEX);
  }
  return tagString;
}

void checkUltrasonicSensor() {
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

  long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH);
  float distance = duration * 0.034 / 2;

  if (distance > 10) {  // Adjust the distance threshold as needed
    isUltrasonicHigh = true;
  } else {
    isUltrasonicHigh = false;
  }

  if (wasUltrasonicHigh && !isUltrasonicHigh) {
    stopPouring();
  }

  wasUltrasonicHigh = isUltrasonicHigh;
}

// Function to start pouring
void startPouring() {
  if (isUltrasonicHigh) {
    digitalWrite(solenoidRelayPin, LOW);
    digitalWrite(pumpRelayPin, LOW);
    isPouring = true;
    pouringStartTime = millis();
  }
}

void stopPouring() {
  if (!isUltrasonicHigh) {
    digitalWrite(solenoidRelayPin, HIGH);
    digitalWrite(pumpRelayPin, HIGH);
    isPouring = false;
  }
}

// Flow sensor interrupt handler
void pulseCounter() {
  if (isPouring) {
    flowPulseCount++;
    totalMilliLitres = flowPulseCount * 2;
  }
}
