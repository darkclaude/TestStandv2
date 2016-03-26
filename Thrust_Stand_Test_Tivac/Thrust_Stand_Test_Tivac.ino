/*

Pin connections for this software:

PWM Output:                  Pin 14 or PB_6
Current Sensor:              Pin 27 or PE_1 
Voltage Sensor:              Pin 29 or PE_3
Load Cell Amp HX711.DOUT     Pin 9 or PA_6
              HX711.PD_SCK   Pin 8 or PA_5 
Electical/Magnetic RPMs      Pin 31 or PF_4 or PUSH1
Optical RPMs                 Pin 17 or PF_0 or PUSH2

Hardware PWM output adapted from: http://codeandlife.com/2012/10/30/stellaris-launchpad-pwm-tutorial/
Stellaris timer code adapted from:  http://patolin.com/blog/2014/06/29/stellaris-launchpad-energia-pt-2-timers/
*/
#include "config.h"
#include "Energia.h"  
#include "inc/hw_memmap.h" 
#include "inc/hw_types.h" 
#include "inc/hw_ints.h" 
#include "inc/hw_timer.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/Debug.h" 
#include "driverlib/interrupt.h" 
#include "driverlib/sysctl.h" 
#include "driverlib/adc.h"
#include "driverlib/rom.h"
#include "driverlib/timer.h"
#include "driverlib/pwm.h" 
#include "HX711.h"             // Requires HX711 Library from: https://github.com/bogde/HX711
#include "Average.h"           // Requires Average Library from: https://github.com/MajenkoLibraries/Average
#include <EEPROM.h>
#include "variables.h"

void setup() {
  //FPUEnable();                  //Enable Hardware Floating Point Unit
  //FPUStackingEnable();          //Enable floating point operation ins ISRs
  SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);  // Set system clock to 80mhz
  
  // initialize serial communication:
  Serial.begin(UARTBAUD);
    
  // attach Interupt for RPM sensor
  if(MAGSENS) {
    pinMode(33, OUTPUT);
    digitalWrite(33,LOW);
    pinMode(32, OUTPUT);
    digitalWrite(32,HIGH);
    pinMode(31, INPUT_PULLUP);
    attachInterrupt(31, countRpms, FALLING);
  }
  if(OPTISENS) {
    pinMode(PUSH2, INPUT_PULLUP);
    attachInterrupt(PUSH2, countRpms2, FALLING);
  }
  
  scale.set_scale(LSCALE);  // Eventually set this via EEPROM
  scale.tare();	            // Reset the scale to 0
  
  adcTimer(SENSORRATE);     // Start timer for load cell and analog reads
  initPWMOut();             // Start PWM output
  initRPMCount();           // Start RPM counter timers
 
}

void loop() {
  isTestRunning = false;  // Stop reads from load cell and reset step counters
  stepCount1 = 0;
  stepCount2 = 0;
  stepDiff1 = 0;
  stepDiff2 = 0;
  updatePWM(MINCOMMAND);  // Double check throttle is at 0
  
  if(!isTared) {
    scale.tare();
    isTared = true;
  }
  
  // Prompt for input and read it
  Serial.println("Type t(Tare), v(Battery Voltage), c(Calibrate), b(Brake Test), k(KV Test), s(Start), or i(Idle)");
  input="";
  while(!Serial.available());
  while(Serial.available()) {
      character = Serial.read();
      input.concat(character);
      delay(2);
  }
  
  Serial.print("Input: ");
  Serial.println(input);
  
  //Check input and run appropriate routine
  
  if(input.indexOf("t") >= 0) {
    tareScale();
  }
  if(input.indexOf("c") >= 0) {
    calibrate();
  }
  if(input.indexOf("i") >= 0) {
    idle();
  }
  if(input.indexOf("b") >= 0) {
    brakeTest();
  } 
  if(input.indexOf("s") >= 0) {
    mainTest();
  } 
  if(input.indexOf("v") >= 0) {
    returnVoltage();
  }
  if(input.indexOf("k") >= 0) {
    kvTest();
  }
  delay(100);
}




