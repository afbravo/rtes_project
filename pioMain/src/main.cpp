/*
REAL TIME EMBEDDED SYSTEMS FINAL PROJECT
Spring 2023

Members:
    Andres Bravo - afb389
    Andre Duong - ld2784
    Anshika Gupta - ag8800
    Aris Panousopoulos - ap6487

Description:
    This project is a security system that uses a gyroscope to record the angular acceleration on the device. This
    information is used to create a lock type system in which the user can create a key by recording it and lock
    the device. The device will the only unlock if the correct password is entered and matches the key. The device
    can also allow the key to be changed under the correct conditions.

Usage:
    When the device is first turned on, the user will be prompted to enter a key. The device needs a key to be entered
    before it can be locked.
    Afterwards the device is locked.

    To unlock device:
        -user must short press the button and move the device correctly matching the key while device is locked.
    To lock device:
        -user must short press the button while device is unlocked.
    To change key:
        -user must long press (3s by default) the button while device is unlocked. NOTE: user can not
        change the key if the device is locked.
*/

#include <mbed.h>
#include "gyro.h"
#include "drivers/LCD_DISCO_F429ZI.h"

//---------------------------------------------Global Constants--------------------------------------------------

//LCD constants
#define BACKGROUND 1
#define FOREGROUND 0
#define GRAPH_PADDING 5

//SPI pins for GYRO communication
#define mosi PF_9
#define miso PF_8
#define sclk PF_7
#define chip_select PC_1

//maximum recording size for the gyro
#define maxRecording 5

//time constants
#define longPressTime 3000000 //3 seconds in microseconds used to determine long press on button
#define timeoutTime 15000000 //15 seconds in microseconds used to determine timeout on gyro recording


//--------------------------------------------Global Objects--------------------------------------------------

//LCD object
LCD_DISCO_F429ZI lcd;

//button as an interrupt object
InterruptIn button(USER_BUTTON);

//gyro object
Gyro::Gyro gyro(mosi, miso, sclk, chip_select);

//timer for button press
Timer timer;
//timer for timeout
Timer timeoutTimer;

//---------------------------------------------Custom Types--------------------------------------------------

//enum for button press status
enum ButtonPress{notPress, shortPress, longPress};
//enum for state of the device
enum State{locked, unlocked};
//enum for LCD state
enum LCDState
{
    EnterKey,
    EnterPassword,
    Locked,
    Unlocked,
    IncorrectPassword,
    CorrectPassword,
    NoKeyAllowed,
    KeyStored,
};

//---------------------------------------------Global Variables--------------------------------------------------

//last x, y, z positions recorded by the gyro
char xPosition, yPosition, zPosition = 0;

//password recording buffer
char recordings[maxRecording][3];
int recordingSize = 0;

//key buffer
char key[maxRecording][3];
int keySize = 0;

//button status
ButtonPress buttonStatus = notPress;
//device status
State status = unlocked;

//LCD graph width and height
uint32_t graph_width = lcd.GetXSize() - 2 * GRAPH_PADDING;
uint32_t graph_height = lcd.GetYSize() - 2 * GRAPH_PADDING;
//LCD text buffer
char display_buf[2][60];

//---------------------------------------------Functions Declarations--------------------------------------------------
void showPosition(Gyro::Gyro *gyro);
void showXYZ(Gyro::Gyro *gyro);
bool checkPassword(void);
void raiseEvent(void);
void fallEvent(void);
void recordGyro(char (*data)[3], int *size, bool timeoutAct);
void updateLCD(LCDState state);

//----------------------------------------------Functions Definitions--------------------------------------------------
void showPosition(Gyro::Gyro *gyro){
    /*
    NOT CURRENTLY USED
    Helper function to show current x, y, and z positions (centered 0, positive 1, negative 2) values.
    Parameters:
        gyro: pointer to the gyro object
    Returns:
        None
    */
    gyro->updatePosition();
    if( (gyro->getYPosition() != yPosition) || 
            (gyro->getXPosition() != xPosition) || 
            (gyro->getZPosition() != zPosition))
        {
            yPosition = gyro->getYPosition();
            xPosition = gyro->getXPosition();
            zPosition = gyro->getZPosition();
            printf("x loc: %d  y loc: %d  z loc: %d\n", xPosition, yPosition, zPosition);
            printf("--------------------------------\n");
        }
}

void showXYZ(Gyro::Gyro *gyro){
    /*
    NOT CURRENTLY USED
    Helper function to show current x, y, and z angular acceleration values.
    Parameters:
        gyro: pointer to the gyro object
    Returns:
        None
    */
    gyro->readXYZ();
    printf("x: %d y: %d z: %d\n", gyro->getX(), gyro->getY(), gyro->getZ());
}

bool checkPassword(void){
    /*
    Function checks if the recorded key matches the entered password.
    Parameters:
        None
    Returns:
        true if the key matches the password
        false if the key does not match the password
    */

    if(recordingSize != keySize){return false;} //check if the sizes are the same to avoid out of bounds error
    for (int i = 0; i < recordingSize; i++){ //loop through each element of the array
        if(recordings[i][0] != key[i][0] || 
            recordings[i][1] != key[i][1] || 
            recordings[i][2] != key[i][2])
        {
            return false;
        }
    }
    return true;
}

void raiseEvent(void){
    /*
    Interrupt handler for button rising edge event (button is pressed).
    This handler will start the button timer to determine if the button
    was short pressed or long pressed.
    Parameters:
        None
    Returns:
        None
    */
    timer.start();
}

void fallEvent(void){
    /*
    Interrupt handler for button falling edge event (button is depressed).
    This handler will stop the button timer to determine if the button
    was short pressed or long pressed. The button status will be updated
    accordingly and the timer will be reset.
    Parameters:
        None
    Returns:
        None
    */
    timer.stop(); //stop timer
    if(timer.elapsed_time().count() > longPressTime){ //check if long press
        buttonStatus = longPress;
    }
    else{ //if not long press, then short press
        buttonStatus = shortPress;
    }
    timer.reset();
}

void recordGyro(char (*data)[3], int *size, bool timeoutAct){
    /*
    Function records the change in the gyro's x, y, and z locations using the gyro's updatePosition() function.
    The function will record the change in location until the maxRecording is reached, the button is pressed,
    or the timeoutTime is reached(if active).
    Parameters:
        data: pointer to the array that will store the data
        size: pointer to the variable that will store the size of the data array
        timeoutAct: boolean that determines if the timeout is active
    Returns:
        None
    */

   //variables
    *size = 0;
    xPosition = 0;
    yPosition = 0;
    zPosition = 0;
    buttonStatus = notPress; //reset button status
    gyro.center(); //centers the gyro to avoid unsynced data
    timeoutTimer.reset(); //reset timeout timer
    timeoutTimer.start(); //start timeout timer

    while (*size < maxRecording)
    {
        gyro.updatePosition(); //update gyro position
        //check if the position has changed in any 3 axis
        if( (gyro.getYPosition() != yPosition) || 
                (gyro.getXPosition() != xPosition) || 
                (gyro.getZPosition() != zPosition))
            {
                //update last position variables
                yPosition = gyro.getYPosition();
                xPosition = gyro.getXPosition();
                zPosition = gyro.getZPosition();
                //store position in data array
                data[*size][0] = xPosition;
                data[*size][1] = yPosition;
                data[*size][2] = zPosition;
                (*size)++;
            }
        
        //checks if the button has been pressed
        if((buttonStatus == longPress) || (buttonStatus == shortPress)){
            buttonStatus = notPress;
            timeoutTimer.stop(); //stop timeout timer, to avoid runaway timer
            break;
        }
        //checks if the timeout has been reached and if it is active
        if((timeoutTimer.elapsed_time().count() > timeoutTime) && timeoutAct){
            timeoutTimer.stop();
            break;
        }
    }
    timeoutTimer.stop(); //stop timer in case maxRecording was reached
}

void updateLCD(LCDState lcdState){
    /*
    Function updates the lcd to the current state of the device. The allowed states are:
        -EnterKey: user is entering a key
        -EnterPassword: user is entering a password attempt
        -Locked: device is locked
        -Unlocked: device is unlocked
        -IncorrectPassword: user entered an incorrect password
        -CorrectPassword: user entered a correct password
        -NoKeyAllowed: user try to entered a key when locked
        -KeyStored: user entered a key and it was stored
    Parameters:
        lcdState: the current state of the device
    Returns:
        None
    */

    // Select the foreground layer of the LCD
    lcd.SelectLayer(FOREGROUND);

    // Clear the screen
    lcd.Clear(LCD_COLOR_BLACK);

    // checks the state of the device and updates the lcd accordingly
    switch (lcdState)
    {
    case EnterKey:
        lcd.SetBackColor(LCD_COLOR_WHITE);
        lcd.SetTextColor(LCD_COLOR_BLUE);
        snprintf(display_buf[0], 60, "Recording Key...");
        lcd.DrawRect(GRAPH_PADDING, lcd.GetYSize() - graph_height - GRAPH_PADDING, graph_width, graph_height);
        break;
    case Unlocked:
        lcd.SetBackColor(LCD_COLOR_BLACK);
        lcd.SetTextColor(LCD_COLOR_GREEN);
        snprintf(display_buf[0], 60, "Unlocked!");
        lcd.DrawRect(GRAPH_PADDING, lcd.GetYSize() - graph_height - GRAPH_PADDING, graph_width, graph_height);
        break;
    case Locked:
        lcd.SetBackColor(LCD_COLOR_WHITE);
        lcd.SetTextColor(LCD_COLOR_RED);
        snprintf(display_buf[0], 60, "LOCKED!");
        lcd.DrawRect(GRAPH_PADDING, lcd.GetYSize() - graph_height - GRAPH_PADDING, graph_width, graph_height);
        break;
    case EnterPassword:
        lcd.SetBackColor(LCD_COLOR_BLACK);
        lcd.SetTextColor(LCD_COLOR_CYAN);
        snprintf(display_buf[0], 60, "Enter Password...");
        lcd.DrawRect(GRAPH_PADDING, lcd.GetYSize() - graph_height - GRAPH_PADDING, graph_width, graph_height);
        break;
    case IncorrectPassword:
        lcd.SetBackColor(LCD_COLOR_BLACK);
        lcd.SetTextColor(LCD_COLOR_RED);
        snprintf(display_buf[0], 60, "Incorrect Password!");
        lcd.DrawRect(GRAPH_PADDING, lcd.GetYSize() - graph_height - GRAPH_PADDING, graph_width, graph_height);
        break;
    case CorrectPassword:
        lcd.SetBackColor(LCD_COLOR_BLACK);
        lcd.SetTextColor(LCD_COLOR_GREEN);
        snprintf(display_buf[0], 60, "Correct Password!");
        lcd.DrawRect(GRAPH_PADDING, lcd.GetYSize() - graph_height - GRAPH_PADDING, graph_width, graph_height);
        break;
    case NoKeyAllowed:
        lcd.SetBackColor(LCD_COLOR_BLACK);
        lcd.SetTextColor(LCD_COLOR_RED);
        snprintf(display_buf[0], 60, "No Key Allowed!");
        lcd.DrawRect(GRAPH_PADDING, lcd.GetYSize() - graph_height - GRAPH_PADDING, graph_width, graph_height);
        break;
    case KeyStored:
        lcd.SetBackColor(LCD_COLOR_BLACK);
        lcd.SetTextColor(LCD_COLOR_CYAN);
        snprintf(display_buf[0], 60, "New Key Stored!");
        lcd.DrawRect(GRAPH_PADDING, lcd.GetYSize() - graph_height - GRAPH_PADDING, graph_width, graph_height);
        break;
    }

    // Display the string at the buffer at line 10, center aligned
    lcd.DisplayStringAt(0, LINE(10), (uint8_t *)display_buf[0], CENTER_MODE);
}

//--------------------------------------------Main Function--------------------------------------------

int main()
{

    //initializes the button by attaching the events
    button.rise(&raiseEvent);
    button.fall(&fallEvent);

    //initializes the gyro
    gyro.init();

    //initializes the status of the device
    //a key needs to be recorded as part of the initialization
    status = unlocked;
    updateLCD(EnterKey);
    recordGyro(key, &keySize, false);
    //displays the key stored message
    updateLCD(KeyStored);
    thread_sleep_for(1500);
    //displays the locked message
    updateLCD(Locked);
    //sets the status to locked
    status = locked;

    while(1){

        if((buttonStatus == longPress) && (status == locked)){
            //display error: cant record new key while locked
            updateLCD(NoKeyAllowed);
            thread_sleep_for(1500);
            updateLCD(Locked);
        }
        else if((buttonStatus == longPress) && (status == unlocked)){
            // record new key
            updateLCD(EnterKey);
            recordGyro(key, &keySize, true);
            updateLCD(KeyStored);
            thread_sleep_for(1500);
            updateLCD(Locked);
            status = locked;
        }
        else if((buttonStatus == shortPress) && (status == locked)){
            // enter password
            updateLCD(EnterPassword);
            recordGyro(recordings, &recordingSize, true);

            if(checkPassword()){
                updateLCD(CorrectPassword);
                thread_sleep_for(1500);
                updateLCD(Unlocked);
                status = unlocked;
            }
            else{
                updateLCD(IncorrectPassword);
                thread_sleep_for(1500);
                updateLCD(Locked);
                status = locked;
            }
        }
        else if((buttonStatus == shortPress) && (status == unlocked)){
            // lock the device
            status = locked;
            updateLCD(Locked);
        }

        if(buttonStatus != notPress){
            buttonStatus = notPress;
        }

        thread_sleep_for(1);

       
}

    return 0;
}
