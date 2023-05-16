#include <mbed.h>
#include "gyro.h"

#define mosi PF_9
#define miso PF_8
#define sclk PF_7
#define chip_select PC_1

#define maxRecording 5
#define longPressTime 3000000 //3 seconds in microseconds

char xPosition, yPosition, zPosition = 0;

InterruptIn button(USER_BUTTON);

Gyro::Gyro gyro(mosi, miso, sclk, chip_select);

Timer timer;

// custom types
enum ButtonPress{notPress, shortPress, longPress};
enum State{locked, unlocked};


ButtonPress buttonStatus = notPress;
State status = unlocked;

bool passwordCorrect = false;

bool buttonPressed = false;

char recordings[maxRecording][3];
char key[maxRecording][3];
int recordingSize = 0;
int keySize = 0;

int a = 1; // for debugging

void showPosition(Gyro::Gyro *gyro){
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

void recordGyro(char data[][3], int *size){
    *size = 0;
    xPosition = 0;
    yPosition = 0;
    zPosition = 0;
    buttonStatus = notPress;
    while(*size < maxRecording){
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
            break;
        }
    }
}

ButtonPress lastButtonStatus = notPress;
State lastStatus = unlocked;

int main()
{

    button.rise(&raiseEvent);
    button.fall(&fallEvent);

    gyro.init();

    //get a recording for the key

    printf("Thing initialized, current status: unlocked\n");

    while(1){

        if((buttonStatus == longPress) && (status == locked)){
            //display error: cant record new key while locked
            printf("Cant record new key while locked\n");
        }
        else if((buttonStatus == longPress) && (status == unlocked)){
            // record new key
            printf("Recording new key\n");
            // maybe lock after recording
            recordGyro(key, &keySize);
            printArray(key, keySize);
            printf("Locked\n");
            status = locked;
        }
        else if((buttonStatus == shortPress) && (status == locked)){
            // enter password
            printf("Enter password\n");
            recordGyro(recordings, &recordingSize);
            printArray(recordings, recordingSize);

            if(checkPassword()){
                printf("Correct password, Unlocked\n");
                status = unlocked;
            }
            else{
                printf("Incorrect password, still Locked\n");
                status = locked;
            }
        }
        else if((buttonStatus == shortPress) && (status == unlocked)){
            // lock the device
            status = locked;
            printf("Locked\n");
        }

        if(buttonStatus != notPress){
            buttonStatus = notPress;
        }

        thread_sleep_for(1);

       
}

    return 0;
}




/*


while(1){
    check if timer is stopped == true{
        if(timer > RecordTimeTrigger && state == unlocked){
            can record new key
            reset timer
        }
        else if(timer > 0 && timer < RecordTimeTriger && state == locked){
            enter password(recording)
            check password(key, recording){
                if(password == true){
                    state = unlocked
                }
                else{
                    display error
                    state = locked
                }
            }
        }
        else if(timer > 0 && state == unlocked){
            state = locked
        }
    }
}

*/