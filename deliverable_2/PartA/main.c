#include "VirtualSerial.h"

uint8_t *append(uint8_t **old, size_t *oldLen, uint8_t *new, size_t newLen) {
  uint8_t *buf = realloc(*old, (*oldLen) + newLen);
  if(buf) {
    *old = buf;
    memcpy((*old) + (*oldLen), new, newLen);
    *oldLen += newLen;
  }
  return buf;
}

int ReceiveData(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo, uint8_t **buf, size_t *len) {
  int16_t rec = 0;

  *len = (size_t) CDC_Device_BytesReceived(CDCInterfaceInfo);
  if(*len == 0) {
    return 1;
  }

  *buf = (uint8_t *) calloc(*len, sizeof(uint8_t));
  if(*buf == NULL) {
    return 2;
  }

  for(uint16_t i=0; i < *len; i++) {
    rec = CDC_Device_ReceiveByte(CDCInterfaceInfo);
    if(rec >= 0) {
      (*buf)[i] = rec & 0xFF;
    } else {
      *len = i+1;
      break;
    }
  }
  return 0;
}

int SendData(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo, uint8_t *buf, size_t len) {
  uint8_t buff[40] = { 0 };
  memcpy(buff, buf, len);
  return CDC_Device_SendData(CDCInterfaceInfo, buff, len);
}

int main(void) {
  char teststring[] = "Hello World!";
  uint8_t *data = NULL;
  size_t dataLen = 0;

  USB_Init();

  while (1) {
    if(CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface) > 0) {
      uint8_t *new = NULL;
      size_t newLen = 0;
      if(ReceiveData(&VirtualSerial_CDC_Interface, &new, &newLen) == 0) {
        append(&data, &dataLen, new, newLen);
      }
      free(new);
    }
    if(dataLen > 0) {
      uint8_t *lf = memchr(data, '\n', dataLen);
      if(lf) {
        size_t lineLen = lf - data + 1; //+1 to copy the \n with the line
        SendData(&VirtualSerial_CDC_Interface, data, lineLen);
        memmove(data, lf + 1, dataLen - lineLen);
        dataLen -= lineLen;
      }
    }

    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
  }
}
