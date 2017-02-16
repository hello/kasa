/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef __FMI_INTERNAL_H__
#define __FMI_INTERNAL_H__

/* 
 * Flash Message Interface
 *
 * This is a very simple messaging interface that the Host and Target use
 * in order to allow the Host to read, write, and erase flash memory on
 * the Target.
 *
 * The Host writes requests to mailbox0, and reads responses
 * from mailbox0.   Flash requests all begin with a command
 * (see below for specific commands), and are followed by
 * command-specific data.
 *
 * Flow control:
 * The Host can only issue a command once the Target issues a
 * "Flash Command Credit", using AR6k Counter #4.  As soon as the
 * Target has completed a command, it issues another Flash Command
 * Credit (so the Host can issue the next Flash command).
 */

#define FLASH_DATASZ_MAX 1024

/* Generic flash command.  */
PREPACK struct flash_cmd_s {
    A_UINT32 command;
} POSTPACK;

#define FLASH_READ                      1
        /*
         * Semantics: Host reads Target flash
         * Request format:
         *    A_UINT32      command (FLASH_READ)
         *    A_UINT32      address
         *    A_UINT32      length, at most FLASH_DATASZ_MAX
         * Response format:
         *    A_UINT8       data[length]
         */

/* READ from flash command */
PREPACK struct read_cmd_s {
    A_UINT32 command;
    A_UINT32 address;
    A_UINT32 length;
} POSTPACK;

#define FLASH_WRITE                     2
        /*
         * Semantics: Host writes Target flash
         * Request format:
         *    A_UINT32      command (FLASH_WRITE)
         *    A_UINT32      address
         *    A_UINT32      length, at most FLASH_DATASZ_MAX
         *    A_UINT8       data[length]
         * Response format: none
         */

/* WRITE to flash command */
PREPACK struct write_cmd_s {
    A_UINT32 command;
    A_UINT32 address;
    A_UINT32 length;
} POSTPACK;

#define FLASH_ERASE                     3
#define FLASH_ERASE_COOKIE 0x00a112ff
        /*
         * Semantics: Host erases ENTIRE Target flash, including any
         * board-specific calibration data that is stored into flash
         * at manufacture time and which is needed for proper operation!
         * Request format:
         *    A_UINT32      command      (FLASH_ERASE)
         *    A_UINT32      magic cookie (FLASH_ERASE_COOKIE)
         * Response format: none
         */

/* ERASE flash command */
PREPACK struct erase_cmd_s {
    A_UINT32 command;
    A_UINT32 magic_cookie;
} POSTPACK;

#define FLASH_PARTIAL_ERASE             4
        /*
         * Semantics: Host partially erases Target flash
         * Request format:
         *    A_UINT32      command (FLASH_PARTIAL_ERASE)
         *    A_UINT32      address
         *    A_UINT32      length
         * Response format: none
         */

/* PARTIAL ERASE flash command */
PREPACK struct partial_erase_cmd_s {
    A_UINT32 command;
    A_UINT32 address;
    A_UINT32 length;
} POSTPACK;

#define FLASH_PART_INIT               5
        /*
         * Semantics: Target initializes the flashpart
         * module (previously loaded into RAM) which contains
         * code that is specific to a particular flash
         * command set (e.g. AMD16).
         *
         * Note: This command should be issued before all
         * other flash commands.  Otherwise, the default
         * flash command set, AMD16, is used.   A request
         * to initialize the flashpart module that is made
         * after other flash commands is ignored.
         *
         * Request format:
         *    A_UINT32      command (FLASH_PART_INIT)
         *    A_UINT32      address of flashpart init function
         * Response format: none
         */

/* PART_INIT flash command */
PREPACK struct part_init_cmd_s {
    A_UINT32 command;
    A_UINT32 address;
} POSTPACK;

#define FLASH_DONE                      99
        /*
         * Semantics: Host indicates that it is done.
         * Request format:
         *    A_UINT32      command (FLASH_DONE)
         * Response format: none
         */

#endif
