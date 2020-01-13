/**
 * Author: Martin Achtner
 * Created: 02.11.2017
 * Deliverable 1 for lecture Embedded Systems and Security
 * 
 * Output "I CAN MORSE" via LED 1
 */

 /**
  * Headers
  */
#include <xmc_gpio.h>
#include <xmc_common.h>



int main(void) {
  PORT1->OUT = 0xFFFF;
  PORT1->IOCR0 = 0b1000000010000000;
  return 0;
}