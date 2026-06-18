/*
  ModbusRTUMasterExample
 
  This example demonstrates how to setup and use the ModbusRTUMaster library (https://github.com/CMB27/ModbusRTUMaster).
  See README.md (https://github.com/CMB27/ModbusRTUMaster/blob/main/examples/ModbusRTUMasterExample/README.md) for hardware setup details.
  
  Created: 2023-07-22
  By: C. M. Bulliner
  Last Modified: 2024-10-27
  By: C. M. Bulliner
  
*/


#include <ModbusRTUMaster.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>


SoftwareSerial modbussw(A1, A0);

int MODBUS_BAUD = 4800;


#define SLAVE_ID_ADDR1 20  //Slave Address stored in EEPROM
#define SLAVE_ID_ADDR2 30  //Slave Address stored in EEPROM
#define SLAVE_ID_ADDR3 40  //Slave Address stored in EEPROM


#define MODBUS_SERIAL modbussw

//#define SLAVE_ID_ADDR4 SLAVE_ID_ADDR2 + sizeof(MODBUS_UNIT_ID) //Slave Address stored in EEPROM

#define BAUD_ADDR 10  //Baud Address stored in EEPROM

#define MODBUS_CONFIG SERIAL_8N1
int MODBUS_UNIT_ID = 4;
int MODBUS_UNIT_ID1 = 4;
int MODBUS_UNIT_ID2 = 3;
int MODBUS_UNIT_ID3 = 2;


ModbusRTUMaster modbus(MODBUS_SERIAL);




//const int rxLed = A3;
//const int txLed = A2;
//SoftwareSerial mySerial(rxLed, txLed);

//#define MODBUS_SERIAL modbussw



const int blinkDuration = 100;
unsigned long rxBlinkStart = 0;
unsigned long txBlinkStart = 0;
bool rxBlinking = false;
bool txBlinking = false;

const int LED_OFF = 255;
const int LED_ON = 0;

/*
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
  // The ATmega328P and ATmega168 only have one HardwareSerial port, and on Arduino boards it is usually connected to a USB/UART bridge.
  // So, for these boards, we will use SoftwareSerial with the lbrary, leaving the HardwareSerial port available to send debugging messages.
  #define SOFTWARE_SERIAL
  #include <SoftwareSerial.h>
  const int rxLed = A3;
  const int txLed = A2;
  SoftwareSerial mySerial(rxLed, txLed);
  #define MODBUS_SERIAL mySerial*/
/*#elif defined(ARDUINO_NANO_ESP32)
  // On the Arduino Nano ESP32, the HardwareSerial port on pins 0 and 1 is Serial0
  #define MODBUS_SERIAL Serial0
#else
  // On the majority of Arduino boards, the HardwareSerial port on pins 0 and 1 is Serial1
  // On the Arduino Mega and Adruino Due, Serial1 is on pins 18 and 19.
  #define MODBUS_SERIAL Serial1*/
//#endif
// You can change the baud, config, and unit id values if you like.
// Just make sure they match the settings you use in ModbusRTUSlaveExample.
// Note, the config value will be ignored when using SoftwareSerial.
// SoftwareSerial only supports SERIAL_8N1.



/*
#if (defined(ARDUINO_NANO_RP2040_CONNECT) && !defined(ARDUINO_ARCH_MBED)) || defined(ARDUINO_NANO_ESP32)
  // These boards operate unsing GPIO numbers that don't correspond to the numbers on the boards.
  // However they do have D# values #defined to correct this.
  const int8_t buttonPins[2] = {D2, D3};
  const int8_t ledPins[4] = {D5, D6, D7, D8};
  const int8_t dePin = D13;
#else*/
// Other boards do not have D# values, and will throw an error if you try to use them.
//const int8_t buttonPins[2] = {2, 3};
//const int8_t ledPins[4] = {5,6,7,8};
//const int8_t dePin = 13;
//#endif
//const int8_t knobPins[2] = {A0, A1};



const uint8_t numCoils = 4;
const uint8_t numDiscreteInputs = 4;
const uint8_t numHoldingRegisters = 8;
const uint8_t numInputRegisters = 8;

bool coils[numCoils];
bool discreteInputs[numDiscreteInputs];
uint16_t holdingRegisters[numHoldingRegisters];
uint16_t inputRegisters[numInputRegisters];

//unsigned long transactionCounter = 0;
//unsigned long errorCounter = 0;

const char* errorStrings[] = {
  "success",
  "invalid id",
  "invalid buffer",
  "invalid quantity",
  "response timeout",
  "frame error",
  "crc error",
  "unknown comm error",
  "unexpected id",
  "exception response",
  "unexpected function code",
  "unexpected response length",
  "unexpected byte count",
  "unexpected address",
  "unexpected value",
  "unexpected quantity"
};



void setup() {
  //pinMode(rxLed, OUTPUT);
  //pinMode(txLed, OUTPUT);
  //analogWrite(rxLed, LED_OFF);
  //analogWrite(txLed, LED_OFF);

  /*#if defined(ARDUINO_NANO_ESP32)
    analogReadResolution(10);
  #endif*/

  Serial.begin(115200);

  MODBUS_SERIAL.begin(MODBUS_BAUD);
  modbus.begin(MODBUS_BAUD, MODBUS_CONFIG);
  /*
  #ifndef SOFTWARE_SERIAL
    MODBUS_SERIAL.begin(MODBUS_BUAD, MODBUS_CONFIG);
    modbus.begin(MODBUS_BUAD, MODBUS_CONFIG);
  #else
    MODBUS_SERIAL.begin(MODBUS_BUAD);
    modbus.begin(MODBUS_BUAD);
  #endif*/
}

void loop() {
  //uint8_t error;


  readcurrentmeter(MODBUS_UNIT_ID1, 0, 4, numHoldingRegisters);

  delay(1000);

  readflowratemeter(MODBUS_UNIT_ID2, 0, 4, numHoldingRegisters);

  delay(1000);


  readtempmeter_PID500U(MODBUS_UNIT_ID3, 32, 4, numHoldingRegisters);


  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    //processCommand(input);
    if (input.startsWith("slaveid1=")) {
      int newId = input.substring(8).toInt();
      if (newId >= 1 && newId <= 247) {
        EEPROM.put(SLAVE_ID_ADDR1, newId);
        Serial.print("New Slave ID stored: ");
        MODBUS_UNIT_ID1 = newId;
        Serial.println(newId);
        modbus.begin(MODBUS_UNIT_ID1, MODBUS_BAUD, SERIAL_8N1);
        Serial.println("Applied to new Slave ID.");
      } else {
        Serial.println("Invalid Slave ID1. Must be 1–247.");
      }
    }

    else if (input.startsWith("slaveid2=")) {
      int newId = input.substring(8).toInt();
      if (newId >= 1 && newId <= 247) {
        EEPROM.put(SLAVE_ID_ADDR2, newId);
        Serial.print("New Slave ID stored: ");
        MODBUS_UNIT_ID2 = newId;
        Serial.println(newId);
        modbus.begin(MODBUS_UNIT_ID2, MODBUS_BAUD, SERIAL_8N1);
        Serial.println("Applied to new Slave ID.");
      } else {
        Serial.println("Invalid Slave ID2. Must be 1–247.");
      }
    }


    else if (input.startsWith("slaveid3=")) {
      int newId = input.substring(8).toInt();
      if (newId >= 1 && newId <= 247) {
        EEPROM.put(SLAVE_ID_ADDR3, newId);
        Serial.print("New Slave ID stored: ");
        MODBUS_UNIT_ID3 = newId;
        Serial.println(newId);
        modbus.begin(MODBUS_UNIT_ID3, MODBUS_BAUD, SERIAL_8N1);
        Serial.println("Applied to new Slave ID.");
      } else {
        Serial.println("Invalid Slave ID3. Must be 1–247.");
      }
    }


    //modbus.poll();
  }
  }

  void readcurrentmeter(int unitid, int startaddr, int regtype, int totalreg)  //reg type 1-coil, 2-input , 3-input reg, 4-holding reg
  {
    MODBUS_UNIT_ID = unitid;

    uint8_t error = modbus.readHoldingRegisters(MODBUS_UNIT_ID, 0, holdingRegisters, totalreg);

    if (error == MODBUS_RTU_MASTER_SUCCESS) {
      Serial.println("=== Current Meter Data ===");
      for (int i = 0; i < totalreg / 2; i++) {
        float voltage = holdingRegisters[i] / 100.0;
        float current = holdingRegisters[i + 4] / 100.0;
        Serial.print("CH");
        Serial.print(i + 1);
        Serial.print(" - Voltage: ");
        Serial.print(voltage);
        Serial.print(" mV");
        Serial.print(" | Current: ");
        Serial.print(current);
        Serial.println(" A");
      }
    } else {
      Serial.print("Modbus error: ");
      Serial.println(errorStrings[error]);
    }
  }

  void readflowratemeter(int unitid, int startaddr, int regtype, int totalreg)  //reg type 1-coil, 2-input , 3-input reg, 4-holding reg
  {

    MODBUS_UNIT_ID = unitid;
    uint8_t error = modbus.readHoldingRegisters(MODBUS_UNIT_ID, 0, holdingRegisters, numHoldingRegisters);
    if (error == MODBUS_RTU_MASTER_SUCCESS) {
      Serial.println("=== Flow Rate Data ===");
      for (int j = 0; j < 1; j++) {
        float flowrate = holdingRegisters[j];
        float volume = holdingRegisters[j + 1];
        Serial.print(" flowrate: ");
        Serial.print(flowrate);
        Serial.print(" L/min");
        Serial.print(" | volume: ");
        Serial.print(volume);
        Serial.println(" mL");
      }
    } else {
      Serial.print("Modbus error: ");
      Serial.println(errorStrings[error]);
    }
  }

  void readtempmeter_PID500U(int unitid, int startaddr, int regtype, int totalreg)  //reg type 1-coil, 2-input , 3-input reg, 4-holding reg
  {

    MODBUS_UNIT_ID = unitid;
    uint8_t error = modbus.readInputRegisters(MODBUS_UNIT_ID, 11, inputRegisters, numInputRegisters);
    if (error == MODBUS_RTU_MASTER_SUCCESS) {
      int pv = inputRegisters[0] / 10;
      Serial.println("=== Temperature Data ===");

      Serial.print("PV: ");
      Serial.println(pv);

    } else {
      Serial.print("Modbus error: ");
      Serial.println(errorStrings[error]);
    }

    error = modbus.readHoldingRegisters(MODBUS_UNIT_ID, 33, holdingRegisters, numHoldingRegisters);
    if (error == MODBUS_RTU_MASTER_SUCCESS) {
      int sv = holdingRegisters[0] / 10;
      //Serial.println("=== Temperature Data ===");

      Serial.print("SV: ");
      Serial.println(sv);

    } else {
      Serial.print("Modbus error: ");
      Serial.println(errorStrings[error]);
    }

  }




//printLog(MODBUS_UNIT_ID, 16, 0, numHoldingRegisters, error);
/*
  error = modbus.writeMultipleCoils(MODBUS_UNIT_ID, 0, coils, numCoils);
  printLog(MODBUS_UNIT_ID, 15, 0, numHoldingRegisters, error);

  error = modbus.readInputRegisters(MODBUS_UNIT_ID, 0, inputRegisters, numInputRegisters);
  printLog(MODBUS_UNIT_ID, 4, 0, numHoldingRegisters, error);

  error = modbus.readDiscreteInputs(MODBUS_UNIT_ID, 0, discreteInputs, numDiscreteInputs);
  printLog(MODBUS_UNIT_ID, 2, 0, numHoldingRegisters, error);
*/
