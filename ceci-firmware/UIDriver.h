/*
 * Handle the screen and buttons of the device (if any)
 */
#ifndef UIDRIVER_H
#define UIDRIVER_H

// rate at which the UI is updated
#define UI_REFRESH_RATE_MS 50

// This constant defines how long the gui should wait before going back to the home screen
#define GO_BACK_HOME_MS 3000
// Increment/decrement amount for target temperature
#define TEMP_INCDEC 0.5

#define SCREEN_WIDTH  128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define CHAR_WIDTH  6 // width in pixels for a single char of size 1 (fixed)
#define CHAR_HEIGHT 8 // height in pixels for a single char of size 1 (fixed)

// Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_RESET -1 

// Which pin is the UP button attached to?
#define UP_BUTTON_GPIO D5
// Which pin is the DOWN button attached to?
#define DN_BUTTON_GPIO D3

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
 * Initialize the user interface
 */
void ui_init(void);

/*
 * Task that runs the UI
 */
void ui_task(void);

#endif
