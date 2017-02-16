/*
 * Copyright (c) 2002-2010 Atheros Communications, Inc.
 *
 * athtst.h: Common header file for athtst example. 
 *
 */
#ifndef _ATHCFG_H
#define _ATHCFG_H
/**
 * @brief Max characters in a command name.
 */ 
#define ATHCFG_MAX_CMD_NAME  32
/**
 * @brief Max characters in a usage string.
 */ 
#define ATHCFG_MAX_CMD_USAGE 128

typedef enum {
    FALSE,
    TRUE
}a_bool_sony__t;

/**
 * @brief Prototype for command handler.
 *
 * @param[in]   nargs       Number of args for this command.
 * @param[in]   args        List of args for this command.
 *
 * @retval Zero indicates success, otherwise failure.
 */ 
typedef int (*athcfg_cmd_hdlr_t)(char *ifname, int nargs, char *args[]);

/**
 * @brief All commands follow this template.
 */ 
typedef struct athcfg_cmd_s {
    char    cmd_name[ATHCFG_MAX_CMD_NAME];   /**< Name of command */
    a_bool_sony__t    handler_present;                 /**< Handled at this level? */
    union {
        struct athcfg_cmd_hdlr_s {
            athcfg_cmd_hdlr_t   fn;          /**< Set if handler_present */
            char    cmd_usage[ATHCFG_MAX_CMD_USAGE];    /**< Usage string */
            int     min_args;                /**< Minimum required args */
            int     max_args;                /**< Maximum possible args */
        } hdlr;
        struct athcfg_cmd_s * next_level;  /**< Set if !handler_present */
    } u;
} athcfg_cmd_t;

#define h_min_args  u.hdlr.min_args
#define h_max_args  u.hdlr.max_args
#define h_cmd_usage u.hdlr.cmd_usage
#define h_fn        u.hdlr.fn

int athcfg_product_info_get(char *ifrn_name, int nargs, char *args[]);
int athcfg_testmode_get(char *ifrn_name, int nargs, char *args[]);
int athcfg_testmode_set(char *ifrn_name, int nargs, char *args[]);
int athcfg_reg_get(char *ifrn_name, int nargs, char *args[]);
int athcfg_reg_set(char *ifrn_name, int nargs, char *args[]);
int athcfg_stainfo_get(char *ifrn_name, int nargs, char *args[]);
int athcfg_scantime_get(char *ifrn_name, int nargs, char *args[]);
int athcfg_mode_get(char *ifrn_name, int nargs, char *args[]);
int athcfg_scan_get(char *ifrn_name, int nargs, char *args[]);
int athcfg_scan_set(char *ifrn_name, int nargs, char *args[]);
int athcfg_version_info_get(char *ifrn_name, int nargs, char *args[]);
int athcfg_txpower_get(char *ifrn_name, int nargs, char *args[]);
int athcfg_stats_get(char *ifrn_name, int nargs, char *args[]);
int suspend_set(char *ifrn_name, int nargs, char *args[]);
#if defined(CE_CUSTOM_1)
int athcfg_widi_mode_set(char *ifrn_name, int nargs, char *args[]);
int athcfg_widi_mode_get(char *ifrn_name, int nargs, char *args[]);
#endif
int chans_get(char *ifrn_name, int nargs, char *args[]);
int csa_set(char *ifrn_name, int nargs, char *args[]);
int ctls_get(char *ifrn_name, int nargs, char *args[]);
#endif /* _ATHCFG_H */

