
#ifndef _USB_DEVICE_COMPOSITE_H_
#define _USB_DEVICE_COMPOSITE_H_ 1
#include "virtual_com.h"

#define CONTROLLER_ID kUSB_ControllerEhci0
#define USB_DEVICE_INTERRUPT_PRIORITY (3U)

typedef struct _usb_device_composite_struct
{
    usb_device_handle deviceHandle;
    usb_cdc_vcom_struct_t cdcVcom[USB_DEVICE_CONFIG_CDC_ACM];
    uint8_t speed;
    uint8_t attach;
    uint8_t currentConfiguration;
    uint8_t
    currentInterfaceAlternateSetting[USB_INTERFACE_COUNT]; 
} usb_device_composite_struct_t;

extern usb_status_t USB_DeviceCdcVcomInit(usb_device_composite_struct_t *deviceComposite);
extern usb_status_t USB_DeviceCdcVcomCallback(class_handle_t handle, uint32_t event, void *param);
extern usb_status_t USB_DeviceCdcVcomSetConfigure(class_handle_t handle, uint8_t configure);
extern void USB_DeviceCdcVcomTask(void);

#endif /* _USB_DEVICE_COMPOSITE_H_ */
