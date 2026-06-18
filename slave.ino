

//TESTED-WORKING 09/07/2025


// This sketch reads analog signals from ADS1115 (via I2C),
// calculates the corresponding current (Amps) using calibration values,Serialbus
// and sends data via both USB Serial and SoftwareSerial (TX/RX).
//
// Commands supported:
//   $a1 - $a4     : Read current in Amps from channel 1–4
//   $v1 - $v4     : Read voltage in millivolts from channel 1–4
//   $sX=mv,amps   : Save calibration values (mv and amps) for channel X (1–4)
//   $all          : Read voltage and current from all 4 channels
//
// TX and RX LEDs connected to pins A2 and A3 blink briefly
// when transmitting or receiving data.
//
// Calibration data is stored in EEPROM, so it persists after reboot.
//
// Voltage input is expected to be below 75 mV,
// so gain is fixed to highest (GAIN_SIXTEEN) for best resolution.

#include <SoftwareSerial.h>
#include <ModbusRTUSlave.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <EEPROM.h>


#define MODBUS_CONFIG SERIAL_8N1

#define MODBUS_SERIAL modbussw

#define SLAVE_ID_ADDR 10 //Slave Address stored in EEPROM
#define BAUD_ADDR 20 //Baud Address stored in EEPROM

#define eepromdelaytime 10000 //This delay is used to update data every 60 seconds

#define SLAVE_ID_CHANGE 10

 //#define MODBUS_SERIAL modbus
SoftwareSerial modbussw(2, 3);

Adafruit_ADS1115 ads;
//ModbusRTUSlave modbus(MODBUS_SERIAL, dePin);
ModbusRTUSlave modbus(MODBUS_SERIAL);

int MODBUS_BAUD=4800;
int MODBUS_UNIT_ID=1;




const int rxLed = A3;
const int txLed = A2;

const int blinkDuration = 100;
unsigned long rxBlinkStart = 0;
unsigned long txBlinkStart = 0;
bool rxBlinking = false;
bool txBlinking = false;

const int LED_OFF = 255;
const int LED_ON = 0;

float mV;
float voltage;
float amps;

const int readaddress = 4; //EEPROM read address
unsigned long eepromSaveTime = 0; //Used to initialize the EEPROM

unsigned long eepromReadTime = 0; // For 30-second display



float set_mV[4] = {75.0, 75.0, 75.0, 75.0};
float set_amps[4] = {400.0, 400.0, 400.0, 400.0};

const int EEPROM_ADDR_MV[4] = {0, 4, 8, 12};
const int EEPROM_ADDR_AMPS[4] = {16, 20, 24, 28};

const uint8_t numCoils = 4;
const uint8_t numDiscreteInputs = 4;
const uint8_t numHoldingRegisters = 20;
const uint8_t numInputRegisters = 8;

bool coils[numCoils];
bool discreteInputs[numDiscreteInputs];
uint16_t holdingRegisters[numHoldingRegisters];
uint16_t inputRegisters[numInputRegisters];

unsigned long lastUpdate = 0;
const unsigned long interval = 3000;  // 3 seconds


void setup() {
  pinMode(rxLed, OUTPUT);
  pinMode(txLed, OUTPUT);
  analogWrite(rxLed, LED_OFF);
  analogWrite(txLed, LED_OFF);

  Serial.begin(115200);
 // modbus.begin(9600);

  EEPROM.get(SLAVE_ID_ADDR, MODBUS_UNIT_ID);
  EEPROM.get(BAUD_ADDR, MODBUS_BAUD);

  if (MODBUS_UNIT_ID < 1 || MODBUS_UNIT_ID > 247){
    MODBUS_UNIT_ID = 1;
    Serial.println("Invalid EEPROM Slave ID, using default 1.");
  }
  if (MODBUS_BAUD != 4800 && MODBUS_BAUD != 9600) {
    MODBUS_BAUD = 4800;
    Serial.println("Invalid EEPROM Baud Rate, using default 4800.");
  }

  Wire.begin();

  ads.begin(0x49);
  ads.setGain(GAIN_SIXTEEN);  // Fixed gain, ±0.256V (ideal for up to 256mV)

  //modbus.configureCoils(coils, numCoils);
  //modbus.configureDiscreteInputs(discreteInputs, numDiscreteInputs);
  modbus.configureHoldingRegisters(holdingRegisters, numHoldingRegisters);
  //modbus.configureInputRegisters(inputRegisters, numInputRegisters);

  //MODBUS_SERIAL.begin(MODBUS_BAUD, MODBUS_CONFIG);
  MODBUS_SERIAL.begin(MODBUS_BAUD);
  modbus.begin(MODBUS_UNIT_ID, MODBUS_BAUD, MODBUS_CONFIG);


  for (int i = 0; i < 4; i++) {
    EEPROM.get(EEPROM_ADDR_MV[i], mV);
    EEPROM.get(EEPROM_ADDR_AMPS[i], amps);

    if (mV > 0.01 && mV < 10000 && amps > 0.01 && amps < 10000) {
      set_mV[i] = mV;
      set_amps[i] = amps;
    } else {
      set_mV[i] = 75.0;
      set_amps[i] = 400.0;

    }
  }
}

void blinkLed(int ledPin, unsigned long &blinkStart, bool &blinking) {
  analogWrite(ledPin, LED_ON);
  blinkStart = millis();
  blinking = true;
}

void processCommand(String input) {
  input.trim();
  input.toLowerCase();
  blinkLed(rxLed, rxBlinkStart, rxBlinking);
  
if (input == "$all") {
    for (int ch = 0; ch < 4; ch++) {
      voltage = ads.computeVolts(ads.readADC_SingleEnded(ch));
      mV = voltage * 1000.0;
      amps = (mV / set_mV[ch]) * set_amps[ch];

      Serial.print("CH");
      Serial.print(ch + 1);
      Serial.print(": Voltage (mV) = ");
      Serial.print(mV, 2);
      Serial.print("  Amps = ");
      Serial.println(amps, 2);
    }
    blinkLed(txLed, txBlinkStart, txBlinking);
    return;
  }
  if (input.startsWith("$a")) {
    if (input.length() == 3) {
      int ch = input.charAt(2) - '1';
      if (ch >= 0 && ch <= 3) {
        voltage = ads.computeVolts(ads.readADC_SingleEnded(ch));
        mV = voltage * 1000.0;
        amps = (mV / set_mV[ch]) * set_amps[ch];

        Serial.print(" ");
        Serial.println(amps, 2);
        modbussw.print(" ");
        modbussw.println(amps, 2);
        blinkLed(txLed, txBlinkStart, txBlinking);
        return;
      }
    }
    Serial.println(F("Invalid $a command. Use $a1 - $a4."));
   // modbussw.println(F("Invalid $a command. Use $a1 - $a4."));
    return;
  }

 else  if (input.startsWith("$v")) {
    if (input.length() == 3) {
      int ch = input.charAt(2) - '1';
      if (ch >= 0 && ch <= 3) {
        voltage = ads.computeVolts(ads.readADC_SingleEnded(ch));
        mV = voltage * 1000.0;

        Serial.print(" ");
        Serial.println(mV, 2);
   
        blinkLed(txLed, txBlinkStart, txBlinking);
        return;
      }
    }
    Serial.println(F("Invalid $v command. Use $v1 - $v4."));
   
    return;
  }

  else if (input.startsWith("$s")) {
    if (input.length() > 3 && input.charAt(2) >= '1' && input.charAt(2) <= '4' && input.charAt(3) == '=') {
      int ch = input.charAt(2) - '1';
      String values = input.substring(4);
      int commaIndex = values.indexOf(',');
      if (commaIndex == -1) {
        Serial.println(F("Invalid format. Use $sX=mv,amps"));
      
        return;
      }

      float new_mV = values.substring(0, commaIndex).toFloat();
      float new_amps = values.substring(commaIndex + 1).toFloat();

      if (new_mV > 0.01 && new_amps > 0.01) {
        set_mV[ch] = new_mV;
        set_amps[ch] = new_amps;

        EEPROM.put(EEPROM_ADDR_MV[ch], set_mV[ch]);
        EEPROM.put(EEPROM_ADDR_AMPS[ch], set_amps[ch]);

        Serial.print(F("Calibration updated for channel "));
        Serial.print(ch + 1);
        Serial.print(F(": mV = "));
        Serial.print(set_mV[ch], 2);
        Serial.print(F(", Amps = "));
        Serial.println(set_amps[ch], 2);

        blinkLed(txLed, txBlinkStart, txBlinking);
      } else {
        Serial.println(F("Invalid calibration values."));
        
      }
      return;
    }
    Serial.println(F("Invalid calibration command format."));
   
    return;
  }

 

  Serial.println(F("CMDUC"));
  //modbus.println(F("CMDUC"));
}

void loop() {
  unsigned long now = millis();

  if (rxBlinking && now - rxBlinkStart >= blinkDuration) {
    analogWrite(rxLed, LED_OFF);
    rxBlinking = false;
  }

  if (txBlinking && now - txBlinkStart >= blinkDuration) {
    analogWrite(txLed, LED_OFF);
    txBlinking = false;
  }

//EEPROM
if (millis() - eepromSaveTime >= 60000) {
  Serial.println("---- EEPROM CH1–CH4 ----");

  for (int ch = 0; ch < 4; ch++) {
    float voltage = ads.computeVolts(ads.readADC_SingleEnded(ch));
    float mV = voltage * 1000.0;
    float amps = (mV / set_mV[ch]) * set_amps[ch];

    // Store to EEPROM
    EEPROM.put(EEPROM_ADDR_MV[ch], mV);
    EEPROM.put(EEPROM_ADDR_AMPS[ch], amps);

    // Read back from EEPROM
    float read_mV = 0.0, read_amps = 0.0;
    EEPROM.get(EEPROM_ADDR_MV[ch], read_mV);
    EEPROM.get(EEPROM_ADDR_AMPS[ch], read_amps);
    Serial.print("CH");
    Serial.print(ch + 1);
    Serial.print(" mV: ");
    Serial.print(read_mV, 2);
    Serial.print(" | Amps: ");
    Serial.println(read_amps, 2);
  }

  eepromSaveTime = millis();  // Reset timer
}


/*
    EEPROM.put(0, mV);
    Serial.println("Saved to EEPROM");
    eepromSaveTime = millis();
    float read_mV;
    EEPROM.get(EEPROM_ADDR_MV[0], read_mV);
    Serial.print("Read one mV from EEPROM: ");
    Serial.println(read_mV, 5);*/
  

  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    processCommand(input);
    if (input.startsWith("slaveid=")) {
      int newId = input.substring(8).toInt();
      if (newId >= 1 && newId <= 247) {
        EEPROM.put(SLAVE_ID_ADDR, newId);
        Serial.print("New Slave ID stored: ");
         MODBUS_UNIT_ID = newId;
        Serial.println(newId);
        modbus.begin(MODBUS_UNIT_ID, MODBUS_BAUD, SERIAL_8N1);
        Serial.println("Applied to new Slave ID."); 
      } else {
        Serial.println("Invalid Slave ID. Must be 1–247.");
      }
    }

    else if (input.startsWith("baud=")) {
      int newBaud = input.substring(5).toInt();
      if (newBaud == 4800 || newBaud == 9600 || newBaud == 19200) {
        EEPROM.put(BAUD_ADDR, newBaud);
        Serial.print("New Baud Rate stored: "); 
        MODBUS_BAUD = newBaud;
        Serial.println(newBaud);
        MODBUS_SERIAL.begin(MODBUS_BAUD);
        modbus.begin(MODBUS_UNIT_ID, MODBUS_BAUD, SERIAL_8N1);
        Serial.println("Applied to new Baud Rate.");
        }
      else {
        Serial.println("Invalid Baud Rate. Use 4800, 9600, or 19200.");
      }
    }
  //STATUS: Checking
  else if (input.equalsIgnoreCase("status")) {
  Serial.print("Current Slave ID: ");
  Serial.println(MODBUS_UNIT_ID);
  Serial.print("Current Baud Rate: ");
  Serial.println(MODBUS_BAUD);
}
  }

  

 // if (Serialbus.available()) {
 //   String input = Serialbus.readStringUntil('\n');
 //   processCommand(input);
 // }

  //3 sec interval
  if (now - lastUpdate >= interval) {  
  lastUpdate = now;

    for (int ch = 0; ch < 4; ch++) {
        voltage = ads.computeVolts(ads.readADC_SingleEnded(ch));
        mV = voltage * 1000.0;
        if (mV <= 0){
          mV = 0;
        } 
        amps = (mV / set_mV[ch]) * set_amps[ch];
        if (amps <= 0){
          amps = 0;
        } 
        holdingRegisters[ch] = (int)(mV*100);                               
        holdingRegisters[ch + 4] = (int)(amps * 100);
 
 // discreteInputs[0] = !digitalRead(buttonPins[0]);
 // discreteInputs[1] = !digitalRead(buttonPins[1]);
  }}
  modbus.poll();



 // analogWrite(ledPins[0], holdingRegisters[0]);
 // analogWrite(ledPins[1], holdingRegisters[1]);
 // digitalWrite(ledPins[2], coils[0]);
 // digitalWrite(ledPins[3], coils[1]);

 // delay(500);
}
