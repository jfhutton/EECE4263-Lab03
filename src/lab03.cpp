#include <Arduino.h>
#include <esp.h>
//#include <Wifi.h>
#include <string.h>
#include "Adafruit_Si7021.h"
#include <LiquidCrystal_I2C.h>

// Globals

// Note:  lcd.init() calls the Wire.h builtin that uses the default
// I2C bus for the ESP32.  Defines below show should show these pin numbers!
// For other boards, there may be a way to pre-initilize the 
// Wire object with this data, but seems to be working with defaults.
#define SCL 22  // Use default SCL for Huzzah32
#define SDA 23  // Use default SDA for Huzzah32
#define SensorI2CAddress 0x40  // This is the default address for the 7021 Sensor.
Adafruit_Si7021 sensor = Adafruit_Si7021();
#define LCDI2CAddress 0x3F  // This is the address for the LCD display (can vary).
LiquidCrystal_I2C lcd(LCDI2CAddress, 16, 2);  // Setup for 16 columns and 2 rows.

// Sensor variables
float humidity = 0;
float temp = 0;
// LCD variables
char tempStr[16];
char humidityStr[16];
// Timing variables
unsigned long msCurrent, msLast;  // for capturing MS for timing
#define msUpdateInt 5000
// Terminal variables
char readChar, cmdChar;
char myStr[80];
bool exitedTerminal = false;
// Heater (Debug) variables
bool enableHeater = false;
int heaterLoop = 0;

// Prototype Functions
void readSensors ();
void clearScreen ();
void displayMenu ();
void displayPrompt ();
void displayTemp ();
void displayHumidity ();
void displayTempAndHumidity ();
void si7021_print_details ();
void findI2CAddresses();

void setup() {


  Serial.begin(115200);
  // wait for serial port to open
  while (!Serial) {
    delay(10);
  }
  Serial.println("\nSerial ready!");

  // Helper code to scan I2C addresses.  Only run for Debug.
  //findI2CAddresses();

  // // Init sensor test
  // // Useful if trying to just run the sensor.  If running the 
  // // Sensor and LCD display, only use the lcd.init() instead of 
  // // the sensor.begin().
  // Serial.println("Si7021 test!");
  // if (!sensor.begin()) {
  //   Serial.println("Did not find Si7021 sensor!");
  //   while (true)
  //     ;
  // }
  // si7021_print_details(); // Details for sensor

  // Init display
  // initialize the LCD
	lcd.init();
	// Turn on the blacklight and print a message.
	lcd.backlight();
	lcd.print("Test2");
  delay(2000);

  // Prep Screen
  displayMenu();
  displayPrompt();

  // Timing
  msCurrent = millis();
  msLast = msCurrent;
}

void loop() {
  msCurrent = millis();

  // Read Sensor at the update interval
  if ( msCurrent > (msLast + msUpdateInt)) {
    msLast = msCurrent;
    heaterLoop++;
    readSensors();
  }

  // Capture and execute Command
  // Prompt printed during Setup for 1st time.
  if (!exitedTerminal) {
    if(Serial.available() > 0) {
      // A character has been typed
      readChar = Serial.read();
      //Serial.print(".."); Serial.print(readChar); Serial.print("..");
      // If the read character is a \r, the the last char was the command.
      if (readChar == '\r' ) {
        // Carriage return indicates an enter
        Serial.println();  // Echo an end of line.
        if        (cmdChar == 'T' || cmdChar == 't') {
          displayTemp();
        } else if (cmdChar == 'H' || cmdChar == 'h') {
          displayHumidity();
        } else if (cmdChar == 'B' || cmdChar == 'b') {
          displayTempAndHumidity();
        } else if (cmdChar == '?') {
          displayMenu();
        } else if (cmdChar == 'X' || cmdChar == 'x') {
          Serial.println("Goodbye!");
          exitedTerminal = true;
        } else {
          sprintf(myStr,"'%c' is not a valid command.",cmdChar);
          displayMenu();
        }
        if (!exitedTerminal) {
          displayPrompt();
        }
      } else if (readChar == '\n' || readChar == ' ') {
          // Do nothing
      } else {
        // Set the read charater to be the command character
        Serial.print(readChar);  // This is with echo off
        cmdChar = readChar;
      }
    }
  }

  // Toggle heater enabled state every 5 read loops
  // An ~1.8 degC temperature increase can be noted when heater is enabled
  if ( heaterLoop > 4) {
    heaterLoop = 0;
    enableHeater = !enableHeater;
    sensor.heater(enableHeater);
    // Serial.print("Heater Enabled State: ");
    // if (sensor.isHeaterEnabled())
    //   Serial.println("ENABLED");
    // else
    //   Serial.println("DISABLED");
  }
}

void readSensors () {
  // Read Sensor
  temp = sensor.readTemperature();
  temp = (temp * 9.0 / 5.0) + 32.0; // Convert Celsius to Fahrenheit
  sprintf(tempStr,     "Temp (F): %4.1f\xDF",temp);
  humidity = sensor.readHumidity();
  sprintf(humidityStr, "Humidity: %4.1f%c",humidity,'%');

  // Update to LCD screen
  lcd.setCursor(0,0);
  lcd.print(tempStr);
  lcd.setCursor(0,1);
  lcd.print(humidityStr);

  //// Debug - Write to serial terminal
  //Serial.print(humidityStr);
  //Serial.println(tempStr);
}

void displayMenu(void) {
  // displays the ETHOS monitor menu
  // REF:  Leveraged from Dr. Nordstrom's Code
  Serial.println("\nWelcome to ETHOS");
  Serial.println("The Electronic Temperature & Humidity Observation System");
  Serial.println();
  Serial.println("Your options:");
  Serial.println("  T  Request temperature");
  Serial.println("  H  Request relative humidity");
  Serial.println("  B  Request both temperature and relative humidity");
  Serial.println("  ?  Help (shows this menu)");
  Serial.println("  X  Exit");
}

void displayPrompt() {
  Serial.println();
  Serial.print("Choice ? ");
}

void displayTemp() {
  char myStr[80];
  sprintf(myStr, "Temperature:  %4.1f degrees F",temp);
  Serial.println(myStr);
}

void displayHumidity() {
  char myStr[80];
  sprintf(myStr, "Relative humidity:  %4.1f%c",humidity,'%');
  Serial.println(myStr);
}

void displayTempAndHumidity() {
  char myStr[80];
  sprintf(myStr, "Temperature:  %4.1f degrees F, Relative humidity:  %4.1f%c",temp,humidity,'%');
  Serial.println(myStr);
}

// Debugging functions
void si7021_print_details () {
  Serial.print("Found model ");
  switch(sensor.getModel()) {
    case SI_Engineering_Samples:
      Serial.print("SI engineering samples"); break;
    case SI_7013:
      Serial.print("Si7013"); break;
    case SI_7020:
      Serial.print("Si7020"); break;
    case SI_7021:
      Serial.print("Si7021"); break;
    case SI_UNKNOWN:
    default:
      Serial.print("Unknown");
  }
  Serial.print(" Rev(");
  Serial.print(sensor.getRevision());
  Serial.print(")");
  Serial.print(" Serial #"); Serial.print(sensor.sernum_a, HEX); Serial.println(sensor.sernum_b, HEX);
}

void findI2CAddresses() {
  byte error, address;
  int nDevices;
  
  Serial.println("\nI2C Scanner");

  Wire.begin();

  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }

  // End with eternal spin loop
  Serial.println("Ending with eternal spin loop...");
  while (1) {
    delay(10000);
  }
}