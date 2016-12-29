/******************************************************************************
 * @file    cmds.c
 * @author  Joy
 * @version V1.0.0
 * @date    17-Nov-2016
 * @brief   cmds 
 ******************************************************************************
 */
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include "cmds.h"
#include "hw_eeprom.h"
#include "env_eeprom.h"

extern int SetLoraVar(const char *Key, unsigned char *Hex, int Num);

/* Private functions ---------------------------------------------------------*/
static int DoReset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int Ret = 0;
    printf("not implemented\n");
    return Ret;
}
void EnvRelocate(void)
{
    char *value = NULL;    
    uint8_t Hex[17] = {0};
    
    if (hush_init_var() < 0)
    {
        return;
    }
    value = getenv("DEVADDR");
    Str2Hex(value, Hex, 4);
    SetLoraVar("DEVADDR", Hex, 4);
    
    value = getenv("DEVEUI");
    Str2Hex(value, Hex, 8);
    SetLoraVar("DEVEUI", Hex, 8);
    
    value = getenv("APPEUI");
    Str2Hex(value, Hex, 8);
    SetLoraVar("APPEUI", Hex, 8);
    
    value = getenv("APPKEY");
    Str2Hex(value, Hex, 16);
    SetLoraVar("APPKEY", Hex, 16);
    
    value = getenv("NETID");
    Str2Hex(value, Hex, 4);
    SetLoraVar("NETID", Hex, 4);
    
    value = getenv("NWKSKEY");
    Str2Hex(value, Hex, 16);
    SetLoraVar("NWKSKEY", Hex, 16);
    
    value = getenv("APPSKEY");
    Str2Hex(value, Hex, 16);
    SetLoraVar("APPSKEY", Hex, 16);
}
int InitEnv(void)
{
    int Ret = 0;
    CmdTbl CmdTb;

    CmdTb.name = "reset";
    CmdTb.usage = "reset cpu";
    CmdTb.help = "";
    CmdTb.maxargs = 1;
    CmdTb.repeatable = 1;
    CmdTb.cmd = DoReset;
    
    MiniShellCmdRegister(&CmdTb);
    
    CmdTb.name = "port";
    CmdTb.usage = "config lorawan port";
    CmdTb.help = "";
    CmdTb.maxargs = 1;
    CmdTb.repeatable = 1;
    CmdTb.cmd = DoReset;

    MiniShellCmdRegister(&CmdTb);

    CmdTb.name = "adr";
    CmdTb.usage = "adaptive speed switch";
    CmdTb.help = "";
    CmdTb.maxargs = 1;
    CmdTb.repeatable = 1;
    CmdTb.cmd = DoReset;  

    MiniShellCmdRegister(&CmdTb);

    CmdTb.name = "dr";
    CmdTb.usage = "config speed";
    CmdTb.help = "";
    CmdTb.maxargs = 1;
    CmdTb.repeatable = 1;
    CmdTb.cmd = DoReset;  

    MiniShellCmdRegister(&CmdTb);

    CmdTb.name = "ch";
    CmdTb.usage = "config sx1278 chan";
    CmdTb.help = "";
    CmdTb.maxargs = 1;
    CmdTb.repeatable = 1;
    CmdTb.cmd = DoReset;

    MiniShellCmdRegister(&CmdTb);

    CmdTb.name = "power";
    CmdTb.usage = "config sx1278 transmit power";
    CmdTb.help = "";
    CmdTb.maxargs = 1;
    CmdTb.repeatable = 1;
    CmdTb.cmd = DoReset;

    MiniShellCmdRegister(&CmdTb);

    CmdTb.name = "mode";
    CmdTb.usage = "config sx1278 node join mode";
    CmdTb.help = "";
    CmdTb.maxargs = 1;
    CmdTb.repeatable = 1;
    CmdTb.cmd = DoReset;

    MiniShellCmdRegister(&CmdTb);

    CmdTb.name = "join";
    CmdTb.usage = "lorawan join cmd";
    CmdTb.help = "";
    CmdTb.maxargs = 1;
    CmdTb.repeatable = 1;
    CmdTb.cmd = DoReset;

    MiniShellCmdRegister(&CmdTb);

    CmdTb.name = "id";
    CmdTb.usage = "lorawan generation mode";
    CmdTb.help = "";
    CmdTb.maxargs = 1;
    CmdTb.repeatable = 1;
    CmdTb.cmd = DoReset;

    MiniShellCmdRegister(&CmdTb);
    // init env
    EnvRelocate();

    return Ret;
}

