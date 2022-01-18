//ch9-hwtimer 
//Modified - removed Blinky code
//To demonstrate hardware timer - switch both interrupt driven 
/****************************************************************************************************************************
  ISR_Timer_Switch.ino
  For ESP32 boards
  Written by Khoi Hoang
  Built by Khoi Hoang https://github.com/khoih-prog/ESP32TimerInterrupt
  Licensed under MIT license
  The ESP32 has two timer groups, each one with two general purpose hardware timers. All the timers are based on 64 bits
  counters and 16 bit prescalers. The timer counters can be configured to count up or down and support automatic reload
  and software reload. They can also generate alarms when they reach a specific value, defined by the software. The value
  of the counter can be read by the software program.
  Now even you use all these new 16 ISR-based timers,with their maximum interval practically unlimited (limited only by
  unsigned long miliseconds), you just consume only one ESP32 timer and avoid conflicting with other cores' tasks.
  The accuracy is nearly perfect compared to software timers. The most important feature is they're ISR-based timers
  Therefore, their executions are not blocked by bad-behaving functions / tasks.
  This important feature is absolutely necessary for mission-critical tasks.
  Based on SimpleTimer - A timer library for Arduino.
  Author: mromani@ottotecnica.com
  Copyright (c) 2010 OTTOTECNICA Italy
  Based on BlynkTimer.h
  Author: Volodymyr Shymanskyy
*****************************************************************************************************************************/
/* Notes:
   Special design is necessary to share data between interrupt code and the rest of your program.
   Variables usually need to be "volatile" types. Volatile tells the compiler to avoid optimizations that assume
   variable can not spontaneously change. Because your function may change variables while your program is using them,
   the compiler needs this hint. But volatile alone is often not enough.
   When accessing shared variables, usually interrupts must be disabled. Even with volatile,
   if the interrupt changes a multi-byte variable between a sequence of instructions, it can be read incorrectly.
   If your data is multiple variables, such as an array and a count, usually interrupts need to be disabled
   or the entire sequence of your code which accesses the data.
   This example will demonstrate the nearly perfect accuracy compared to software timers by printing the actual elapsed millisecs.
   Being ISR-based timers, their executions are not blocked by bad-behaving functions / tasks, such as connecting to WiFi, Internet
   and Blynk services. You can also have many (up to 16) timers to use.
   In this example will use interrupt to detect whenever the SW is active, then use a hardware timer to poll and switch
   ON/OFF a sample relay (lamp)
   We'll see this ISR-based operation will have highest priority, preempts all remaining tasks (if there where any ;)) to assure its
   functionality.
*/

// These define's must be placed at the beginning before #include "TimerInterrupt_Generic.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG           0
#define _TIMERINTERRUPT_LOGLEVEL_       0

#include "ESP32TimerInterrupt.h"

// Init ESP32 timer 1
ESP32Timer ITimer1(1);

#define TIMER_INTERVAL_MS             1000
#define DEBOUNCE_TIME                 30
#define LED_PIN                       21
#define BUTTON1_PIN                   5
#define BUTTON2_PIN                   4

volatile unsigned long  lastDebounceTime_button1  = 0;
volatile unsigned long  lastDebounceTime_button2  = 0;
volatile bool           SwitchReset_button1  = true;
volatile bool           SwitchReset_button2  = true;

volatile bool           buttonPressed_button1     = false;
volatile bool           alreadyTriggered_button1  = false;
volatile bool           buttonPressed_button2     = false;
volatile bool           alreadyTriggered_button2  = false;

volatile bool           LampState    = false;

void IRAM_ATTR Falling_button1();
void IRAM_ATTR Rising_button1();
void IRAM_ATTR Falling_button2();
void IRAM_ATTR Rising_button2();
void IRAM_ATTR ButtonCheck();
void IRAM_ATTR ToggleLED();


void IRAM_ATTR Rising_button1() {
  unsigned long currentTime  = millis();
  unsigned long TimeDiff;

  TimeDiff = currentTime - lastDebounceTime_button1;

  if (digitalRead(BUTTON1_PIN) && (TimeDiff > DEBOUNCE_TIME) ) {
    buttonPressed_button1 = false;
    lastDebounceTime_button1 = currentTime;
  }
}

void IRAM_ATTR Falling_button1() {
  unsigned long currentTime  = millis();

  if ( !digitalRead(BUTTON1_PIN) && (currentTime > lastDebounceTime_button1 + DEBOUNCE_TIME)) {
    lastDebounceTime_button1 = currentTime;
    buttonPressed_button1 = true;
  }
}

//create herer Rising and Falling ISR for button2


void IRAM_ATTR Rising_button2() {
  unsigned long currentTime  = millis();
  unsigned long TimeDiff;

  TimeDiff = currentTime - lastDebounceTime_button2;

  if (digitalRead(BUTTON2_PIN) && (TimeDiff > DEBOUNCE_TIME) ) {
    buttonPressed_button2 = false;
    lastDebounceTime_button2 = currentTime;
  }
}

void IRAM_ATTR Falling_button2() {
  unsigned long currentTime  = millis();

  if ( !digitalRead(BUTTON2_PIN) && (currentTime > lastDebounceTime_button2 + DEBOUNCE_TIME)) {
    lastDebounceTime_button2 = currentTime;
    buttonPressed_button2 = true;
  }
}




void IRAM_ATTR HWCheckButton(void)
{
  if ((!alreadyTriggered_button1 && buttonPressed_button1)|| (!alreadyTriggered_button2 && buttonPressed_button2))
  {
    if(buttonPressed_button1)
        alreadyTriggered_button1 = true;
    else
        alreadyTriggered_button2 = true;
   ButtonCheck();
  }
  else if (!buttonPressed_button1 || !buttonPressed_button2 )
  {
    // Reset flag when button released to avoid triggered repeatedly
    if(buttonPressed_button1)
        alreadyTriggered_button1 = false;
    else
        alreadyTriggered_button2 = false;
    ButtonCheck();
  }
}


void IRAM_ATTR ButtonCheck()
{
  boolean SwitchState_button1 = (digitalRead(BUTTON1_PIN));
  boolean SwitchState_button2 = (digitalRead(BUTTON2_PIN));

  if (!SwitchState_button1 && SwitchReset_button1)
  {
    ToggleLED();
    SwitchReset_button1 = false;
  }
  else if (SwitchState_button1)
    SwitchReset_button1 = true;

  if (!SwitchState_button2 && SwitchReset_button2)
  {
    ToggleLED();
    SwitchReset_button2 = false;
  }
  else if (SwitchState_button2)
    SwitchReset_button2 = true;
}


void IRAM_ATTR ToggleLED()
{
  LampState = !LampState;
  digitalWrite(LED_PIN, LampState);
}


void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BUTTON1_PIN), Falling_button1, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON1_PIN), Rising_button1, RISING);
  // copy and change lens above to attch interrupt to button2
  attachInterrupt(digitalPinToInterrupt(BUTTON2_PIN), Falling_button1, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON2_PIN), Rising_button1, RISING);

  
  Serial.begin(115200);
  while (!Serial);

  Serial.print(F("\nStarting ISR_Timer_Switch on ")); Serial.println(ARDUINO_BOARD);
  Serial.println(ESP32_TIMER_INTERRUPT_VERSION);
  Serial.print(F("CPU Frequency = ")); Serial.print(F_CPU / 1000000); Serial.println(F(" MHz"));
  // Interval in microsecs (us),  ms ==> multiply by 1000
  // Be sure to place this HW Timer well ahead blocking calls, because it needs to be initialized.
  if (ITimer1.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, HWCheckButton)) {
    Serial.print(F("Starting  ITimer1 OK,  from restart - millis() = "));
    Serial.println(millis());
  } else
    Serial.println(F("Can't set ITimer1. Select another freq. or timer"));
}


void loop()
{
  vTaskDelete(nullptr);
}
