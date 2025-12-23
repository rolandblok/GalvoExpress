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

//  q = square       z = zero
//  s = sinus/circle m = mid
//  w = sawtooth     h = high
//  t = stair
//  r = random
char waveFrom = 'p';

void (*resetFunc)(void) = 0;

#define PIN_LASER_ENABLE D3

MCP4725 MCP_1(0x60);
MCP4725 MCP_2(0x61);
uint16_t count;
uint32_t lastTime = 0;
#define MCP_MAX_BITS 4095


//  LOOKUP TABLE SINE
#define SINE_STEPS 144
uint16_t sine[SINE_STEPS];
uint32_t sine_idx = 0;

// Init ESP8266 only and only Timer 1
#define TICK_TIME_US (10)  // 10 microseconds
ESP8266Timer ITimer;
uint32_t tick_counter = 0;
uint32_t ticks_per_step = 10;  // adjust this to change speed


uint32 IPS_counter = 0;
uint32 TPS_counter = 0;
uint32 FPS_counter = 0;


int sqr_counter = 0;
void IRAM_ATTR TimerHandler() {

  IPS_counter++;
  tick_counter++;  // increment the tick counter

  if (tick_counter >= ticks_per_step) {  // if we reached the ticks
    TPS_counter++;
    tick_counter = 0;  // reset the tick counter

    sine_idx++;
    if (sine_idx >= SINE_STEPS) sine_idx = 0;
    uint32_t sine_idx_1 = sine_idx;
    uint32_t sine_idx_2 = (sine_idx + SINE_STEPS / 4) % SINE_STEPS;

    switch (waveFrom) {
      case 'p':  // patern
        int x, y;
        bool laser_on;
        patern_get_next_step(x, y, laser_on);

        // digitalWrite(PIN_LASER_ENABLE, laser_on ? HIGH : LOW);
        // fast switch laser on/off
        if (laser_on)
          GPOS = (1 << PIN_LASER_ENABLE);
        else
          GPOC = (1 << PIN_LASER_ENABLE);

        MCP_1.setValue(x);
        MCP_2.setValue(y);
        break;

      case 'r':  // random
        MCP_1.setValue(random(MCP_MAX_BITS + 1));
        MCP_2.setValue(random(MCP_MAX_BITS + 1));
        break;
    }
  }
}


void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  patern_setup();

  pinMode(PIN_LASER_ENABLE, OUTPUT);
  digitalWrite(PIN_LASER_ENABLE, HIGH);  // Enable laser driver (active HIGH)

  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();

  Serial.print("MCP4725_wave_generator - ");
  Serial.println(__FILE__);
  Serial.print("MCP4725_VERSION: ");
  Serial.println(MCP4725_VERSION);
  Serial.println();

  Wire.begin();
  //  Wire.setClock(3400000);

  //  fill table
  for (int i = 0; i < SINE_STEPS; i++) {
    sine[i] = MCP_MAX_BITS / 2 + round((MCP_MAX_BITS / 2) * sin((2 * PI * i) / SINE_STEPS));
  }

  //  init MCP4725
  MCP_1.begin();
  MCP_2.begin();
  Wire.setClock(400000);

  //  scan I2C bus for devices
  byte error, address;
  int nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");

      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  MCP_1.setValue(0);
  if (!MCP_1.isConnected()) {
    Serial.println(" 4725 not connected!");
  }
  MCP_2.setValue(0);
  if (!MCP_2.isConnected()) {
    Serial.println("MCP_2 4725 not connected!");
  }

  // interval (in hz)
  if (ITimer.attachInterruptInterval(TICK_TIME_US, TimerHandler)) {
    Serial.println("# interrupt timer started");
  } else {
    Serial.println("# interrupt timer start failed");
  }

  // // --- Configure Hardware Timer1 ---
  // // timer1 is 1-shot or repeating depending on the call
  // // TIM_DIV16 → 5 MHz timer → 0.2 µs ticks
  // timer1_attachInterrupt(TimerHandler);
  // timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
  // timer1_write(TICK_TIME_US);   // <-- shortest stable interval

  patern_create_circle();
  waveFrom = 'p';


  Serial.println("# MCP_1 and MCP_2  connected");
  Serial.println("# commands: + - * / 0-9 c A a q s w t r z m h");
}


void loop() {
  FPS_counter++;
  if (Serial.available()) {
    int c = Serial.read();
    switch (c) {
      case '+':
        ticks_per_step++;
        break;
      case '-':
        ticks_per_step--;
        break;
      case '*':
        ticks_per_step *= 10;
        break;
      case '/':
        ticks_per_step /= 10;
        break;
      case '0' ... '9':
        ticks_per_step *= 10;
        ticks_per_step += (c - '0');
        break;
      case 'A':
        break;
      case 'a':
        break;
      case 's':
        patern_create_square(0, MCP_MAX_BITS, 0, MCP_MAX_BITS);
        Serial.println("# Square patern created");
        waveFrom = 'p';
        break;
      case 'c':
        patern_create_circle();
        Serial.println("# Circle patern created");
        waveFrom = 'p';
        break;
      case 'q':
        patern_create_square(0, MCP_MAX_BITS, 0, MCP_MAX_BITS);
        Serial.println("# Square patern created");
        waveFrom = 'p';
        break;
      case 'r':
        waveFrom = 'r';
        Serial.println("# Random wave selected");
        break;
      case 'z':  //zero
        patern_create_point(0, 0, false);
        waveFrom = c;
        break;
      case 'm':  // mid
        patern_create_point(MCP_MAX_BITS / 2, MCP_MAX_BITS / 2, false);
        waveFrom = c;
        break;
      case 'h':  // highest point
        patern_create_point(MCP_MAX_BITS, MCP_MAX_BITS, false);
        waveFrom = c;
        break;
      case 'x':
        patern_double_square(true, true);
        Serial.println("# Double square patern created (one on, two off)");
        waveFrom = 'p';
        break;
      case 'R':
        Serial.println("# REBOOT");
        Serial.end();  //clears the serial monitor  if used
        resetFunc();
        delay(1000);
        break;
      default:
        break;
    }
    Serial.println("# " + String(ticks_per_step) + " ticks_per_step, wave: " + String((char)waveFrom));
  }

  static uint32_t lastTime = millis();
  if (millis() - lastTime >= 1000) {
    int usPF = (int)(1e6 / FPS_counter);
    int usPT = (int)(1e6 / TPS_counter);
    int usPI = (int)(1e6 / IPS_counter);
    double paterns_per_second = TPS_counter / patern_get_length();

    lastTime = millis();
    Serial.print("# usPF: " + String(usPF) + ", ");
    Serial.print(" usPT: " + String(usPT) + ", ");
    Serial.print(" usPI: " + String(usPI) + ", ");
    Serial.print(" ticks_per_step: " + String(ticks_per_step) + ",");
    Serial.print(" paterns_per_second: " + String(paterns_per_second, 2) + "\n");
    TPS_counter = 0;
    IPS_counter = 0;
    FPS_counter = 0;
  }
}


//  -- END OF FILE --
