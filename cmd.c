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

#define DR_NUM                   16         /* data rate num*/

typedef enum DRType{
    US915 = 0,
    EU868,
    CUSTOM,
    DRTYPENUM,
}DRType_e;

typedef struct DRData{
    DRType_e DrType;
    unsigned char CurDr;
    unsigned char DrUp[DR_NUM];
    unsigned char SpreadFac[DR_NUM];
    unsigned char DrDown[DR_NUM];
    int BandWidth[DR_NUM];
    int DrNum;
}DRData_t;

static DRData_t DrData;

extern void SX1276Reset( void );
extern int SetLoraVar(const char *Key, unsigned char *Hex, int Num);

/* Private functions ---------------------------------------------------------*/
static int DoReset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int Ret = 0;
    printf("reset...\n");//can not printf 
    DISABLE_IRQ();
    HAL_NVIC_SystemReset();
    SX1276Reset();
    return Ret;
}
static int DoVersion(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int Ret = 0;
    printf("%x\n", VERSION);
    return Ret;
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
const char *DrType2Str(DRType_e Type)
{
    static const char *TypeName[] ={"US915", "EU868", "CUSTOM", "ERROR"};

    if (DRTYPENUM >= sizeof (TypeName) / sizeof (char *))
    {
        return TypeName[3];
    }
    
    return Type < DRTYPENUM ? TypeName[Type]:TypeName[3];
}

static void DoDrPrint(DRData_t *pDrData)
{
    int i  = 0;
    int DrNum = 0;

    if (!pDrData)
    {
        printf("err dr data!\n");
        return ; 
    }
    DrNum = pDrData->DrNum;
    for (i = 0; i<DrNum; i++)
    {
        printf("%s DR%d SF%d BW%d K\n", DrType2Str(pDrData->DrType), pDrData->DrUp[i],
                               pDrData->SpreadFac[i], pDrData->BandWidth[i]);
    }
}

static int DoDr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int Ret = 0;
    int DataRateUp = 0;
    int DataRateDown = 0; /*down map datarate */
    int SpreadFactor = 0;
    int BandWidth = 0;
    int CurDr     = 0;
    
    /*-----------------------------------------------------------------------------
       *  -d  DR0->DR15 : set dr is dr0->dr5
       *          dr  dr2
       *          display:
       *  --scheme  EU868 EU868-like US915
       *          dr  --scheme 
       *          display all DR: US915 DR2 SF5 BW500K
       *          dr --scheme  eu868
       *          dr --scheme  us915
       *          dr --scheme  custom
       *     dr custom drx sfx bw [drx (rxwin1)]
       *     The RX1 window data rate depends on the transmit data rate
       *          dr custom dr0 sf7 125         // set DR0 to SF7 and bw125KHZ
       *          dr custom dr3 sf10 500 dr4    // set dr3 to sf10 and bw500KHZ and map dr3 with dr4
       *-----------------------------------------------------------------------------*/

    if (argc < 2 || argc > 6 || argc == 4)
    {
        PrintUsage(cmdtp);
        return Ret;
    }
    if (argc == 2)
    {
        if (strcmp(argv[1], "--scheme") == 0)
        {
            DoDrPrint(&DrData);
        }
        else if (strncmp(argv[1], "dr", 2) == 0)
        {
            if (1 != sscanf(argv[1], "dr%d", &CurDr))
            {
                printf("param err!\n");
                return Ret;
            }
            if (CurDr >= DR_NUM || CurDr < 0)
            {
                printf("param err!\n");
                return Ret;
            }
            DrData.CurDr = CurDr & 0xff;
        }
        else
        {
            PrintUsage(cmdtp);
            return Ret;
        }
    }
    else if (argc == 3)
    {
        if (strcmp(argv[1], "--scheme") == 0)
        {
            if (strcmp(argv[2], "eu868"))
            {
                DrData.DrType = EU868;
            }
            else if (strcmp(argv[2], "us915"))
            {
                DrData.DrType = US915;
            }
            else if (strcmp(argv[2], "custom"))
            {
                DrData.DrType = CUSTOM;
            }
            else
            {
                PrintUsage(cmdtp);
                return Ret;
            }
        }
        else
        {
            PrintUsage(cmdtp);
            return Ret;
        }
    }
    else if (argc == 5 || argc == 6)
    {
        if (strcmp(argv[1], "custom") == 0)
        {
            DrData.DrType = CUSTOM;
        }
        else if (strcmp(argv[1], "us915") == 0)
        {
            DrData.DrType = US915;
        }
        else if (strcmp(argv[1], "eu868") == 0)
        {
            DrData.DrType = EU868;
        }
        else
        {
            Ret = -1;
            return Ret;
        }
        if (1 != sscanf(argv[2], "dr%d", &DataRateUp))
        {
            printf("param err!\n");
            return Ret;
        }
        if (1 != sscanf(argv[3], "sf%d", &SpreadFactor))
        {
            printf("param err!\n");
            return Ret;
        }
        if (1 != sscanf(argv[4], "%d", &BandWidth))
        {
            printf("param err!\n");
            return Ret;
        }
        if (DataRateUp >= DR_NUM || DataRateUp < 0)
        {
            printf("param err!\n");
            return Ret;
        }
        DrData.SpreadFac[DataRateUp] = SpreadFactor & 0xff;
        DrData.BandWidth[DataRateUp] = BandWidth & 0xff;
        if (argc == 6)
        {
            if (1 != sscanf(argv[5], "dr%d", &DataRateDown))
            {
                printf("param err!\n");
                return Ret;
            }
            if (DataRateDown >= DR_NUM || DataRateDown < 0)
            {
                printf("param err!\n");
                return Ret;
            }
            DrData.DrDown[DataRateUp] = DataRateDown & 0xff;
        }
        
    }
    return Ret;
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
            if (strcmp(argv[2], "static") == 0)
            {
                setenv("id", "static");
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

    memset(&DrData, 0, sizeof (DRData_t));    
    DrData.DrNum = DR_NUM;
#ifdef CONFIG_MINISHELL
    if (hush_init_var() < 0)
    {
        return;
    }
#endif
#ifdef CONFIG_MINISHELL
    Value = getenv("DEVADDR");
#else
    Value = CONFIG_DEVADDR;
#endif 
    if (strcmp(Value, "00:00:00:00") != 0)
    {
        Str2Hex(Value, Hex, 4);
    }
    else
    {
        Str2Hex(CONFIG_DEVADDR, Hex, 4);
    }   
    SetLoraVar("DEVADDR", Hex, 4);
#ifdef CONFIG_MINISHELL
    Value = getenv("DEVEUI");
#else
    Value = CONFIG_DEVEUI;
#endif 
    if (strcmp(Value, "00:00:00:00:00:00:00:00") != 0)
    {
        Str2Hex(Value, Hex, 8);
    }
    else
    {
        Str2Hex(CONFIG_DEVEUI, Hex, 8);
    }
    SetLoraVar("DEVEUI", Hex, 8);
#ifdef CONFIG_MINISHELL
    Value = getenv("APPEUI");
#else
    Value = CONFIG_APPEUI;
#endif
    if (strcmp(Value, "00:00:00:00:00:00:00:00") != 0)
    {
        Str2Hex(Value, Hex, 8);
    }
    else
    {
        Str2Hex(CONFIG_APPEUI, Hex, 8);
    }
    SetLoraVar("APPEUI", Hex, 8);
#ifdef CONFIG_MINISHELL
    Value = getenv("APPKEY");
#else
    Value = CONFIG_APPKEY;
#endif
    if (strcmp(Value, "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00") != 0)
    {
        Str2Hex(Value, Hex, 16);
    }
    else
    {
        Str2Hex(CONFIG_APPKEY, Hex, 16);
    }
    SetLoraVar("APPKEY", Hex, 16);
#ifdef CONFIG_MINISHELL
    Value = getenv("NETID");
#else
    Value = CONFIG_NETID;
#endif
    if (strcmp(Value, "00:00:00") != 0)
    {
        Str2Hex(Value, Hex, 3);
    }
    else
    {
        Str2Hex(CONFIG_NETID, Hex, 3);
    }
    SetLoraVar("NETID", Hex, 3);
#ifdef CONFIG_MINISHELL
    Value = getenv("NWKSKEY");
#else
    Value = CONFIG_NWKSKEY;
#endif
    if (strcmp(Value, "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00") != 0)
    {
        Str2Hex(Value, Hex, 16);
    }
    else
    {
        Str2Hex(CONFIG_NWKSKEY, Hex, 16);
    }
    SetLoraVar("NWKSKEY", Hex, 16);
#ifdef CONFIG_MINISHELL
    Value = getenv("APPSKEY");
#else
    Value = CONFIG_APPSKEY;
#endif
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
            .help = "set/get [num] num in (1-255)\neg:port set 111",
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
            .usage = "config speed, spread fac, up dr, down dr, baud width",
#ifdef CONFIG_SYS_LONGHELP
            .help = " --scheme [eu868/us915/custom]\n"
                    "dr  --scheme              \t - display all data rate\n"
                    "dr dr[0-15]               \t - set cur data rate\n"
                    "dr custom dr0 sf7 125     \t - set DR0 to SF7 and bw125KHZ\n"
                    "dr custom dr3 sf10 500 dr4\t - set dr3 to sf10 and bw500KHZ and map dr3 with dr4\n",
#endif            
            .maxargs = 6,
            .repeatable = 1,
            .cmd = DoDr,  
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
            .usage = "lorawan generation mode : random or static",
#ifdef CONFIG_SYS_LONGHELP
            .help = "id set/get [random/static]\n"
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

