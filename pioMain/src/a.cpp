#include <mbed.h>


/*
ToDO

poll data from gyro using SPI

CS -PC1

MOSI -PF9
MISO - PF8
SCK - PF7

*/

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

char read_ni = 0x80; //10000000
char read_wi = 0xc0; // 11000000
char write_ni = 0x00; //00000000
char write_wi = 0x40; //01000000


SPI spi(PF_9, PF_8, PF_7); // mosi, miso, sclk
DigitalOut cs(PC_1);

short int x, y, z;

void setup_gyro(){
  cs = 0;
  /*write to CTRL_REG1
      data output rate 800Hz 11xx xxxx
      bandwidth 110Hz xx11 xxxx
      normal mode xxxx 1xxx
      x y z enabled xxxx x111
    data to write 1111 1111 = 0xFF
  */
  spi.write(write_ni | ctrl_reg1);
  spi.write(0xFF);

  cs = 1;
  thread_sleep_for(1);
  cs = 0;
  // check if write was successful
  spi.write(read_ni | ctrl_reg1);
  char reg1 = spi.write(0x00);
  if(reg1 != 0xFF){
    printf("write to CTRL_REG1 failed\n");
  }
  printf("reg1: %d\n", reg1);
  cs = 1;

  //high pass filter ctrl reg 2
  /*
  normal mode 0000 xxxx
  cut off freq at 200 of 1 xxxx 0100
  data to write 0000 0100 = 0x04
  */
  cs = 0;
  spi.write(write_ni | ctrl_reg2);
  spi.write(0x04);
  cs = 1;
}

void read_gyro(){
  cs = 0;

  //check if new data is available
  spi.write(read_ni | status_reg);
  char status = spi.write(0x00);
  cs = 1;
  if((status & 0x08) == 0){
    return;
  }


  cs = 0;

  //read x high
  spi.write(read_wi | out_x_h);
  x = spi.write(0x00) << 8;
  //read x low
  x |= spi.write(0x00);

  //read y high
  y = spi.write(0x00) << 8;
  //read y low
  y |= spi.write(0x00);

  //read z high
  z = spi.write(0x00) << 8;
  //read z low
  z |= spi.write(0x00);

  cs = 1;

  printf("x: %d y: %d z: %d\n", x, y, z);

}

char check_id(){
  cs = 0;
  spi.write(read_ni | who_am_i);
  char who  = spi.write(0x00);
  cs = 1;
  return who;
}

//q: 11010011 in decimal: 211

// int main() {

//   // put your setup code here, to run once:
//   cs = 1;

//   // setup spi
//   spi.lock();
//   spi.format(8,0); //8 bits per transfer, mode 0
//   spi.frequency(1000000); //1MHz

//   setup_gyro();

//   //check if chip is responding
//   char who = check_id();
//   printf("id: %d\n", who);
//   if(who != 211){
//     printf("gyro not responding\n");
//   }

//   while(1) {
//     read_gyro();
//   }
// }