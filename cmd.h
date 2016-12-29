/******************************************************************************
 * @file    cmd.h
 * @author  Joy
 * @version V1.0.0
 * @date    17-Nov-2016
 * @brief   hw usart api
 ******************************************************************************
 */
#ifndef __CMD_H__
#define __CMD_H__
#ifdef __cplusplus
 extern "C" {
#endif

#include "minishell_core.h"

typedef              cmd_tbl_t             CmdTbl;
#define Shell_Cmd_Register(cmd)            uboot_cmd_register(cmd)

/**
* @brief    register cmd
*
* @return 
*/
int Init_Env(void);

#ifdef __cplusplus
}
#endif
#endif /* __CMD_H__ */
