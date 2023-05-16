#include <mbed.h>
#include <chrono>
#include "drivers/LCD_DISCO_F429ZI.h"

#define BACKGROUND 1
#define FOREGROUND 0
#define GRAPH_PADDING 5

// Define an instance of the LCD driver for the STM32F429 Discovery board
LCD_DISCO_F429ZI lcd;

// Calculate the dimensions of the graph to be displayed on the LCD
uint32_t graph_width = lcd.GetXSize() - 2 * GRAPH_PADDING;
uint32_t graph_height = lcd.GetYSize() - 2 * GRAPH_PADDING;

// Buffer to hold the strings to be displayed on the LCD
char display_buf[2][60];

// Define an enumeration to represent the possible states of the system
enum State
{
  RECORDING_STATE,
  UNLOCKED_STATE,
  LOCKED_STATE,
  ENTER_KEY_STATE
};

// Define a volatile variable to hold the current state, initially set to UNLOCKED_STATE
volatile State state = UNLOCKED_STATE;

// Define a volatile variable to count the number of button presses
volatile int button_press_count = 0;

// Define a timer to manage state transitions
Timeout state_timer;

// Function to update the screen based on the current state
void update_screen_state()
{
  // Select the foreground layer of the LCD
  lcd.SelectLayer(FOREGROUND);

  // Clear the screen
  lcd.Clear(LCD_COLOR_BLACK);

  // Change the display based on the current state
  switch (state)
  {
  case RECORDING_STATE:
    lcd.SetBackColor(LCD_COLOR_BLACK);
    lcd.SetTextColor(LCD_COLOR_BLUE);
    snprintf(display_buf[0], 60, "Recording Key...");
    lcd.DrawRect(GRAPH_PADDING, lcd.GetYSize() - graph_height - GRAPH_PADDING, graph_width, graph_height);
    break;
  case UNLOCKED_STATE:
    lcd.SetBackColor(LCD_COLOR_BLACK);
    lcd.SetTextColor(LCD_COLOR_GREEN);
    snprintf(display_buf[0], 60, "Unlocked!");
    lcd.DrawRect(GRAPH_PADDING, lcd.GetYSize() - graph_height - GRAPH_PADDING, graph_width, graph_height);
    break;
  case LOCKED_STATE:
    lcd.SetBackColor(LCD_COLOR_BLACK);
    lcd.SetTextColor(LCD_COLOR_RED);
    snprintf(display_buf[0], 60, "LOCKED!");
    lcd.DrawRect(GRAPH_PADDING, lcd.GetYSize() - graph_height - GRAPH_PADDING, graph_width, graph_height);
    break;
  case ENTER_KEY_STATE:
    lcd.SetBackColor(LCD_COLOR_BLACK);
    lcd.SetTextColor(LCD_COLOR_CYAN);
    snprintf(display_buf[0], 60, "Enter Password...");
    lcd.DrawRect(GRAPH_PADDING, lcd.GetYSize() - graph_height - GRAPH_PADDING, graph_width, graph_height);
    break;
  }

  // Display the current state message on the screen
  lcd.DisplayStringAt(0, LINE(10), (uint8_t *)display_buf[0], CENTER_MODE);
}

// Define an interrupt to handle user button presses
InterruptIn user_button(USER_BUTTON);

// Function to handle state transitions based on button presses
void state_handler()
{
    // On button press, determine the current state and transition to the next state
    if (button_press_count == 1 && state == UNLOCKED_STATE)
    {
      state = RECORDING_STATE; 
      button_press_count = 0;
      update_screen_state();
      state_timer.attach([]{ state = LOCKED_STATE; update_screen_state(); }, 5s); // Record for 5s then move to LOCKED_STATE
    }
       // Check if the button press happened in the LOCKED_STATE
    else if (button_press_count == 1 && state == LOCKED_STATE)
    {
      // If so, transition to the ENTER_KEY_STATE
      state = ENTER_KEY_STATE;
      // Reset the button press count
      button_press_count = 0;
      // Update the screen to reflect the new state
      update_screen_state();
      // Set a timer to transition back to LOCKED_STATE if no other action is taken
      state_timer.attach([]{ state = LOCKED_STATE; update_screen_state(); }, 10s);
    }
    // Check if the button press happened in the ENTER_KEY_STATE
    else if (button_press_count == 1 && state == ENTER_KEY_STATE)
    {
      // If so, transition to the UNLOCKED_STATE
      state = UNLOCKED_STATE;
      // Reset the button press count
      button_press_count = 0;
      // Update the screen to reflect the new state
      update_screen_state();
    }
}

// Function to handle the button press event
void button_press_handler()
{
    // Increment the button press count
    button_press_count++;
    // Call the state handler to process the button press
    state_handler();
}

// Function to handle state transitions when a timeout occurs
void state_timeout_handler()
{
  // Check the current state and transition to the appropriate next state
  switch (state)
  {
    case UNLOCKED_STATE:
      state = LOCKED_STATE;
      update_screen_state();
      break;
    case LOCKED_STATE:
      // Stay in LOCKED_STATE, no need to transition
      update_screen_state();
      break;
    case ENTER_KEY_STATE:
      // Stay in ENTER_KEY_STATE, no need to transition
      update_screen_state();
      break;
    default:
      break;
  }
}

enum LCDState
{
    EnterKey,
    EnterPassword,
    Locked,
    Unlocked,
    IncorrectPassword,
    CorrectPassword,
    NoKeyAllowed
};

int main()
{
  // Configure the user button to call the button press handler when a rising edge is detected
  user_button.rise(&button_press_handler);

  // Call this at the start of the program to display the initial state
  update_screen_state();

  // Set an initial timeout to transition to the LOCKED_STATE after 15 seconds
  state_timer.attach(state_timeout_handler, 15s);

  while (1) 
  {
    // Keep the main loop empty as everything is event-driven
  }
}