/*
 * Copyright 2017 - 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "usb_device_class.h"
#include "usb_device_cdc_acm.h"

#include "usb_device_descriptor.h"

/*******************************************************************************
* Variables
******************************************************************************/

usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param);
extern usb_status_t USB_DeviceCdcVcomCallback(class_handle_t handle, uint32_t event, void *param);
extern uint8_t ep_vcom_cic[USB_DEVICE_CONFIG_CDC_ACM];
extern uint8_t ep_vcom_dic_bulk_in[USB_DEVICE_CONFIG_CDC_ACM];
extern uint8_t ep_vcom_dic_bulk_out[USB_DEVICE_CONFIG_CDC_ACM];
extern uint8_t if_vcom_cic[USB_DEVICE_CONFIG_CDC_ACM];
extern uint8_t if_vcom_dic[USB_DEVICE_CONFIG_CDC_ACM];

// CDC Data EP
usb_device_endpoint_struct_t g_cdcVcomDicEndpoints[USB_DEVICE_CONFIG_CDC_ACM][USB_CDC_VCOM_DIC_ENDPOINT_COUNT];

// CDC Control EP
usb_device_endpoint_struct_t g_cdcVcomCicEndpoints[USB_DEVICE_CONFIG_CDC_ACM][USB_CDC_VCOM_CIC_ENDPOINT_COUNT];

// CDC3 Control interface
usb_device_interface_struct_t g_cdcVcomCicInterface[USB_DEVICE_CONFIG_CDC_ACM];

// CDC3 Data interface
usb_device_interface_struct_t g_cdcVcomDicInterface[USB_DEVICE_CONFIG_CDC_ACM];

// CDC3 Interfaces = Control interface + Data interface
usb_device_interfaces_struct_t g_cdcVcomInterfaces[USB_DEVICE_CONFIG_CDC_ACM][USB_CDC_VCOM_INTERFACE_COUNT];

usb_device_interface_list_t g_UsbDeviceCdcVcomInterfaceList[USB_DEVICE_CONFIG_CDC_ACM][USB_DEVICE_CONFIGURATION_COUNT];

usb_device_class_struct_t g_UsbDeviceCdcVcomConfig[USB_DEVICE_CONFIG_CDC_ACM];

usb_device_class_config_struct_t g_CompositeClassConfig[USB_DEVICE_CONFIG_CDC_ACM];

usb_device_class_config_list_struct_t g_UsbDeviceCompositeConfigList = 
{
    g_CompositeClassConfig, USB_DeviceCallback, USB_DEVICE_CONFIG_CDC_ACM,
};


/* Define device descriptor */
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceDescriptor[] = {
    /* Size of this descriptor in bytes */
    USB_DESCRIPTOR_LENGTH_DEVICE,
    /* DEVICE Descriptor Type */
    USB_DESCRIPTOR_TYPE_DEVICE,
    /* USB Specification Release Number in Binary-Coded Decimal (i.e., 2.10 is 210H). */
    USB_SHORT_GET_LOW(USB_DEVICE_SPECIFIC_BCD_VERSION), USB_SHORT_GET_HIGH(USB_DEVICE_SPECIFIC_BCD_VERSION),
    /* Class code (assigned by the USB-IF). */
    USB_DEVICE_CLASS,
    /* Subclass code (assigned by the USB-IF). */
    USB_DEVICE_SUBCLASS,
    /* Protocol code (assigned by the USB-IF). */
    USB_DEVICE_PROTOCOL,
    /* Maximum packet size for endpoint zero (only 8, 16, 32, or 64 are valid) */
    USB_CONTROL_MAX_PACKET_SIZE,
    /* Vendor ID (assigned by the USB-IF) */
    0xC9U, 0x1FU,
    /* Product ID (assigned by the manufacturer) */
    0xA3, 0x00,
    /* Device release number in binary-coded decimal */
    USB_SHORT_GET_LOW(USB_DEVICE_DEMO_BCD_VERSION), USB_SHORT_GET_HIGH(USB_DEVICE_DEMO_BCD_VERSION),
    /* Index of string descriptor describing manufacturer */
    0x01,
    /* Index of string descriptor describing product */
    0x02,
    /* Index of string descriptor describing the device's serial number */
    0x03,
    /* Number of possible configurations */
    USB_DEVICE_CONFIGURATION_COUNT,
};

#define DESCRIPTOR_INSTANCE_LEN (                                                          \
                             USB_IAD_DESC_SIZE +                                           \
                             USB_DESCRIPTOR_LENGTH_INTERFACE +                             \
                             USB_DESCRIPTOR_LENGTH_CDC_HEADER_FUNC +                       \
                             USB_DESCRIPTOR_LENGTH_CDC_CALL_MANAG +                        \
                             USB_DESCRIPTOR_LENGTH_CDC_ABSTRACT +                          \
                             USB_DESCRIPTOR_LENGTH_CDC_UNION_FUNC +                        \
                             USB_DESCRIPTOR_LENGTH_ENDPOINT +                              \
                             USB_DESCRIPTOR_LENGTH_INTERFACE +                             \
                             USB_DESCRIPTOR_LENGTH_ENDPOINT +                              \
                             USB_DESCRIPTOR_LENGTH_ENDPOINT)
                             
#define CONFIG_DECRIPTOR_LENGTH (  USB_DESCRIPTOR_LENGTH_CONFIGURE +                       \
                                  (DESCRIPTOR_INSTANCE_LEN) * USB_DEVICE_CONFIG_CDC_ACM) 

const uint8_t descriptor_cdc_temp_config[USB_DESCRIPTOR_LENGTH_CONFIGURE] = 
{
    USB_DESCRIPTOR_LENGTH_CONFIGURE,
    USB_DESCRIPTOR_TYPE_CONFIGURE,
    /* Total length of data returned for this configuration. */
    USB_SHORT_GET_LOW(CONFIG_DECRIPTOR_LENGTH),
    USB_SHORT_GET_HIGH(CONFIG_DECRIPTOR_LENGTH),
    
    USB_INTERFACE_COUNT,
    USB_COMPOSITE_CONFIGURE_INDEX,    
    0,
    (USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_D7_MASK) |
        (USB_DEVICE_CONFIG_SELF_POWER << USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_SELF_POWERED_SHIFT) |
        (USB_DEVICE_CONFIG_REMOTE_WAKEUP << USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_REMOTE_WAKEUP_SHIFT),
    USB_DEVICE_MAX_POWER
};

const uint8_t descriptor_cdc_temp_instance[DESCRIPTOR_INSTANCE_LEN] = 
{
    USB_IAD_DESC_SIZE,
    USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION,
    0, /* The first interface number associated with this function */
    0x02,
    USB_CDC_VCOM_CIC_CLASS, 
    USB_CDC_VCOM_CIC_SUBCLASS,
    0x00,
    0x02,

    /* CDC Interface Descriptor */
    USB_DESCRIPTOR_LENGTH_INTERFACE, 
    USB_DESCRIPTOR_TYPE_INTERFACE, 
    0, 
    0x00,
    USB_CDC_VCOM_CIC_ENDPOINT_COUNT, 
    USB_CDC_VCOM_CIC_CLASS, 
    USB_CDC_VCOM_CIC_SUBCLASS, 
    USB_CDC_VCOM_CIC_PROTOCOL,
    0x00,

    /* 1 */
    USB_DESCRIPTOR_LENGTH_CDC_HEADER_FUNC, /* Size of this descriptor in bytes */
    USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE,  /* CS_INTERFACE Descriptor Type */
    USB_CDC_HEADER_FUNC_DESC, 
    0x10,
    0x01, 

    /* 2 */
    USB_DESCRIPTOR_LENGTH_CDC_CALL_MANAG, /* Size of this descriptor in bytes */
    USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE, /* CS_INTERFACE Descriptor Type */
    USB_CDC_CALL_MANAGEMENT_FUNC_DESC,
    0x01, /*Bit 0: Whether device handle call management itself 1, Bit 1: Whether device can send/receive call
             management information over a Data Class Interface 0 */
    0x01, /* Indicates multiplexed commands are handled via data interface */

    /*3*/
    USB_DESCRIPTOR_LENGTH_CDC_ABSTRACT,   /* Size of this descriptor in bytes */
    USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE, /* CS_INTERFACE Descriptor Type */
    USB_CDC_ABSTRACT_CONTROL_FUNC_DESC,
    0x06, /* Bit 0: Whether device supports the request combination of Set_Comm_Feature, Clear_Comm_Feature, and
             Get_Comm_Feature 0, Bit 1: Whether device supports the request combination of Set_Line_Coding,
             Set_Control_Line_State, Get_Line_Coding, and the notification Serial_State 1, Bit ...  */

    /*4*/
    USB_DESCRIPTOR_LENGTH_CDC_UNION_FUNC, /* Size of this descriptor in bytes */
    USB_DESCRIPTOR_TYPE_CDC_CS_INTERFACE, /* CS_INTERFACE Descriptor Type */
    USB_CDC_UNION_FUNC_DESC, 
    0,        /* The interface number of the Communications or Data Class interface  */
    0,        /* Interface number of subordinate interface in the Union  */

    /*Notification Endpoint */
    USB_DESCRIPTOR_LENGTH_ENDPOINT, USB_DESCRIPTOR_TYPE_ENDPOINT,
    0 | (USB_IN << 7U), USB_ENDPOINT_INTERRUPT,
    USB_SHORT_GET_LOW(FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE),
    USB_SHORT_GET_HIGH(FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE), FS_CDC_VCOM_INTERRUPT_IN_INTERVAL,

    /* Data Interface Descriptor */
    USB_DESCRIPTOR_LENGTH_INTERFACE, 
    USB_DESCRIPTOR_TYPE_INTERFACE, 
    0, 
    0x00,
    USB_CDC_VCOM_DIC_ENDPOINT_COUNT, 
    USB_CDC_VCOM_DIC_CLASS, 
    USB_CDC_VCOM_DIC_SUBCLASS, 
    USB_CDC_VCOM_DIC_PROTOCOL,
    0x00, /* Interface Description String Index*/

    /*Bulk IN Endpoint descriptor */
    USB_DESCRIPTOR_LENGTH_ENDPOINT, 
    USB_DESCRIPTOR_TYPE_ENDPOINT, 
    0 | (USB_IN << 7U),
    USB_ENDPOINT_BULK, 
    USB_SHORT_GET_LOW(FS_CDC_VCOM_BULK_IN_PACKET_SIZE),
    USB_SHORT_GET_HIGH(FS_CDC_VCOM_BULK_IN_PACKET_SIZE), 
    0x00, /* The polling interval value is every 0 Frames */

    /*Bulk OUT Endpoint descriptor */
    USB_DESCRIPTOR_LENGTH_ENDPOINT, 
    USB_DESCRIPTOR_TYPE_ENDPOINT,
    0 | (USB_OUT << 7U), 
    USB_ENDPOINT_BULK,
    USB_SHORT_GET_LOW(FS_CDC_VCOM_BULK_OUT_PACKET_SIZE), 
    USB_SHORT_GET_HIGH(FS_CDC_VCOM_BULK_OUT_PACKET_SIZE),
    0x00, /* The polling interval value is every 0 Frames */
};

/* Define configuration descriptor */
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceConfigurationDescriptor[CONFIG_DECRIPTOR_LENGTH];

/* Define string descriptor */
USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceString0[] = {
    2U + 2U, USB_DESCRIPTOR_TYPE_STRING, 0x09, 0x04,
};

USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceString1[] = {
    2U + 2U * 18U, USB_DESCRIPTOR_TYPE_STRING,
    'N',           0x00U,
    'X',           0x00U,
    'P',           0x00U,
    ' ',           0x00U,
    'S',           0x00U,
    'E',           0x00U,
    'M',           0x00U,
    'I',           0x00U,
    'C',           0x00U,
    'O',           0x00U,
    'N',           0x00U,
    'D',           0x00U,
    'U',           0x00U,
    'C',           0x00U,
    'T',           0x00U,
    'O',           0x00U,
    'R',           0x00U,
    'S',           0x00U,
};

USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceString2[] = {2U + 2U * 18U, USB_DESCRIPTOR_TYPE_STRING,
                                'U',           0,
                                'S',           0,
                                'B',           0,
                                ' ',           0,
                                'C',           0,
                                'O',           0,
                                'M',           0,
                                'P',           0,
                                'O',           0,
                                'S',           0,
                                'I',           0,
                                'T',           0,
                                'E',           0,
                                ' ',           0,
                                'D',           0,
                                'E',           0,
                                'M',           0,
                                'O',           0};

USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceString3[] = {2U + 2U * 16U, USB_DESCRIPTOR_TYPE_STRING,
                                '0',           0x00U,
                                '1',           0x00U,
                                '2',           0x00U,
                                '3',           0x00U,
                                '4',           0x00U,
                                '5',           0x00U,
                                '6',           0x00U,
                                '7',           0x00U,
                                '8',           0x00U,
                                '9',           0x00U,
                                'A',           0x00U,
                                'B',           0x00U,
                                'C',           0x00U,
                                'D',           0x00U,
                                'E',           0x00U,
                                'F',           0x00U};

USB_DMA_INIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t g_UsbDeviceString4[] = {2U + 2U * 17U, USB_DESCRIPTOR_TYPE_STRING,
                                'M',           0,
                                'C',           0,
                                'U',           0,
                                ' ',           0,
                                'C',           0,
                                'D',           0,
                                'C',           0,
                                ' ',           0,
                                'C',           0,
                                'D',           0,
                                'C',           0,
                                '2',           0,
                                ' ',           0,
                                'D',           0,
                                'E',           0,
                                'M',           0,
                                'O',           0};

/* Define string descriptor size */
uint32_t g_UsbDeviceStringDescriptorLength[USB_DEVICE_STRING_COUNT] = {
    sizeof(g_UsbDeviceString0), sizeof(g_UsbDeviceString1), sizeof(g_UsbDeviceString2), sizeof(g_UsbDeviceString3),
    sizeof(g_UsbDeviceString4)};

uint8_t *g_UsbDeviceStringDescriptorArray[USB_DEVICE_STRING_COUNT] = {
    g_UsbDeviceString0, g_UsbDeviceString1, g_UsbDeviceString2, g_UsbDeviceString3, g_UsbDeviceString4};

usb_language_t g_UsbDeviceLanguage[USB_DEVICE_LANGUAGE_COUNT] = {{
    g_UsbDeviceStringDescriptorArray, g_UsbDeviceStringDescriptorLength, (uint16_t)0x0409,
}};

usb_language_list_t g_UsbDeviceLanguageList = {
    g_UsbDeviceString0, sizeof(g_UsbDeviceString0), g_UsbDeviceLanguage, USB_DEVICE_LANGUAGE_COUNT,
};


usb_status_t USB_DeviceGetDeviceDescriptor(usb_device_handle handle,
                                           usb_device_get_device_descriptor_struct_t *deviceDescriptor)
{
    deviceDescriptor->buffer = g_UsbDeviceDescriptor;
    deviceDescriptor->length = USB_DESCRIPTOR_LENGTH_DEVICE;
    return kStatus_USB_Success;
}


usb_status_t USB_DeviceGetConfigurationDescriptor(
    usb_device_handle handle, usb_device_get_configuration_descriptor_struct_t *configurationDescriptor)
{
    if (USB_COMPOSITE_CONFIGURE_INDEX > configurationDescriptor->configuration)
    {
        configurationDescriptor->buffer = g_UsbDeviceConfigurationDescriptor;
        configurationDescriptor->length = USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL;
        return kStatus_USB_Success;
    }
    return kStatus_USB_InvalidRequest;
}

usb_status_t USB_DeviceGetStringDescriptor(usb_device_handle handle,
                                           usb_device_get_string_descriptor_struct_t *stringDescriptor)
{
    if (stringDescriptor->stringIndex == 0)
    {
        stringDescriptor->buffer = (uint8_t *)g_UsbDeviceLanguageList.languageString;
        stringDescriptor->length = g_UsbDeviceLanguageList.stringLength;
    }
    else
    {
        uint8_t langId = 0;
        uint8_t langIndex = USB_DEVICE_STRING_COUNT;

        for (; langId < USB_DEVICE_LANGUAGE_COUNT; langId++)
        {
            if (stringDescriptor->languageId == g_UsbDeviceLanguageList.languageList[langId].languageId)
            {
                if (stringDescriptor->stringIndex < USB_DEVICE_STRING_COUNT)
                {
                    langIndex = stringDescriptor->stringIndex;
                }
                break;
            }
        }

        if (USB_DEVICE_STRING_COUNT == langIndex)
        {
            return kStatus_USB_InvalidRequest;
        }
        stringDescriptor->buffer = (uint8_t *)g_UsbDeviceLanguageList.languageList[langId].string[langIndex];
        stringDescriptor->length = g_UsbDeviceLanguageList.languageList[langId].length[langIndex];
    }
    return kStatus_USB_Success;
}

static void set_speed_hs(uint8_t ep_cic, uint8_t ep_dic_in, uint8_t ep_dic_out,usb_descriptor_union_t *ptr1)
{
    if ((ep_cic == (ptr1->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) && 
                   ((ptr1->endpoint.bEndpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) ==
                   USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_IN))
    {
        ptr1->endpoint.bInterval = HS_CDC_VCOM_INTERRUPT_IN_INTERVAL;
        USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(HS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE,
                                           ptr1->endpoint.wMaxPacketSize);
    }
    else if ((ep_dic_in ==
             (ptr1->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
              ((ptr1->endpoint.bEndpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) ==
         USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_IN))
    {
        USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(HS_CDC_VCOM_BULK_IN_PACKET_SIZE, ptr1->endpoint.wMaxPacketSize);
    }
    else if ((ep_dic_out ==
             (ptr1->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
             ((ptr1->endpoint.bEndpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) ==
         USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_OUT))
    {
        USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(HS_CDC_VCOM_BULK_OUT_PACKET_SIZE, ptr1->endpoint.wMaxPacketSize);
    }
}
static void set_speed_fs(uint8_t ep_cic, uint8_t ep_dic_in, uint8_t ep_dic_out,usb_descriptor_union_t *ptr1)
{
    if ((ep_cic ==
        (ptr1->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
        ((ptr1->endpoint.bEndpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) ==
         USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_IN))
    {
        ptr1->endpoint.bInterval = FS_CDC_VCOM_INTERRUPT_IN_INTERVAL;
        USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE,
                                           ptr1->endpoint.wMaxPacketSize);
    }
    else if ((ep_dic_in ==
             (ptr1->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
             ((ptr1->endpoint.bEndpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) ==
         USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_IN))
    {
        USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(FS_CDC_VCOM_BULK_IN_PACKET_SIZE, ptr1->endpoint.wMaxPacketSize);
    }
    else if ((ep_dic_out ==
             (ptr1->endpoint.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK)) &&
             ((ptr1->endpoint.bEndpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK) ==
         USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_OUT))
    {
        USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(FS_CDC_VCOM_BULK_OUT_PACKET_SIZE, ptr1->endpoint.wMaxPacketSize);
    }
}
usb_status_t USB_DeviceSetSpeed(usb_device_handle handle, uint8_t speed)
{
    usb_descriptor_union_t *ptr1;
    usb_descriptor_union_t *ptr2;

    ptr1 = (usb_descriptor_union_t *)(&g_UsbDeviceConfigurationDescriptor[0]);
    ptr2 = (usb_descriptor_union_t *)(&g_UsbDeviceConfigurationDescriptor[USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL - 1]);

    while (ptr1 < ptr2)
    {
        if (ptr1->common.bDescriptorType == USB_DESCRIPTOR_TYPE_ENDPOINT)
        {
            if (USB_SPEED_HIGH == speed)
            {
                for(int i=0; i<USB_DEVICE_CONFIG_CDC_ACM; i++)
                {
                    set_speed_hs(ep_vcom_cic[i], 
                                 ep_vcom_dic_bulk_in[i],
                                 ep_vcom_dic_bulk_out[i],
                                 ptr1);
                }
            }
            else
            {
                for(int i=0; i<USB_DEVICE_CONFIG_CDC_ACM; i++)
                {
                    set_speed_fs(ep_vcom_cic[i], 
                                 ep_vcom_dic_bulk_in[i],
                                 ep_vcom_dic_bulk_out[i],
                                 ptr1);
                }
            }
        }
        ptr1 = (usb_descriptor_union_t *)((uint8_t *)ptr1 + ptr1->common.bLength);
    }

    
    for (int i = 0; i < USB_DEVICE_CONFIG_CDC_ACM; i++)
    {
        for (int j = 0; j < USB_CDC_VCOM_CIC_ENDPOINT_COUNT; j++)
        {
            if (USB_SPEED_HIGH == speed)
            {
                g_cdcVcomCicEndpoints[i][j].maxPacketSize = HS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE;
            }
            else
            {
                g_cdcVcomCicEndpoints[i][j].maxPacketSize = FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE;
            }
        }
    }

    // 1
    for (int j = 0; j < USB_DEVICE_CONFIG_CDC_ACM; j++)
    {
        for (int i = 0; i < USB_CDC_VCOM_DIC_ENDPOINT_COUNT; i++)
        {
            if (USB_SPEED_HIGH == speed)
            {
                if (g_cdcVcomDicEndpoints[j][i].endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK)
                {
                    g_cdcVcomDicEndpoints[j][i].maxPacketSize = HS_CDC_VCOM_BULK_IN_PACKET_SIZE;
                }
                else
                {
                    g_cdcVcomDicEndpoints[j][i].maxPacketSize = HS_CDC_VCOM_BULK_OUT_PACKET_SIZE;
                }
            }
            else
            {
                if (g_cdcVcomDicEndpoints[j][i].endpointAddress & USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK)
                {
                    g_cdcVcomDicEndpoints[j][i].maxPacketSize = FS_CDC_VCOM_BULK_IN_PACKET_SIZE;
                }
                else
                {
                    g_cdcVcomDicEndpoints[j][i].maxPacketSize = FS_CDC_VCOM_BULK_OUT_PACKET_SIZE;
                }
            }
        }
    }
    

    return kStatus_USB_Success;
}

usb_device_class_struct_t g_UsbDeviceCdcVcomConfig[USB_DEVICE_CONFIG_CDC_ACM];
void config_descriptor_init(void);
void ep_init(void);
void interface_init(void);

void usb_data_init(void)
{
    ep_init();
    interface_init();
    
    for(int i=0; i<USB_DEVICE_CONFIG_CDC_ACM; i++)
    {
        // config
        g_CompositeClassConfig[i].classCallback = USB_DeviceCdcVcomCallback;
        g_CompositeClassConfig[i].classHandle   = (class_handle_t)NULL;
        g_CompositeClassConfig[i].classInfomation = &g_UsbDeviceCdcVcomConfig[i];

        g_UsbDeviceCdcVcomConfig[i].configurations = USB_DEVICE_CONFIGURATION_COUNT;
        g_UsbDeviceCdcVcomConfig[i].type           = kUSB_DeviceClassTypeCdc;
        g_UsbDeviceCdcVcomConfig[i].interfaceList  = g_UsbDeviceCdcVcomInterfaceList[i];

        // interface list
        for(int j = 0; j<USB_DEVICE_CONFIGURATION_COUNT; j++)
        {
            g_UsbDeviceCdcVcomInterfaceList[i][j].count      = USB_CDC_VCOM_INTERFACE_COUNT;
            g_UsbDeviceCdcVcomInterfaceList[i][j].interfaces = g_cdcVcomInterfaces[i];
        }
        
        // interfaces
        g_cdcVcomInterfaces[i][0].classCode        = USB_CDC_VCOM_CIC_CLASS;
        g_cdcVcomInterfaces[i][0].subclassCode     = USB_CDC_VCOM_CIC_SUBCLASS;
        g_cdcVcomInterfaces[i][0].protocolCode     = USB_CDC_VCOM_CIC_PROTOCOL;
        g_cdcVcomInterfaces[i][0].interfaceNumber  = if_vcom_cic[i];
        g_cdcVcomInterfaces[i][0].interface        = &g_cdcVcomCicInterface[i];
        g_cdcVcomInterfaces[i][0].count            = sizeof(usb_device_interface_struct_t) / sizeof(usb_device_interfaces_struct_t);
        
        g_cdcVcomInterfaces[i][1].classCode        = USB_CDC_VCOM_DIC_CLASS;
        g_cdcVcomInterfaces[i][1].subclassCode     = USB_CDC_VCOM_DIC_SUBCLASS;
        g_cdcVcomInterfaces[i][1].protocolCode     = USB_CDC_VCOM_DIC_PROTOCOL;
        g_cdcVcomInterfaces[i][1].interfaceNumber  = if_vcom_dic[i];
        g_cdcVcomInterfaces[i][1].interface        = &g_cdcVcomDicInterface[i];
        g_cdcVcomInterfaces[i][1].count            = sizeof(usb_device_interface_struct_t) / sizeof(usb_device_interfaces_struct_t);
        
        //
        g_cdcVcomCicInterface[i].alternateSetting      = 0;
        g_cdcVcomCicInterface[i].classSpecific         = NULL;
        g_cdcVcomCicInterface[i].endpointList.count    = USB_CDC_VCOM_CIC_ENDPOINT_COUNT;
        g_cdcVcomCicInterface[i].endpointList.endpoint = g_cdcVcomCicEndpoints[i];
        
        g_cdcVcomDicInterface[i].alternateSetting      = 0;
        g_cdcVcomDicInterface[i].classSpecific         = NULL;
        g_cdcVcomDicInterface[i].endpointList.count    = USB_CDC_VCOM_DIC_ENDPOINT_COUNT;
        g_cdcVcomDicInterface[i].endpointList.endpoint = g_cdcVcomDicEndpoints[i];
        
        // end point
        g_cdcVcomCicEndpoints[i][0].endpointAddress = ep_vcom_cic[i];
        g_cdcVcomCicEndpoints[i][0].transferType  = USB_ENDPOINT_INTERRUPT;
        g_cdcVcomCicEndpoints[i][0].maxPacketSize = FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE;   
            
        for(int j = 0; j<USB_CDC_VCOM_DIC_ENDPOINT_COUNT; j++)
        {
            g_cdcVcomDicEndpoints[i][j].transferType = USB_ENDPOINT_BULK;
            if(j == 0)
            {
                g_cdcVcomDicEndpoints[i][j].endpointAddress = ep_vcom_dic_bulk_in[i] | (USB_IN  << 7U);
                g_cdcVcomDicEndpoints[i][j].maxPacketSize = FS_CDC_VCOM_BULK_IN_PACKET_SIZE; 
            }
            else
            {
                g_cdcVcomDicEndpoints[i][j].endpointAddress = ep_vcom_dic_bulk_out[i] | (USB_OUT << 7U);
                g_cdcVcomDicEndpoints[i][j].maxPacketSize = FS_CDC_VCOM_BULK_OUT_PACKET_SIZE; 
            }
        }
    }
    
    config_descriptor_init();
}


void config_descriptor_init(void)
{
    uint8_t *p;
    
    // copy config
    memcpy(g_UsbDeviceConfigurationDescriptor+0,
           descriptor_cdc_temp_config, 
           USB_DESCRIPTOR_LENGTH_CONFIGURE);
    
    // copy cdc descritptor
    for(int i=0; i<USB_DEVICE_CONFIG_CDC_ACM; i++)
    {
        memcpy(g_UsbDeviceConfigurationDescriptor + USB_DESCRIPTOR_LENGTH_CONFIGURE + DESCRIPTOR_INSTANCE_LEN*i,
               descriptor_cdc_temp_instance,
               DESCRIPTOR_INSTANCE_LEN);        
    }
    
    //* update ep and interface
    for(int i=0; i<USB_DEVICE_CONFIG_CDC_ACM; i++)
    {
        p = g_UsbDeviceConfigurationDescriptor + USB_DESCRIPTOR_LENGTH_CONFIGURE + DESCRIPTOR_INSTANCE_LEN*i;
        p[2] = if_vcom_cic[i];
        p += USB_IAD_DESC_SIZE;
        p[2] = if_vcom_cic[i];
        p += USB_DESCRIPTOR_LENGTH_INTERFACE;
        p += USB_DESCRIPTOR_LENGTH_CDC_HEADER_FUNC; // 1
        p += USB_DESCRIPTOR_LENGTH_CDC_CALL_MANAG;  // 2
        p += USB_DESCRIPTOR_LENGTH_CDC_ABSTRACT;    // 3
        p[3] = if_vcom_cic[i];
        p[4] = if_vcom_dic[i];
        p += USB_DESCRIPTOR_LENGTH_CDC_UNION_FUNC;  // 4
        p[2] = ep_vcom_cic[i] | (USB_IN << 7U),
        p += USB_DESCRIPTOR_LENGTH_ENDPOINT; 
        // data interface.
        p[2] = if_vcom_dic[i];
        p += USB_DESCRIPTOR_LENGTH_INTERFACE;
        p[2] = ep_vcom_dic_bulk_in[i]  | (USB_IN  << 7U);
        p += USB_DESCRIPTOR_LENGTH_ENDPOINT;
        p[2] = ep_vcom_dic_bulk_out[i] | (USB_OUT << 7U);        
    }
}




// assign endpoint
/*
#define USB_CDC_VCOM_CIC_INTERRUPT_IN_ENDPOINT   (1)
#define USB_CDC_VCOM_DIC_BULK_IN_ENDPOINT        (2)
#define USB_CDC_VCOM_DIC_BULK_OUT_ENDPOINT       (2)

#define USB_CDC_VCOM_CIC_INTERRUPT_IN_ENDPOINT_2 (3)
#define USB_CDC_VCOM_DIC_BULK_IN_ENDPOINT_2      (4)
#define USB_CDC_VCOM_DIC_BULK_OUT_ENDPOINT_2     (4)

#define USB_CDC_VCOM_CIC_INTERRUPT_IN_ENDPOINT_3 (5)
#define USB_CDC_VCOM_DIC_BULK_IN_ENDPOINT_3      (6)
#define USB_CDC_VCOM_DIC_BULK_OUT_ENDPOINT_3     (6)
*/
uint8_t ep_vcom_cic[USB_DEVICE_CONFIG_CDC_ACM];
uint8_t ep_vcom_dic_bulk_in[USB_DEVICE_CONFIG_CDC_ACM];
uint8_t ep_vcom_dic_bulk_out[USB_DEVICE_CONFIG_CDC_ACM];
void ep_init(void)
{
    for(int i=0; i<USB_DEVICE_CONFIG_CDC_ACM; i++)
    {
        ep_vcom_cic[i]          = 1 + i*2;
        ep_vcom_dic_bulk_in[i]  = 2 + i*2;
        ep_vcom_dic_bulk_out[i] = 2 + i*2;
    }
}

/*
#define USB_CDC_VCOM_CIC_INTERFACE_INDEX   (0)
#define USB_CDC_VCOM_DIC_INTERFACE_INDEX   (1)
#define USB_CDC_VCOM_CIC_INTERFACE_INDEX_2 (2)
#define USB_CDC_VCOM_DIC_INTERFACE_INDEX_2 (3)
#define USB_CDC_VCOM_CIC_INTERFACE_INDEX_3 (4)
#define USB_CDC_VCOM_DIC_INTERFACE_INDEX_3 (5)
*/
uint8_t if_vcom_cic[USB_DEVICE_CONFIG_CDC_ACM];
uint8_t if_vcom_dic[USB_DEVICE_CONFIG_CDC_ACM];
void interface_init(void)
{
    for(int i=0; i<USB_DEVICE_CONFIG_CDC_ACM; i++)
    {
        if_vcom_cic[i]  = 0 + i*2;
        if_vcom_dic[i]  = 1 + i*2;
    }
}





