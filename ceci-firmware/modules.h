/**
 * Header file to contain all the module function definition.
 * 
 * There shouldn't be many, since each module is initialized and then relays on qd_sched to do its stuff
 */
#ifndef MODULES_H
#define MODULES_H

/**************
 * mod_sensors
 **************/
void mod_sensors_init(SPIFFSIniFile* conf);

/*****************
 * mod_thermostat
 *****************/
void mod_thermostat_init(SPIFFSIniFile* conf);

/*
 * Getters and setters for thermostat parameters
 */
float thermostat_get_temp(void);
void thermostat_set_temp(float);
char thermostat_get_mode(void);
void thermostat_set_mode(char);

/************
 * mod_relay
 ************/
void mod_relay_init(SPIFFSIniFile* conf);

/*********
 * mod_ui
 *********/

/*
 * The buttons interact with a state machine having the following states:
 *  BTN_RELEASED     - Default state, the button is not pressed/has not been pressed
 *  PRESS_DEBOUNCE   - State where the machine stays until the switch contacts have stabilized after the button is pressed
 *  KEYPRESS         - The button is pressed
 *  KEYRELEASE       - The button has been released after being pressed
 *  RELEASE_DEBOUNCE - State where the machine stays until the switch contacts have stabilized after the button is released
 */
enum switch_state_t {BTN_RELEASED, PRESS_DEBOUNCE, KEYPRESS, KEYRELEASE, RELEASE_DEBOUNCE};
typedef enum switch_state_t switch_state_t;

/*
 * Also the display has its own state machine, depending on the content it has to display.
 * Pressing the buttons changes the state and each state draws something different:
 *  HOME  - Shows the last temperature reading
 *  SET_TEMP - Allows the user to change the target temperature, it is activated by pushing one of the buttons
 *  SET_MODE - Change the mode: Auto, always ON, always OFF. Effective on exit
 */
enum gui_state_t {HOME, SET_TEMP, SET_MODE};
typedef enum gui_state_t gui_state_t;

/*
 * Initialize GUI
 */
void mod_ui_init(void);

#endif
