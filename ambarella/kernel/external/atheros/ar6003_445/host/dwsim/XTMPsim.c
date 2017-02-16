//------------------------------------------------------------------------------
// <copyright file="XTMPsim.c" company="Atheros">
//    Copyright (c) 2004-2007 Atheros Corporation.  All rights reserved.
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
// Author(s): ="Atheros"
//==============================================================================

/*
 * This code supports a simulation environment based on the
 * Tensilica Xtensa Instruction Set Simulator (XT ISS) accessing
 * an Atheros AR6000 through its Diagnostic Window.
 *
 * In particular, this file uses XTensa Modeling Protocol (XTMP)
 * to provide access to memory over the cpu's IPort and DPort and
 * to AR6K SoC registers over XLMI.
 */



#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include "hw/rtc_reg.h"
#include "hw/mbox_reg.h"
#include "hw/uart_reg.h"

#define XtensaToolsVersion 60003
#include  "iss/mp.h"

#define FIRMWARE_IMAGE  "fw.sim.out"
#define MEMORY_IMAGE    "olca.mem"
#define XT_CONFIG_NAME  "mercury_2006_10_01" /* mercury_fpga2 */
#define DEBUG_PORT      6002


#define ROMSZ (80*1024)
#define RAMSZ (184*1024)
#define RAMSTART 0x100000       /* phys addr of start of RAM */
#define MEMSTART 0x80000        /* phys addr of start of memory */
#define RAMEND (RAMSTART+RAMSZ)
#define RAMEND_OFFSET (RAMEND-MEMSTART)

/*
 * Hacky: 4KB area ("fixmap") at the end of RAM always maps to the end of
 * target emulation memory. This allows us to quickly set up DataSets in
 * target memory before simulation begins.
 */
#define FIXMAP_AREA_SZ 0x1000

bool endianness;
unsigned int memWidth;

XTMP_deviceStatus
AR6002_SOC_XTMP_post(void *notused, XTMP_deviceXfer *xfer);

XTMP_deviceStatus
AR6002_SOC_XTMP_peek(void *notused, u32 *dst, XTMP_address addr, u32 size);

XTMP_deviceStatus
AR6002_SOC_XTMP_poke(void *notused, XTMP_address addr, u32 size, const u32 *src);

XTMP_deviceStatus
AR6002_MEM_XTMP_post(void *notused, XTMP_deviceXfer *xfer);

XTMP_deviceStatus
AR6002_MEM_XTMP_peek(void *notused, u32 *dst, XTMP_address addr, u32 size);

XTMP_deviceStatus
AR6002_MEM_XTMP_poke(void *notused, XTMP_address addr, u32 size, const u32 *src);

int
PHYSMEMADDR_TO_SIMOFF(XTMP_address addr, u32 *poffset);

extern diag_init(void);
extern void diag_read(unsigned int addr, unsigned int *value);
extern void diag_write(unsigned int addr, unsigned int value);

long int data_start, data_end;
u32 data_start_offset, data_end_offset;

XTMP_register interrupt_reg;

/* Interrupt Numbers (matches target/.../cmnos_xtensa.h) */
#define A_INUM_MBOX            4
#define A_INUM_GPIO            6
#define A_INUM_ERROR           7 /* TBD */
#define A_INUM_TIMER           8
#define A_INUM_WMAC            9

#define IMASK_ERROR       (1<<A_INUM_ERROR)
#define IMASK_GPIO        (1<<A_INUM_GPIO)
#define IMASK_TIMER       (1<<A_INUM_TIMER)
#define IMASK_MBOX        (1<<A_INUM_MBOX)
#define IMASK_WMAC        (1<<A_INUM_WMAC)

/*
 * Check for pending interrupts to the emulation hardware and
 * post/clear interrupts to the Instruction Set Simulator.
 */
void
update_interrupt_status(u32 int_status)
{
    u32 new_value;
    static old_interrupt_value = 0;

    new_value = 0;

#if 0
    {
        u32 force_intr;

        /*
         * This is a hack that shouldn't generally be needed.
         * You can use EJTAG or whatever to set LOCAL_SCRATCH_ADDRESS[4]
         * to non-zero in order to cause the XTMP model to send
         * a timer interrupt to the CPU.  Set to non-zero in
         * order to manually clear that interrupt.
         *
         * This allows us to run software with a waiti instruction
         * and guarantee an easy way to wake it up.  Normally,
         * though we run XTMP simulations without waiti so this
         * hack isn't needed.
         */

        diag_read (&((A_UINT32 *)LOCAL_SCRATCH_ADDRESS[4]), &force_intr);
        if (force_intr) {
            new_value |= IMASK_TIMER;
        }
    }
#endif

    if (int_status & (INT_STATUS_RTC_ALARM_MASK |
                      INT_STATUS_HF_TIMER_MASK |
                      INT_STATUS_KEYPAD_MASK |
                      INT_STATUS_SI_MASK |
                      INT_STATUS_WDT_INT_MASK))
    {
        error("Unexpected interrupt status: 0x%x\n", int_status);
    }

    if (int_status & INT_STATUS_MAC_MASK) {
        new_value |= IMASK_WMAC;
    }

    if (int_status & INT_STATUS_MAILBOX_MASK) {
        new_value |= IMASK_MBOX;
    }

    if (int_status & (INT_STATUS_LF_TIMER0_MASK |
                      INT_STATUS_LF_TIMER1_MASK |
                      INT_STATUS_LF_TIMER2_MASK |
                      INT_STATUS_LF_TIMER3_MASK))
    {
        new_value |= IMASK_TIMER;
    }

    if (int_status & (INT_STATUS_GPIO_MASK |
                      INT_STATUS_UART_MASK))
    {
        new_value |= IMASK_GPIO;
    }

    if (int_status & (INT_STATUS_ERROR_MASK |
                      INT_STATUS_WDT_INT_MASK))
    {
        new_value |= IMASK_ERROR;
    }

    if (old_interrupt_value != new_value) {
#if 0
        printf("Update interrupts int_status=0x%x interrupts=0x%x\n",
                int_status, new_value);
#endif
        XTMP_setRegisterValue(interrupt_reg, &new_value);
        old_interrupt_value = new_value;
    }
}

void
sync_interrupt_status(int signum)
{
    u32 int_status;

    diag_read(INT_STATUS_ADDRESS, &int_status);
    update_interrupt_status(int_status);
}


/* Start of Mercury's memory mapped physical memory */
u8 *mercmem;

int dwsim_dbg = 1;

int
DISABLE_DEBUG_MSGS(void)
{
        int rv = dwsim_dbg;
        dwsim_dbg = 0;
        return rv;
}

void
RESTORE_DEBUG_MSGS(int saved_state)
{
        dwsim_dbg = saved_state;
}


void
debug(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    if (dwsim_dbg) {
        vprintf(format, ap);
        fflush(stdout);
    }
    va_end(ap);
}

XTMP_deviceFunctions AR6002_SOC_XTMP_functions =
{
    AR6002_SOC_XTMP_post,
    AR6002_SOC_XTMP_peek,
    AR6002_SOC_XTMP_poke
};

XTMP_deviceFunctions AR6002_MEM_XTMP_functions =
{
    AR6002_MEM_XTMP_post,
    AR6002_MEM_XTMP_peek,
    AR6002_MEM_XTMP_poke
};

XTMP_core core;
XTMP_device SOCdev;

#define WAR_FIXED_IRAM 1 /* Fixed in RA-2006.5 Tensilica release */

void
find_image_symbol(char *symbol, char *elf_image, long int *pval, u32 *poffset)
{
        FILE *f;
        char addr_str[9];
        char cmd_string[1024];

        sprintf(cmd_string,
            "xt-nm %s | grep ' %s' | cut -f1 -d ' '",
            elf_image, symbol);

        f = popen(cmd_string, "r");
        if (f == NULL) {
            perror("Cannot lookup symbol(1)");
            exit(1);
        }
        if (fgets(addr_str, 9, f) == NULL) {
            perror("Cannot determine symbol(2)");
            exit(1);
        }
        pclose(f);
        *pval = strtol(addr_str, NULL, 16);
        PHYSMEMADDR_TO_SIMOFF((XTMP_address)*pval, poffset) ;
}

void
XTMP_main(int argc, char **argv)
{
        XTMP_params params;
        XTMP_device /* XTMP_memory */
                IROMmemory,
                DROMmemory,
#if defined(WAR_FIXED_IRAM)
                IRAM0memory,
                IRAM1memory,
#endif /* WAR_FIXED_IRAM */
                DRAM0memory,
                DRAM1memory;
        XTMP_connector SOCconn;
        bool rv;
        u32 sz;
        XTMP_address addr;
        unsigned int debugPort;
        int memfd;
        char *firmware_image;
        int save_dwsim_dbg;

        diag_init(); /* Initialize access to SoC through Diagnostic Window */

        debug("XTENSA_SYSTEM environment variable is: %s\n",
               getenv("XTENSA_SYSTEM"));
        debug("XTENSA_CORE environment variable is: %s\n",
               getenv("XTENSA_CORE"));
        {
            char *config_name;
            if ((config_name = getenv("XTENSA_CORE")) == NULL) {
                config_name = XT_CONFIG_NAME;
            }
            debug("Fetch paramters for configuration: %s...\n", config_name);
            params = XTMP_paramsNew(config_name, 0);
        }

        save_dwsim_dbg = DISABLE_DEBUG_MSGS();
        debug("Create CPU core...\n");
        core = XTMP_coreNew("AR6KCPU", params, 0);

        endianness = XTMP_isBigEndian(core);
        debug("Core endianness: %s\n", endianness ? "BE" : "LE");

        memWidth = XTMP_byteWidth(core);
        debug("Memory width: %d bytes\n", memWidth);

        debug("Configure memory...\n");
        rv = XTMP_getLocalMemoryAddress(params, XTMP_PT_DRAM, 0, &addr);
        debug("DRAM0 addr = 0x%x\n", rv ? addr : 0);
        rv = XTMP_getLocalMemorySize(params, XTMP_PT_DRAM, 0, &sz);
        debug("DRAM0 sz = 0x%x\n", rv ? sz : 0);
        if (rv && sz ) {
#if 0
            /* Simple DRAM */
            DRAM0memory = XTMP_localMemoryNew("DRAM0",
                                              memWidth,
                                              endianness,
                                              sz);
#else
            /* DRAM explicitly handled by XTMP model */
            DRAM0memory = XTMP_deviceNew("DRAM0", &AR6002_MEM_XTMP_functions,
                                                NULL, memWidth, sz);
#endif
            XTMP_connectToCore(core, XTMP_PT_DRAM, 0, DRAM0memory,  addr);
        }

        rv = XTMP_getLocalMemoryAddress(params, XTMP_PT_DRAM, 1, &addr);
        debug("DRAM1 addr = 0x%x\n", rv ? addr : 0);
        rv = XTMP_getLocalMemorySize(params, XTMP_PT_DRAM, 1, &sz);
        debug("DRAM1 sz = 0x%x\n", rv ? sz : 0);
        if (rv && sz) {
#if 0
            /* Simple DRAM */
            DRAM1memory = XTMP_localMemoryNew("DRAM1",
                                              memWidth,
                                              endianness,
                                              sz);
#else
            /* DRAM explicitly handled by XTMP model */
            DRAM1memory = XTMP_deviceNew("DRAM1", &AR6002_MEM_XTMP_functions,
                                                NULL, memWidth, sz);
#endif
            XTMP_connectToCore(core, XTMP_PT_DRAM, 1, DRAM1memory,  addr);
        }

        rv = XTMP_getLocalMemoryAddress(params, XTMP_PT_DROM, 0, &addr);
        debug("DR0M addr = 0x%x\n", rv ? addr : 0);
        rv = XTMP_getLocalMemorySize(params, XTMP_PT_DROM, 0, &sz);
        debug("DR0M sz = 0x%x\n", rv ? sz : 0);
        if (rv && sz) {
#if 0
            /* Simple DROM */
            DROMmemory = XTMP_localMemoryNew("DROM",
                                             memWidth,
                                             endianness,
                                             sz);
#else
            /* DROM explicitly handled by XTMP model */
            DROMmemory = XTMP_deviceNew("DROM", &AR6002_MEM_XTMP_functions,
                                                NULL, memWidth, sz);
#endif
            XTMP_connectToCore(core, XTMP_PT_DROM, 0, DROMmemory,  addr);
        }

#if defined(WAR_FIXED_IRAM)
        rv = XTMP_getLocalMemoryAddress(params, XTMP_PT_IRAM, 0, &addr);
        debug("IRAM0 addr = 0x%x\n", rv ? addr : 0);
        rv = XTMP_getLocalMemorySize(params, XTMP_PT_IRAM, 0, &sz);
        debug("IRAM0 sz = 0x%x\n", rv ? sz : 0);
        if (rv && sz) {
#if 0
            /* Simple IRAM */
            IRAM0memory = XTMP_localMemoryNew("IRAM0",
                                              memWidth,
                                              endianness,
                                              sz);
#else
            /* IRAM explicitly handled by XTMP model */
            IRAM0memory = XTMP_deviceNew("IRAM0", &AR6002_MEM_XTMP_functions,
                                                NULL, memWidth, sz);
#endif
            XTMP_connectToCore(core, XTMP_PT_IRAM, 0, IRAM0memory,  addr);
        }

        rv = XTMP_getLocalMemoryAddress(params, XTMP_PT_IRAM, 1, &addr);
        debug("IRAM1 addr = 0x%x\n", rv ? addr : 0);
        rv = XTMP_getLocalMemorySize(params, XTMP_PT_IRAM, 1, &sz);
        debug("IRAM1 sz = 0x%x\n", rv ? sz : 0);
        if (rv && sz) {
#if 0
            /* Simple IRAM */
            IRAM1memory = XTMP_localMemoryNew("IRAM1",
                                              memWidth,
                                              endianness,
                                              sz);
#else
            /* IRAM explicitly handled by XTMP model */
            IRAM1memory = XTMP_deviceNew("IRAM1", &AR6002_MEM_XTMP_functions,
                                                NULL, memWidth, sz);
#endif
            XTMP_connectToCore(core, XTMP_PT_IRAM, 1, IRAM1memory,  addr);
        }
#endif /* WAR_FIXED_IRAM */

        rv = XTMP_getLocalMemoryAddress(params, XTMP_PT_IROM, 0, &addr);
        debug("IROM addr = 0x%x\n", rv ? addr : 0);
        rv = XTMP_getLocalMemorySize(params, XTMP_PT_IROM, 0, &sz);
        debug("IROM sz = 0x%x\n", rv ? sz : 0);
        if (rv && sz) {
#if 0
            /* Simple IROM */
            IROMmemory = XTMP_localMemoryNew("IROM",
                                             memWidth,
                                             endianness,
                                             sz);
#else
            /* IROM explicitly handled by XTMP model */
            IROMmemory = XTMP_deviceNew("IROM", &AR6002_MEM_XTMP_functions,
                                                NULL, memWidth, sz);
#endif
            XTMP_connectToCore(core, XTMP_PT_IROM, 0, IROMmemory,  addr);
        }

        rv = XTMP_getLocalMemoryAddress(params, XTMP_PT_XLMI, 0, &addr);
        debug("XLMI addr = 0x%x\n", rv ? addr : 0);
        rv = XTMP_getLocalMemorySize(params, XTMP_PT_XLMI, 0, &sz);
        debug("XLMI sz = 0x%x\n", rv ? sz : 0);
        if (rv) {
            debug("Configure SoC...\n");
            SOCdev = XTMP_deviceNew("SoC", &AR6002_SOC_XTMP_functions, NULL,
                                        memWidth, sz);
            XTMP_connectToCore(core, XTMP_PT_XLMI, 0, SOCdev, addr);
        }

        RESTORE_DEBUG_MSGS(save_dwsim_dbg);

        {
            /*
             * We use a local memmap'ed file for local simulation memory.
             * This makes it easier to pre-populate memory (as in ROM) and
             * to share a memory image (e.g. to debug a problem).
             */
            char *memory_image;

            if ((memory_image=getenv("OLCA_MEMORY_IMAGE")) == NULL) {
                memory_image = MEMORY_IMAGE;
            }
            debug("Using memory image file %s\n", memory_image);

            memfd = open(memory_image, O_RDONLY);
            if (memfd < 0) {
                perror("Cannot open memory image.\n");
                exit(1);
            }

            /*
             * mercmem is used to simulate read-only memory.
             * Writable memory is handled on the target emulation system.
             */
            mercmem = (u8 *)mmap(0, ROMSZ+RAMSZ-FIXMAP_AREA_SZ,
                                        PROT_READ|PROT_WRITE,
                                        MAP_PRIVATE, memfd, 0);
            if (mercmem == MAP_FAILED) {
                perror("Cannot mmap memory image.\n");
                exit(1);
            }
        }

        if ((firmware_image=getenv("OLCA_FIRMWARE_IMAGE")) == NULL) {
            firmware_image = FIRMWARE_IMAGE;
        }

        find_image_symbol("_data_start", firmware_image,
                        &data_start, &data_start_offset);

        find_image_symbol("_end", firmware_image,
                        &data_end, &data_end_offset);

        printf("Writable data range 0x%x-0x%x (offsets=0x%x-0x%x)\n",
                        data_start, data_end,
                        data_start_offset, data_end_offset);
#if 1
        /* Load firmware ELF image */
        {
            int save_dwsim_dbg;

            /* While loading, temporarily disable debug messages. */
            save_dwsim_dbg = DISABLE_DEBUG_MSGS();
            rv = XTMP_loadProgram(core, firmware_image, 0);
            RESTORE_DEBUG_MSGS(save_dwsim_dbg);

            printf("%s firmware image: %s\n",
                rv ? "Loaded" : "FAILED to load",
                firmware_image);
        }
#endif

        debugPort = XTMP_enableDebug(core, DEBUG_PORT);
        if (debugPort == 0) {
            printf("Default debug port is BUSY.\n");
            debugPort = XTMP_enableDebug(core, 0);
        }

#if 0
        /* Print a list of all register names/numbers */
        {
            int i;
            printf("REGISTERS\n");
            for (i=0; i<XTMP_getRegisterCount(core); i++) {
                XTMP_register reg = XTMP_lookupRegisterByIndex(core, i);
                if (reg) {
                    const char *name = XTMP_getRegisterName(reg);
                    printf("%d: %s\n", i, name);
                }
            }
        }
#endif


        XTMP_setWaitForDebugger(core, true);
        printf("Waiting for debugger to connect....Use:\n");
        printf("target remote localhost:%d\n", debugPort);

        if ((interrupt_reg = XTMP_lookupRegisterByName(core, "INTERRUPT")) == 0) {
            error("No INTERRUPT register?!");
        }

#if 0
        /* Timed polling for interrupt status changes */
        {
            /*
             * Interrupt support.  For now, we check for changes in pending
             * interrupts
             * 1) after writing an SoC register.
             * 2) periodically
             * If needed, we may convert interrupt support to be interrupt
             * driven.  This would take cooperation from Target-side code.
             */
            struct itimerval itval;
            struct sigaction action;
            sigset_t sigset;

            sigemptyset(&sigset);
            action.sa_handler = sync_interrupt_status;
            action.sa_mask = sigset;
            action.sa_flags = 0;
            if (sigaction(SIGALRM, &action, NULL) < 0) {
                perror("Cannot set sigaction SIGALRM");
                exit(1);
            }

            itval.it_value.tv_sec=0;
            itval.it_value.tv_usec=300000; /* Check every 300Ms */
            itval.it_interval.tv_sec=0;
            itval.it_interval.tv_usec=300000; /* Check every 300Ms */
            if (setitimer(ITIMER_REAL, &itval, NULL) < 0) {
                perror("Cannot setitimer");
                exit(1);
            }
        }
#endif
        XTMP_stepSystem(-1);
}

void
print_xfer(char *id, XTMP_deviceXfer *xfer)
{
    unsigned int trans_type;
    u32 *data;

    if (XTMP_xferIsRequest(xfer)) {
        debug("%s: Transfer Request ", id);
    }

    if (XTMP_xferIsResponse(xfer)) {
        error("Transfer Response?! ");
    }

    trans_type = XTMP_xferGetType(xfer);

    switch(trans_type) {
        case XTMP_READ: {
            debug("READ ");
            break;
        }
        case XTMP_WRITE: {
            debug("WRITE ");
            break;
        }
        case XTMP_BLOCKREAD: {
            debug("BLOCKREAD?! ");
            break;
        }
        case XTMP_BLOCKWRITE: {
            debug("BLOCKWRITE?! ");
            break;
        }
        case XTMP_RCW: {
            debug("RCW?! ");
            break;
        }
        default: {
            debug("UNKNOWN TRANSFER TYPE: 0x%x?! ", trans_type);
            break;
        }
    }

    data = XTMP_xferGetData(xfer);
    debug("Address=0x%x ", XTMP_xferGetAddress(xfer));
    debug("Size=0x%x ", XTMP_xferGetSize(xfer));
    debug("Data=0x%x ", data);
    if (trans_type == XTMP_WRITE) {
        debug("Value=0x%x\n", *((int *)data));
    }
    debug("ByteEnables=0x%x ", XTMP_xferGetByteEnables(xfer));

    if (XTMP_xferIsFetch(xfer))
        debug("Instruction fetch ");

    debug("\n");
}

/* Handle CPU loads/stores to SoC addresses */
XTMP_deviceStatus
AR6002_SOC_XTMP_post(void *notused,
                     XTMP_deviceXfer *xfer)
{
#if 0
    print_xfer("SOC", xfer);
#endif

    if (XTMP_xferGetType(xfer) == XTMP_READ) {
        AR6002_SOC_XTMP_peek(notused,
                             XTMP_xferGetData(xfer),
                             XTMP_xferGetAddress(xfer),
                             XTMP_xferGetSize(xfer));
    } else if (XTMP_xferGetType(xfer) == XTMP_WRITE) {
        AR6002_SOC_XTMP_poke(notused,
                             XTMP_xferGetAddress(xfer),
                             XTMP_xferGetSize(xfer),
                             XTMP_xferGetData(xfer));
    }

    if (XTMP_respond(xfer, XTMP_OK) != XTMP_DEVICE_OK)
        error("AR6002_SOC_XTMP_post: XTMP_respond failed?!\n");

    return XTMP_DEVICE_OK;
}

u32 uart_holding_register;
u32 mboxpio_holding_register;

#define MAC_BASE_ADDRESS 0xFFF0000 /* AR6000 WMAC base address */

/* Convert an AR6002 SOC address into an equivalent AR6000 address */
#define AR6002_TO_AR6000(addr) (((addr) - 0x4000) | 0x0c000000)

/* TBDXXX: We don't currently trap invalid accesses */

/* ISS/debugger reads */
XTMP_deviceStatus
AR6002_SOC_XTMP_peek(void *notused,
                     u32 *dst,
                     XTMP_address addr,
                     u32 size)
{
    u32 value;

    if (size != 4) {
        printf("Ignore bogus SOC READ (addr=0x%x size=%d)\n", addr, size);
        return XTMP_DEVICE_ERROR;
    }

#if 0
    if (addr == (XTMP_address)read_trap) {
        u32 pc1, ar0;

        pc1 = XTMP_getStagePC(core, 0);
        XTMP_copyRegisterValue(XTMP_lookupRegisterByName(core, "AR0"), &ar0);
        printf("SOC_XTMP_peek: 0x%x/0x%x Attempt to PEEK at 0x%x\n", pc1, ar0, read_trap);
    }
#endif

    /* Translate from AR6002 to AR60000 */
    if ((addr & 0xffff0000) == 0x00020000 /* MAC_BASE_ADDRESS */ ) {
        /* Convert from AR6002 XLMI/MAC address to to AR6000 MAC address */
        diag_read(MAC_BASE_ADDRESS | (addr & 0xffff), &value);
        if (addr == 0x000200c0 /* MAC_BASE_ADDRESS | AR_ISR_RAC */) {
            /* If WMAC RAC register, then sync interrupt status */
            sync_interrupt_status(0);
        }
    } else if (addr == 0x0000c034 /* UART_BASE_ADDRESS | SLSR_ADDRESS */ ) {
        value = uart_holding_register;
    } else if (addr == 0x00018000 /* MBOX_BASE_ADDRESS | MBOX_FIFO_ADDRESS */) {
        value = mboxpio_holding_register;
    } else {
        /* Normal SOC read */
        diag_read(AR6002_TO_AR6000(addr), &value);
    }

    sched_yield();
#if 0
    debug("SOC PEEK %d bytes from offset 0x%x, value=0x%x\n", size, addr, value);
#endif
    XTMP_insert32LE32(dst, addr, value);

    /*
     * Special post-processing -- whenever we read from INT_STATUS, we update
     * the Xtensa interrupt status before responding. Besides providing
     * state consistency, this supports interrupt polling in the idle loop.
     */
    if (addr == 0x00004050 /* RTC_BASE_ADDRESS|INT_STATUS_ADDRESS */) {
        update_interrupt_status(value);
    }

    return XTMP_DEVICE_OK;
}

/* ISS/debugger writes */
XTMP_deviceStatus
AR6002_SOC_XTMP_poke(void *notused,
                     XTMP_address addr,
                     u32 size,
                     const u32 *src)
{
    u32 value;

    if (size != 4) {
        debug("Ignore bogus SOC WRITE (addr=0x%x size=%d)\n", addr, size);
        return XTMP_DEVICE_ERROR;
    }

    value = XTMP_extract32LE32(src, addr);
    /* debug("SOC_XTMP_poke %d bytes (value=0x%x) to offset 0x%x\n", size, value, addr); */
    /* Translate from AR6002 to AR60000 */
    if ((addr & 0xffff0000) == 0x00020000 /* MAC_BASE_ADDRESS */ ) {
        /* Convert from AR6002 XLMI/MAC address to AR6K MAC address */
        diag_write(MAC_BASE_ADDRESS | (addr & 0xffff), value);
    } else if (addr == 0x0000c034 /* UART_BASE_ADDRESS | SLSR_ADDRESS */ ) {
        diag_read(LSR_ADDRESS, &uart_holding_register);
    } else if (addr == 0x000180f0 /* MBOX_BASE_ADDRESS | MBOX_TXFIFO_POP_ADDRESS */) {
        diag_read(MBOX_FIFO_ADDRESS, &mboxpio_holding_register);
    } else {
        /* Normal SOC write */
        diag_write(AR6002_TO_AR6000(addr), value);
    }

    sync_interrupt_status(0);

    return XTMP_DEVICE_OK;
}

/* Handle CPU loads/stores to MEMORY addresses */
XTMP_deviceStatus
AR6002_MEM_XTMP_post(void *notused,
                     XTMP_deviceXfer *xfer)
{
#if 0
    print_xfer("MEM", xfer);
#endif

    if (XTMP_xferGetType(xfer) == XTMP_READ) {
        AR6002_MEM_XTMP_peek(notused,
                             XTMP_xferGetData(xfer),
                             XTMP_xferGetAddress(xfer),
                             XTMP_xferGetSize(xfer));
    } else if (XTMP_xferGetType(xfer) == XTMP_WRITE) {
        AR6002_MEM_XTMP_poke(notused,
                             XTMP_xferGetAddress(xfer),
                             XTMP_xferGetSize(xfer),
                             XTMP_xferGetData(xfer));
    } else {
        debug("Unexpected Transfer Type: 0x%x\n", XTMP_xferGetType(xfer));
        error("AR6002_MEM_XTMP_post failed?!\n");
    }

    if (XTMP_respond(xfer, XTMP_OK) != XTMP_OK) {
        error("AR6002_MEM_XTMP_post: XTMP_respond failed?!\n");
    }

    return XTMP_DEVICE_OK;
}

/*
 * Convert from a physical address to an offset into simulated memory.
 * Returns 0 if physical address is OK; non-zero on bogus address.
 */
int
PHYSMEMADDR_TO_SIMOFF(XTMP_address addr, u32 *poffset)
{
    u32 offset;
    u32 physaddress = (u32)addr & 0x3fffff;

#if 1
    if ((physaddress >= 0xe0000) && (physaddress < 0xf4000)) {
        goto good_addr; /* ROM */
    }

    if ((physaddress >= 0x100000) && (physaddress < RAMEND)) {
        goto good_addr; /* RAM */
    }
#endif

    debug("PHYSMEMADDR_TO_SIMOFF rejects address=0x%x\n", addr);

    return 1;

good_addr:
    /* Memory Physical Space is a 21-bit space starting at 0x80000 */
    offset = ((physaddress) & 0x1fffff) - 0x80000;
    *poffset = offset;
    return 0;
}

/* ISS/debugger reads */
XTMP_deviceStatus
AR6002_MEM_XTMP_peek(void *notused,
                     u32 *dst,
                     XTMP_address addr,
                     u32 size)
{
    u8 *buf;
    u32 offset;
    u32 data_word;

    /* debug("PEEK %d bytes from offset 0x%x\n", size, addr); */
    if (PHYSMEMADDR_TO_SIMOFF(addr, &offset)) {
        return XTMP_DEVICE_ERROR;
    }

    if (offset < data_start_offset) {
        /* Service read-only data locally */
        buf = &mercmem[offset];
    } else if (offset < data_end_offset) {
        /* Service writable data from the AR6K system's memory. */
        diag_read((unsigned int)(offset-data_start_offset+0xa0000000),
                  &data_word);
        buf = &(((u8 *)&data_word)[offset & 3]);
        sched_yield();
    } else if (offset < RAMEND_OFFSET-FIXMAP_AREA_SZ) {
        /* Service text locally */
        buf = &mercmem[offset];
    } else {
        /* Service fixmap from the end of AR6K system's memory. */
        diag_read((unsigned int)(0xa0014000-RAMEND_OFFSET+offset), &data_word);
        buf = &(((u8 *)&data_word)[offset & 3]);
        sched_yield();
    }

    for (;size; size--, addr++, buf++) {
        XTMP_insert32LE8(dst, addr, *buf);
    }

    return XTMP_DEVICE_OK;
}

/* ISS/debugger writes */
XTMP_deviceStatus
AR6002_MEM_XTMP_poke(void *notused,
                     XTMP_address addr,
                     u32 size,
                     const u32 *src)
{
    u8 *buf;
    u32 offset;
    u32 data_word;
    unsigned int target_offset = 0;

    if (PHYSMEMADDR_TO_SIMOFF(addr, &offset)) {
        return XTMP_DEVICE_ERROR;
    }
    buf = &mercmem[offset];

    if (offset < data_start_offset) {
        /* Service read-only data locally */
        buf = &mercmem[offset];
    } else if (offset < data_end_offset) {
        /* Service writable data from the AR6K system's memory. */
        target_offset = (unsigned int)(offset-data_start_offset+0xa0000000);
        if (size < 4) {
            diag_read(target_offset, &data_word);
        }
        buf = &(((u8 *)&data_word)[offset & 3]);
    } else if (offset < RAMEND_OFFSET-FIXMAP_AREA_SZ) {
        /* Service text locally */
        buf = &mercmem[offset];
    } else  {
        /* Service DataSets from the fixmap of AR6K system's memory. */
        target_offset=(unsigned int)(0xa0014000-RAMEND_OFFSET+offset);
        if (size < 4) {
            diag_read(target_offset, &data_word);
        }
        buf = &(((u8 *)&data_word)[offset & 3]);
    }

    for (;size; size--, addr++, buf++) {
        *buf = XTMP_extract32LE8(src, addr);
    }

    if (target_offset) {
        diag_write(target_offset, data_word);
    }

    return XTMP_DEVICE_OK;
}
