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
  thread_sleep_for(1);
  /*write to CTRL_REG1
      data output rate 100Hz 00xx xxxx
      bandwidth 12.5Hz xx00 xxxx
      normal mode xxxx 1xxx
      x y z enabled xxxx x111
    data to write 0000 1111 = 0x0F
  */
  spi.write(write_ni | ctrl_reg1);
  spi.write(0x0F);
  thread_sleep_for(1);

  cs = 1;
}

void read_gyro(){
  cs = 0;
  thread_sleep_for(1);

  //check if new data is available
  spi.write(read_ni | status_reg);
  char status = spi.write(0x00);
  if((status & 0b00001000) == 0){
    printf("no new data\n");
    return;
  }

  //read x high
  spi.write(read_ni | out_x_h);
  x = spi.write(0x00) << 8;
  //read x low
  spi.write(read_ni | out_x_l);
  x |= spi.write(0x00);

  //read y high
  spi.write(read_ni | out_y_h);
  y = spi.write(0x00) << 8;
  //read y low
  spi.write(read_ni | out_y_l);
  y |= spi.write(0x00);

  //read z high
  spi.write(read_ni | out_z_h);
  z = spi.write(0x00) << 8;
  //read z low
  spi.write(read_ni | out_z_l);
  z |= spi.write(0x00);

  cs = 1;

  printf("x: %d y: %d z: %d\n", x, y, z);

}

//q: 11010011 in decimal: 211

int main() {

  // put your setup code here, to run once:
  cs = 1;

  // setup spi
  spi.format(8,0); //8 bits per transfer, mode 0
  spi.frequency(1000000); //1MHz

  setup_gyro();

  //check if chip is responding
  cs = 0;
  thread_sleep_for(1);
  spi.write(read_ni | who_am_i);
  char who = spi.write(0x00);
  cs = 1;

  if(who != 0xD3){
    printf("gyro not responding, who am i: %x\n", who);
    return 0;
  }

  while(1) {
    read_gyro();
  }
}