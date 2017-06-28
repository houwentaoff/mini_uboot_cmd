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
#include <stdbool.h>
#include "cmd.h"
#include "hw_eeprom.h"
#include "utilities.h"
#include "version.h"
#include "radio.h"

#include "LoRaMac.h"
#include "LoRa.h"

#define DR_NUM                   16         /* data rate num*/
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

//#define CN470_DEFAULT_TX_POWER                    TX_POWER_1


#define MAX_SF      12
#define MIN_SF      7
#define MAX_POWER   7
#define MIN_POWER   0
#define MIN_FREQ_RANGE  400000000
#define MAX_FREQ_RANGE  600000000

//#define DEFAULT_TX_TIMEOUT
#define BLOCK_SIZE   32

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
//typedef struct global GlobalData_t;
//unsigned char    data[1024]; /* Environment data        */

unsigned int  other_addr = CONFIG_OTHER_VAR_OFFSET; /* e2p data*/

typedef struct global
{
    uint32_t    crc;        /* CRC32 over data bytes    */    
    LoRaMacTxConfig_t TxConfig;
    DRData_t          DRData;
    /**/
    TestMsg_t         TestMsg;
}GlobalData_t;
//void (*save)(GlobalData_t *self);
//void (*load)(GlobalData_t *self);

extern void SX1276Reset( void );
extern int SetLoraVar(const char *Key, unsigned char *Hex, int Num);
extern TestMsg_t *GetTestMsg(void);
extern void SetLoRaMacParams(const int8_t *ChannelsTxPower, const int8_t *ChannelsDatarate);
extern int8_t GetIndexOfTxpower(uint32_t txPower);
extern void SetLoRaParam(const uint32_t *TxDutyCycleTime, const int8_t *TxDatarate);
static int CheckAndSetTxParam(LoRaMacTxConfig_t *TxConfig);


/* Private functions ---------------------------------------------------------*/
#ifdef CONFIG_MINISHELL
typedef struct Ch_Config
{
    unsigned char index;
    float freq;
    int dr_min;
    int dr_max;
}Ch_Config_t;
#define MAX_CHANS   8
Ch_Config_t Chs[MAX_CHANS];

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


static int DoTestMsg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int Ret = 0;
    int i = 0;
    int total = 0;
    TestMsg_t *testMsg = GetTestMsg();
    
    if (argc < 2)
    {
        PrintUsage(cmdtp);
        return Ret;
    }
    if (!strcmp(argv[1], "--byte"))
    {
        if (argc != 3)
        {
            Ret = -1;
            return Ret;
        }
        total = atoi(argv[2]);
        if (total > 1024*1024)
        {
            printf("too big\n");
            Ret = -1;
            return Ret;
        }
        testMsg->total = total;
        testMsg->sended= 0;
        return Ret;       
    }    
    for (i=1; i<argc; i++)
    {
        if (!strcmp(argv[i], "--status"))
        {
            printf("%%%d ", (testMsg->sended*100)/testMsg->total);
        }
        else if (!strcmp(argv[i], "--left"))
        {
            printf("%d ", testMsg->total-testMsg->sended);
        }
        else if (!strcmp(argv[i], "--total"))
        {
            printf("%d ", testMsg->total);
        }
        else
        {
            break;
        }
    }
    if (i != argc)
    {
        Ret = -1;
        printf("param err!\n");
        return Ret;
    }
    printf("\n");
    return Ret;
}
static int DoConfigTx(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    LoRaMacTxConfig_t *TxConfig = LoRaMacGetTxConfig();
    /*
        *  txconfig 
        *  power   Sets the output power [dBm]
            LoRa: [0: 125 kHz, 1: 250 kHz,
        *                                 2: 500 kHz, 3: Reserved] 
        *   datarate   6-12
        *   coderate   1-4  
        *   preambleLen  8
        *
        *   timeout    [ms]
        *   eg:txconfig --power [0-7]  --bandwidth [125/250/500] --sf [7-12] --timeout=3000
        *       txconfig get
        */ 
    int Ret = 0;
    int i =0;
    int8_t txPower=0;
    uint32_t bandwidth=0;
    uint32_t datarate=0;
    uint32_t  freq = 0;
    //uint8_t coderate=0;
    //bool fixLen;
    //bool crcOn; 
    //bool FreqHopOn;
    //uint8_t HopPeriod;
    //bool iqInverted;
    uint32_t timeout=0;        
    uint8_t setMap = 0;
    uint32_t sleepTime = 0;
    int8_t indexDatarate = 0;
    int8_t indexTxPower = 0;
    
    if (argc < 2)
    {
        PrintUsage(cmdtp);
        return Ret;
    }
    if (!strcmp(argv[1], "get"))
    {
        printf("txconfig  power %d bandwidth %u sf %u timeout %u freq %u sleeptime %u\n",
            TxConfig->txPower, TxConfig->bandwidth, TxConfig->datarate, TxConfig->timeout,
            TxConfig->frequency, TxConfig->sleepTime);
        return Ret;
    }
    for (i=1; i<argc; i++)
    {
        if (!strcmp(argv[i], "--power"))
        {
            txPower = atoi(argv[++i]);
            if (txPower < MIN_POWER && txPower > MAX_POWER)
            {
                break;
            }
            //if (txPower < 1 || txPower > 20)
            //{
            //    break;
            //}
            setMap |= 0x1;
        }
        else if (!strcmp(argv[i], "--bandwidth"))
        {
            bandwidth = (uint32_t)atol(argv[++i]);
            if (bandwidth != 125 && bandwidth!=250 && bandwidth!=500)
            {
                break;
            }
            setMap |= 0x1<<1;
        }
        else if (!strcmp(argv[i], "--freq"))
        {
            freq = (uint32_t)atol(argv[++i]);
            if (freq < MIN_FREQ_RANGE|| freq > MAX_FREQ_RANGE)
            {
                break;
            }  
            setMap |= 0x1<<5;
        }
        else if (!strcmp(argv[i], "--sf"))
        {
            datarate = (uint32_t)atol(argv[++i]);
            if (datarate < MIN_SF || datarate > MAX_SF)
            {
                break;
            }            
            setMap |= 0x1<<2;
        }        
        else if (!strcmp(argv[i], "--timeout"))
        {
            timeout = (uint32_t)atol(argv[++i]);
            setMap |= 0x1<<3;
        }
        else if (!strcmp(argv[i], "--sleeptime"))
        {
            sleepTime = (uint32_t)atol(argv[++i]);
            setMap |= 0x1<<4;
        }
    }
    if (i != argc)
    {
        Ret = -1;
        printf("param err!\n");
        return Ret;
    }
    
    if (setMap&0x1)
    {
        TxConfig->txPower = txPower;
        indexTxPower = GetIndexOfTxpower(txPower);
        SetLoRaMacParams(&indexTxPower, NULL);
    }
    if (setMap&(0x1<<1))
    {
        TxConfig->bandwidth = bandwidth;
    }
    if (setMap&(0x1<<2))
    {
        TxConfig->datarate = datarate;
        indexDatarate = GetIndexOfDatarate(datarate);
        SetLoRaMacParams(NULL, &indexDatarate);        
        SetLoRaParam(NULL, &indexDatarate);
    }
    if (setMap&(0x1<<3))
    {
        TxConfig->timeout = timeout;
    }
    if (setMap&(0x1<<4))
    {
        TxConfig->sleepTime = sleepTime;
        SetLoRaParam(&sleepTime, NULL);
    }
    if (setMap&(0x1<<5))
    {
        TxConfig->frequency = freq;
    }
    
    //memcpy((unsigned char *)other_addr+offsetof(GlobalData_t, TxConfig), TxConfig, sizeof(LoRaMacTxConfig_t));
    write_phy_addr(offsetof(GlobalData_t, TxConfig), TxConfig, sizeof(LoRaMacTxConfig_t));
    return Ret;
}
static int DoCh(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    /*-----------------------------------------------------------------------------
      * Set channel parameter of LoRaWAN modem, Set frequecy zero to disable one channel.
        Format:
        AT+CH="LCn", ["Freq"], ["DR_MIN"], ["DR_MAX"]
        // Change the LCn channel frequency to "Freq"
        // "Freq" is in MHz.
        // Available DR_MIN/DR_MAX range DR0 ~ DR15
        // Disable channel LC2
        eg: AT+CH=2, 0
      *-----------------------------------------------------------------------------*/
    int Ret = 0;
 
    ChannelParams_t *chan = NULL;
    if (argc < 3)
    {
        PrintUsage(cmdtp);
        return Ret;
    }
    if (argc == 3)
    {
        if (strcmp(argv[1], "get") == 0)
        {
            int chanNum;
            chanNum = atoi(argv[2]);
            if (chanNum < 0 || chanNum > 8)
            {
                printf("err param\n");
                Ret = -1;
                return Ret;
            }
            chan = LoRaMacGetChan(chanNum);
            printf("%d:%u,DR%d DR%d\n", chanNum, chan->Frequency, chan->DrRange.Value & 0xf, chan->DrRange.Value & 0xf0 >> 4);
        }
        else
        {
            printf("err para\n");
            Ret = -1;
            return Ret;
        }
    }
    if (argc > 4)
    {
        if (strcmp(argv[1], "set") == 0)
        {
            int chanNum;
            float freq;
            int dr_min;
            int dr_max;
            if (argc == 4)
            {
                chanNum = atoi(argv[2]);
                freq = atof(argv[3]);
            }
            else if (argc == 5)
            {
                chanNum = atoi(argv[2]);
                freq = atof(argv[3]);
                sscanf(argv[4], "DR%i", &dr_min);
            }
            else if (argc == 6)
            {
                chanNum = atoi(argv[2]);
                freq = atof(argv[3]);
                sscanf(argv[4], "DR%i", &dr_min);
                sscanf(argv[5], "DR%i", &dr_max);
            }
            if (chanNum < 0 || chanNum > 8)
            {
                printf("err param\n");
                Ret = -1;
                return Ret;
            }
            //Chs[chanNum].freq = freq;
            //Chs[chanNum].dr_min = dr_min;
            //Chs[chanNum].dr_max = dr_max;
            chan = LoRaMacGetChan(chanNum);
            chan->Frequency = (uint32_t)(freq*100);
            chan->DrRange.Value = dr_max&0xf<<4 | dr_min&0xf;
            
        }
        else
        {
            printf("err para\n");
            Ret = -1;
            return Ret;
        }
    }
    return Ret;
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
#endif
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
    LoRaMacTxConfig_t *TxConfig = LoRaMacGetTxConfig();
    //memcpy(TxConfig, (unsigned char*)other_addr+offsetof(GlobalData_t, TxConfig), sizeof(LoRaMacTxConfig_t));
    read_phy_addr(TxConfig, offsetof(GlobalData_t, TxConfig), sizeof(LoRaMacTxConfig_t));
    CheckAndSetTxParam(TxConfig);
}

static int CheckAndSetTxParam(LoRaMacTxConfig_t *TxConfig)
{
    int8_t datarate = 0;
    int8_t txPower = 0;

    if (!TxConfig)
    {
        return -1;
    }

    TxConfig->coderate = 0;
    TxConfig->preambleLen = 0;
    TestMsg_t *TestMsg = GetTestMsg();
    TestMsg->blockSize = BLOCK_SIZE;
    TestMsg->sended = 0;
    TestMsg->total = 0;
    
    if (TxConfig->datarate < MIN_SF|| TxConfig->datarate > MAX_SF)
    {
        TxConfig->datarate = CONFIG_DEFAULT_SF;
    }
    if (TxConfig->txPower < MIN_POWER|| TxConfig->txPower > MAX_POWER)
    {
        TxConfig->txPower = CONFIG_DEFAULT_POWER;
    }
    if (TxConfig->timeout == 0 || TxConfig->timeout == 0xffffffff)
    {
        TxConfig->timeout = DEFAULT_TX_TIMEOUT;
    }
    if (TxConfig->sleepTime == 0 || TxConfig->sleepTime == 0xffffffff)
    {
        TxConfig->sleepTime = CONFIG_DEFAULT_SLEEPTIME;
    }
    if (TxConfig->frequency < MIN_FREQ_RANGE || TxConfig->frequency > MAX_FREQ_RANGE)
    {
        TxConfig->frequency = CONFIG_DEFAULT_FREQ;
    }
    if (TxConfig->bandwidth != 125 && TxConfig->bandwidth != 250 && TxConfig->bandwidth != 500)
    {
        TxConfig->bandwidth = CONFIG_DEFAULT_BANDWIDTH;
    }
    datarate = GetIndexOfDatarate(TxConfig->datarate);
    txPower = GetIndexOfTxpower(TxConfig->txPower);
    
    SetLoRaParam(&TxConfig->sleepTime, &datarate);
    SetLoRaMacParams(&txPower, &datarate);
    if (TxConfig->bandwidth != 125 && TxConfig->bandwidth != 250 && TxConfig->bandwidth != 500)
    {
        TxConfig->bandwidth = DEFAULT_BANDWIDTH;
    }
    return 0;
}
int Init_Env(void)
{
    int Ret = 0;
#ifdef CONFIG_MINISHELL
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
            .help = "ch [num] [Freq] [DR_MIN] [DR_MAX]\n"
                    "Change the LCn channel frequency to \"Freq\""
                    "\"Freq\" is in MHz. Available DR_MIN/DR_MAX range DR0 ~ DR15"
                    "Set frequecy zero to disable one channel",
#endif            
            .maxargs = 1,
            .repeatable = 1,
            .cmd = DoCh,
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
        {
            .name = "txconfig",
            .usage = "txconfig [get]/...",
#ifdef CONFIG_SYS_LONGHELP
            .help = "txconfig [get] --power 2  --bandwidth 500  --sf 7  --timeout 3000  --freq 434000000 --sleeptime 2000\n"
                    "eg: txconfig get\n"
                    "    txconfig  --power 2  --bandwidth 500  --sf 7  --timeout 3000  --freq 434000000 --sleeptime 2000\n",
#endif            
            .maxargs = 13,
            .repeatable = 1,
            .cmd = DoConfigTx,
        },
        {
            .name = "testmsg",
            .usage = "testmsg --byte [1024] /[--status] [--left] [ --total]",
#ifdef CONFIG_SYS_LONGHELP
            .help = ""
                    "eg: \n"
                    "    testmsg --byte 1024\n"
                    "    testmsg  --status --left --total\n",
#endif            
            .maxargs = 4,
            .repeatable = 1,
            .cmd = DoTestMsg,
        },          
        
    };

    for (i=0; i<sizeof(CmdArray)/sizeof(CmdTbl); i++)
    {
        Shell_Cmd_Register(&CmdArray[i]);
    }
#endif    
    // init env
    EnvRelocate();

    return Ret;
}

