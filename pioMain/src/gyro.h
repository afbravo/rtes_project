/*
Gyro Class

This class is used to interface with the I3G4250D Gyroscope. It uses the SPI
interface to communicate with the device. The class is used to read the x, y, z
angular acceleration values from the device. It also keeps track of the current
position of the device. The position is updated when the angular acceleration
values change by a certain amount.

*/

// safeguards
#ifndef gyro_h
#define gyro_h

#include <mbed.h>

// register addresses
#define who_am_i 0x0F
#define ctrl_reg1 0x20
#define ctrl_reg2 0x21
#define ctrl_reg3 0x22
#define ctrl_reg4 0x23
#define ctrl_reg5 0x24

#define status_reg 0x27
#define out_x_l 0x28
#define out_x_h 0x29
#define out_y_l 0x2A
#define out_y_h 0x2B
#define out_z_l 0x2C
#define out_z_h 0x2D

// read and write commands
#define read_no_incr 0x80 // write with address not incremented
#define read_w_incr 0xc0 // write with address incremented
#define write_no_incr 0x00 // read with address not incremented
#define write_w_incr 0x40 // read with address incremented

// data to write to registers
#define setup_hex 0xFF
#define high_pass_filter 0x04
#define new_data 0x08
#define id 211

// position values
#define change_trigger 20000
#define reset_trigger 5000


namespace Gyro{

    class Gyro{
        public:
            // constructor and destructor
            Gyro(PinName mosi, PinName miso, PinName sclk, PinName chip_select);
            ~Gyro();
            // public methods
            void init();
            void readXYZ();
            void updatePosition();
            void center(){yPosition = 0; xPosition = 0; zPosition = 0;}
            short int getX(){return x;}
            short int getY(){return y;}
            short int getZ(){return z;}
            char getYPosition(){return yPosition;}
            char getXPosition(){return xPosition;}
            char getZPosition(){return zPosition;}
            bool check_gyro();
        private:
            SPI spi;
            DigitalOut cs;
            short int x;
            short int y;
            short int z;
            int yPosition;
            int xPosition;
            int zPosition;
            bool yChange;
            bool xChange;
            bool zChange;
            void getPosition(const short int *data, int *currentPosition, bool *change);
    };
}

#endif
