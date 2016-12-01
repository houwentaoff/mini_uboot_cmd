/******************************************************************************
  * @file    config.h
  * @author  joy
  * @version V1.0.0
  * @date    23-Nov-2016
  * @brief   global config 
  ******************************************************************************
  */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CONFIG_DEVADDR    "02:5B:10:27"                                      /* env DEVADDR */
#define CONFIG_DEVEUI     "BE:7A:00:00:00:00:02:EA"                          /* env DEVEUI */
#define CONFIG_APPEUI     "01:01:01:01:01:01:01:01"                          /* env APPEUI */
#define CONFIG_APPKEY     "E5:42:2A:1B:4D:F9:13:4B:C7:52:5D:F4:FE:BA:53:6E"  /* env APPKEY */
#define CONFIG_NETID      "01:01:01"                                         /* env NETID   */
#define CONFIG_NWKSKEY    "C2:E2:8C:F2:C3:82:E8:43:EA:9D:9E:30:54:92:A0:4C"  /* env NWKSKEY */
#define CONFIG_APPSKEY    "05:24:F2:51:78:EE:D5:C2:35:D4:5C:05:3B:27:9F:6E"  /* env APPSKEY */
#define CONFIG_ID         "manual"                                           /* AT command id   */
#define CONFIG_MODE       "lwabp"                                            /* AT command mode */

#define CONFIG_PORT       2                                                  /* AT command port */
#define CONFIG_ADR        1                                                  /* AT command adr  */

#define CONFIG_ENV_SIZE                 (0x400)                              /* 1K */
#define ENV_HEADER_SIZE	                (sizeof(uint32_t))                   /* env head size */
#define CONFIG_SYS_DEF_EEPROM_ADDR      0x00                                 /* eeprom device addr */
#define CONFIG_ENV_OFFSET               0x08080800                           /* eeprom addr : 0x08080800 --0x08080BFF   1k */

#define CONFIG_SYS_LONGHELP

#define MINISHELL_VERSION_STRING        ""                                   /* "minishell 2016.11.22 joy" */
#define CONFIG_SYS_PROMPT               "lorawan>"                           /*  prompt */

#endif
