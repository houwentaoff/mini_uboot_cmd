/******************************************************************************
 * @file    cmd.c
 * @author  Joy
 * @version V1.0.0
 * @date    17-Nov-2016
 * @brief   cmds 
 ******************************************************************************
 */
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "cmd.h"
#include "hw_eeprom.h"
#include "utilities.h"
#include "version.h"

extern int SetLoraVar(const char *Key, unsigned char *Hex, int Num);

/* Private functions ---------------------------------------------------------*/
static int DoReset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int Ret = 0;
    printf("not implemented\n");
    return Ret;
}
static int DoVersion(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    printf("%x\n", VERSION);
    return 0;
}

static void PrintUsage(cmd_tbl_t *cmdtp)
{
    const char *usage;
    
    if (!cmdtp)
    {
        printf("err param\n");
    }
    usage = cmdtp->usage;
    if (usage)
    {
        printf("%-*s- %s\n", 8,
            cmdtp->name, usage);
    }
#ifdef CONFIG_SYS_LONGHELP
    if (cmdtp->help)
    {
        printf("%-*s- %s\n", 8,
            cmdtp->name, cmdtp->help);
    }
#endif            
}
static int DoAdr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int Ret = 0;
    char *Value = NULL;  
    
    if (argc != 2 && argc != 3)
    {
        PrintUsage(cmdtp);
        return Ret;
    }
    if (argc == 2)
    {
        if (strcmp(argv[1], "get") == 0)
        {
            Value = getenv("adr");
            if (Value)
            {
                printf("%s\n", Value);
            }
            else
            {
                printf("not set!\n");
            }
        }
        else
        {
            printf("err para\n");
            Ret = -1;
            return Ret;
        }
    }
    if (argc == 3)
    {
        if (strcmp(argv[1], "set") == 0)
        {
            if (strcmp(argv[2], "1") == 0)
            {
                setenv("adr", "1");
            }
            else if (strcmp(argv[2], "0") == 0)
            {
                setenv("adr", "0");
            }
            else
            {
                printf("err para\n");
            }
        }
        else
        {
            printf("err para\n");
        }
    }
    return Ret;
}

static int DoMode(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int Ret = 0;
    char *Value = NULL;  
    
    if (argc != 2 && argc != 3)
    {
        PrintUsage(cmdtp);
        return Ret;
    }
    
    if (argc == 2)
    {
        if (strcmp(argv[1], "get") == 0)
        {
            Value = getenv("mode");
            if (Value)
            {
                printf("%s\n", Value);
            }
            else
            {
                printf("not set!\n");
            }
        }
        else
        {
            printf("err para\n");
            Ret = -1;
            return Ret;
        }
    }
    if (argc == 3)
    {
        if (strcmp(argv[1], "set") == 0)
        {
            if (strcmp(argv[2], "lwotaa") == 0)
            {
                setenv("mode", "lwotaa");
            }
            else if (strcmp(argv[2], "lwabp") == 0)
            {
                setenv("mode", "lwabp");
            }
            else
            {
                printf("err para\n");
            }
        }
        else
        {
            printf("err para\n");
        }
    }

    return Ret;
}

static int DoId(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int Ret = 0;
    char *Value = NULL;    
    
    if (argc != 2 && argc != 3)
    {
        PrintUsage(cmdtp);
        return Ret;
    }
    if (argc == 2)
    {
        if (strcmp(argv[1], "get") == 0)
        {
            Value = getenv("id");
            if (Value)
            {
                printf("%s\n", Value);
            }
            else
            {
                printf("not set!\n");
            }
        }
        else
        {
            printf("err para\n");
            Ret = -1;
            return Ret;
        }
    }
    if (argc == 3)
    {
        if (strcmp(argv[1], "set") == 0)
        {
            if (strcmp(argv[2], "manual") == 0)
            {
                setenv("id", "manual");
            }
            else if (strcmp(argv[2], "random") == 0)
            {
                setenv("id", "random");
            }
            else
            {
                printf("err para\n");
            }
        }
        else
        {
            printf("err para\n");
        }
    }
    
    return Ret;
}

static int DoPort(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int Ret = 0;
    int port = 0;
    unsigned char por;
    char *Value = NULL;    
    
    if (argc != 2 && argc != 3)
    {
        PrintUsage(cmdtp);
        return Ret;
    }
    if (argc == 2)
    {
        if (strcmp(argv[1], "get") == 0)
        {
            Value = getenv("port");
            if (Value)
            {
                printf("%s\n", Value);
            }
            else
            {
                printf("not set!\n");
            }
        }
        else
        {
            printf("err para\n");
            Ret = -1;
            return Ret;
        }
    }
    if (argc == 3)
    {
        if (strcmp(argv[1], "set") == 0)
        {
            port = atoi(argv[2]);
            if (port >0 && port <=255)
            {
                setenv("port", argv[2]);
                por = port&0xff;
                SetLoraVar("port", &por, 1);
            }
            else
            {
                printf("err para\n");
            }
        }
        else
        {
            printf("err para\n");
        }
    }

    return Ret;
}

void EnvRelocate(void)
{
    char *Value = NULL;    
    uint8_t Hex[16+1] = {0};
    
    if (hush_init_var() < 0)
    {
        return;
    }
    Value = getenv("DEVADDR");
    if (strcmp(Value, "00:00:00:00") != 0)
    {
        Str2Hex(Value, Hex, 4);
    }
    else
    {
        Str2Hex(CONFIG_DEVADDR, Hex, 4);
    }   
    SetLoraVar("DEVADDR", Hex, 4);
    
    Value = getenv("DEVEUI");
    if (strcmp(Value, "00:00:00:00:00:00:00:00") != 0)
    {
        Str2Hex(Value, Hex, 8);
    }
    else
    {
        Str2Hex(CONFIG_DEVEUI, Hex, 8);
    }
    SetLoraVar("DEVEUI", Hex, 8);

    Value = getenv("APPEUI");
    if (strcmp(Value, "00:00:00:00:00:00:00:00") != 0)
    {
        Str2Hex(Value, Hex, 8);
    }
    else
    {
        Str2Hex(CONFIG_APPEUI, Hex, 8);
    }
    SetLoraVar("APPEUI", Hex, 8);

    Value = getenv("APPKEY");
    if (strcmp(Value, "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00") != 0)
    {
        Str2Hex(Value, Hex, 16);
    }
    else
    {
        Str2Hex(CONFIG_APPKEY, Hex, 16);
    }
    SetLoraVar("APPKEY", Hex, 16);

    Value = getenv("NETID");
    if (strcmp(Value, "00:00:00") != 0)
    {
        Str2Hex(Value, Hex, 3);
    }
    else
    {
        Str2Hex(CONFIG_NETID, Hex, 3);
    }
    SetLoraVar("NETID", Hex, 3);

    Value = getenv("NWKSKEY");
    if (strcmp(Value, "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00") != 0)
    {
        Str2Hex(Value, Hex, 16);
    }
    else
    {
        Str2Hex(CONFIG_NWKSKEY, Hex, 16);
    }
    SetLoraVar("NWKSKEY", Hex, 16);

    Value = getenv("APPSKEY");
    if (strcmp(Value, "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00") != 0)
    {
        Str2Hex(Value, Hex, 16);
    }
    else
    {
        Str2Hex(CONFIG_APPSKEY, Hex, 16);
    }
    SetLoraVar("APPSKEY", Hex, 16);
    
}
int Init_Env(void)
{
    int Ret = 0;
    int i =0;
    CmdTbl CmdArray[]={
        {
            .name = "version",
            .maxargs = 1,
            .repeatable = 1,
            .cmd = DoVersion,
            .usage = "print lorawan node version",
#ifdef CONFIG_SYS_LONGHELP
            .help  = "",
#endif                 
        },
        {
            .name = "reset",
            .usage = "reset cpu",
#ifdef CONFIG_SYS_LONGHELP
            .help = "",
#endif            
            .maxargs = 1,
            .repeatable = 1,
            .cmd = DoReset,
        },
        {
            .name = "port",
            .usage = "config lorawan port",
#ifdef CONFIG_SYS_LONGHELP
            .help = "port set/get [num] num in (1-255)\neg:port set 111",
#endif            
            .maxargs = 3,
            .repeatable = 1,
            .cmd = DoPort,
        },
        {
            .name = "adr",
            .usage = "adaptive speed switch",
#ifdef CONFIG_SYS_LONGHELP
            .help = "adr set/get [1/0] 1 is true 0 is false.\neg: adr set 1",
#endif            
            .maxargs = 3,
            .repeatable = 1,
            .cmd = DoAdr,  
        },
        {
            .name = "dr",
            .usage = "config speed",
#ifdef CONFIG_SYS_LONGHELP
            .help = "",
#endif            
            .maxargs = 1,
            .repeatable = 1,
            .cmd = DoReset,  
        },
        {
            .name = "ch",
            .usage = "config sx1278 chan",
#ifdef CONFIG_SYS_LONGHELP
            .help = "",
#endif            
            .maxargs = 1,
            .repeatable = 1,
            .cmd = DoReset,
        },
        {
            .name = "power",
            .usage = "config sx1278 transmit power",
#ifdef CONFIG_SYS_LONGHELP
            .help = "",
#endif            
            .maxargs = 1,
            .repeatable = 1,
            .cmd = DoReset,
        },
        {
            .name = "mode",
            .usage = "config sx1278 node join mode",
#ifdef CONFIG_SYS_LONGHELP
            .help = "mode set/get [lwotaa/lwabp]",
#endif            
            .maxargs = 3,
            .repeatable = 1,
            .cmd = DoMode,
        },
        {
            .name = "join",
            .usage = "lorawan join cmd",
#ifdef CONFIG_SYS_LONGHELP
            .help = "",
#endif            
            .maxargs = 1,
            .repeatable = 1,
            .cmd = DoReset,
        },
        {
            .name = "id",
            .usage = "lorawan generation mode : random or manual",
#ifdef CONFIG_SYS_LONGHELP
            .help = "id set/get [random/manual]\n"
                    "eg: id get\n"
                    "    id set random\n",
#endif            
            .maxargs = 3,
            .repeatable = 1,
            .cmd = DoId,
        },
        
    };

    for (i=0; i<sizeof(CmdArray)/sizeof(CmdTbl); i++)
    {
        Shell_Cmd_Register(&CmdArray[i]);
    }
    // init env
    EnvRelocate();

    return Ret;
}

