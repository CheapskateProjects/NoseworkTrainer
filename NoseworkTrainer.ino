/*
  Nosework trainer

  This project is a simple dig training device which uses multiple distance sensors to detect if the dog is close to any of the sensors.

  If dog spends enough time close to a selected sensor a treat signal will be given. The target time and target sensor can be selected
  using buttons and the current values will be displayed on the screen. 

  created   Sep 2018
  by CheapskateProjects

  ---------------------------
  The MIT License (MIT)

  Copyright (c) 2018 CheapskateProjects

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NewPing.h>

// Screen pins
#define OLED_MOSI  12 
#define OLED_CLK   11
#define OLED_DC    SDA
#define OLED_CS    SCL
#define OLED_RESET 13

// Common trigger pin for all of the sensors
#define TRIGGER_PIN  2
// The max distance sensors will read
#define MAX_DISTANCE 100
// Dog fur is not a great reflector for sound. Allow some failed readings to prevent false negatives. 
#define SENSITIVITY 10
// Max allowed triggering distance. This is used to separate sensors from each others. Sensors may not be installed closer to each other than this distance. 
#define MAX_TRIGGER_DISTANCE 20

// Debug value. If true, instead of setting the treat pin we will give information on serial. Arduino Uno doesn't have enough pins so this is a workaround to enable debugging. 
#define DEBUG false

// Current target delay (how long the dog should be present on a sensor). 
float delay_limit = 1.0f;
// How much the time is changed when a button is pressed. Here half second increment. 
float delay_delta = 0.5f;
// Currently selected sensor. Not same as the pin id as pins start from 3. 
int selected = 1;

// Some help variables
// measured distance
unsigned long distance;
// current max distance. Always reseted when sensor is changed as we may have obstacle near the sensor. 
unsigned long currentMax;
// time variable for last time the clock was reseted
unsigned long last_reset;
// How many fails since last successful reading. 
unsigned long fail_counter=0;

// Initializing ping for each of the sensors
NewPing ping[8] = 
{
  NewPing(TRIGGER_PIN, 3, MAX_DISTANCE),
  NewPing(TRIGGER_PIN, 4, MAX_DISTANCE),
  NewPing(TRIGGER_PIN, 5, MAX_DISTANCE),
  NewPing(TRIGGER_PIN, 6, MAX_DISTANCE),
  NewPing(TRIGGER_PIN, 7, MAX_DISTANCE),
  NewPing(TRIGGER_PIN, 8, MAX_DISTANCE),
  NewPing(TRIGGER_PIN, 9, MAX_DISTANCE),
  NewPing(TRIGGER_PIN, 10, MAX_DISTANCE),
};

// Display
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

void setup()
{
  // Delay at the begining is not required but it may in some cases be required for reprogramming. 
  delay(5000);

  // Initialize Display
  display.begin(SSD1306_SWITCHCAPVCC);
  display.display();
  delay(2000);

  // Initialize buttons
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  pinMode(A2, INPUT_PULLUP);
  pinMode(A3, INPUT_PULLUP);

  // Initialize serial or treat pin
  if(DEBUG)
  {
    Serial.begin(115200);
  }
  else
  {
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);
  }

  // Initialize max distance for current sensor
  currentMax = ping[selected-1].ping_cm()-1;// Prevent measuring the wall closer than MAX_TRIGGER_DISTANCE
}

void loop()
{
        if(DEBUG)
        {
          Serial.println("Start loop");
        }

        /*
         * Handle changing the selected sensor
         */
        if (digitalRead(A0) == LOW)
        {
          selected -= 1;
          if(selected < 1)
          {
            selected = 8;
          }
          currentMax = ping[selected-1].ping_cm()-1;// Prevent measuring the wall closer than MAX_TRIGGER_DISTANCE
        }
        else if (digitalRead(A2) == LOW)
        {
          selected += 1;
          if(selected > 8)
          {
            selected = 1;
          }
          currentMax = ping[selected-1].ping_cm()-1;// Prevent measuring the wall closer than MAX_TRIGGER_DISTANCE
        }

        /* 
         * Handle changing the target delay 
         */
        else if (digitalRead(A1) == LOW)
        {
          delay_limit += delay_delta;
        }
        else if (digitalRead(A3) == LOW)
        {
          delay_limit -= delay_delta;
          if(delay_limit < 0.0f)
          {
            delay_limit += delay_delta;
          }
        }

        /*
         * Handle measuring distance for current sensor and see if treat should be given
         */
        else
        {
          distance = ping[selected-1].ping_cm();
        
          if(distance > 0 && distance <= MAX_TRIGGER_DISTANCE && distance <= currentMax)
          {
            if(DEBUG)
            {
              Serial.println("RANGE" + String((millis() - last_reset)) + " / " + String((delay_limit*1000)));
            }
            
            fail_counter = 0;
        
            if(millis() - last_reset >= (delay_limit*1000))
            {
              display.clearDisplay();
              display.fillScreen(BLACK);
              display.setTextSize(3);
              display.setTextColor(WHITE);
              display.setCursor(0,0);
              display.print("TREAT");
              display.display();
              
              if(DEBUG)
              {
                Serial.println("TREAT!");
                delay(1000);
              }
              else
              {
                digitalWrite(1, HIGH);
                delay(1000);
                digitalWrite(1, LOW);
              }
              
              last_reset = millis();
            }
          }
          else
          {
            if(DEBUG)
            {
              Serial.println("OUT OF RANGE");
            }
            fail_counter++;
            if(fail_counter >= SENSITIVITY)
            {
              last_reset = millis(); 
            }
          }
  
          
        }

        /**
         * Just refreshing the screen to show current details: selected sensor, target delay
         */
        display.clearDisplay();
        display.fillScreen(BLACK);
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(0,0);
        display.print("sensor:");
        display.println(String(selected));
        display.print("time:");
        display.print(String(delay_limit));
        display.print("s ");
        display.display();

        // 10 measures every second
        delay(100);
}
