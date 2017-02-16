/*
 * Copyright (c) 2002-2010 Atheros Communications, Inc.
 *
 * athtst.c: example code for IOCTL() control.
 *
 * Usage : athtst [interface_name] [command_name] [operation_name] [parameters]
 */
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <string.h>
#include <sys/errno.h>
#include <stdio.h>
#include <net/if.h>
#include <errno.h>
#include <math.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>


//#include <acfg_api.h>
//#include "../acfg_test_app/acfg_tool.h"


#include "athtst.h"
#include "ioctl_vendor_ce.h"      /* Vendor Include */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <string.h>
#include <sys/errno.h>
#include <stdio.h>
#include <net/if.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
/************************* SETTING ROUTINES **************************/
/*------------------------------------------------------------------*/
#define IS_WORD_ALIGNED(x) (x % 4 == 0)
#define    N(a)    (sizeof (a) / sizeof (a[0]))

/**
 * Display the command usage as user inputs a wrong command
 */
#define COMMAND_USAGE_INFO\
    do {\
        printf("!! ERROR !! Unacceptable command\n");\
        printf("Please enter a valid command as below -- \n");\
        printf("athtst [interface_name] [command_name] [operation_name] [parameters]\n");\
        printf("For Example -- athtst ath0 ssid set Atheros\n\n");\
    } while(0)

/**
 * @brief Handlers for product-info related commands.
 */
static athcfg_cmd_t athcfg_product_info_cmds[] =
{
    {
        .cmd_name = "get",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_product_info_get ,
        .cmd_usage = "product-info get" ,
        .min_args = 0 ,
        .max_args = 0 }},
    },
    {
        .cmd_name = "set",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = NULL ,
        .cmd_usage = "product-info set" ,
        .min_args = 0 ,
        .max_args = 0 }},
    },    
};
    
/**
 * @brief Handlers for TESTMODE-related commands.
 */
static  athcfg_cmd_t athcfg_testmode_cmds[] =
{
    {
        .cmd_name = "get",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_testmode_get ,
        .cmd_usage = "testmode get <cmd>" ,
        .min_args = 0 ,
        .max_args = 1 }},
    },
    {
        .cmd_name = "set",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_testmode_set ,
        .cmd_usage = "testmode set <cmd> <param>" ,
        .min_args = 1 ,
        .max_args = 3 }}, 
    },
};

/**
 * @brief Handlers for Register Read/Write-related commands 
 */
static athcfg_cmd_t athcfg_reg_cmds[] =
{
    {
        .cmd_name = "get",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_reg_get ,
        .cmd_usage = "reg get <reg_start_addr> [<reg_end_addr>]" ,
        .min_args = 1 ,
        .max_args = 2 }},
    },
    {
        .cmd_name = "set",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_reg_set ,
        .cmd_usage = "reg set <reg_addr> <reg_val>" ,
        .min_args = 2 , 
        .max_args = 2 }},
    },
};

/**
 * @brief Handlers for sta-info related commands.
 */
static  athcfg_cmd_t athcfg_stainfo_cmds[] =
{
    {
        .cmd_name = "get",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_stainfo_get ,
        .cmd_usage = "stainfo get" ,
        .min_args = 0 ,
        .max_args = 0 }},
    },
    {
        .cmd_name = "set",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = NULL ,
        .cmd_usage = "stainfo set" ,
        .min_args = 0 ,
        .max_args = 0 }},
    },    
};

/**
 * @brief Handlers for scantime commands.
 */
static  athcfg_cmd_t athcfg_scantime_cmds[] =
{
    {
        .cmd_name = "get",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_scantime_get ,
        .cmd_usage = "scantime get" ,
        .min_args = 0 ,
        .max_args = 0 }},
    },  
    {
        .cmd_name = "set",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = NULL ,
        .cmd_usage = "scantime set" ,
        .min_args = 0 ,
        .max_args = 0 }}, 
    },
};

/**
 * @brief Handlers for phy-mode related commands.
 */
static  athcfg_cmd_t athcfg_mode_cmds[] =
{
    {
        .cmd_name = "get",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_mode_get ,
        .cmd_usage = "mode get" ,
        .min_args = 0 ,
        .max_args = 0 }},
    },
    {
        .cmd_name = "set",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = NULL ,
        .cmd_usage = "mode set" ,
        .min_args = 0 ,
        .max_args = 0 }},
    },    
};

/**
 * @brief Handlers for SCAN-related commands.
 */
static athcfg_cmd_t athcfg_scan_cmds[] =
{
    {
        .cmd_name = "get", 
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_scan_get, 
        .cmd_usage = "scan get" ,
        .min_args = 0 ,
        .max_args = 0 }},
    },
    {
        .cmd_name = "set",
        .handler_present = TRUE, 
        .u = { .hdlr = { .fn = athcfg_scan_set ,
        .cmd_usage = "scan set" ,
        .min_args = 0 , 
        .max_args = 0 }}, 
    },
};

/**
 * @brief Handlers for product-info related commands.
 */
static athcfg_cmd_t athcfg_version_info_cmds[] =
{
    {
        .cmd_name = "get",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_version_info_get ,
        .cmd_usage = "version-info get" ,
        .min_args = 0 ,
        .max_args = 0 }},
    },
    {
        .cmd_name = "set",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = NULL ,
        .cmd_usage = "version-info set" ,
        .min_args = 0 ,
        .max_args = 0 }},
    },
};

/**
 * @brief Handlers for TXPOWER-related commands.
 */
static athcfg_cmd_t athcfg_txpower_cmds[] =
{
    {
        .cmd_name = "get",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_txpower_get ,
        .cmd_usage = "txpower get" ,
        .min_args = 0 ,
        .max_args = 0 }},
    },
    {
        .cmd_name = "set",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = NULL ,
        .cmd_usage = "txpower set" ,
        .min_args = 0 ,
        .max_args = 0 }}, 
    },
};

/**
 * @brief Handlers for STATS-related commands.
 */
static athcfg_cmd_t athcfg_stats_cmds[] =
{
    {
        .cmd_name = "get",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_stats_get ,
        .cmd_usage = "stats get" ,
        .min_args = 0 ,
        .max_args = 0 }},
    },
    {
        .cmd_name = "set",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = NULL ,
        .cmd_usage = "stats set" ,
        .min_args = 0 ,
        .max_args = 0 }}, 
    },
};

/**
 * @brief Handlers for STATS-related commands.
 */
static athcfg_cmd_t suspend_cmds[] =
{
    {
        .cmd_name = "set",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = suspend_set ,
        .cmd_usage = "suspend <val>" ,
        .min_args = 1 , 
        .max_args = 1 }},
    },
};

#if defined(CE_CUSTOM_1)
/**
 * @brief Handlers for WiDi mode commands 
 */
static athcfg_cmd_t athcfg_widi_cmds[] =
{
    {
        .cmd_name = "set",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_widi_mode_set ,
        .cmd_usage = "widi set <1/0>" ,
        .min_args = 1 ,
        .max_args = 1 }},
    },
    {
        .cmd_name = "get",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = athcfg_widi_mode_get ,
        .cmd_usage = "widi get" ,
        .min_args = 0 , 
        .max_args = 0 }},
    },
};
#endif
/**
 * @brief Handlers for channel-related commands.
 */
static athcfg_cmd_t chan_cmds[] =
{
    {
        .cmd_name = "get",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = chans_get ,
        .cmd_usage = "chans get" ,
        .min_args = 0 , 
        .max_args = 0 }},
    },
};

/**
 * @brief Handlers for csa-related commands.
 */
static athcfg_cmd_t csa_cmds[] =
{
    {
        .cmd_name = "set",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = csa_set ,
        .cmd_usage = "csa set <freq>" ,
        .min_args = 1 , 
        .max_args = 1 }},	
    },
};

/**
 * @brief Handlers for ctl-related commands.
 */
static athcfg_cmd_t ctl_cmds[] =
{
    {
        .cmd_name = "get",
        .handler_present = TRUE,
        .u = { .hdlr = { .fn = ctls_get ,
        .cmd_usage = "ctls get <freq | auto>" ,
        .min_args = 1 , 
        .max_args = 1 }},	
    },
};
/**
 * @brief Handlers for first-level commands.
 */ 
static athcfg_cmd_t athcfg_cmds[] = 
{
    {
         .cmd_name = "product-info",
         .handler_present = FALSE,
         .u.next_level = athcfg_product_info_cmds,
    },
    {
         .cmd_name = "testmode",
         .handler_present = FALSE,
         .u.next_level = athcfg_testmode_cmds,
    }, 
    {
         .cmd_name = "reg",
         .handler_present = FALSE,
         .u.next_level = athcfg_reg_cmds,
    },
    {
         .cmd_name = "stainfo",
         .handler_present = FALSE,
         .u.next_level = athcfg_stainfo_cmds,
    }, 
    {
         .cmd_name = "scantime",
         .handler_present = FALSE,
         .u.next_level = athcfg_scantime_cmds,
    }, 
    {
         .cmd_name = "mode",
         .handler_present = FALSE,
         .u.next_level = athcfg_mode_cmds,
    },  
    {
         .cmd_name = "scan",
         .handler_present = FALSE,
         .u.next_level = athcfg_scan_cmds,
    },    
    {
         .cmd_name = "version-info",
         .handler_present = FALSE,
         .u.next_level = athcfg_version_info_cmds,
    },
    {
         .cmd_name = "txpower",
         .handler_present = FALSE,
         .u.next_level = athcfg_txpower_cmds,
    },
    {
         .cmd_name = "stats",
         .handler_present = FALSE,
         .u.next_level = athcfg_stats_cmds,
    },
    {
         .cmd_name = "suspend",
         .handler_present = FALSE,
         .u.next_level = suspend_cmds,
    },
#if defined(CE_CUSTOM_1)
    {
         .cmd_name = "widi",
         .handler_present = FALSE,
         .u.next_level = athcfg_widi_cmds,
    }, 
#endif
    {
         .cmd_name = "chans",
         .handler_present = FALSE,
         .u.next_level = chan_cmds,
    },
	{
         .cmd_name = "csa",
         .handler_present = FALSE,
         .u.next_level = csa_cmds,
    },
    {
         .cmd_name = "ctls",
         .handler_present = FALSE,
         .u.next_level = ctl_cmds,
    },
};

/*****************************************************************************
 *                      Helper functions                                     *
 *****************************************************************************/
/**
 * @brief Looks for Next Level
 */
athcfg_cmd_t * 
athcfg_cmd_lookup(athcfg_cmd_t prev_level[], int size, char *key)
{
    int i = 0;
    for(i = 0; i < size; i++){
        if(!(strcmp(prev_level[i].cmd_name, key))) {
            if (! (prev_level[i].handler_present))
                return((prev_level[i].u.next_level));
            else 
                return (&prev_level[i]);
        }
    }
    return 0;
}

/**
 * @brief Counting the number of elements in an ARRAY
 */
int
athcfg_arr_nelem(athcfg_cmd_t *prev_level)
{
   if(! (strcmp(prev_level->cmd_name,"get") ))
        return(2);
    else
        return(sizeof(athcfg_cmds) /sizeof(athcfg_cmds[0]) );
}

/**
 * @brief Display the list of commands supported 
 */
static void 
athcfg_usage(athcfg_cmd_t *cmds)
{
    /*
     * Cycle through and print all possible usage strings from this level on.
     */
    int i = 0,j = 0;
    int space = 0;
    athcfg_cmd_t *next_level;
 
    printf("athtst Usage:-- \n");   
    
    for(i = 0; i < (sizeof(athcfg_cmds)/sizeof(athcfg_cmds[0]))  ; i++) {
    if (!cmds[i].handler_present)
        printf(" %s",cmds[i].cmd_name);
        next_level = cmds[i].u.next_level;
    
        for(j = 0; j < 2; j++) {
            if(j == 0){
            for(space = 0; space <= 25-(strlen(cmds[i].cmd_name)); space++) 
                printf(" ");   
                printf("[");       
            }   
            if (next_level[j].handler_present)
                printf(" %s ",next_level[j].cmd_name);

            if ( (next_level[0].handler_present) && (next_level[1].handler_present) && j == 0 )
                printf("|");
        }
        printf("]\n");
    }
}

/**
 * @brief Call IOCTL in SHIM
 */
static int
athcfg_do_ioctl(int number, athcfg_wcmd_t *i_req)
{
    int s;
    struct ifreq ifr;

    /* Assign Vendor ID */
    i_req->iic_vendor = ATHCFG_WCMD_VENDORID;
        
    strcpy(ifr.ifr_name, i_req->iic_ifname);
    ifr.ifr_data = (void *)i_req;

    /* Try to open the socket, if success returns it */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if(s < 0) {
        fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
        return -1;
    }

    if ((ioctl (s,number, &ifr)) !=0) {
        fprintf(stderr, "Error doing ioctl(): %s\n", strerror(errno));
		close(s);
        return -1;
    }
    close(s);
    
    return 0;
}

/** 
 *@brief The main program starts
 */
int
main(int argc, char *argv[])
{
    athcfg_cmd_t *level = NULL;
    athcfg_cmd_t *prev_level = NULL;
    int num_elems = 0;
    int check1 = 0;
    int check2 = 0;
    char *ifrn_name;
    int i = 0; 

    if (argc <=1) {
        athcfg_usage(athcfg_cmds);
        return 0;
    }

    /* Check the validity of the user input */
    if (argc <= 3) {
        fprintf(stderr, "Too few args!  ");
        COMMAND_USAGE_INFO;
        athcfg_usage(athcfg_cmds);
        return EINVAL;
    }

    /* take the interface for further processing */
    ifrn_name = argv[1];

    /* check the validity of command_name */
    for(i = 0; i < (sizeof(athcfg_cmds)/sizeof(athcfg_cmds[0])) ; i++) {
        if( !(strcmp (argv[2], athcfg_cmds[i].cmd_name)) ) {
            check1 = 1; 
            break;
        }     
    }

    if(!(check1)) { 
        COMMAND_USAGE_INFO;    
        athcfg_usage(athcfg_cmds);
        return EINVAL;  
    }
   
    if(!(strcmp(argv[3], "get")) || !(strcmp(argv[3], "set")) || !(strcmp(argv[3], "enable")) || !(strcmp(argv[3], "disable")))  
        check2 = 1;

    if(!(check2)) {
        COMMAND_USAGE_INFO;
        athcfg_usage(athcfg_cmds);
        return EINVAL;
    }

    /*
     * Start at the top-most level and stop when a handler is found
     */ 
    prev_level = &athcfg_cmds[0];
    for (i = 2; i <= 3; i++) {
        num_elems = athcfg_arr_nelem(prev_level);
        level = athcfg_cmd_lookup(prev_level, num_elems, argv[i]);
        if (level == NULL) {
            fprintf(stderr, "Invalid arg %s!\n", argv[i]);
            athcfg_usage(athcfg_cmds);
            return EINVAL;
        }
        prev_level = level;
    }
    /*
     * Pass all remaining args as long as they are within bounds.
     */
    if (argc - i < level->h_min_args || argc - i > level->h_max_args) {
        fprintf(stderr, "Unacceptable arg count: minimum argument --  %d\n",\
                              level->h_min_args);
        fprintf(stderr,"            maximum argument --  %d\n",\
                              level->h_max_args);
        athcfg_usage(athcfg_cmds);
        return EINVAL;
    }

    if (level->h_fn == NULL) {
        fprintf(stderr, "operatio not yet support!\n");
        return EINVAL;
    }
        
    return level->h_fn(argv[1], argc - i, &argv[i]);
}

/*****************************************************************************
 *                      Command Handlers                                     *  
 *****************************************************************************/
/**
 * @brief Handlers for get Product-Info.
 */
int
athcfg_product_info_get(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req;
    athcfg_wcmd_product_info_t product_info;
    
    memset(&i_req, 0, sizeof(athcfg_wcmd_t));
    memset(&product_info, 0, sizeof(athcfg_wcmd_product_info_t));

    assert(nargs == 0);
    args[0] = NULL;
    
    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_GET_DEV_PRODUCT_INFO;
    i_req.d_productinfo = product_info;

    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
     return EIO;

    printf("idVendor       -- 0x%x\n", i_req.d_productinfo.idVendor);
    printf("idProduct      -- 0x%x\n", i_req.d_productinfo.idProduct);
    printf("product        -- %s\n", i_req.d_productinfo.product);
    printf("manufacturer   -- %s\n", i_req.d_productinfo.manufacturer);
    printf("serial         -- %s\n", i_req.d_productinfo.serial);

    return 0;
}

/**
 * @brief Handlers for Get/Set TESTMODE.
 */
int
athcfg_testmode_set(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req; 
    athcfg_wcmd_testmode_t testmode; 

    memset(&i_req,0, sizeof(athcfg_wcmd_t));
    memset(&testmode, 0, sizeof(athcfg_wcmd_testmode_t));
    
    if (!strcmp(args[0], "bssid")) {
        testmode.operation = ATHCFG_WCMD_TESTMODE_BSSID;
        
        if((strlen(args[1]) == ATHCFG_WCMD_MAC_STR_LEN)) {
            unsigned int addr[ATHCFG_WCMD_ADDR_LEN];
            int i;
            
            sscanf(args[1],"%x:%x:%x:%x:%x:%x",(unsigned int *)&addr[0],\
            (unsigned int *)&addr[1],\
            (unsigned int *)&addr[2],\
            (unsigned int *)&addr[3],\
            (unsigned int *)&addr[4],\
            (unsigned int *)&addr[5] );

            for(i=0; i<ATHCFG_WCMD_ADDR_LEN; i++) {
                testmode.bssid[i] = addr[i];
            }
        }
    }
    else if (!strcmp(args[0], "chan")) {
        testmode.operation = ATHCFG_WCMD_TESTMODE_CHAN;
        testmode.chan = atoi(args[1]); 
    }
    else if (!strcmp(args[0], "rx")) {
        testmode.operation = ATHCFG_WCMD_TESTMODE_RX;
        testmode.rx = atoi(args[1]); 
    }
    else if (!strcmp(args[0], "ant")) {
        testmode.operation = ATHCFG_WCMD_TESTMODE_ANT;
        testmode.antenna = atoi(args[1]); 
    }    
    else {
        printf("!! ERROR !! \n");
        printf("Choose one from the list below -->\n");
        printf("                 bssid [BSSID]\n");
        printf("                 chan [ChanID]\n");
        printf("                 rx [1|0]\n");
        printf("                 ant [0|1|2]\n");
        return 0;
    }
    
    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_SET_TESTMODE;
    i_req.d_testmode = testmode;

    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
        return EIO;

    return 0;
}

int
athcfg_testmode_get(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req; 
    athcfg_wcmd_testmode_t testmode; 

    memset(&i_req,0, sizeof(athcfg_wcmd_t));
    memset(&testmode, 0, sizeof(athcfg_wcmd_testmode_t));

	if(nargs == 0){
        printf("!! ERROR !! \n");
		printf("Choose one from the list below -->\n");
        printf("                 bssid \n");
        printf("                 chan \n");
        printf("                 rx \n");
        printf("                 result \n");
        printf("                 ant \n");
        return 0;
	}
    
    if (!strcmp(args[0], "bssid")) {
        testmode.operation = ATHCFG_WCMD_TESTMODE_BSSID;
    }
    else if (!strcmp(args[0], "chan")) {
        testmode.operation = ATHCFG_WCMD_TESTMODE_CHAN;
    }
    else if (!strcmp(args[0], "rx")) {
        testmode.operation = ATHCFG_WCMD_TESTMODE_RX;
    }
    else if (!strcmp(args[0], "result")) {
        testmode.operation = ATHCFG_WCMD_TESTMODE_RESULT;
    }     
    else if (!strcmp(args[0], "ant")) {
        testmode.operation = ATHCFG_WCMD_TESTMODE_ANT;
    }

    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_GET_TESTMODE;
    i_req.d_testmode = testmode;
    
    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
        return EIO;

    if (!strcmp(args[0], "bssid")) {
        printf("bssid=%02x:%02x:%02x:%02x:%02x:%02x\n", 
            i_req.d_testmode.bssid[0],
            i_req.d_testmode.bssid[1],
            i_req.d_testmode.bssid[2],
            i_req.d_testmode.bssid[3],
            i_req.d_testmode.bssid[4],
            i_req.d_testmode.bssid[5]);
    }
    else if (!strcmp(args[0], "chan")) {
        printf("chan=%d\n", i_req.d_testmode.chan);
    }
    else if (!strcmp(args[0], "rx")) {
        printf("rx=%d\n", i_req.d_testmode.rx);
    }
    else if (!strcmp(args[0], "result")) {        
        printf("rssi=%d, %d, %d, %d\n", 
                    i_req.d_testmode.rssi_combined,
                    i_req.d_testmode.rssi0, 
                    i_req.d_testmode.rssi1,
                    i_req.d_testmode.rssi2);
        
    }     
    else if (!strcmp(args[0], "ant")) {
        printf("ant=%d\n", i_req.d_testmode.antenna);
    }     
    
    return 0;
}

/**
 * @brief Handlers for Read/Write Register.
 */
int 
athcfg_reg_get(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req;
    athcfg_wcmd_reg_t reg;
    a_uint32_t start_addr, end_addr;
    int ret;
        
    memset(&i_req,0, sizeof(athcfg_wcmd_t));
    memset(&reg, 0, sizeof(athcfg_wcmd_reg_t));

    ret = sscanf(args[0], "0x%x", &start_addr);
    if (ret != 1) {
        fprintf(stderr, "Register address must be in hex\n");
        return 0; 
    }

    if (nargs == 2) {
        ret = sscanf(args[1], "0x%x", &end_addr);
        if (ret != 1) {
           fprintf(stderr, "Register address must be in hex\n");
           return 0; 
        }
    } else {
        end_addr = start_addr;
    }

    if (!IS_WORD_ALIGNED(start_addr) || !IS_WORD_ALIGNED(end_addr)) {
        fprintf(stderr, "Register addresses must be on a 4-byte boundary\n");
        return 0;
    }

    if (end_addr < start_addr) {
        fprintf(stderr, "end address must be greater than the start address\n");
        return 0;
    }

    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));

    /* to have upper limit for end_addr to avoid Coverity checking failure */
#define UPPER_LIMIT 0x100000
    if (end_addr >= start_addr + UPPER_LIMIT) {
        fprintf(stderr, "end address must be smaller than 0x%x\n",
            start_addr + UPPER_LIMIT);
        return 0;
    }

    for (reg.addr = start_addr; reg.addr <= end_addr; reg.addr += 4) {
        i_req.iic_cmd = ATHCFG_WCMD_GET_REG;
        i_req.d_reg = reg;
        if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
            return EIO;
        printf("reg_addr: 0x%x reg_val: 0x%x\n", reg.addr, i_req.d_reg.val);
    }

    return 0;
}

int 
athcfg_reg_set(char *ifrn_name, int nargs, char *args[])
{
#if 1//add by randy
   athcfg_wcmd_t i_req;
    athcfg_wcmd_reg_t reg;
    int ret1, ret2;

    memset(&i_req,0, sizeof(athcfg_wcmd_t));
    memset(&reg, 0, sizeof(athcfg_wcmd_reg_t));

    assert(nargs == 2);

    ret1 = sscanf(args[0], "0x%x", &reg.addr);
    ret2 = sscanf(args[1], "0x%x", &reg.val);
    if (ret1 != 1 || ret2 != 1) {
        fprintf(stderr, "Register address and value must be in hex\n");
        return 0;
    }

    if (!IS_WORD_ALIGNED(reg.addr)) {
        fprintf(stderr, "Register addresses must be on a 4-byte boundary\n");
        return 0;
    }

    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_SET_REG;
    i_req.d_reg = reg;

    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
        return EIO;

    printf("reg_addr: 0x%x set to reg_val: 0x%x\n", reg.addr, reg.val);

    return 0;
#else
   // a_uint32_t offset;
   // a_uint32_t value;
    a_status_t status = A_STATUS_FAILED;
	athcfg_wcmd_reg_t reg;
	int ret1, ret2;
    ret1 = sscanf(args[0], "0x%x", &reg.addr);
    ret2 = sscanf(args[1], "0x%x", &reg.val);
    if (ret1 != 1 || ret2 != 1) {
        fprintf(stderr, "Register address and value must be in hex\n");
        return 0;
    }
	
    if (!IS_WORD_ALIGNED(reg.addr)) {
        fprintf(stderr, "Register addresses must be on a 4-byte boundary\n");
        return 0;
    }	
    status = acfg_set_reg((a_uint8_t *)ifrn_name, reg.addr, reg.val);

    return acfg_to_os_status(status) ;
#endif
}

/**
 * @brief Handlers for get sta-info.
 */
typedef struct _dot11nRateInfo {
#define RATEINFO_FLAGS_20M       0x0001    
#define RATEINFO_FLAGS_40M       0x0002    
#define RATEINFO_FLAGS_SHORTGI   0x0004
    unsigned long rateFlags;
    unsigned long rateKbps;
    unsigned int mcs;
} dot11nRateInfo_t;    

static int
_stainfo_get_rateinfo(unsigned long rate, unsigned char mcs, int *is20M, int *isSGI)
{
    dot11nRateInfo_t ath_11nRateInfo[] = {
     /* Ref. : 802.11n 2009 20.6 - Parameters for HT MCSs*/
     /* 20 MHz, N=1*/
     /*  6.5 Mb, BPSK  1/2      */ { (RATEINFO_FLAGS_20M),                         6500, 0x0 },
     /*  7.2 Mb, BPSK  1/2, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI),  7200, 0x0 },
     /*   13 Mb, QPSK  1/2      */ { (RATEINFO_FLAGS_20M),                        13000, 0x1 },    
     /* 14.4 Mb, QPSK  1/2, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI), 14400, 0x1 },    
     /* 19.5 Mb, QPSK  3/4      */ { (RATEINFO_FLAGS_20M),                        19500, 0x2 },    
     /* 21.7 Mb, QPSK  3/4, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI), 21700, 0x2 },    
     /*   26 Mb, 16QAM 1/2      */ { (RATEINFO_FLAGS_20M),                        26000, 0x3 },    
     /* 28.9 Mb, 16QAM 1/2, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI), 28900, 0x3 },    
     /*   39 Mb, 16QAM 3/4      */ { (RATEINFO_FLAGS_20M),                        39000, 0x4 },    
     /* 43.3 Mb, 16QAM 3/4, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI), 43300, 0x4 },    
     /*   52 Mb, 64QAM 2/3      */ { (RATEINFO_FLAGS_20M),                        52000, 0x5 },    
     /* 57.8 Mb, 64QAM 2/3, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI), 57800, 0x5 },    
     /* 58.5 Mb, 64QAM 3/4      */ { (RATEINFO_FLAGS_20M),                        58500, 0x6 },    
     /*   65 Mb, 64QAM 3/4, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI), 65000, 0x6 },    
     /*   65 Mb, 64QAM 5/6      */ { (RATEINFO_FLAGS_20M),                        65000, 0x7 },    
     /* 72.2 Mb, 64QAM 5/6, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI), 72200, 0x7 },    
     /* 20 MHz, N=2*/     
     /*   13 Mb, BPSK  1/2      */ { (RATEINFO_FLAGS_20M),                        13000, 0x8 },
     /* 14.4 Mb, BPSK  1/2, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI), 14400, 0x8 },
     /*   26 Mb, QPSK  1/2      */ { (RATEINFO_FLAGS_20M),                        26000, 0x9 },    
     /* 28.9 Mb, QPSK  1/2, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI), 28900, 0x9 },    
     /*   39 Mb, QPSK  3/4      */ { (RATEINFO_FLAGS_20M),                        39000, 0xa },    
     /* 43.3 Mb, QPSK  3/4, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI), 43300, 0xa },    
     /*   52 Mb, 16QAM 1/2      */ { (RATEINFO_FLAGS_20M),                        52000, 0xb },    
     /* 57.8 Mb, 16QAM 1/2, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI), 57800, 0xb },    
     /*   78 Mb, 16QAM 3/4      */ { (RATEINFO_FLAGS_20M),                        78000, 0xc },    
     /* 86.7 Mb, 16QAM 3/4, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI), 86700, 0xc },    
     /*  104 Mb, 64QAM 2/3      */ { (RATEINFO_FLAGS_20M),                       104000, 0xd },    
     /*115.6 Mb, 64QAM 2/3, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI),115600, 0xd },    
     /*  117 Mb, 64QAM 3/4      */ { (RATEINFO_FLAGS_20M),                       117000, 0xe },    
     /*  130 Mb, 64QAM 3/4, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI),130000, 0xe },    
     /*  130 Mb, 64QAM 5/6      */ { (RATEINFO_FLAGS_20M),                       130000, 0xf },    
     /*144.4 Mb, 64QAM 5/6, SGI */ { (RATEINFO_FLAGS_20M|RATEINFO_FLAGS_SHORTGI),144400, 0xf },    
     /* 40 MHz, N=1*/
     /* 13.5 Mb, BPSK  1/2      */ { (RATEINFO_FLAGS_40M),                        13500, 0x0 },
     /*   15 Mb, BPSK  1/2, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI), 15000, 0x0 },
     /*   27 Mb, QPSK  1/2      */ { (RATEINFO_FLAGS_40M),                        27000, 0x1 },    
     /*   30 Mb, QPSK  1/2, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI), 30000, 0x1 },    
     /* 40.5 Mb, QPSK  3/4      */ { (RATEINFO_FLAGS_40M),                        40500, 0x2 },    
     /*   45 Mb, QPSK  3/4, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI), 45000, 0x2 },    
     /*   54 Mb, 16QAM 1/2      */ { (RATEINFO_FLAGS_40M),                        54000, 0x3 },    
     /*   60 Mb, 16QAM 1/2, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI), 60000, 0x3 },    
     /*   81 Mb, 16QAM 3/4      */ { (RATEINFO_FLAGS_40M),                        81000, 0x4 },    
     /*   90 Mb, 16QAM 3/4, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI), 90000, 0x4 },    
     /*  108 Mb, 64QAM 2/3      */ { (RATEINFO_FLAGS_40M),                       108000, 0x5 },    
     /*  120 Mb, 64QAM 2/3, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI),120000, 0x5 },    
     /*121.5 Mb, 64QAM 3/4      */ { (RATEINFO_FLAGS_40M),                       121500, 0x6 },    
     /*  135 Mb, 64QAM 3/4, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI),135000, 0x6 },    
     /*  135 Mb, 64QAM 5/6      */ { (RATEINFO_FLAGS_40M),                       135000, 0x7 },    
     /*  150 Mb, 64QAM 5/6, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI),150000, 0x7 },    
     /* 40 MHz, N=2*/     
     /*   27 Mb, BPSK  1/2      */ { (RATEINFO_FLAGS_40M),                        27000, 0x8 },
     /*   30 Mb, BPSK  1/2, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI), 30000, 0x8 },
     /*   54 Mb, QPSK  1/2      */ { (RATEINFO_FLAGS_40M),                        54000, 0x9 },    
     /*   60 Mb, QPSK  1/2, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI), 60000, 0x9 },    
     /*   81 Mb, QPSK  3/4      */ { (RATEINFO_FLAGS_40M),                        81000, 0xa },    
     /*   90 Mb, QPSK  3/4, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI), 90000, 0xa },    
     /*  108 Mb, 16QAM 1/2      */ { (RATEINFO_FLAGS_40M),                       108000, 0xb },    
     /*  120 Mb, 16QAM 1/2, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI),120000, 0xb },    
     /*  162 Mb, 16QAM 3/4      */ { (RATEINFO_FLAGS_40M),                       162000, 0xc },    
     /*  180 Mb, 16QAM 3/4, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI),180000, 0xc },    
     /*  216 Mb, 64QAM 2/3      */ { (RATEINFO_FLAGS_40M),                       216000, 0xd },    
     /*  240 Mb, 64QAM 2/3, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI),240000, 0xd },    
     /*  243 Mb, 64QAM 3/4      */ { (RATEINFO_FLAGS_40M),                       243000, 0xe },    
     /*  270 Mb, 64QAM 3/4, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI),270000, 0xe },    
     /*  270 Mb, 64QAM 5/6      */ { (RATEINFO_FLAGS_40M),                       270000, 0xf },    
     /*  300 Mb, 64QAM 5/6, SGI */ { (RATEINFO_FLAGS_40M|RATEINFO_FLAGS_SHORTGI),300000, 0xf },             
    };
    int i, rateCnt = N(ath_11nRateInfo);

    *is20M = *isSGI = 0;
    for (i = 0; i < rateCnt; i++)
    {
        if ((rate == ath_11nRateInfo[i].rateKbps) &&
            (mcs == ath_11nRateInfo[i].mcs))
        {
            *is20M = ((ath_11nRateInfo[i].rateFlags & RATEINFO_FLAGS_20M) ? 1 : 0);
            *isSGI = ((ath_11nRateInfo[i].rateFlags & RATEINFO_FLAGS_SHORTGI) ? 1 : 0);

            break;
        }
    }

    if (i == rateCnt)
        return -1;
    else
        return 0;
}

int
athcfg_stainfo_get(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req;
    athcfg_wcmd_sta_t stainfo;
    
    memset(&i_req, 0, sizeof(athcfg_wcmd_t));
    memset(&stainfo, 0, sizeof(athcfg_wcmd_sta_t));

    assert(nargs == 0);
    args[0] = NULL;
    
    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_GET_STAINFO;
    i_req.d_stainfo = stainfo;

    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
        return EIO;

    printf("Link-%s  \n", (i_req.d_stainfo.flags & ATHCFG_WCMD_STAINFO_LINKUP) ? "UP" : "DOWN");
    printf("BSSID      %02x:%02x:%02x:%02x:%02x:%02x  \n", i_req.d_stainfo.bssid[0],
                                                           i_req.d_stainfo.bssid[1],
                                                           i_req.d_stainfo.bssid[2],
                                                           i_req.d_stainfo.bssid[3],
                                                           i_req.d_stainfo.bssid[4],
                                                           i_req.d_stainfo.bssid[5]);
    printf("Assoc-time %d  \n", i_req.d_stainfo.assoc_time);
    printf("4way-time  %d  \n", i_req.d_stainfo.wpa_4way_handshake_time);
    printf("2way-time  %d  \n", i_req.d_stainfo.wpa_2way_handshake_time);
    printf("RX-rate    %d  ", i_req.d_stainfo.rx_rate_kbps); 
    if (i_req.d_stainfo.rx_rate_mcs != ATHCFG_WCMD_STAINFO_MCS_NULL)
    {   /*11n rate*/
        int is20M, isSGI;
        
        if (_stainfo_get_rateinfo(i_req.d_stainfo.rx_rate_kbps, i_req.d_stainfo.rx_rate_mcs, &is20M, &isSGI) == 0)
            printf("[MCS  %d %s %s ]", i_req.d_stainfo.rx_rate_mcs, 
                                         (is20M ? "- 20M" : "- 40M"),
                                         (isSGI ? "- Short GI" : ""));
        else
            printf("[MCS  %d - Error ]", i_req.d_stainfo.rx_rate_mcs);
    }
    printf("\n");  
    printf("TX-rate    %d  \n", i_req.d_stainfo.tx_rate_kbps); 
    printf("RX-RSSI           %d  \n", i_req.d_stainfo.rx_rssi);
    printf("RX-Beacon-RSSI    %d  \n", i_req.d_stainfo.rx_rssi_beacon);
    printf("Short GI   %s  \n", (i_req.d_stainfo.flags & ATHCFG_WCMD_STAINFO_SHORTGI) ? "Enable" : "Disable");
    printf("Phy-Mode   ");
    if(i_req.d_stainfo.phymode == ATHCFG_WCMD_PHYMODE_11A)
        printf("11A\n");
    else if (i_req.d_stainfo.phymode == ATHCFG_WCMD_PHYMODE_11B)
        printf("11B\n");
    else if (i_req.d_stainfo.phymode == ATHCFG_WCMD_PHYMODE_11G)
        printf("11G\n");
    else if (i_req.d_stainfo.phymode == ATHCFG_WCMD_PHYMODE_11NA_HT20)
        printf("11NA HT20\n");
    else if (i_req.d_stainfo.phymode == ATHCFG_WCMD_PHYMODE_11NA_HT40)
        printf("11NA HT40\n");
    else if (i_req.d_stainfo.phymode == ATHCFG_WCMD_PHYMODE_11NG_HT20)
        printf("11NG HT20\n");
    else if (i_req.d_stainfo.phymode == ATHCFG_WCMD_PHYMODE_11NG_HT40)
        printf("11NG HT40\n");
    else
        printf("UNKNOWN\n");  
    
    return 0;
}

/**
 * @brief Handlers for get scantime.
 */
int
athcfg_scantime_get(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req;
    athcfg_wcmd_scantime_t scantime;
    
    memset(&i_req, 0, sizeof(athcfg_wcmd_t));
    memset(&scantime, 0, sizeof(athcfg_wcmd_scantime_t));
    
    assert(nargs == 0);
    args[0] = NULL;
    
    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_GET_SCANTIME;
    i_req.d_scantime = scantime;

    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
     return EIO;

    printf("Scan-time %d  \n", i_req.d_scantime.scan_time);
 
    return 0; 
}

/**
 * @brief Handlers for get phy-mode.
 */
int
athcfg_mode_get(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req;
    athcfg_wcmd_phymode_t phymode;
    
    memset(&i_req, 0, sizeof(athcfg_wcmd_t));
    memset(&phymode, 0, sizeof(athcfg_wcmd_phymode_t));

    assert(nargs == 0);
    args[0] = NULL;
    
    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_GET_MODE;
    i_req.d_phymode = phymode;

    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
     return EIO;

    printf("MODE -- ");
    if(i_req.d_phymode == ATHCFG_WCMD_PHYMODE_11A)
        printf("11A\n");
    else if (i_req.d_phymode == ATHCFG_WCMD_PHYMODE_11B)
        printf("11B\n");
    else if (i_req.d_phymode == ATHCFG_WCMD_PHYMODE_11G)
        printf("11G\n");
    else if (i_req.d_phymode == ATHCFG_WCMD_PHYMODE_11NA_HT20)
        printf("11NA HT20\n");
    else if (i_req.d_phymode == ATHCFG_WCMD_PHYMODE_11NA_HT40)
        printf("11NA HT40\n");
    else if (i_req.d_phymode == ATHCFG_WCMD_PHYMODE_11NG_HT20)
        printf("11NG HT20\n");
    else if (i_req.d_phymode == ATHCFG_WCMD_PHYMODE_11NG_HT40)
        printf("11NG HT40\n");
    else
        printf("Invalid return from driver\n");     

    return 0; 
}

/**
 * @brief Handlers for get/set scan.
 */
int
athcfg_scan_get(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req;
    athcfg_wcmd_scan_t *scan;
    athcfg_wcmd_scan_result_t *result;
    a_uint8_t offset = 0, loop = 0, count, i;
    a_uint8_t cnt = 0;

    scan = malloc(sizeof(athcfg_wcmd_scan_t));
    memset(&i_req, 0, sizeof(athcfg_wcmd_t));
    memset(scan, 0, sizeof(athcfg_wcmd_scan_t));

    assert(nargs == 0);
    args[0] = NULL;
    
    printf("Scan Result for interface [%s] is as below --\n",ifrn_name);
    do
    {  
        if (loop == 0)
            scan->cnt = scan->more = scan->offset = 0;
    
        strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
        i_req.iic_cmd = ATHCFG_WCMD_GET_SCAN;
        i_req.d_scan = scan;

        loop++;
        if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
            return EIO;
        
        offset += i_req.d_scan->cnt;
        scan->offset = offset;
        
        if (i_req.d_scan->cnt) 
        {
            for(count = 0; count < i_req.d_scan->cnt ; count++) 
            {
                result = &i_req.d_scan->result[count];
                printf("[%02d] %s \n", cnt++, result->isr_ssid);
                printf("    BSSID %02x:%02x:%02x:%02x:%02x:%02x ", 
                            result->isr_bssid[0], result->isr_bssid[1], result->isr_bssid[2],
                            result->isr_bssid[3], result->isr_bssid[4], result->isr_bssid[5]); 
                printf("Chan %3d Freq %4d  ", result->isr_ieee, result->isr_freq);
                printf("Rssi %3d  \n", result->isr_rssi);
                printf("    NRate %2d  ", result->isr_nrates);
                printf("ERP 0x%02x\n", result->isr_erp);
                printf("    Rate ");
                for(i = 0; i < strlen((char*)result->isr_rates); i++)
                    printf("%d  ",result->isr_rates[i]); 
                printf("Cap-Info 0x%04x\n", result->isr_capinfo);

                if(result->isr_wpa_ie.len ) 
                {
                    printf("    WPA-IE ");
                    for(i = 0; i < result->isr_wpa_ie.len; i++)
                        printf("%02x ", result->isr_wpa_ie.data[i]);
                    printf("\n");                                              
                }
                
                if(result->isr_wme_ie.len ) 
                { 
                    printf("    WME-IE ");
                    for(i = 0; i < result->isr_wme_ie.len; i++)
                        printf("%02x ",result->isr_wme_ie.data[i]);
                    printf("\n");
                }                               
                            
                if(result->isr_ath_ie.len )  
                {
                    printf("    ATH-IE ");
                    for(i = 0; i < result->isr_ath_ie.len; i++)
                        printf("%02x ",result->isr_ath_ie.data[i]);
                    printf("\n");
                }
                
                if(result->isr_rsn_ie.len )  
                {
                    printf("    RSN-IE ");
                    for(i = 0; i < result->isr_rsn_ie.len; i++)
                        printf("%02x ",result->isr_rsn_ie.data[i]);
                    printf("\n");
                }     
                          
                if(result->isr_wps_ie.len )  
                {
                    printf("    WPS-IE ");
                    for(i = 0; i < result->isr_wps_ie.len; i++)
                        printf("%02x ",result->isr_wps_ie.data[i]);
                    printf("\n");
                }        

                if(result->isr_htcap_ie.len )  
                {
                   printf("    HTCAP-IE ");
                   for(i = 0; i < result->isr_htcap_ie.len; i++)
                       printf("%02x ",result->isr_htcap_ie.data[i]);
                   printf("\n");

                   printf("    HTCAP-MCSSET "); 
                   for(i = 0; i < ATHCFG_WCMD_MAX_HT_MCSSET; i ++)
                       printf("%02x ",result->isr_htcap_mcsset[i]);
                   printf("\n");
                }
                
                if(result->isr_htinfo_ie.len )  
                {
                   printf("    HTINFO-IE ");
                   for(i = 0; i < result->isr_htinfo_ie.len; i++)
                       printf("%02x ",result->isr_htinfo_ie.data[i]);
                    printf("\n");
                }        
                printf("\n");
            }                            
        }
        else 
            printf("There are no AP in the scan list \n");      
    } while(i_req.d_scan->more);

    free(scan);
    //printf("Total %d run, %d entry\n",loop, cnt);
    
    return 0;  
}

int
athcfg_scan_set(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req;
    
    memset(&i_req, 0, sizeof(athcfg_wcmd_t));
    
    assert(nargs == 0);
    args[0] = NULL;
    
    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_SET_SCAN;

    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
     return EIO;
 
    return 0;   
}

/**
 * @brief Handlers for get Version-Info.
 */
int
athcfg_version_info_get(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req;
    athcfg_wcmd_version_info_t version_info;
    
    memset(&i_req, 0, sizeof(athcfg_wcmd_t));
    memset(&version_info, 0, sizeof(athcfg_wcmd_version_info_t));

    assert(nargs == 0);
    args[0] = NULL;
    
    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_GET_DEV_VERSION_INFO;
    i_req.d_versioninfo = version_info;

    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
     return EIO;

    printf("driver         -- %s\n", i_req.d_versioninfo.driver);
    printf("version        -- %s\n", i_req.d_versioninfo.version);
    printf("fw_version     -- %s\n", i_req.d_versioninfo.fw_version);

    return 0; 
}

/**
 * @brief Handlers for get TX-POWER.
 */
int
athcfg_txpower_get(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req; 
    athcfg_wcmd_txpower_t txpower; 
    a_int32_t i;
    const char *rateString[] = {" 6mb OFDM ", " 9mb OFDM ", "12mb OFDM ", "18mb OFDM ",
                               "24mb OFDM ", "36mb OFDM ", "48mb OFDM ", "54mb OFDM ",
                               "1L   CCK  ", "2L   CCK  ", "2S   CCK  ", "5.5L CCK  ",
                               "5.5S CCK  ", "11L  CCK  ", "11S  CCK  ", "XR        ",
                               "HT20mcs 0 ", "HT20mcs 1 ", "HT20mcs 2 ", "HT20mcs 3 ",
                               "HT20mcs 4 ", "HT20mcs 5 ", "HT20mcs 6 ", "HT20mcs 7 ",
                               "HT40mcs 0 ", "HT40mcs 1 ", "HT40mcs 2 ", "HT40mcs 3 ",
                               "HT40mcs 4 ", "HT40mcs 5 ", "HT40mcs 6 ", "HT40mcs 7 ",
							   "HT20mcs 8 ", "HT20mcs 9 ", "HT20mcs 10", "HT20mcs 11",
                               "HT20mcs 12", "HT20mcs 13", "HT20mcs 14", "HT20mcs 15",
							   "HT40mcs 8 ", "HT40mcs 9 ", "HT40mcs 10", "HT40mcs 11",
                               "HT40mcs 12", "HT40mcs 13", "HT40mcs 14", "HT40mcs 15",
    };

    memset(&i_req,0, sizeof(athcfg_wcmd_t));
    memset(&txpower, 0, sizeof(athcfg_wcmd_txpower_t));

    assert (nargs == 0);
    args[0] = NULL;

    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_GET_TX_POWER;
    i_req.d_txpower = txpower;

    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
        return EIO;
   
    /* the unit of i_req.d_txpower.txpowertable[x] is 0.5 dBm. */
    /* show current tx power table in 1 dBm unit               */
    for (i = 0; i < ATHCFG_WCMD_TX_POWER_TABLE_SIZE; i += 4) 
    {  
        if (i == 12)  
        {   /* w/o XR rate */   
            printf(" %s %3d.%1d dBm | %s %3d.%1d dBm | %s %3d.%1d dBm \n",
                    rateString[i    ], i_req.d_txpower.txpowertable[i    ] / 2, (i_req.d_txpower.txpowertable[i    ] % 2) * 5,
                    rateString[i + 1], i_req.d_txpower.txpowertable[i + 1] / 2, (i_req.d_txpower.txpowertable[i + 1] % 2) * 5,
                    rateString[i + 2], i_req.d_txpower.txpowertable[i + 2] / 2, (i_req.d_txpower.txpowertable[i + 2] % 2) * 5);
        }
        else
        {
            printf(" %s %3d.%1d dBm | %s %3d.%1d dBm | %s %3d.%1d dBm | %s %3d.%1d dBm\n",
                    rateString[i    ], i_req.d_txpower.txpowertable[i    ] / 2, (i_req.d_txpower.txpowertable[i    ] % 2) * 5,
                    rateString[i + 1], i_req.d_txpower.txpowertable[i + 1] / 2, (i_req.d_txpower.txpowertable[i + 1] % 2) * 5,
                    rateString[i + 2], i_req.d_txpower.txpowertable[i + 2] / 2, (i_req.d_txpower.txpowertable[i + 2] % 2) * 5,
                    rateString[i + 3], i_req.d_txpower.txpowertable[i + 3] / 2, (i_req.d_txpower.txpowertable[i + 3] % 2) * 5);
        }
    }

    return 0;   
}

/**
 * @brief Handlers for get STATS.
 */
int
athcfg_stats_get(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req; 
    athcfg_wcmd_stats_t stats; 

    memset(&i_req,0, sizeof(athcfg_wcmd_t));
    memset(&stats, 0, sizeof(athcfg_wcmd_stats_t));

    assert (nargs == 0);
    args[0] = NULL;

    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_GET_STATS;
    i_req.d_stats = stats;

    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
        return EIO;

    printf("STATS --\n");
    printf(" TX Packets          : %lld \n", i_req.d_stats.txPackets);   
    printf(" TX Retry            : %lld \n", i_req.d_stats.txRetry);   
    //printf(" TX Aggr. Excess Retry : %lld \n", i_req.d_stats.txAggrExcessiveRetry);    
    //printf(" TX Sub-frame Retry    : %lld \n", i_req.d_stats.txSubRetry);    
 
    
    return 0;
}

#ifndef ANDROID

/*
 * Avoid including other kernel header to avoid conflicts with C library
 * headers.
 */
#define _LINUX_TYPES_H
#define _LINUX_SOCKET_H
#define _LINUX_IF_H

#include <sys/types.h>
#include <net/if.h>
typedef __uint32_t __u32;
typedef __int32_t __s32;
typedef __uint16_t __u16;
typedef __int16_t __s16;
typedef __uint8_t __u8;
#ifndef __user
#define __user
#endif /* __user */

#endif /* ANDROID */
#include <linux/wireless.h>
#include "ieee80211_ioctl.h"
int 
suspend_set(char *ifrn_name, int nargs, char *args[])
{
#if 1//use athcfg_wcmd
    athcfg_wcmd_t i_req;
    athcfg_wcmd_suspend_t suspend;
    int ret1;
    memset(&i_req,0, sizeof(athcfg_wcmd_t));
    memset(&suspend, 0, sizeof(athcfg_wcmd_suspend_t));

    assert(nargs == 1);

    ret1 = sscanf(args[0], "%d", &suspend.val);
 //   ret2 = sscanf(args[1], "0x%x", &reg.val);
    if (ret1 != 1) {
        fprintf(stderr, "Register address and value must be interge\n");
        return 0;
    }

	strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
	i_req.iic_cmd = ATHCFG_WCMD_SET_SUSPEND;
	i_req.d_suspend = suspend;
	if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
		return EIO;
    printf("suspend mode: %d\n", suspend.val);

    return 0;
#else
    athcfg_wcmd_t i_req;
    athcfg_wcmd_reg_t reg;
    int ret1;
	int suspend_mode;
    memset(&i_req,0, sizeof(athcfg_wcmd_t));
    memset(&reg, 0, sizeof(athcfg_wcmd_reg_t));

    assert(nargs == 1);

    ret1 = sscanf(args[0], "%d", &suspend_mode);
    if (ret1 != 1) {
        fprintf(stderr, "Register address and value must be interge\n");
        return 0;
    }
	
	{
		int s;
		struct iwreq iwr;
		memset(&iwr, 0, sizeof(iwr));
		strncpy(iwr.ifr_name, ifrn_name, strlen(ifrn_name));
		iwr.u.mode = 221;//IEEE80211_PARAM_SUSPEND_SET;
		memcpy(iwr.u.name + sizeof(__u32), &suspend_mode, sizeof(suspend_mode));
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if(s < 0) {
			fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
			return -1;
		}		
		if (ioctl(s, IEEE80211_IOCTL_SETPARAM, &iwr) < 0) {	
			fprintf(stderr, "Error doing ioctl(): %s\n", strerror(errno));
			close(s);
			return -1;			
		}
		close(s);
	}

    printf("suspend mode: %d\n", suspend_mode);

    return 0;
#endif	
}

#if defined(CE_CUSTOM_1)
/**
 * @brief Handlers for Enable/Disable WiDi mode (Configure bit#21(Ignore NAV) at reg 0x28048).
 */
int 
athcfg_widi_mode_set(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req;
    a_uint32_t widi_enable=0;
    int ret;

    memset(&i_req,0, sizeof(athcfg_wcmd_t));

    assert(nargs == 1);

    ret = sscanf(args[0], "%u", &widi_enable);
    if ( ret != 1 || widi_enable > 1 ) {
        fprintf(stderr, "Only allow 0 or 1\n");
        return 0;
    }

    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_SET_WIDI;
    i_req.d_widi = (a_uint8_t)widi_enable;

    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
        return EIO;

    printf("WiDi mode: %s\n", widi_enable ? "on" : "off");

    return 0;

}

/**
 * @brief Handlers for Get WiDi mode status.
 */
int
athcfg_widi_mode_get(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req; 

    memset(&i_req,0, sizeof(athcfg_wcmd_t));

    assert (nargs == 0);
    args[0] = NULL;

    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_GET_WIDI;

    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
        return EIO;

    printf("Current WiDi mode: %s\n", i_req.d_widi ? "on" : "off");
	
    return 0;
}
#endif

static void __chan_flag_to_string(a_uint32_t flags, char *string)
{
	string[0] = '\0';
	if (flags & IEEE80211_CHAN_DISABLED)
		strcpy(string + strlen(string), "[DISABLE]");

	if (flags & IEEE80211_CHAN_PASSIVE_SCAN)
		strcpy(string + strlen(string), "[PASSIVE_SCAN]");

	if (flags & IEEE80211_CHAN_NO_IBSS)
		strcpy(string + strlen(string), "[NO_IBSS]");

	if (flags & IEEE80211_CHAN_RADAR)
		strcpy(string + strlen(string), "[RADER]");

	strcpy(string + strlen(string), "[HT20]");
	if ((flags & IEEE80211_CHAN_NO_HT40PLUS) &&
	    (flags & IEEE80211_CHAN_NO_HT40MINUS)) {
		;
	} else {
		if (flags & IEEE80211_CHAN_NO_HT40PLUS)
			strcpy(string + strlen(string), "[HT40-]");
		else if (flags & IEEE80211_CHAN_NO_HT40MINUS)
			strcpy(string + strlen(string), "[HT40+]");
		else
			strcpy(string + strlen(string), "[HT40+][HT40-]");
	}

	return;
}
int 
chans_get(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req;
    athcfg_wcmd_chan_list_t chans;
    int i;
	char flag_string[96];
    memset(&i_req,0, sizeof(athcfg_wcmd_t));
    memset(&chans, 0, sizeof(athcfg_wcmd_chan_list_t));
    assert(nargs == 0);
    args[0] = NULL;
	strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
	i_req.iic_cmd = ATHCFG_WCMD_GET_CHAN_LIST;

	if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
		return EIO;
    
	chans = i_req.d_chans;
	printf("2.4GHz\n\r");
	for (i = 0; i < chans.n_2ghz_channels; i++) {
		struct ieee80211_channel *chan;

		chan = &chans.ce_ath6kl_2ghz_a_channels[i];

		if (chan->flags & IEEE80211_CHAN_DISABLED)
			continue;

		__chan_flag_to_string(chan->flags, flag_string);
		printf(" CH%4d - %4d %s (%.1f dBm)\n",
				chan->hw_value,
				chan->center_freq,
				flag_string,1.00*(chan->max_power));
	}
	printf("5GHz\n\r");
	for (i = 0; i < chans.n_5ghz_channels; i++) {
		struct ieee80211_channel *chan;

		chan = &chans.ce_ath6kl_5ghz_a_channels[i];

		if (chan->flags & IEEE80211_CHAN_DISABLED)
			continue;

		__chan_flag_to_string(chan->flags, flag_string);
		printf(" CH%4d - %4d %s (%.1f dBm)\n",
				chan->hw_value,
				chan->center_freq,
				flag_string,1.00*(chan->max_power));
	}	
    return 0;
}

int 
csa_set(char *ifrn_name, int nargs, char *args[])
{
	athcfg_wcmd_t i_req;
    athcfg_wcmd_csa_t csa;
    int ret1;

    memset(&i_req,0, sizeof(athcfg_wcmd_t));
    memset(&csa, 0, sizeof(athcfg_wcmd_csa_t));

    assert(nargs == 1);

    ret1 = sscanf(args[0], "%d", &csa.freq);
    if (ret1 != 1) {
        fprintf(stderr, "Register address and value must be in interge\n");
        return 0;
    }

    strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
    i_req.iic_cmd = ATHCFG_WCMD_SET_CSA;
    i_req.d_csa = csa;

    if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
        return EIO;

    printf("csa cmd:%d\n", csa.freq);

    return 0;
}

typedef enum {
    FCC        = 0x10,
    MKK        = 0x40,
    ETSI       = 0x30,
    SD_NO_CTL  = 0xE0,
    NO_CTL     = 0xFF,
    CTL_MODE_M = 0x0F,
    CTL_11A    = 0,
    CTL_11B    = 1,
    CTL_11G    = 2,
    CTL_TURBO  = 3,
    CTL_108G   = 4,
    CTL_11G_HT20 = 5,
    CTL_11A_HT20 = 6,
    CTL_11G_HT40 = 7,
    CTL_11A_HT40 = 8,
} CONFORMANCE_TEST_LIMITS;

void __ctl_mode_to_string(a_uint8_t	ctlmode,char	*string) 
{
	
	string[0] = '\0';
	switch(ctlmode&0xF0)
	{
	case FCC:
		strcpy(string + strlen(string), "[FCC]");
		break;
	case MKK:
		strcpy(string + strlen(string), "[MKK]");
		break;
	case ETSI:
		strcpy(string + strlen(string), "[ETSI]");
		break;
	case SD_NO_CTL:
		strcpy(string + strlen(string), "[SD_NO_CTL]");
		break;
	case NO_CTL:
		strcpy(string + strlen(string), "[NO_CTL]");
		break;
	default:
		strcpy(string + strlen(string), "[Not Support]");
		break;
	}
	switch(ctlmode&0xF)
	{
	case CTL_11A:
		strcpy(string + strlen(string), "[CTL_11A]");
		break;
	case CTL_11B:
		strcpy(string + strlen(string), "[CTL_11B]");
		break;
	case CTL_11G:
		strcpy(string + strlen(string), "[CTL_11G]");
		break;
	case CTL_TURBO:
		strcpy(string + strlen(string), "[CTL_TURBO]");
		break;
	case CTL_108G:
		strcpy(string + strlen(string), "[CTL_108G]");
		break;
	case CTL_11G_HT20:
		strcpy(string + strlen(string), "[CTL_11G_HT20]");
		break;
	case CTL_11A_HT20:
		strcpy(string + strlen(string), "[CTL_11A_HT20]");
		break;
	case CTL_11G_HT40:
		strcpy(string + strlen(string), "[CTL_11G_HT40]");
		break;
	case CTL_11A_HT40:
		strcpy(string + strlen(string), "[CTL_11A_HT40]");
		break;		
	default:
		strcpy(string + strlen(string), "[Not Support]");
		break;
	}	
}

typedef enum {
	RATE_1Mb       = 0,
	RATE_2Mb       = 1,
	RATE_5_5Mb     = 2,
	RATE_11Mb      = 3,
	RATE_6Mb       = 4,
	RATE_9Mb       = 5,
	RATE_12Mb      = 6,
	RATE_18Mb      = 7,
	RATE_24Mb      = 8,
	RATE_36Mb      = 9,
	RATE_48Mb      = 10,
	RATE_54Mb      = 11,
	RATE_MCS_0_20  = 12,
	RATE_MCS_1_20  = 13,
	RATE_MCS_2_20  = 14,
	RATE_MCS_3_20  = 15,
	RATE_MCS_4_20  = 16,
	RATE_MCS_5_20  = 17,
	RATE_MCS_6_20  = 18,
	RATE_MCS_7_20  = 19,
	RATE_MCS_8_20  = 20,
	RATE_MCS_9_20  = 21,
	RATE_MCS_10_20 = 22,
	RATE_MCS_11_20 = 23,
	RATE_MCS_12_20 = 24,
	RATE_MCS_13_20 = 25,
	RATE_MCS_14_20 = 26,
	RATE_MCS_15_20 = 27,
	RATE_MCS_0_40  = 28,
	RATE_MCS_1_40  = 29,
	RATE_MCS_2_40  = 30,
	RATE_MCS_3_40  = 31,
	RATE_MCS_4_40  = 32,
	RATE_MCS_5_40  = 33,
	RATE_MCS_6_40  = 34,
	RATE_MCS_7_40  = 35,
	RATE_MCS_8_40  = 36,
	RATE_MCS_9_40  = 37,
	RATE_MCS_10_40 = 38,
	RATE_MCS_11_40 = 39,
	RATE_MCS_12_40 = 40,
	RATE_MCS_13_40 = 41,
	RATE_MCS_14_40 = 42,
	RATE_MCS_15_40 = 43,
} ATH_RATE_TABLE;


int __ctl_rate_to_string(a_uint32_t use_5g,a_uint32_t	rate_idx,char	*string) 
{
	string[0] = '\0';

	switch(rate_idx)
	{
	case RATE_1Mb      :
		if(use_5g == 1)
			return -1;
		strcpy(string + strlen(string), "802.11b / Data rate 1M");
			break;	
	case RATE_2Mb      :
		if(use_5g == 1)
			return -1;	
		strcpy(string + strlen(string), "802.11b / Data rate 2M");
			break;		
	case RATE_5_5Mb    :
		if(use_5g == 1)
			return -1;	
		strcpy(string + strlen(string), "802.11b / Data rate 5.5M");
			break;		
	case RATE_11Mb     :
		if(use_5g == 1)
			return -1;	
		strcpy(string + strlen(string), "802.11b / Data rate 11M");
			break;		
	case RATE_6Mb      :
		if(use_5g == 1) {
			strcpy(string + strlen(string), "802.11a / Data rate 6M");
		} else {
			strcpy(string + strlen(string), "802.11g / Data rate 6M");
		}
			break;		
	case RATE_9Mb      :
		if(use_5g == 1) {
			strcpy(string + strlen(string), "802.11a / Data rate 9M");
		} else {
			strcpy(string + strlen(string), "802.11g / Data rate 9M");
		}
			break;		
	case RATE_12Mb     :
		if(use_5g == 1) {
			strcpy(string + strlen(string), "802.11a / Data rate 12M");
		} else {
			strcpy(string + strlen(string), "802.11g / Data rate 12M");
		}
			break;		
	case RATE_18Mb     :
		if(use_5g == 1) {
			strcpy(string + strlen(string), "802.11a / Data rate 18M");
		} else {
			strcpy(string + strlen(string), "802.11g / Data rate 18M");
		}
			break;		
	case RATE_24Mb     :
		if(use_5g == 1) {
			strcpy(string + strlen(string), "802.11a / Data rate 24M");
		} else {
			strcpy(string + strlen(string), "802.11g / Data rate 24M");
		}
			break;		
	case RATE_36Mb     :
		if(use_5g == 1) {
			strcpy(string + strlen(string), "802.11a / Data rate 36M");
		} else {
			strcpy(string + strlen(string), "802.11g / Data rate 36M");
		}
			break;		
	case RATE_48Mb     :
		if(use_5g == 1) {
			strcpy(string + strlen(string), "802.11a / Data rate 48M");
		} else {
			strcpy(string + strlen(string), "802.11g / Data rate 48M");
		}
			break;		
	case RATE_54Mb     :
		if(use_5g == 1) {
			strcpy(string + strlen(string), "802.11a / Data rate 54M");
		} else {
			strcpy(string + strlen(string), "802.11g / Data rate 54M");
		}
			break;
	case RATE_MCS_0_20 :
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS0");
			break;	
	case RATE_MCS_1_20 :
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS1");
			break;	
	case RATE_MCS_2_20 :
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS2");
			break;	
	case RATE_MCS_3_20 :
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS3");
			break;	
	case RATE_MCS_4_20 :
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS4");
			break;	
	case RATE_MCS_5_20 :
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS5");
			break;	
	case RATE_MCS_6_20 :
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS6");
			break;	
	case RATE_MCS_7_20 :
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS7");
			break;	
	case RATE_MCS_8_20 :
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS8");
			break;	
	case RATE_MCS_9_20 :
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS9");
			break;	
	case RATE_MCS_10_20:
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS10");
			break;	
	case RATE_MCS_11_20:
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS11");
			break;	
	case RATE_MCS_12_20:
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS12");
			break;	
	case RATE_MCS_13_20:
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS13");
			break;	
	case RATE_MCS_14_20:
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS14");
			break;	
	case RATE_MCS_15_20:
		strcpy(string + strlen(string), "802.11n HT20 / Data rate MCS15");
			break;	
	case RATE_MCS_0_40 :
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS0");
			break;	
	case RATE_MCS_1_40 :
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS1");
			break;	
	case RATE_MCS_2_40 :
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS2");
			break;	
	case RATE_MCS_3_40 :
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS3");
			break;	
	case RATE_MCS_4_40 :
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS4");
			break;	
	case RATE_MCS_5_40 :
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS5");
			break;	
	case RATE_MCS_6_40 :
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS6");
			break;	
	case RATE_MCS_7_40 :
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS7");
			break;	
	case RATE_MCS_8_40 :
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS8");
			break;	
	case RATE_MCS_9_40 :
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS9");
			break;	
	case RATE_MCS_10_40:
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS10");
			break;	
	case RATE_MCS_11_40:
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS11");
			break;	
	case RATE_MCS_12_40:
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS12");
			break;	
	case RATE_MCS_13_40:
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS13");
			break;	
	case RATE_MCS_14_40:
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS14");
			break;	
	case RATE_MCS_15_40:
		strcpy(string + strlen(string), "802.11n HT40 / Data rate MCS15");
			break;	
	default:
		strcpy(string + strlen(string), "Not support");
		return -1;
	}
	return 0;
}

int 
ctls_get(char *ifrn_name, int nargs, char *args[])
{
    athcfg_wcmd_t i_req;
    athcfg_wcmd_ctl_list_t ctls;
    int i,j;
	char ctlmode_string[32];
	char rate_string[32],all[4];
	int ch_idx=0,ret;

	assert(nargs == 1);
    ret = sscanf(args[0], "%d", &ch_idx);
    if (ret != 1) {
		ret = sscanf(args[0], "%s", &all[0]);
		if (ret != 1) {
			fprintf(stderr, "please use auto or channel parameter(1~14 or 36~165)\n");
			return 0; 
		}
		if(strcmp("auto",all) != 0) {
			fprintf(stderr, "please use auto or frequenceas parameter\n");
			return 0;
		}
    } else {
		if(!(((ch_idx>=1) && (ch_idx<=14)) ||
		  ((ch_idx>=36) && (ch_idx<=165)) ))  {
			fprintf(stderr, "Channel idx must be in (1 ~ 14) or (36~165)\n");
			return 0;		  
		}
	}

    memset(&i_req,0, sizeof(athcfg_wcmd_t));
    memset(&ctls, 0, sizeof(athcfg_wcmd_ctl_list_t));
	strncpy(i_req.iic_ifname, ifrn_name, strlen(ifrn_name));
	i_req.iic_cmd = ATHCFG_WCMD_GET_CTL_LIST;

	if (athcfg_do_ioctl(ATHCFG_WCMD_IOCTL, &i_req) < 0)
		return EIO;
    
	ctls = i_req.d_ctls;

	#if 1
	for (i = 0; i < ctls.n_2ghz_channels; i++) {
		struct ctl_info *ctl;
		ctl = &ctls.ce_ath6kl_2ghz_a_ctls[i];

		if (ctl->flags & IEEE80211_CHAN_DISABLED) {
			continue;
		}
		if (ctl->ch_idx == ch_idx || ch_idx == 0) {//dump information
			int ht20_2G_support = 0;
			int ht40_2G_support = 0;
			__ctl_mode_to_string(ctl->ctlmode[TARGET_POWER_MODE_CCK],ctlmode_string);
			if(ctl->txpower[TARGET_POWER_MODE_CCK] == 63) {
				printf("CH%d (%4d) CTL Mode(%s) Power(TBD dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string);				
			} else {
				printf("CH%d (%4d) CTL Mode(%s) Power(%.1f dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string,
									0.50*(ctl->txpower[TARGET_POWER_MODE_CCK]));
			}
			__ctl_mode_to_string(ctl->ctlmode[TARGET_POWER_MODE_OFDM],ctlmode_string);
			if(ctl->txpower[TARGET_POWER_MODE_OFDM] == 63) {
				printf("CH%d (%4d) CTL Mode(%s) Power(TBD dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string);				
			} else {			
				printf("CH%d (%4d) CTL Mode(%s) Power(%.1f dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string,
									0.50*(ctl->txpower[TARGET_POWER_MODE_OFDM]));	
			}
			__ctl_mode_to_string(ctl->ctlmode[TARGET_POWER_MODE_HT20],ctlmode_string);
			if(ctl->txpower[TARGET_POWER_MODE_HT20] == 63) {
				printf("CH%d (%4d) CTL Mode(%s) Power(TBD dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string);
			} else {			
				printf("CH%d (%4d) CTL Mode(%s) Power(%.1f dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string,
									0.50*(ctl->txpower[TARGET_POWER_MODE_HT20]));
				ht20_2G_support = 1;
			}
			
			__ctl_mode_to_string(ctl->ctlmode[TARGET_POWER_MODE_HT40],ctlmode_string);
			if(ctl->txpower[TARGET_POWER_MODE_HT40] == 63) {
				printf("CH%d (%4d) CTL Mode(%s) Power(TBD dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string);				
			} else {
				printf("CH%d (%4d) CTL Mode(%s) Power(%.1f dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string,
									0.50*(ctl->txpower[TARGET_POWER_MODE_HT40]));	
				ht40_2G_support = 1;
			}
			printf("CH%d (%4d) \n",
					ctl->ch_idx,
					ctl->center_freq);
			for(j = 0;j<CE_RATE_MAX;j++) {
				if ((j>=RATE_MCS_0_20) && (j<=RATE_MCS_15_20)) {
					if (ht20_2G_support == 0) {
						continue;
					}
				}
				if ((j>=RATE_MCS_0_40) && (j<=RATE_MCS_15_40)) {
					if (ht40_2G_support == 0) {
						continue;
					}
				}
				if (__ctl_rate_to_string(0,j,rate_string) == -1)
					continue;
				printf("	%s / Power (%.1f dBm)\n",
						rate_string,0.50*(ctl->cal_txpower[j]));				
			}
		}
	}
	#endif
	//printf("5GHz,ctls.n_5ghz_channels=%d\n\r",ctls.n_5ghz_channels);
	#if 1
	for (i = 0; i < ctls.n_5ghz_channels; i++) {
		struct ctl_info *ctl;

		ctl = &ctls.ce_ath6kl_5ghz_a_ctls[i];

		if (ctl->flags & IEEE80211_CHAN_DISABLED) {
			continue;
		}

		if (ctl->ch_idx == ch_idx || ch_idx == 0) {//dump information
			int ht20_5G_support = 0;
			int ht40_5G_support = 0;
			__ctl_mode_to_string(ctl->ctlmode[TARGET_POWER_MODE_OFDM],ctlmode_string);
			if(ctl->txpower[TARGET_POWER_MODE_OFDM] == 63) {
				printf("CH%d (%4d) CTL Mode(%s) Power(TBD dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string);				
			} else {
				printf("CH%d (%4d) CTL Mode(%s) Power(%.1f dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string,
									0.50*(ctl->txpower[TARGET_POWER_MODE_OFDM]));
			}
			__ctl_mode_to_string(ctl->ctlmode[TARGET_POWER_MODE_HT20],ctlmode_string);
			if(ctl->txpower[TARGET_POWER_MODE_HT20] == 63) {
				printf("CH%d (%4d) CTL Mode(%s) Power(TBD dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string);				
			} else {			
				printf("CH%d (%4d) CTL Mode(%s) Power(%.1f dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string,
									0.50*(ctl->txpower[TARGET_POWER_MODE_HT20]));
				ht20_5G_support = 1;
			}
			__ctl_mode_to_string(ctl->ctlmode[TARGET_POWER_MODE_HT40],ctlmode_string);
//printf("CH%d %d dBm\n",ctl->ch_idx,ctl->txpower[TARGET_POWER_MODE_HT40]);			
			if(ctl->txpower[TARGET_POWER_MODE_HT40] == 63) {
				printf("CH%d (%4d) CTL Mode(%s) Power(TBD dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string);				
			} else {				
				printf("CH%d (%4d) CTL Mode(%s) Power(%.1f dBm)\n",
									ctl->ch_idx,
									ctl->center_freq,
									ctlmode_string,
									0.50*(ctl->txpower[TARGET_POWER_MODE_HT40]));
				ht40_5G_support = 1;
			}
			printf("CH%d (%4d) \n",
					ctl->ch_idx,
					ctl->center_freq);
			for(j = 0;j<CE_RATE_MAX;j++) {
				if ((j>=RATE_MCS_0_20) && (j<=RATE_MCS_15_20)) {
					if (ht20_5G_support == 0) {
						continue;
					}
				}
				if ((j>=RATE_MCS_0_40) && (j<=RATE_MCS_15_40)) {
					if (ht40_5G_support == 0) {
						continue;
					}
				}
				if (__ctl_rate_to_string(1,j,rate_string) == -1)
					continue;
				printf("	%s / Power (%.1f dBm)\n",
						rate_string,0.50*(ctl->cal_txpower[j]));				
			}
		}
	}	
	#endif
    return 0;
}
