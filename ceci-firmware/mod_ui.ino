/**
 * User interface module.
 * Responsible for handling and reacting to user interaction.
 */
#include "qd_sched.h"
#include "modules.h"

#ifdef HAS_MOD_UI
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// rate at which the UI is updated
#define UI_REFRESH_RATE_MS 50

// This constant defines how long the gui should wait before going back to the home screen
#define GO_BACK_HOME_MS 3000
// Increment/decrement amount for target temperature
#define TEMP_INCDEC 0.5

// OLED display contrast. Must be between 0 and 255
#define SCREEN_CONTRAST 0x0F // 16

#define CHAR_WIDTH  6 // width in pixels for a single char of size 1 (fixed)
#define CHAR_HEIGHT 8 // height in pixels for a single char of size 1 (fixed)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static switch_state_t up_btn_state = BTN_RELEASED;
static switch_state_t dn_btn_state = BTN_RELEASED;

static gui_state_t gui_state = HOME;
static unsigned long state_change_millis = 0;
static float target_temp_new = 20.0; // new target temperature, to be saved on exit
static unsigned int tstat_mode_new = 0; // new thermostat mode, to be saved on exit

/*
 * State machine for the switches.
 */
switch_state_t btn_state_machine(switch_state_t state, int pin)
{
  switch_state_t newstate = state;
  switch (state) {
    case BTN_RELEASED:
      if (digitalRead(pin) == 0) // button pressed
        newstate = PRESS_DEBOUNCE;
      break;

    case PRESS_DEBOUNCE:
      if (digitalRead(pin) == 0)
        newstate = KEYPRESS;
      else
        newstate = BTN_RELEASED;
      break;

    case KEYPRESS:
      if (digitalRead(pin) == 1) // button released
        newstate = RELEASE_DEBOUNCE;
      break;

    case RELEASE_DEBOUNCE:
      if (digitalRead(pin) == 0)
        newstate = KEYPRESS;
      else
        newstate = KEYRELEASE;
      break;

    case KEYRELEASE:
      newstate = BTN_RELEASED;
      break;
  }

  return newstate;
}

void ui_task(void)
{
  // Run the button state machine
  up_btn_state = btn_state_machine(up_btn_state, UP_BUTTON_GPIO);
  dn_btn_state = btn_state_machine(dn_btn_state, DN_BUTTON_GPIO);

  // Draw display
  display.clearDisplay();
  // top bar with name & WiFi RSSI
  display.drawLine(0, CHAR_HEIGHT, SCREEN_WIDTH-1, CHAR_HEIGHT, WHITE);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(node_name);
  display.setCursor((SCREEN_WIDTH - 4*CHAR_WIDTH), 0);
  display.write(' '); // ensure there's at least a space before the RSSI value
  display.println(String(WiFi.RSSI()));

  switch(gui_state) {
    case HOME:
      // big current temperature
      display.setCursor(0, 24);
      display.setTextSize(5);
      // have a look here: https://www.arduino.cc/reference/en/language/variables/data-types/stringobject/
      display.println(String(get_temperature(SENSOR_NOWAKE).value, 1));

      if ((up_btn_state == KEYRELEASE) && (dn_btn_state == KEYRELEASE))
        gui_state = SET_MODE;
      else if ((up_btn_state == KEYRELEASE) || (dn_btn_state == KEYRELEASE))
        gui_state = SET_TEMP;

      // TODO: load tstat_mode_new and target_temp_new
      //if (dn_btn_state == KEYPRESS) // software reset with long down button press to put the device in download mode
      //  if ((millis() - state_change_millis) >= 3*GO_BACK_HOME_MS)
      //    ESP.restart();
      //else
        state_change_millis = millis();
      break;
      
    case SET_TEMP:
      display.setTextSize(2);
      display.setCursor(0, CHAR_HEIGHT + CHAR_HEIGHT/2);
      display.println("SET:");

      display.setCursor(4*CHAR_WIDTH, (CHAR_HEIGHT + CHAR_HEIGHT/2) + 2*CHAR_HEIGHT);
      display.setTextSize(4);
      display.println(String(target_temp_new, 1));
      
      if ((up_btn_state == KEYRELEASE) && (dn_btn_state != KEYRELEASE)) { // react to up button only
        target_temp_new += TEMP_INCDEC;
        state_change_millis = millis();
      } else if ((dn_btn_state == KEYRELEASE) && (up_btn_state != KEYRELEASE)) { // react to down button only
        target_temp_new -= TEMP_INCDEC;
        state_change_millis = millis();
      }
      
      if ((millis() - state_change_millis) >= GO_BACK_HOME_MS) {
        gui_state = HOME;
        // TODO: save temperature settings
      }
      break;
    
    case SET_MODE:
      display.setTextSize(2);
      display.setCursor(0, CHAR_HEIGHT + CHAR_HEIGHT/2);
      display.println("MODE:");

      display.setCursor(5*CHAR_WIDTH, (CHAR_HEIGHT + CHAR_HEIGHT/2) + 2*CHAR_HEIGHT);
      display.setTextSize(4);

      if (tstat_mode_new == 0)
          display.println("AUTO");
      else if (tstat_mode_new == 1)
          display.println("OFF");
      else if (tstat_mode_new == 2)
          display.println("ON");

      if ((up_btn_state == KEYRELEASE) && (dn_btn_state != KEYRELEASE)) { // react to up button only
        tstat_mode_new = (tstat_mode_new + 1) %3;
        state_change_millis = millis();
      } else if ((dn_btn_state == KEYRELEASE) && (up_btn_state != KEYRELEASE)) { // react to down button only
        tstat_mode_new = (tstat_mode_new - 1) %3;
        state_change_millis = millis();
      }
     
      if ((millis() - state_change_millis) >= GO_BACK_HOME_MS) {
        gui_state = HOME;
        // TODO: save mode settings
      }
      break;
  }

  display.display();
}

void mod_ui_init(void)
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;) yield(); // Don't proceed, loop forever
  }
  
  // Set the contrast and clear the buffer
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(SCREEN_CONTRAST);
  display.clearDisplay();

  // Show loading text
  display.setTextSize(2); // font twice as big
  display.setTextColor(WHITE);
  display.setCursor(0, 2*CHAR_HEIGHT);
  display.println(F("Loading..."));
  display.display();
  //display.startscrollright(0x00, 0x0F);

  // Set GPIOs
  pinMode(UP_BUTTON_GPIO, INPUT_PULLUP);
  pinMode(DN_BUTTON_GPIO, INPUT_PULLUP);

  // start UI task
  sched_put_task(&ui_task, UI_REFRESH_RATE_MS, false);
  LOG("Loaded mod_ui.");
}

#else
void mod_ui_init(void){ /* do nothing */ }
#endif
