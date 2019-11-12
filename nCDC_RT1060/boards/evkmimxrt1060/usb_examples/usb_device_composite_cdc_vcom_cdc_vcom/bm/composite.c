
#include <stdio.h>
#include <stdlib.h>
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "usb_device_class.h"
#include "usb_device_cdc_acm.h"
#include "usb_device_ch9.h"
#include "usb_device_descriptor.h"
#include "fsl_device_registers.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "composite.h"
#include "usb_phy.h"
#include "pin_mux.h"

void BOARD_InitHardware(void);
void USB_DeviceClockInit(void);
void USB_DeviceIsrEnable(void);

extern usb_device_class_config_list_struct_t g_UsbDeviceCompositeConfigList;
usb_device_composite_struct_t g_composite;

usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param);
extern usb_device_class_struct_t g_UsbDeviceCdcVcomConfig[USB_DEVICE_CONFIG_CDC_ACM];

void USB_OTG1_IRQHandler(void)
{
    USB_DeviceEhciIsrFunction(g_composite.deviceHandle);
}

void USB_OTG2_IRQHandler(void)
{
    USB_DeviceEhciIsrFunction(g_composite.deviceHandle);
}

void USB_DeviceClockInit(void)
{
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL, BOARD_USB_PHY_TXCAL45DP, BOARD_USB_PHY_TXCAL45DM,
    };

    if (CONTROLLER_ID == kUSB_ControllerEhci0)
    {
        CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 480000000U);
        CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 480000000U);
    }
    else
    {
        CLOCK_EnableUsbhs1PhyPllClock(kCLOCK_Usbphy480M, 480000000U);
        CLOCK_EnableUsbhs1Clock(kCLOCK_Usb480M, 480000000U);
    }
    USB_EhciPhyInit(CONTROLLER_ID, BOARD_XTAL0_CLK_HZ, &phyConfig);
}
void USB_DeviceIsrEnable(void)
{
    uint8_t irqNumber;
    uint8_t usbDeviceEhciIrq[] = USBHS_IRQS;
    irqNumber = usbDeviceEhciIrq[CONTROLLER_ID - kUSB_ControllerEhci0];

    NVIC_SetPriority((IRQn_Type)irqNumber, USB_DEVICE_INTERRUPT_PRIORITY);
    EnableIRQ((IRQn_Type)irqNumber);
}

usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;
    uint16_t *temp16 = (uint16_t *)param;
    uint8_t *temp8 = (uint8_t *)param;

    switch (event)
    {
        case kUSB_DeviceEventBusReset:
        {
            g_composite.attach = 0;
            error = kStatus_USB_Success;
            for (uint8_t i = 0; i < USB_DEVICE_CONFIG_CDC_ACM; i++)
            {
                g_composite.cdcVcom[i].recvSize = 0;
                g_composite.cdcVcom[i].sendSize = 0;
                g_composite.cdcVcom[i].attach = 0;
            }

            if (kStatus_USB_Success == USB_DeviceClassGetSpeed(CONTROLLER_ID, &g_composite.speed))
            {
                USB_DeviceSetSpeed(handle, g_composite.speed);
            }
        }
        break;
        case kUSB_DeviceEventSetConfiguration:
            if (param)
            {
                g_composite.attach = 1;
                g_composite.currentConfiguration = *temp8;

                USB_DeviceCdcVcomSetConfigure(g_composite.cdcVcom[0].cdcAcmHandle, *temp8);
                error = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceEventSetInterface:
            if (g_composite.attach)
            {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                uint8_t alternateSetting = (uint8_t)(*temp16 & 0x00FFU);
                if (interface < USB_INTERFACE_COUNT)
                {
                    g_composite.currentInterfaceAlternateSetting[interface] = alternateSetting;
                    error = kStatus_USB_Success;
                }
            }
            break;
        case kUSB_DeviceEventGetConfiguration:
            if (param)
            {
                *temp8 = g_composite.currentConfiguration;
                error = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceEventGetInterface:
            if (param)
            {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                if (interface < USB_INTERFACE_COUNT)
                {
                    *temp16 = (*temp16 & 0xFF00U) | g_composite.currentInterfaceAlternateSetting[interface];
                    error = kStatus_USB_Success;
                }
                else
                {
                    error = kStatus_USB_InvalidRequest;
                }
            }
            break;
        case kUSB_DeviceEventGetDeviceDescriptor:
            if (param)
            {
                error = USB_DeviceGetDeviceDescriptor(handle, (usb_device_get_device_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetConfigurationDescriptor:
            if (param)
            {
                error = USB_DeviceGetConfigurationDescriptor(handle,
                                                             (usb_device_get_configuration_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetStringDescriptor:
            if (param)
            {
                error = USB_DeviceGetStringDescriptor(handle, (usb_device_get_string_descriptor_struct_t *)param);
            }
            break;
        default:
            break;
    }

    return error;
}

void usb_data_init(void);
void USB_DeviceApplicationInit(void)
{
    USB_DeviceClockInit();
    usb_data_init();
    g_composite.speed = USB_SPEED_FULL;
    g_composite.attach = 0;
    for(int i=0; i<USB_DEVICE_CONFIG_CDC_ACM; i++)
        g_composite.cdcVcom[i].cdcAcmHandle = (class_handle_t)NULL;

    g_composite.deviceHandle = NULL;

    if (kStatus_USB_Success !=
        USB_DeviceClassInit(CONTROLLER_ID, &g_UsbDeviceCompositeConfigList, &g_composite.deviceHandle))
    {
        usb_echo("USB device composite demo init failed\r\n");
        return;
    }
    else
    {
        usb_echo("USB device composite demo\r\n");
        for(int i=0; i<USB_DEVICE_CONFIG_CDC_ACM; i++)
            g_composite.cdcVcom[i].cdcAcmHandle = g_UsbDeviceCompositeConfigList.config[i].classHandle;
        USB_DeviceCdcVcomInit(&g_composite);
    }

    USB_DeviceIsrEnable();
    USB_DeviceRun(g_composite.deviceHandle);
}

void USB_DeviceCdcVcomTask(void)
{
    extern volatile usb_device_composite_struct_t *g_deviceComposite;
    usb_status_t error = kStatus_USB_Error;
    volatile usb_cdc_vcom_struct_t *vcomInstance;

    for (uint8_t i = 0; i < USB_DEVICE_CONFIG_CDC_ACM; i++)
    {
        vcomInstance = &g_deviceComposite->cdcVcom[i];
        if ((1 == vcomInstance->attach) && (1 == vcomInstance->startTransactions))
        {
            /* User Code */
            if ((0 != vcomInstance->recvSize) && (0xFFFFFFFFU != vcomInstance->recvSize))
            {
                int32_t i;

                /* Copy Buffer to Send Buff */
                for (i = 0; i < vcomInstance->recvSize; i++)
                {
                    vcomInstance->currSendBuf[vcomInstance->sendSize++] = vcomInstance->currRecvBuf[i];
                }
                vcomInstance->recvSize = 0;
            }

            if (vcomInstance->sendSize)
            {
                uint32_t size = vcomInstance->sendSize;
                vcomInstance->sendSize = 0;

                error = USB_DeviceCdcAcmSend(vcomInstance->cdcAcmHandle, vcomInstance->bulkInEndpoint,
                                             vcomInstance->currSendBuf, size);

                if (error != kStatus_USB_Success)
                {
                    /* Failure to send Data Handling code here */
                }
            }
        }
    }
}

void main(void)
{
    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    USB_DeviceApplicationInit();

    while (1)
    {
        USB_DeviceCdcVcomTask();
    }
}
