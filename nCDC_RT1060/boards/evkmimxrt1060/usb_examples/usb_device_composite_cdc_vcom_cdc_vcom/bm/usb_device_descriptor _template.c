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


#define CONFIG_DECRIPTOR_LENGTH (  USB_DESCRIPTOR_LENGTH_CONFIGURE +                       \
                            (USB_IAD_DESC_SIZE +                                           \
                             USB_DESCRIPTOR_LENGTH_INTERFACE +                             \
                             USB_DESCRIPTOR_LENGTH_CDC_HEADER_FUNC +                       \
                             USB_DESCRIPTOR_LENGTH_CDC_CALL_MANAG +                        \
                             USB_DESCRIPTOR_LENGTH_CDC_ABSTRACT +                          \
                             USB_DESCRIPTOR_LENGTH_CDC_UNION_FUNC +                        \
                             USB_DESCRIPTOR_LENGTH_ENDPOINT +                              \
                             USB_DESCRIPTOR_LENGTH_INTERFACE +                             \
                             USB_DESCRIPTOR_LENGTH_ENDPOINT +                              \
                             USB_DESCRIPTOR_LENGTH_ENDPOINT) * USB_DEVICE_CONFIG_CDC_ACM) 

extern const uint8_t descriptor_template_CDC_config[];
extern const uint8_t descriptor_template_CDC_IAD[];
extern const uint8_t descriptor_template_CDC_cic_interface[];
extern const uint8_t descriptor_template_CDC_cs[];
extern const uint8_t descriptor_template_CDC_cic_ep[];
extern const uint8_t descriptor_template_CDC_IAD[];
extern const uint8_t descriptor_template_CDC_IAD[];
extern const uint8_t descriptor_template_CDC_IAD[];

const uint8_t descriptor_template_CDC_config[] = 
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
    USB_DEVICE_MAX_POWER,
};
const uint8_t descriptor_template_CDC_IAD[] = 
{
    USB_IAD_DESC_SIZE,
    USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION,
    USB_CDC_VCOM_CIC_INTERFACE_INDEX_3, /* The first interface number associated with this function */
    0x02,
    USB_CDC_VCOM_CIC_CLASS, 
    USB_CDC_VCOM_CIC_SUBCLASS,
    0x00,
    0x02,
};
const uint8_t descriptor_template_CDC_cic_interface[] = 
{
    USB_DESCRIPTOR_LENGTH_INTERFACE, 
    USB_DESCRIPTOR_TYPE_INTERFACE, 
    USB_CDC_VCOM_CIC_INTERFACE_INDEX_3, 
    0x00,
    USB_CDC_VCOM_CIC_ENDPOINT_COUNT, 
    USB_CDC_VCOM_CIC_CLASS, 
    USB_CDC_VCOM_CIC_SUBCLASS, 
    USB_CDC_VCOM_CIC_PROTOCOL,
    0x00,
};
const uint8_t descriptor_template_CDC_cs[] = 
{
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
    USB_CDC_VCOM_CIC_INTERFACE_INDEX_3,        /* The interface number of the Communications or Data Class interface  */
    USB_CDC_VCOM_DIC_INTERFACE_INDEX_3,        /* Interface number of subordinate interface in the Union  */
};
const uint8_t descriptor_template_CDC_cic_ep[] = 
{
    USB_DESCRIPTOR_LENGTH_ENDPOINT, //7
    USB_DESCRIPTOR_TYPE_ENDPOINT,
    USB_CDC_VCOM_CIC_INTERRUPT_IN_ENDPOINT_3 | (USB_IN << 7U), 
    USB_ENDPOINT_INTERRUPT,
    USB_SHORT_GET_LOW(FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE),
    USB_SHORT_GET_HIGH(FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE), 
    FS_CDC_VCOM_INTERRUPT_IN_INTERVAL
};












