/******************************************************************************
 * @file    minishell_core.h
 * @author  Joy
 * @version V1.0.0
 * @date    14-Nov-2016
 * @brief    minishell  api
 ******************************************************************************
 */
#ifndef __MINISHELL_CORE_H__
#define __MINISHELL_CORE_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "config.h"

typedef void(*callback)(void *);/* callback (other loop) main loop used */

struct cmd_tbl_s {
    char  *name;/*  Command Name*/
    int  maxargs;/*  maximum number of arguments*/
    int repeatable;/*  autorepeat allowed?autorepeat*/
    /*  Implementation function*/
    int (*cmd)(struct cmd_tbl_s *, int, int, char * const []);
    char *usage;/*  Usage message(short)short*/
#ifdef CONFIG_SYS_LONGHELP
    char *help;/*  Help  message(long)long*/
#endif
#ifdef CONFIG_AUTO_COMPLETE
    /*  do auto completion on the arguments */
    int (*complete)(int argc, char * const argv[], char last_char, int maxv, char *cmdv[]);
#endif
};

typedef struct cmd_tbl_s cmd_tbl_t;

/**
 * @brief         register cmd to cmd list.
 *
 * @param cmd     you have to fill it. 
 *
 * @return        0:success -1:error
 */
int uboot_cmd_register (cmd_tbl_t *cmd);

/**
 * @brief         main loop
 *
 * @param cb      you have to fill if you need to process other things.
 * @param para    unnecessary
 *
 * @return 
 */
void run_main_loop(callback cb, void *para);
/**
 * @brief           hush table init
 *
 * @return          0: success -1:fail
 */
int hush_init_var(void);

#ifndef __GLIBC__
/**
 * @brief           Same as system function getenv
 *
 * @param name
 *
 * @return 
 */
char *getenv(const char *name);
/**
 * @brief           set env var to sys
 *
 * @param varname   name
 * @param varvalue  value
 *
 * @return 
 */
int setenv(const char *varname, const char *varvalue);
#endif

#ifdef __cplusplus
}
#endif
#endif/* __MINISHELL_CORE_H__ */
