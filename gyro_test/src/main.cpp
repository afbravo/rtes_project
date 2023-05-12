#include <mbed.h>
#include "gyro.h"

#define mosi PF_9
#define miso PF_8
#define sclk PF_7
#define chip_select PC_1

char xPosition, yPosition, zPosition = 0;

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
 
int main(){

    Gyro::Gyro gyro(mosi, miso, sclk, chip_select);

    gyro.init();
    
    while(1){
        showPosition(&gyro);
    }

    return 0;
}