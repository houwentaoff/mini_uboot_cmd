/******************************************************************************
 * @file    cmds.h
 * @author  Joy
 * @version V1.0.0
 * @date    17-Nov-2016
 * @brief   hw usart api
 ******************************************************************************
 */
#ifndef __CMDS_H__
#define __CMDS_H__
#ifdef __cplusplus
 extern "C" {
#endif

#include "uboot_cmd.h"

#define CmdTbl                               cmd_tbl_t
#define MiniShellCmdRegister(cmd)            uboot_cmd_register(cmd)

/**
* @brief    register cmd
*
* @return 
*/
int InitEnv(void);

#ifdef __cplusplus
}
#endif
#endif /* __CMDS_H__ */
