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

#define DURATION_DOT 1
#define DURATION_DASH 3
#define DURATION_IN_CHAR 1
#define DURATION_BETWEEN_CHAR 3
#define DURATION_BETWEEN_WORDS 7
#define DURATION_BETWEEN_TRANSMISSION 50

/**
 * Global Data
 * */
int global_morsecounter = 0;
int length_counter = 0;
int length_signal = 0;
bool in_char = false;
const char output[20] = "I CAN MORSE";
const char LUT_alphabet[26][5] =\
{".-", "-...", "-.-.", "-..", ".", "..-.",\
"--.", "....", "..", ".---", "-.-", ".-..",\
"--", "-.", "---", ".--.", "--.-", ".-.", "...",\
"-", "..-", "...-", ".--", "-..-", "-.--", "--.."};
const char LUT_numbers[10][6] =\
{"-----", ".----", "..---", "...--", "....-",\
".....", "-....", "--...", "---..", "----."};
char morsebuffer[201];

/**
 * Functions
 * */

/**
 * Fill morsebuffer with morsecode generated from output and LUT
 * '_' is used to mark 3 dots pause between characters
 * ' ' is used to mark 7 dots pause between words
 * */
void fill_morsebuffer(void){
  int ascii_index = 0;
  int counter = 0;
  // Look at whole output string
  for(int i = 0; output[i] != '\0'; i++){
      // Look up morse code
      if(output[i] <= '9' && output[i] >= '0'){
        ascii_index = output[i] - '0';
        for(int j = 0; LUT_numbers[ascii_index][j] != '\0'; j++){
          morsebuffer[counter] = LUT_numbers[ascii_index][j];
          counter++;
        }
      }
      else if(output[i] >= 'A' && output[i] <= 'Z'){
        ascii_index = output[i] - 'A';
        for(int j = 0; LUT_alphabet[ascii_index][j] != '\0'; j++){
          morsebuffer[counter] = LUT_alphabet[ascii_index][j];
          counter++;
        }
      }
      else if(output[i] >= 'a' && output[i] <= 'z'){
        ascii_index = output[i] - 'a';
        for(int j = 0; LUT_alphabet[ascii_index][j] != '\0'; j++){
          morsebuffer[counter] = LUT_alphabet[ascii_index][j];
          counter++;
        }
      }
      // Look for space between words
      if(output[i+1] == ' '){
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

/**
 * Functions
 */
// Handler for SysTick Event
void SysTick_Handler(void) {
  if(length_counter == length_signal){    
    length_counter = 0;
    if(in_char == true){
      in_char = false;
      length_signal = DURATION_IN_CHAR;
      XMC_GPIO_SetOutputLow(XMC_GPIO_PORT1, 1);
    }
    else if(morsebuffer[global_morsecounter] == '\0'){
      global_morsecounter = 0;
      length_signal = DURATION_BETWEEN_TRANSMISSION;
      }
    else {
      switch(morsebuffer[global_morsecounter]){
        case '.':
          if(morsebuffer[global_morsecounter + 1] == '.' || morsebuffer[global_morsecounter + 1] == '-'){
            in_char = true;
          }
          length_signal = DURATION_DOT;
          XMC_GPIO_SetOutputHigh(XMC_GPIO_PORT1, 1); break;
        case '-':
          if(morsebuffer[global_morsecounter + 1] == '.' || morsebuffer[global_morsecounter + 1] == '-'){
            in_char = true;
          }
          length_signal = DURATION_DASH;
          XMC_GPIO_SetOutputHigh(XMC_GPIO_PORT1, 1); break;
        case '_':
          length_signal = DURATION_BETWEEN_CHAR;
          XMC_GPIO_SetOutputLow(XMC_GPIO_PORT1, 1); break;
        case ' ':
          length_signal = DURATION_BETWEEN_WORDS;
          XMC_GPIO_SetOutputLow(XMC_GPIO_PORT1, 1); break;
      }
      global_morsecounter++;
    }
  }
  length_counter++; 
}

int main(void) {
    // Core Clock 120 MHz --> reload with 12e6 --> 100ms Period
    SysTick_Config(SystemCoreClock/10);
    // LED Config from example_project
    const XMC_GPIO_CONFIG_t LED_config = \
          {.mode=XMC_GPIO_MODE_OUTPUT_PUSH_PULL,\
          .output_level=XMC_GPIO_OUTPUT_LEVEL_LOW,\
          .output_strength=XMC_GPIO_OUTPUT_STRENGTH_STRONG_SHARP_EDGE};

    XMC_GPIO_Init(XMC_GPIO_PORT1, 1, &LED_config);
  fill_morsebuffer();
  while(1){
    
  }
  return 0;
}