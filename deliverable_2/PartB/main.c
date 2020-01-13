#include "VirtualSerial.h"
#include <xmc_gpio.h>
#include <xmc_ccu4.h>

void blink(void);

const XMC_GPIO_CONFIG_t LED_config = \
      {.mode=XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT3,\
       .output_level=XMC_GPIO_OUTPUT_LEVEL_LOW,\
       .output_strength=XMC_GPIO_OUTPUT_STRENGTH_STRONG_SHARP_EDGE};

XMC_CCU4_SLICE_COMPARE_CONFIG_t CCU_compare_config =
    {.timer_mode          = XMC_CCU4_SLICE_TIMER_COUNT_MODE_EA,
     .monoshot            = false,
     .shadow_xfer_clear   = 0U,
     .dither_timer_period = 0U,
     .dither_duty_cycle   = 0U,
     .prescaler_mode      = XMC_CCU4_SLICE_PRESCALER_MODE_NORMAL,
     .mcm_enable          = 0U,
     .prescaler_initval   = 0xBU,
     .float_limit         = 0U,
     .dither_limit        = 0U,
     .passive_level       = XMC_CCU4_SLICE_OUTPUT_PASSIVE_LEVEL_LOW,
     .timer_concatenation = 0U
    };

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

int ProcessData(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo, uint8_t *buf, size_t len) {
  uint8_t buff[40] = { 0 };
  for(register size_t i=0; i < len; i++) {
    if(buf[i] >= 'a' && buf[i] <= 'z') {
      buff[i] = buf[i] + 'A'-'a';
    } else if(buf[i] >= 'A' && buf[i] <= 'Z') {
      buff[i] = buf[i] + 'a'-'A';
    } else {
      buff[i] = buf[i];
    }
  }
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
        ProcessData(&VirtualSerial_CDC_Interface, data, lineLen);
        memmove(data, lf + 1, dataLen - lineLen);
        dataLen -= lineLen;
      }
    }

    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
  }
}

void blink(void) {
  XMC_CCU4_Init(CCU40, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
  XMC_CCU4_SLICE_CompareInit(CCU40_CC42, &CCU_compare_config);
  XMC_CCU4_SLICE_CompareInit(CCU40_CC43, &CCU_compare_config);
  XMC_CCU4_SLICE_SetTimerCompareMatch(CCU40_CC42, 0x8000);
  XMC_CCU4_SLICE_SetTimerCompareMatch(CCU40_CC43, 0x8000);
  XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU40_CC42, 0xFFFF);
  XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU40_CC43, 0xFA00);
  XMC_CCU4_EnableShadowTransfer(CCU40, XMC_CCU4_SHADOW_TRANSFER_SLICE_2 | XMC_CCU4_SHADOW_TRANSFER_SLICE_3);
  XMC_GPIO_Init(XMC_GPIO_PORT1,  0, &LED_config);
  XMC_GPIO_Init(XMC_GPIO_PORT1,  1, &LED_config);
  XMC_CCU4_EnableClock(CCU40, 2);
  XMC_CCU4_EnableClock(CCU40, 3);
  XMC_CCU4_SLICE_StartTimer(CCU40_CC42);
  XMC_CCU4_SLICE_StartTimer(CCU40_CC43);
}
