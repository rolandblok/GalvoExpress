// The microcontroller controls 2 DAC which can control (via opamping) the galvo mirrors.
// It uses event trigger to do timing of the contours it plots.
//
//
// Manual LM358 OPAMP, including inverter
// https://www.tinytronics.nl/product_files/000233_Data_Sheet_of_HGSEMI_LM358.pdf
// https://verstraten-elektronica.blogspot.com/p/op-amp-als-inverter.html
//
// Opamp Amplifier
// https://verstraten-elektronica.blogspot.com/p/op-amp-als-niet-inverterende-versterker.html

//  Laser galvo Instructabel
// https://www.instructables.com/DIY-STEPDIR-LASER-GALVO-CONTROLLER/

// MCP4922 DAC library
// https://ww1.microchip.com/downloads/en/DeviceDoc/22250A.pdf
// https://www.best-microcontroller-projects.com/mcp4922.html
//     VoutA VRefA Vss VREFB VOUTB  SHDNn  LDACn
//   ----------------------------------------
//  |   14   13   12   11    10      9    8  |
//  >    MCP4922                             |
//  |    1    2    3    4     5      6    7  |
//   ----------------------------------------
//       Vdd  NC  CSn  SCK  SDI     NC   NC

// WEMOS D1 mini pinout
//  3V3 = VDD (1)
//  G   = VSS (12) 
//  MISO = D6 = GPIO12 = SDI = SDO NOT NEEDED, CANNOT READ FROM MCP4725
//  MOSI = D7 = GPIO13 = SDO = SDI (5)
//  SCK  = D5 = GPIO14 = SCK (4)
//  CS   = D8 = GPIO15 = CSn (3) AND LDACn (8) tied together
//  VCC  = SHDNn

#include <SPI.h>

// The software was made for Wemos D1 mini, but could easily be adapted for any u-controller
// Import required libraries
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "This ain't a ESP8266 or ESP32, !"
#endif

#include "MCP4725.h"
#include "Wire.h"

#include <ESP8266TimerInterrupt.h>

#include "patern.hpp"


void (*resetFunc)(void) = 0;

const int PIN_LASER_ENABLE = D3; // GPIO0
const int PIN_CS = D8;  // GPIO15

uint32_t lastTime = 0;
#define MCP_MAX_BITS 4095


// Init ESP8266 only and only Timer 1
#define TICK_TIME_US (30)  // microseconds
ESP8266Timer ITimer;
uint32_t tick_counter = 1;
uint32_t ticks_per_step = 1;  // adjust this to change speed


uint32 IPS_counter = 0;
uint32 TPS_counter = 0;
uint32 LPS_counter = 1;


void writeDAC(uint16_t valueA, uint16_t valueB) {
  valueA &= 0x0FFF;  // Ensure 12-bit values
  valueB &= 0x0FFF;

  // MCP4922 command format: [A/B][BUF][GA][SHDN][D11:D0]
  uint16_t cmdA = 0x3000 | valueA;  // 0011 xxxx xxxx xxxx
  uint16_t cmdB = 0xB000 | valueB;  // 1011 xxxx xxxx xxxx
    
  // Fast GPIO operations instead of digitalWrite
  GPOC = (1 << 15);  // PIN_CS LOW (GPIO15)

  SPI.transfer16(cmdB);
    GPOS = (1 << 15);  // PIN_CS HIGH (GPIO15)
      GPOC = (1 << 15);  // PIN_CS LOW (GPIO15)
  SPI.transfer16(cmdA);
  
  GPOS = (1 << 15);  // PIN_CS HIGH (GPIO15)
}


volatile bool update_dac = false;
void IRAM_ATTR TimerHandler() {

  IPS_counter++;
  tick_counter++;  // increment the tick counter

  if (tick_counter >= ticks_per_step) {  // if we reached the ticks
    TPS_counter++;
    tick_counter = 1;  // reset the tick counter

    update_dac = true;  // set flag to update DAC in main loop


  }
}


void setup() {
  Serial.begin(115200);
  // while (!Serial)
  //   ;

  // Serial.print("MCP4725_wave_generator - ");
  Serial.println(__FILE__);
  Serial.println("MCP4922 driven galvo driver - setup started...\n");

  // setup paterns
  patern_setup();

  // Laser Enable pin
  pinMode(PIN_LASER_ENABLE, OUTPUT);
  digitalWrite(PIN_LASER_ENABLE, HIGH);  // Enable laser driver (active HIGH)

  // SPI setup
  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);  // CSn inactive
  SPI.begin();
  SPI.setFrequency(15000000);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  // Disable WiFi to save power
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();

  // interval (in hz)
  if (ITimer.attachInterruptInterval(TICK_TIME_US, TimerHandler)) {
    Serial.println("# interrupt timer started");
  } else {
    Serial.println("# interrupt timer start failed");
  }

  // create default patern
  patern_create_circle();

  Serial.println("# Setup done.");
}


void loop() {
  static bool uploading = false;
  static bool uploading_first_line = true;
  static int  uploading_previous_x = 0;
  static int  uploading_previous_y = 0;
  static int  uploading_first_x = 0;
  static int  uploading_first_y = 0;
  static bool uploading_first_laser_on = false;
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    if (uploading) {
      if (input.equals("upload_end")) {
        uploading = false;
        Serial.println("# upload_end");
        // add line back to first point
        patern_add_line(uploading_previous_x, uploading_previous_y, uploading_first_x, uploading_first_y, uploading_first_laser_on);
        // Serial.println("# Closing line to first point: " + String(uploading_first_x) + "," + String(uploading_first_y) + "," + String(uploading_first_laser_on));
        patern_upload_stop();
      } else {
        // format "x,y,laser_on", example: "2048,1024,TRUE"
        int commaIndex = input.indexOf(',');
        int commaIndex2 = input.indexOf(',', commaIndex + 1);
        if (commaIndex != -1 && commaIndex2 != -1) {
          int x = input.substring(0, commaIndex).toInt();
          int y = input.substring(commaIndex + 1, commaIndex2).toInt();
          String laserOnStr = input.substring(commaIndex2 + 1);
          bool laser_on = (laserOnStr.equalsIgnoreCase("TRUE"));

          if (uploading_first_line) {
            uploading_previous_x = x;
            uploading_previous_y = y;
            uploading_first_x = x;
            uploading_first_y = y;
            uploading_first_laser_on = laser_on;
            uploading_first_line = false;
            // Serial.println("# First point uploaded.: " + String(x) + "," + String(y) + "," + String(laser_on));
          } else {
            // add line from previous point to current point
            patern_add_line(uploading_previous_x, uploading_previous_y, x, y, laser_on);
            uploading_previous_x = x;
            uploading_previous_y = y;
            // Serial.println("# Point uploaded.: " + String(x) + "," + String(y) + "," + String(laser_on));
          }
        } else {
          
          Serial.println("# Invalid upload format. Use: x,y,laser_on, not: " + input);   
        }
      }
    
    // no upload in progress, checking commands
    }  else if (input.equals("upload_start")) {
        Serial.println("# upload_start");
        uploading = true;
        patern_upload_start();
        uploading_first_line = true;

    } else if (input.equals("log")) {
        patern_serial_log_current_patern();
        

    } else if (input.equals("+")){
        ticks_per_step++;
        Serial.println("# ticks_per_step increased by 1");
        Serial.println("# " + String(ticks_per_step) + " ticks_per_step");
   } else if (input.equals("-")){ 
        ticks_per_step--;
        Serial.println("# ticks_per_step decreased by 1");
        Serial.println("# " + String(ticks_per_step) + " ticks_per_step");
   } else if (input.equals("*")){
        ticks_per_step *= 10;
        Serial.println("# ticks_per_step multiplied by 10");
        Serial.println("# " + String(ticks_per_step) + " ticks_per_step");
   } else if (input.equals("/")){ 
        ticks_per_step /= 10;
        Serial.println("# ticks_per_step divided by 10");
        Serial.println("# " + String(ticks_per_step) + " ticks_per_step");
   } else if (input.equals("s")) {
        patern_create_square(0, MCP_MAX_BITS, 0, MCP_MAX_BITS);
        Serial.println("# Square patern created");
    } else if (input.equals("c")) {
        patern_create_circle();
        Serial.println("# Circle patern created");
    } else if (input.equals("q")) {
        patern_create_square(0, MCP_MAX_BITS, 0, MCP_MAX_BITS);
        Serial.println("# Square patern created");
    } else if (input.equals("z")) {  //zero
        patern_create_point(1, 1, false);
    } else if (input.equals("m")) {  // mid
        patern_create_point(MCP_MAX_BITS / 2, MCP_MAX_BITS / 2, false);
    } else if (input.equals("h")) {  // highest point
        patern_create_point(MCP_MAX_BITS, MCP_MAX_BITS, false);
    } else if (input.equals("x")) {
        patern_double_square(true, true);
        Serial.println("# Double square patern created (one on, two off)");
    } else if (input.equals("REBOOT")) {
        Serial.println("# REBOOT");
        Serial.end();  //clears the serial monitor  if used
        resetFunc();
        delay(1000);
    } else {
      Serial.println("# Unknown command: " + input);
    }
  }

  // Update DAC if flag is set by timer interrupt
  if (update_dac) {
    update_dac = false;  // reset flag

    int next_x = 0;
    int next_y = 0;
    bool next_laser_on = false;
    patern_get_next_step(next_x, next_y, next_laser_on);

    // send to DAC
    writeDAC(next_x, next_y);

    // fast switch laser on/off
    if (next_laser_on)
      GPOS = (1 << PIN_LASER_ENABLE);
    else
      GPOC = (1 << PIN_LASER_ENABLE);
  }

  static uint32_t lastTime = millis();
  if (!uploading && (millis() - lastTime >= 1000)) {
    // LPS> 000
    int usPL = (int)(1e6 / LPS_counter);
    int usPT = (int)(1e6 / TPS_counter);
    int usPI = (int)(1e6 / IPS_counter);
    double paterns_per_second = TPS_counter / patern_get_length();

    lastTime = millis();
    Serial.print("# usPL: " + String(usPL) + ", ");
    Serial.print(" usPT: " + String(usPT) + ", ");
    Serial.print(" usPI: " + String(usPI) + ", ");
    Serial.print(" ticks_per_step: " + String(ticks_per_step) + ",");
    Serial.print(" paterns_per_second: " + String(paterns_per_second, 2) + "\n");
    TPS_counter = 0;
    IPS_counter = 0;
    LPS_counter = 1;

  } else {
    LPS_counter++;
  }
}


//  -- END OF FILE --
