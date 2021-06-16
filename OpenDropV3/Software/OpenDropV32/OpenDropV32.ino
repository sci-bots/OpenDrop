/*
  Basic Code to run the OpenDrop V3, Research platfrom for digital microfluidics
  Object codes are defined in the OpenDrop.h library
  Written by Urs Gaudenz from GaudiLabs
  2017
 */

#include <stdlib.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Vrekrer_scpi_parser.h"

// Create an SCPI parser to handle serial control
SCPI_Parser my_instrument;

#include <OpenDrop.h>
#include <OpenDropAudio.h>

#include "hardware_def.h"

OpenDrop OpenDropDevice = OpenDrop(); 

Drop *myDrop = OpenDropDevice.getDrop();

bool FluxCom[16][8];

float voltage_rms = 240;
int JOY_value;
int joy_x,joy_y;
int x,y;
int del_counter=0;
int del_counter2=0;

bool SWITCH_state=true;
bool SWITCH_state2=true;
int j=0; 
  
// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);

  // Register SCPI commands
  my_instrument.RegisterCommand(F("*IDN?"), &identify);
  my_instrument.RegisterCommand(F("CHANnels:STATe"), &set_state_of_channels);
  my_instrument.RegisterCommand(F("CHANnels:STATe?"), &get_state_of_channels);
  my_instrument.RegisterCommand(F("VOLTage"), &set_voltage);
  my_instrument.RegisterCommand(F("VOLTage?"), &get_voltage);

  OpenDropDevice.begin();
  OpenDropDevice.set_voltage(240,false,1000);
  OpenDropDevice.set_Fluxels(FluxCom);

  pinMode(JOY_pin, INPUT);  
                  
  OpenDropAudio.begin(16000);
  OpenDropAudio.playMe(2);
  delay (2000);
  
  OpenDropDevice.drive_Fluxels();
  OpenDropDevice.update_Display();
  Serial.println("Welcome to OpenDrop");

  myDrop->begin(7,4);
  OpenDropDevice.update();
}

// the loop function runs over and over again forever


void loop() {
  if (Serial.available()>0) {
    digitalWrite(LED_Rx_pin,HIGH);
    my_instrument.ProcessInput(Serial, "\n");
  } else {
    digitalWrite(LED_Rx_pin,LOW);
  }

  del_counter--;

  if (del_counter<0) {
    OpenDropDevice.update_Display();
    del_counter=1000;
  }

  SWITCH_state=digitalRead(SW1_pin);

  if (!SWITCH_state) { 
    OpenDropAudio.playMe(1);
    Menu(OpenDropDevice);
    OpenDropDevice.update_Display();
    del_counter2=200;
  }

  JOY_value = analogRead(JOY_pin);

  if ((JOY_value<950)&(del_counter2==0)) {
    if  (JOY_value<256) {
      myDrop->move_right();Serial.println("right");
    }

    if  ((JOY_value>725)&&(JOY_value<895)) {
      myDrop->move_up();    Serial.println("up");
    }
    
    if  ((JOY_value>597)&&(JOY_value<725)) {
      myDrop->move_left();Serial.println("left");
    }
    
    if  ((JOY_value>256)&&(JOY_value<597)) {
      myDrop->move_down();Serial.println("down");
    }

    OpenDropDevice.update_Drops();
    OpenDropDevice.update(); 
    del_counter2=200;
    del_counter=1000;
  } 

  if (JOY_value>950) del_counter2=0;
  if (del_counter2>0) del_counter2--;
}

void identify(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  // Return a string that uniquely identifies the OpenDrop.
  // The string is of the form "GaudiLabs,<model>,<serial number>,<software revision>".
  interface.println(F("GaudiLabs,OpenDrop v3.2,#00,0.0"));
}

void set_voltage(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  // For simplicity no bad parameter check is done.
  if (parameters.Size() > 0) {
    voltage_rms = constrain(String(parameters[0]).toFloat(), 0, 240);
    OpenDropDevice.set_voltage(voltage_rms,false,1000);  
  }
}

void get_voltage(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  interface.println(String(voltage_rms, DEC));
}

void set_state_of_channels(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  if (parameters.Size() == 1 && strlen(parameters[0]) == 32) {
    // Unpack the hex-encoded string.
    for (int x = 0; x < 16; x++) {
      char hex_str[2];
      memcpy(hex_str, &parameters[0][2 * x], 2);
      uint8_t data = String(strtol(hex_str, NULL, 16)).toInt();
      for (int y = 0; y < 8; y++) {
        FluxCom[x][y] = (data & (0x01 << (7-y))) > 0;
      }
    }
    // Update the hardware.
    OpenDropDevice.set_Fluxels(FluxCom);
    OpenDropDevice.drive_Fluxels();
    OpenDropDevice.update_Display();
  }
}

void get_state_of_channels(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  // Iterate through the fluxels and generate a hex-encoded string of the channel
  // states.
  for (int x = 0; x < 16; x++) {
    uint16_t data = 0;
    for (int y = 0; y < 8; y++) {
      data += (FluxCom[x][y] & 0x01) << (7-y);
    }
    char str[2];
    sprintf(str, "%02x", data);
    interface.print(str);
  }
  interface.println();
}
