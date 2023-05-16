#include <mbed.h>
#include "gyro.h"
#include "drivers/LCD_DISCO_F429ZI.h"

//lcd constants
#define BACKGROUND 1
#define FOREGROUND 0
#define GRAPH_PADDING 5

#define mosi PF_9
#define miso PF_8
#define sclk PF_7
#define chip_select PC_1

#define maxRecording 5
#define longPressTime 3000000 //3 seconds in microseconds

#define timeoutTime 15000000 //15 seconds in microseconds

char xPosition, yPosition, zPosition = 0;

LCD_DISCO_F429ZI lcd;

InterruptIn button(USER_BUTTON);

Gyro::Gyro gyro(mosi, miso, sclk, chip_select);

Timer timer;
Timer timeoutTimer;

// custom types
enum ButtonPress{notPress, shortPress, longPress};
enum State{locked, unlocked};
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

ButtonPress buttonStatus = notPress;
State status = unlocked;

uint32_t graph_width = lcd.GetXSize() - 2 * GRAPH_PADDING;
uint32_t graph_height = lcd.GetYSize() - 2 * GRAPH_PADDING;

char display_buf[2][60];

bool passwordCorrect = false;

bool buttonPressed = false;

char recordings[maxRecording][3];
char key[maxRecording][3];
int recordingSize = 0;
int keySize = 0;


void showPosition(Gyro::Gyro *gyro){
    /*
    This function does the following:
    -reads the gyro's x, y, and z values
    -prints them to the terminal if they have changed
    -prints a line of dashes to the terminal if they have changed
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
    gyro->readXYZ();
    printf("x: %d y: %d z: %d\n", gyro->getX(), gyro->getY(), gyro->getZ());
}

bool checkPassword(void){
    //check recording against key
    if(recordingSize != keySize){return false;}
    for (int i = 0; i < recordingSize; i++){
        if(recordings[i][0] != key[i][0] || 
            recordings[i][1] != key[i][1] || 
            recordings[i][2] != key[i][2])
        {
            return false;
        }
    }
    return true;
}

void printArray(char array[][3], int size){
    printf("[");
    for (int i = 0; i < size; i++)
    {
            printf("[%d %d %d] , ", array[i][0], array[i][1], array[i][2]);
    }
    printf("]\n");
}


void raiseEvent(){
    timer.start();
}


void fallEvent(){
    timer.stop();
    if(timer.elapsed_time().count() > longPressTime){
        buttonStatus = longPress;
    }
    else{
        buttonStatus = shortPress;
    }
    timer.reset();
}

void recordGyro(char (*data)[3], int *size, bool timeoutAct){
    *size = 0;
    xPosition = 0;
    yPosition = 0;
    zPosition = 0;
    buttonStatus = notPress;
    gyro.center();
    timeoutTimer.reset();
    timeoutTimer.start();
    while (*size < maxRecording)
    {
        gyro.updatePosition();
        if( (gyro.getYPosition() != yPosition) || 
                (gyro.getXPosition() != xPosition) || 
                (gyro.getZPosition() != zPosition))
            {
                yPosition = gyro.getYPosition();
                xPosition = gyro.getXPosition();
                zPosition = gyro.getZPosition();
                data[*size][0] = xPosition;
                data[*size][1] = yPosition;
                data[*size][2] = zPosition;
                (*size)++;
            }
        
        if((buttonStatus == longPress) || (buttonStatus == shortPress)){
            buttonStatus = notPress;
            timeoutTimer.stop();
            break;
        }
        if((timeoutTimer.elapsed_time().count() > timeoutTime) && timeoutAct){
            timeoutTimer.stop();
            break;
        }
    }
    timeoutTimer.stop();
}

void updateLCD(LCDState lcdState){
      // Select the foreground layer of the LCD
    lcd.SelectLayer(FOREGROUND);

    // Clear the screen
    lcd.Clear(LCD_COLOR_BLACK);

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

  lcd.DisplayStringAt(0, LINE(10), (uint8_t *)display_buf[0], CENTER_MODE);
}

int main()
{

    button.rise(&raiseEvent);
    button.fall(&fallEvent);

    gyro.init();

    //get a recording for the key

    status = unlocked;
    updateLCD(EnterKey);
    recordGyro(key, &keySize, false);
    updateLCD(KeyStored);
    thread_sleep_for(1500);
    updateLCD(Locked);
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
            // maybe lock after recording
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
            printArray(recordings, recordingSize);

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