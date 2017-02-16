//------------------------------------------------------------------------------
// <copyright file="gpio_api.h" company="Atheros">
//    Copyright (c) 2004-2010 Atheros Corporation.  All rights reserved.
// 
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
// </copyright>
// 
// <summary>
// 	Wifi driver for AR6002
// </summary>
//
//------------------------------------------------------------------------------
//==============================================================================
// Host-side General Purpose I/O API.
//
// Author(s): ="Atheros"
//==============================================================================
#ifndef _GPIO_API_H_
#define _GPIO_API_H_

/*
 * Send a command to the Target in order to change output on GPIO pins.
 */
A_STATUS wmi_gpio_output_set(struct wmi_t *wmip,
                             A_UINT32 set_mask,
                             A_UINT32 clear_mask,
                             A_UINT32 enable_mask,
                             A_UINT32 disable_mask);

/*
 * Send a command to the Target requesting input state of GPIO pins.
 */
A_STATUS wmi_gpio_input_get(struct wmi_t *wmip);

/*
 * Send a command to the Target to change the value of a GPIO register.
 */
A_STATUS wmi_gpio_register_set(struct wmi_t *wmip,
                               A_UINT32 gpioreg_id,
                               A_UINT32 value);

/*
 * Send a command to the Target to fetch the value of a GPIO register.
 */
A_STATUS wmi_gpio_register_get(struct wmi_t *wmip, A_UINT32 gpioreg_id);

/*
 * Send a command to the Target, acknowledging some GPIO interrupts.
 */
A_STATUS wmi_gpio_intr_ack(struct wmi_t *wmip, A_UINT32 ack_mask);

#endif /* _GPIO_API_H_ */
