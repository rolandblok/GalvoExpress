//
//    FILE: MCP4725_wave_generator.ino
//  AUTHOR: Rob Tillaart
// PURPOSE: demo function generators
//     URL: https://github.com/RobTillaart/MCP4725
//     URL: https://github.com/RobTillaart/FunctionGenerator
//
//  depending on the platform, the range of "smooth" sinus is limited.
//  other signals are less difficult so have a slightly larger range.
//
//  PLATFORM     SINUS    SQUARE  SAWTOOTH  TRIANGLE
//  UNO          -100 Hz
//  ESP32        -200 Hz  -1000   -250      -100
//

// Manual LM358 OPAMP, including inverter
// https://www.tinytronics.nl/product_files/000233_Data_Sheet_of_HGSEMI_LM358.pdf
// https://verstraten-elektronica.blogspot.com/p/op-amp-als-inverter.html
// 
// Opamp Amplifier
// https://verstraten-elektronica.blogspot.com/p/op-amp-als-niet-inverterende-versterker.html

//  Laser galvo Instructabel
// https://www.instructables.com/DIY-STEPDIR-LASER-GALVO-CONTROLLER/

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

//  frequency
//  use + - * /  to control it
uint16_t   freq = 100;
uint32_t period = 0;
uint32_t halvePeriod = 0;


//  q = square       z = zero
//  s = sinus        m = mid
//  w = sawtooth     h = high
//  t = stair
//  r = random
char waveFrom = 's';

void(*resetFunc) (void) = 0; 

MCP4725 MCP_1(0x60);
MCP4725 MCP_2(0x61);
uint16_t count;
uint32_t lastTime = 0;


//  LOOKUP TABLE SINE
#define SINE_STEPS 361  
uint16_t sine[SINE_STEPS];
uint32_t sine_idx = 0;

// Init ESP8266 only and only Timer 1
#define TICK_TIME_US (10)  // 10 microseconds
ESP8266Timer ITimer;
uint32_t tick_counter = 0;
uint32_t ticks_per_step = 10; // adjust this to change speed

bool toggle = false;
void IRAM_ATTR TimerHandler()
{

    tick_counter++; // increment the tick counter

    if (tick_counter >= ticks_per_step) { // if we reached the ticks 
      tick_counter = 0; // reset the tick counter
      
      sine_idx++;
      if (sine_idx >= SINE_STEPS) sine_idx = 0;

      switch (waveFrom)
      {
        case 'q':
          if (toggle) MCP_1.setValue(4095);
          else MCP_1.setValue(0);
          toggle = !toggle;
          break;
        case 'w':
          MCP_1.setValue(sine_idx * 4095 / SINE_STEPS); 
          break;
        case 't':
          if (sine_idx < SINE_STEPS / 2) {
            // Ramp up: 0 to 4095
            MCP_1.setValue(sine_idx * 2 * 4095 / SINE_STEPS);
          } else {
            // Ramp down: 4095 to 0
            MCP_1.setValue(4095 - ((sine_idx - SINE_STEPS / 2) * 2 * 4095 / SINE_STEPS));
          }
          break;
        case 'r':
          MCP_1.setValue(random(4096));
          break;
        case 'z':  //  zero
          MCP_1.setValue(0);
          break;
        case 'h':  //  high
          MCP_1.setValue(4095);
          break;
        case 'm':  //  mid
          MCP_1.setValue(2047);
          break;
        default:
        case 's':
          MCP_1.setValue(sine[sine_idx]);   //  fetch from lookup table
          break;
      }
    }
}


void setup()
{
  Serial.begin(115200);
  while(!Serial);

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
  for (int i = 0; i < SINE_STEPS; i++)
  {
    sine[i] = 2047 + round(2047 * sin(i * PI / 180));
  }

  //  init MCP4725
  MCP_1.begin();
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
  if (!MCP_1.isConnected())
  {
    Serial.println(" 4725 not connected!");
  }
  MCP_2.setValue(0);
  if (!MCP_2.isConnected())
  {
    Serial.println("MCP_2 4725 not connected!");
  }

  // interval (in hz)
  if ( ITimer.attachInterruptInterval(TICK_TIME_US, TimerHandler)) {
    Serial.println("# interrupt timer started");
  } else{
    Serial.println("# interrupt timer start failed");
  }

  // // --- Configure Hardware Timer1 ---
  // // timer1 is 1-shot or repeating depending on the call
  // // TIM_DIV16 → 5 MHz timer → 0.2 µs ticks
  // timer1_attachInterrupt(TimerHandler);
  // timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
  // timer1_write(TICK_TIME_US);   // <-- shortest stable interval


  Serial.println("# MCP_1 and MCP_2  connected");
  Serial.println("# commands: + - * / 0-9 c A a q s w t r z m h");
  

}


void loop()
{

  if (Serial.available())
  {
    int c = Serial.read();
    switch (c)
    {
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
      case 'c':
        ticks_per_step = 0;
        break;
      case 'A':
        break;
      case 'a':
        break;
      case 'q':
      case 's':
      case 'w':
      case 't':
      case 'r':
      case 'z':
      case 'm':
      case 'h':
        waveFrom = c;
        break;
      case 'R':
        Serial.println("REBOOT");
        Serial.end();  //clears the serial monitor  if used
        resetFunc();
        delay(1000);
        break;
      default:
        break;
    }
    Serial.print(ticks_per_step);
    Serial.println();
  }
}


//  -- END OF FILE --

