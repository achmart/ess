/**
 * g_chipid = {0xc1, 0x0, 0x4, 0xd, 0x4, 0x20, 0x4, 0x59, 0x82, 0x6, 0x0, 0x10, 0xa, 0x0, 0x0, 0x0}
 * &g_chipid = 0x200ffc4
 * 
 * 
 * 
 * 
 * */
#include "VirtualSerial.h"
#include "base64.h"
#include <sodium.h>
#include "randombytes_dilbert.h"
#include "testkey.h"

#define MAX_MESSAGE_SIZE 43800
#define HEADER_SIZE 38

/* Clock configuration */
XMC_SCU_CLOCK_CONFIG_t clock_config =
{
    .syspll_config.p_div = 2,
    .syspll_config.n_div = 80,
    .syspll_config.k_div = 4,
    .syspll_config.mode = XMC_SCU_CLOCK_SYSPLL_MODE_NORMAL,
    .syspll_config.clksrc = XMC_SCU_CLOCK_SYSPLLCLKSRC_OSCHP,
    .enable_oschp = true,
    .calibration_mode = XMC_SCU_CLOCK_FOFI_CALIBRATION_MODE_FACTORY,
    .fsys_clksrc = XMC_SCU_CLOCK_SYSCLKSRC_PLL,
    .fsys_clkdiv = 1,
    .fcpu_clkdiv = 1,
    .fccu_clkdiv = 1,
    .fperipheral_clkdiv = 1
};

void SystemCoreClockSetup(void)
{
    /* Setup settings for USB clock */
    XMC_SCU_CLOCK_Init(&clock_config);

    XMC_SCU_CLOCK_EnableUsbPll();
    XMC_SCU_CLOCK_StartUsbPll(2, 64);
    XMC_SCU_CLOCK_SetUsbClockDivider(4);
    XMC_SCU_CLOCK_SetUsbClockSource(XMC_SCU_CLOCK_USBCLKSRC_USBPLL);
    XMC_SCU_CLOCK_EnableClock(XMC_SCU_CLOCK_USB);

    SystemCoreClockUpdate();
}

int main(void){
    
    uint16_t total_received_bytes = 0;
    uint16_t received_bytes = 0;
    uint16_t length_decoded_text = 0;
    uint16_t length_encoded_text = 0;
    unsigned char received_char = 0;
    // Max Size of received Data:
    // 3 Control + 36 Header + 43692 Text = 43731
    char *buffer = (char *)calloc(MAX_MESSAGE_SIZE + crypto_secretbox_MACBYTES, sizeof(char));
    enum myread_state {IDLE, READ_START, READ_HEADER, READ_TEXT};
    enum myread_state read_state = READ_START;

    USB_Init();
    // Replace RNG
    randombytes_set_implementation(&randombytes_dilbert_implementation);
    if(sodium_init() == -1)
        return -1;

    while (1) {
        
        // Read Bytes from Serial as long as possible, normally you get only one
        received_bytes = CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface);
        for (uint16_t i = 0; i < received_bytes; i++) {
            received_char = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
            switch (read_state) {
                // Read until first 0x01 arrives
                case READ_START:
                    if(received_char == 0x01) {
                        buffer[total_received_bytes] = received_char;
                        total_received_bytes++;
                        read_state = READ_HEADER;
                    }
                    break;
                // Read header until 0x02 arrives
                case READ_HEADER:
                    if((received_char >= 'A' && received_char <= 'Z') || (received_char >= 'a' && received_char <= 'z')
                        || (received_char >= '0' && received_char <= '9') || received_char == '-' || received_char == '_'
                        || received_char == '=') {
                            buffer[total_received_bytes] = received_char;
                            total_received_bytes++;
                        } else if(received_char == 0x02) {
                            buffer[total_received_bytes] = received_char;
                            total_received_bytes++;
                            read_state = READ_TEXT;
                        } else if(received_char == 0x01) {
                            total_received_bytes = 0;
                            buffer[total_received_bytes] = received_char;
                            total_received_bytes++;
                        } else {
                            // TODO only until 36 bytes arrived
                            read_state = READ_START;
                            total_received_bytes = 0;
                        }
                        break;
                // Read text until 0x03 arrives
                // TODO check for buffer overflow
                case READ_TEXT:
                    if((received_char >= 'A' && received_char <= 'Z') || (received_char >= 'a' && received_char <= 'z')
                        || (received_char >= '0' && received_char <= '9') || received_char == '-' || received_char == '_'
                        || received_char == '=') {
                            buffer[total_received_bytes] = received_char;
                            total_received_bytes++;
                        } else if(received_char == 0x03) {
                            buffer[total_received_bytes] = received_char;
                            read_state = IDLE;
                        } else if(received_char == 0x01) {
                            // Discard old message and start with new message
                            total_received_bytes = 0;
                            buffer[total_received_bytes] = received_char;
                            total_received_bytes++;
                            read_state= READ_HEADER;
                        } else {
                            read_state = READ_START;
                            total_received_bytes = 0;
                        }
                        break;
                case IDLE:
                        break;
            }
        }

        // Complete Message arrived
        if(read_state == IDLE) {
            
            // Decoded length of plaintext = 24 bits = 3 Bytes
            // reserve extra byte as zero to stop decoding routine
            char *encoded_length = (char *)calloc(5, sizeof(char));
            memcpy(encoded_length, &buffer[1], 4);
            char* decoded_length = (char *)calloc(3, sizeof(char));
            Base64decode(decoded_length, encoded_length);

            // Only proceed if transmitted length is right
            length_encoded_text = *((uint16_t *)decoded_length);
            //length_decoded_text = 3*length_encoded_text/4;
            if((total_received_bytes - HEADER_SIZE) == length_encoded_text) {

                // Decode nonce = 192 bits = 24 Byte
                // reserve extra byte as zero to stop decoding routine
                char *encoded_nonce = (char *)calloc(33, sizeof(char));
                memcpy(encoded_nonce, &buffer[5], 32);
                char *decoded_nonce = (char *)calloc(24, sizeof(char));
                Base64decode(decoded_nonce, encoded_nonce);

                // Decode Text in place
                // Stops when it reaches byte 0x03
                length_decoded_text = Base64decode(&buffer[HEADER_SIZE], &buffer[HEADER_SIZE]);

                // Set all remaining bytes to zero
                for (uint16_t i = HEADER_SIZE + length_decoded_text; i < MAX_MESSAGE_SIZE + crypto_secretbox_MACBYTES; i++ ) {
                    buffer[i] = 0;
                }
                // Encrpyt Text
                // Create key
                
                unsigned char *key = (unsigned char*)calloc(32, sizeof(char));
                memcpy(key, g_chipid, 16);
                memcpy(key+16, g_chipid, 16);
                
                
                uint16_t length_decoded_cipher = length_decoded_text + crypto_secretbox_MACBYTES;

                //unsigned char *encrypted = (unsigned char*)calloc(length_decoded_cipher, sizeof(char));

                crypto_secretbox_easy((unsigned char*)&buffer[HEADER_SIZE], (unsigned char*)&buffer[HEADER_SIZE],
                                        length_decoded_text, (unsigned char*)decoded_nonce, key);
            /*    
                // Encode cipher
                uint16_t length_encoded_cipher = Base64encode_len(length_decoded_cipher);
                char *encoded_cipher = (char *)calloc(length_encoded_cipher, sizeof(char));
                Base64encode(encoded_cipher, (const char*)&buffer[HEADER_SIZE], length_decoded_cipher);
                // Strip last zeros
                while (encoded_cipher[length_encoded_cipher-1] == 0) {
                    length_encoded_cipher--;
                }
            */
            
                uint16_t length_encoded_cipher = Base64encode_len(length_decoded_cipher);
                // Move cipher in bytes of 3 to make room for encoding                
                for (uint16_t i = (length_decoded_cipher-1)/3; i > 0; i--) {
                    memmove(&buffer[HEADER_SIZE + 4*i], &buffer[HEADER_SIZE + 3*i], 3);
                    buffer[HEADER_SIZE + 4*i +3] = 0;
                }
                // Encode every 3 Bit at a time
                // Last Byte has to be treated differently
                char temp[3] = {0};
                char last[3] = {0};
                int16_t i = (length_decoded_cipher-1)/3;
                int16_t lastindex = i;
                memcpy(last, &buffer[HEADER_SIZE + 4*i], 3);                                
                for (; i >= 0; i--) {
                    memcpy(temp, &buffer[HEADER_SIZE + 4*i], 3);
                    Base64encode(&buffer[HEADER_SIZE + 4*i], temp, 3);
                }
                if (last[1]== 0)
                    buffer[HEADER_SIZE + 4*lastindex +2] = '=';
                if (last[2]== 0)
                    buffer[HEADER_SIZE + 4*lastindex +3] = '=';
                
            
                while (buffer[HEADER_SIZE + length_encoded_cipher-1] == 0) {
                    length_encoded_cipher--;
                }
            
                // Decode every      
                // Write length of encoded ciphertext into Header
                *((uint16_t *)decoded_length) = length_encoded_cipher;

                // Encode header
                Base64encode(encoded_length, decoded_length, 3);

                // Make Protocol
                buffer[0] = 0x01;
                memcpy(&buffer[1], encoded_length, 4);
                buffer[5] = 0x02;
                memmove(&buffer[6], &buffer[HEADER_SIZE], length_encoded_cipher);
                buffer[length_encoded_cipher + 6] = 0x03;
            
                // Send Protocol
                /*
                if(length_encoded_cipher + 7 > 32768) {
                    // send first packet
                    CDC_Device_SendData(&VirtualSerial_CDC_Interface, buffer, 32761);
                    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
                    length_encoded_cipher -= 32761;
                    // Send rest
                    CDC_Device_SendData(&VirtualSerial_CDC_Interface, &buffer[32761], length_encoded_cipher+7);
                } else {
                    
                }*/
                CDC_Device_SendData(&VirtualSerial_CDC_Interface, buffer, length_encoded_cipher+7);
                //CDC_Device_SendData(&VirtualSerial_CDC_Interface, &buffer[HEADER_SIZE], length_encoded_text);

                
                
                //free(encrypted);
                
                //free(encoded_cipher);
                free(key);
                free(decoded_nonce);
                free(encoded_nonce);
            }
            free(decoded_length);
            free(encoded_length);
            
           
            
            read_state = READ_START;
            total_received_bytes = 0;

        }  

        CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
        
    }
    
}