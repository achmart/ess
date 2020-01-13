/**
 * Author: Martin Achtner
 * Created: 03.11.2017
 * Deliverable 1 for lecture Embedded Systems and Security
 * 
 * output_buffer "I CAN MORSE" via LED 1
 */

 /**
  * Headers
  */
#include <xmc_gpio.h>
#include <xmc_eru.h>
#include <string.h>

#define LED1 P1_1
#define BUTTON1 P1_14
#define BUTTON2 P1_15

#define DURATION_DOT 100
#define DURATION_DASH 300
#define DURATION_IN_CHAR 100
#define DURATION_BETWEEN_CHAR 300
#define DURATION_BETWEEN_WORDS 700

/**
 * Global Data
 * */
bool button1_pressed = false;
bool button2_pressed = false;
bool in_char = false;

int button1_pressed_counter = 0;
int counter_morse_transmission_position = 0;
int actual_length_counter = 0;
int nominal_length_signal = 0;
int time_between_buttons = 0;
int output_time = 0;

const char morsephrase[20] = "I CAN MORSE";
char output_buffer[20];
char morsebuffer[201];
const char LUT_alphabet[26][5] =\
{".-", "-...", "-.-.", "-..", ".", "..-.",\
"--.", "....", "..", ".---", "-.-", ".-..",\
"--", "-.", "---", ".--.", "--.-", ".-.", "...",\
"-", "..-", "...-", ".--", "-..-", "-.--", "--.."};
const char LUT_numbers[10][6] =\
{"-----", ".----", "..---", "...--", "....-",\
".....", "-....", "--...", "---..", "----."};

/**
 * Functions
 */

/**
 * Fill morsebuffer with morsecode generated from output_buffer and LUT
 * '_' is used to mark 3 dots pause between characters
 * ' ' is used to mark 7 dots pause between words
 * */
void fill_morsebuffer(void){
  int ascii_index = 0;
  int counter = 0;
  // Look at whole output_buffer string
  for(int i = 0; output_buffer[i] != '\0'; i++){
      // Look up morse code
      if(output_buffer[i] <= '9' && output_buffer[i] >= '0'){
        ascii_index = output_buffer[i] - '0';
        for(int j = 0; LUT_numbers[ascii_index][j] != '\0'; j++){
          morsebuffer[counter] = LUT_numbers[ascii_index][j];
          counter++;
        }
      }
      else if(output_buffer[i] >= 'A' && output_buffer[i] <= 'Z'){
        ascii_index = output_buffer[i] - 'A';
        for(int j = 0; LUT_alphabet[ascii_index][j] != '\0'; j++){
          morsebuffer[counter] = LUT_alphabet[ascii_index][j];
          counter++;
        }
      }
      else if(output_buffer[i] >= 'a' && output_buffer[i] <= 'z'){
        ascii_index = output_buffer[i] - 'a';
        for(int j = 0; LUT_alphabet[ascii_index][j] != '\0'; j++){
          morsebuffer[counter] = LUT_alphabet[ascii_index][j];
          counter++;
        }
      }
      // Look for space between words
      if(output_buffer[i+1] == ' '){
        morsebuffer[counter] = ' ';
        counter++;
        i++;
      }
      // Only space between two characters
      else {
        morsebuffer[counter] = '_';
        counter++;
      }
  }
  // Terminate morse string
  morsebuffer[counter] = '\0';
}

void morse(void) {
  while(1){
    if(actual_length_counter >= nominal_length_signal){    
      actual_length_counter = 0;
      // Check if in transmission of a character
      if(in_char == true){
        in_char = false;
        nominal_length_signal = DURATION_IN_CHAR;
        XMC_GPIO_SetOutputLow(XMC_GPIO_PORT1, 1);
      }
      // Check for end of morsephrase
      else if(morsebuffer[counter_morse_transmission_position] == '\0'){
        counter_morse_transmission_position = 0;
        button1_pressed = false;
        button2_pressed = false;
        break;
        }
        // Set Counting goal for new morse signal
      else {
        switch(morsebuffer[counter_morse_transmission_position]){
          case '.':
            if(morsebuffer[counter_morse_transmission_position + 1] == '.' || morsebuffer[counter_morse_transmission_position + 1] == '-'){
              in_char = true;
            }
            nominal_length_signal = DURATION_DOT;
            XMC_GPIO_SetOutputHigh(XMC_GPIO_PORT1, 1); break;
          case '-':
            if(morsebuffer[counter_morse_transmission_position + 1] == '.' || morsebuffer[counter_morse_transmission_position + 1] == '-'){
              in_char = true;
            }
            nominal_length_signal = DURATION_DASH;
            XMC_GPIO_SetOutputHigh(XMC_GPIO_PORT1, 1); break;
          case '_':
            nominal_length_signal = DURATION_BETWEEN_CHAR;
            XMC_GPIO_SetOutputLow(XMC_GPIO_PORT1, 1); break;
          case ' ':
            nominal_length_signal = DURATION_BETWEEN_WORDS;
            XMC_GPIO_SetOutputLow(XMC_GPIO_PORT1, 1); break;
        }
        counter_morse_transmission_position++;
      }
    }
  }
}

void output_button1(void){
  strncpy(output_buffer, morsephrase, 19);
  fill_morsebuffer();
  morse();
}

void output_button2(void) {
    // Calculate digits for String transformation
    int digits = 0;
    int calc_digits = output_time;
    do {
      calc_digits /= 10;
      digits++;
    } while(calc_digits);
    // write digits to string
    int divider = 10;
    int local_output = output_time;
    output_buffer[digits] = '\0';
    for(int i = digits-1; i >= 0; i--){
      // Save last digit
      output_buffer[i] = local_output%divider + '0';
      // Throw away last digit
      local_output /= divider;
    }
  // Convert and Morse
  fill_morsebuffer();
  morse();
  
}

// Handler for SysTick Event
void SysTick_Handler(void) {
  time_between_buttons++;
  actual_length_counter++;
}

/**
 * MAIN APPLICATION
 * */
int main(void) {
  // Core Clock 120 MHz --> 1ms Period = CoreClock/1000
  SysTick_Config(SystemCoreClock/1000);

  XMC_GPIO_SetMode(LED1, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
  
  XMC_GPIO_SetMode(BUTTON1, XMC_GPIO_MODE_INPUT_PULL_UP);
  XMC_GPIO_SetMode(BUTTON2, XMC_GPIO_MODE_INPUT_PULL_UP);
  
  while(1){
    // Check for button press
    if(XMC_GPIO_GetInput(BUTTON1) == 0 && button1_pressed == false) {
      button1_pressed = true;
      output_time = time_between_buttons;
      time_between_buttons = 0;
    } else if(XMC_GPIO_GetInput(BUTTON2) == 0 && button2_pressed == false) {
      button2_pressed = true;
    }
    // Check for release of pressed button
    else if(XMC_GPIO_GetInput(BUTTON1) == 1 && button1_pressed == true) {
      button1_pressed_counter++;
      output_button1();
    } else if(XMC_GPIO_GetInput(BUTTON2) == 1 && button2_pressed == true) {
      output_button2();
    }
  }
  return 0;
}