#include "gyro.h"

// Gyro constructor, sets up SPI and chip select
Gyro::Gyro::Gyro(PinName mosi, PinName miso, PinName sclk, PinName chip_select):
    spi(mosi, miso, sclk), cs(chip_select)
    {
        spi.lock();
        spi.frequency(1000000); 
        spi.format(8,0); 
        xPosition = 0;
        yPosition = 0;
        zPosition = 0;
        xChange = 1;
        yChange = 1;
        zChange = 1;
        cs = 1;
    }

// Gyro destructor
Gyro::Gyro::~Gyro(){
    //nothing to do here
}

// Gyro public methods
void Gyro::Gyro::init(){
    /*
    Gyro initialization method used to set up the gyro.
    It writes to control register 1 and 2 to do the following:
    -sets the output data rate to 800Hz
    -sets the bandwidth to 110Hz
    -sets the power mode to normal
    -sets the axes to be enabled
    -sets the high pass filter to be enabled
    -sets the high pass filter cutoff frequency to 16Hz

    Function also checks that the registers were written to correctly.

    Parameters:
        None
    Returns:
        None
    */

    printf("Initializing gyro...\n");

    //write to ctrl_reg1 to enable all axes
    cs = 0;
    spi.write(write_no_incr | ctrl_reg1);
    spi.write(setup_hex);
    cs = 1;

    //check that ctrl_reg1 was written to correctly
    cs = 0;
    spi.write(read_no_incr | ctrl_reg1);
    char reg1 = spi.write(0x00);
    cs = 1;
    if(reg1 != setup_hex){
        printf("Error: ctrl_reg1 was not written correctly\n");
    }

    //write to ctrl_reg2 to enable high pass filter
    cs = 0;
    spi.write(write_no_incr | ctrl_reg2);
    spi.write(high_pass_filter);
    cs = 1;

    //check that ctrl_reg2 was written to correctly
    cs = 0;
    spi.write(read_no_incr | ctrl_reg2);
    char reg2 = spi.write(0x00);
    cs = 1;
    if(reg2 != high_pass_filter){
        printf("Error: ctrl_reg2 was not written correctly\n");
    }

    //check that the gyro is working
    if(Gyro::check_gyro()){
        printf("Gyro is initialized\n");
    }
    else{
        printf("Error: Gyro not responding\n");
    }
}

void Gyro::Gyro::readXYZ(){
    /*
    Method used to read the x, y, and z values from the gyro.
    It first checks the status register to see if new data is available.
    If new data is available, it reads the x, y, and z values from the
    appropriate registers.
    If not, it returns.

    It writes using incrementing address mode to read the x, y, and z values.

    Parameters:
        None
    Returns:
        None
    */

    //check if new data is available
    cs = 0;
    spi.write(read_no_incr | status_reg);
    char status = spi.write(0x00);
    cs = 1;
    if((status & new_data) == 0){
        return;
    }

    //read x y z using incrementing address mode
    cs = 0;
    spi.write(read_w_incr | out_x_h);
    x = spi.write(0x00) << 8;
    x |= spi.write(0x00);

    y = spi.write(0x00) << 8;
    y |= spi.write(0x00);

    z = spi.write(0x00) << 8;
    z |= spi.write(0x00);
    cs = 1;
}

bool Gyro::Gyro::check_gyro(){
    /*
    Method used to check that the gyro is working properly. It reads the
    who_am_i register and checks that it is equal to the id.

    Parameters:
        None
    Returns:
        True if the gyro is working properly, false otherwise
    */
    cs = 0;
    spi.write(read_no_incr | who_am_i);
    char who  = spi.write(0x00);
    cs = 1;
    return who == id;
}

void Gyro::Gyro::updatePosition(){
    /*
    Method used to update the x, y, and z positions of the gyro.
    It calls the getPosition method for each axis.

    Parameters:
        None
    Returns:
        None
    */
    readXYZ();
    getPosition(&x, &xPosition, &xChange);
    getPosition(&y, &yPosition, &yChange);
    getPosition(&z, &zPosition, &zChange);
}

void Gyro::Gyro::getPosition(const short int *data, int *currentPosition, bool *change){
    /*
    Method used to update the position of a single axis of the gyro.
    It takes in the current data, the current position, and a boolean
    indicating whether the position can be changed.

    If the data is greater than the change trigger and the position can be
    changed, the position is updated.

    If the data is less than the negative change trigger and the position can
    be changed, the position is updated.

    If the data is between the change trigger and the negative change trigger,
    the position can be changed once again.

    Parameters:
        data: the current data from the gyro
        currentPosition: the current position of the gyro
        change: a boolean indicating whether the position can be changed
    Returns:
        None
    */

    if((*data > change_trigger) & *change){
        //update position
        if(*currentPosition == 0){
            *currentPosition = 1;
        }
        else if(*currentPosition == 2){
            *currentPosition = 0;
        }
        *change = 0; //set change to 0 so position can't be changed again
    }
    else if((*data < -change_trigger) & *change){
        if(*currentPosition == 0){
            *currentPosition = 2;
        }
        else if(*currentPosition == 1){
            *currentPosition = 0;
        }
        *change = 0; //set change to 0 so position can't be changed again
    }
    else if((*data < change_trigger) & (*data > -change_trigger)){
        *change = 1; //set change to 1 so position can be changed again
    }
}