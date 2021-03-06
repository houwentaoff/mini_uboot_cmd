/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Jz.
 *       Filename:  minishell_core.c
 *
 *    Description:  This file is porting from the project uboot so keep its style.
 *         Others:
 *
 *        Version:  1.0
 *        Created:  11/11/2016 02:07:33 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Joy. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Jz
 *
 * =====================================================================================
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include "config.h"
#include "errno.h"
#include "minishell_core.h"

#ifndef __CHECKER__
#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

/*  Flags for himport_r(), hexport_r(), hdelete_r(), and hsearch_r() */
#define H_NOCLEAR       (1 << 0) /*  do not clear hash table before importing */
#define H_FORCE         (1 << 1) /*  overwrite read-only/write-once variables */
#define H_INTERACTIVE   (1 << 2) /*  indicate that an import is user directed */
#define H_HIDE_DOT      (1 << 3) /*  don't print env vars that begin with '.' */ 

#define ENV_SIZE               (CONFIG_ENV_SIZE - ENV_HEADER_SIZE)  /* env size */

#define CMD_FLAG_REPEAT  0x0001/*  repeat last command*/
#define CMD_FLAG_BOOTD   0x0002/*  command is from bootd*/ 

#define GD_FLG_ENV_READY    0x00080    /* Env. imported into hash table   */

#define CONFIG_SYS_HELP_CMD_WIDTH   8       
#define CONFIG_SYS_MAXARGS     16            /* all cmd max args */
#define CONFIG_SYS_CBSIZE      256          /* Console I/O Buffer Size */
#define CONFIG_SYS_CMDCOUNT    30           /* Cmd Count */
#define CONFIG_SYS_VARCOUNT    20           /*  env var count*/

#define CONFIG_ENV_MIN_ENTRIES CONFIG_SYS_VARCOUNT
#define CONFIG_ENV_MAX_ENTRIES (CONFIG_SYS_VARCOUNT+2)

#define CRC_DEFAULT       0x12223341

#define     CMD_DEBUG               0

#define __stringify_1(x...)    #x
#define __stringify(x...)    __stringify_1(x)

#define __set_errno(val) do { errno = val; } while (0)

#if CMD_DEBUG
#define cmd_debug(...)  printf(__VA_ARGS__)           /*  */
#else
#define cmd_debug(...) 
#endif

#define cmd_err(fmt, args...)                                                               \
do                                                                                          \
{                                                                                           \
        printf("[minishell][\033[1;31mERROR\033[0m] ---> %s ():line[%d]:", __func__, __LINE__);   \
        printf(" "fmt, ##args);                                                             \
}while(0)    /*  */

enum env_op {
    env_op_create,
    env_op_delete,
    env_op_overwrite,
};
typedef  int ssize_t;
typedef unsigned long phys_addr_t;
typedef unsigned long phys_size_t;
typedef unsigned int ulong;
typedef unsigned int uint;

/*  Action which shall be performed in the call the hsearch.  */
typedef enum {
    FIND,
    ENTER
} ACTION;

typedef struct entry {
    const char *key;
    char *data;
    int (*callback)(const char *name, const char *value, enum env_op op,
            int flags);
    int flags;
} ENTRY;

typedef struct _ENTRY {
    int used;
    ENTRY entry;
} _ENTRY;

/*
 *  * Family of hash table handling functions.  The functions also
 *  * have reentrant counterparts ending with _r.  The non-reentrant
 *  * functions all work on a signle internal hashing table.
 *  */

/*  Data type for reentrant functions.  */
struct hsearch_data {
    struct _ENTRY *table;
    unsigned int size;
    unsigned int filled;
    /*
     ** Callback function which will check whether the given change for variable
     ** "item" to "newval" may be applied or not, and possibly apply such change.
     ** When (flag & H_FORCE) is set, it shall not print out any error message and
     ** shall force overwriting of write-once variables.
     ** Must return 0 for approval, 1 for denial.
     **/
    int (*change_ok)(const ENTRY *__item, const char *newval, enum env_op,
            int flag);
};

enum command_ret_t {
    CMD_RET_SUCCESS,/*  0 = Success */
    CMD_RET_FAILURE,/*  1 = Failure */
    CMD_RET_USAGE = -1,/*  Failure, please report 'usage' error */
};

typedef struct global_data {
    unsigned int flags;
    unsigned int  env_addr;     /* Address  of Environment struct */
    unsigned char env_valid;    /* Checksum of Environment valid? */
    unsigned int  other_addr;
}gd_t;

typedef struct environment_s {
    uint32_t    crc;        /* CRC32 over data bytes    */
#ifdef CONFIG_SYS_REDUNDAND_ENVIRONMENT  /* (from uboot) */
    unsigned char    flags;        /* active/obsolete flags    */
#endif
    unsigned char    data[ENV_SIZE]; /* Environment data        */
} env_t;

callback g_mini_shellcb = NULL;

extern int32_t EnvWrite(const char *Name, const char *Value);
extern int32_t EnvRead(const char *Name, char *Value);
extern int32_t E2promBusRead(unsigned int DevAddr, unsigned int Offset,
               unsigned char *Buffer, unsigned int Cnt);
extern int32_t E2promBusWrite(unsigned int DevAddr, unsigned int Offset,
                unsigned char *Buffer, unsigned int Cnt);
extern int32_t E2promBusErase(unsigned int DevAddr, unsigned int Offset,
                unsigned int Cnt);
extern unsigned long SimpleStrtoul(const char *cp, char **endp,
                unsigned int Base);


static int do_help(struct cmd_tbl_s *cmdtp, int flag, int argc, char * const argv[]);
#ifdef MINISHELL_VERSION
static int do_version(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
#endif
static int do_env_print(cmd_tbl_t *cmdtp, int flag, int argc,
            char * const argv[]);
static int do_env_set(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
static int do_env_save(cmd_tbl_t *cmdtp, int flag, int argc,
               char * const argv[]);
static int do_env_default(cmd_tbl_t *cmdtp, int __flag,
              int argc, char * const argv[]);
static int do_mem_md(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
static int do_eeprom ( cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[]);

cmd_tbl_t cmd_tbls[CONFIG_SYS_CMDCOUNT] = {
    {
        .name = "help",
        .maxargs = CONFIG_SYS_MAXARGS,
        .repeatable = 1,
        .cmd   = do_help,
        .usage = "print command description/usage\n",
#ifdef CONFIG_SYS_LONGHELP
        .help  = "\n"
                 "   - print brief description of all commands\n"
                 "help command ...\n"
                 "   - print detailed usage of 'command'",
#endif                 
    },
    {
        .name = "default",
        .maxargs = 8,
        .repeatable = 0,
        .cmd = do_env_default,
        .usage = "load default env",
#ifdef CONFIG_SYS_LONGHELP
        .help  = "- [-f] -a - [forcibly] reset default environment\n"
                     "default [-f] var [...] - [forcibly] reset variable(s) to their default values\n",
#endif                 
    },      
#ifdef MINISHELL_VERSION
    {
        .name = "version",
        .maxargs = 1,
        .repeatable = 1,
        .cmd = do_version,
        .usage = "print monitor, compiler and linker version",
#ifdef CONFIG_SYS_LONGHELP
        .help  = "",
#endif                 
    },
#endif
    {
        .name = "printenv",
        .maxargs = CONFIG_SYS_MAXARGS,
        .repeatable = 1,
        .cmd   = do_env_print,
        .usage = "print environment variables",
#ifdef CONFIG_SYS_LONGHELP
        .help  = "[-a]\n    - print [all] values of all environment variables\n"
                 "printenv name ...\n"
                 "    - print value of environment variable 'name'",
#endif                 
    },
    {
        .name  = "setenv",
        .maxargs = CONFIG_SYS_MAXARGS,
        .repeatable = 0,
        .cmd   = do_env_set,
        .usage = "set environment variables",
#ifdef CONFIG_SYS_LONGHELP
        .help  = "name value ...\n"
                 "set environment variable 'name' to 'value ...'\n"
                 "setenv name\n"
                 "delete environment variable 'name'",
#endif                 
    }, 
    {
        .name  = "saveenv",
        .maxargs = 1,
        .repeatable = 0,
        .cmd   = do_env_save,
        .usage = "save environment variables to persistent storage",
#ifdef CONFIG_SYS_LONGHELP
        .help  = "",
#endif                 
    },
    {
        .name  = "md",
        .maxargs = 3,
        .repeatable = 1,
        .cmd   = do_mem_md,
        .usage = "memory display",
#ifdef CONFIG_SYS_LONGHELP
        .help  = "[.b, .w, .l] address [# of objects]\n"
                 "eg:md.w 0x08080800 10",
#endif                 
    },
    {
        .name  = "eeprom",
        .maxargs = 4,
        .repeatable = 1,
        .cmd   = do_eeprom,
        .usage = "EEPROM sub-system",
#ifdef CONFIG_SYS_LONGHELP
        .help  = "read off cnt\n"
                 "eeprom erase [all] off cnt\n"
                 "       - read `cnt' bytes at EEPROM offset `off'\n"
                 "eg:eeprom erase 0xC00 0x400\n"
                 "eg:eeprom read 0xC00 0x10\n"
                 "eg:eeprom erase all",
#endif                 
    },
};
const char __weak version_string[] = MINISHELL_VERSION_STRING;
const char *env_name_spec = "EEPROM";
#ifdef MINISHELL_VERSION
static int cmd_count = 8;    /* cur cmd count : in arrary (cmd_tbls)*/
#else
static int cmd_count = 7;
#endif
/*  test if ctrl-c was pressed */
static int ctrlc_disabled = 0;/*  see disable_ctrl() */
static int ctrlc_was_pressed = 0;
char        console_buffer[CONFIG_SYS_CBSIZE + 1]; /*  console I/O buffer*/
static const char erase_seq[] = "\b \b";/* erase sequence */
static const char   tab_seq[] = "        ";/* used to expand TABs*/
#ifdef LEGACY_STYLE
static const char *names[] = {"DEVADDR", "DEVEUI", "APPEUI", "APPKEY", "NETID", "NWKSKEY", "APPSKEY", NULL};
#endif
_ENTRY entry[CONFIG_SYS_VARCOUNT];
static gd_t global_data;
struct hsearch_data env_htab = {
    .table     = entry,
    .size      = CONFIG_SYS_VARCOUNT,
    .filled    = 0,
    .change_ok = NULL,/* env_flags_validate, (from uboot) */
};
int errno = 0;
static int env_id = 1;
static unsigned char env_buf[CONFIG_ENV_SIZE];

static char default_environment[] = {
    "DEVADDR=" CONFIG_DEVADDR                         "\0"
    "DEVEUI="  CONFIG_DEVEUI                          "\0"
    "APPEUI="  CONFIG_APPEUI                          "\0"
    "APPKEY="  CONFIG_APPKEY                          "\0"
    "NETID="   CONFIG_NETID                           "\0"
    "NWKSKEY=" CONFIG_NWKSKEY                         "\0"
    "APPSKEY=" CONFIG_APPSKEY                         "\0"
    "id="      CONFIG_ID                              "\0"
    "mode="    CONFIG_MODE                            "\0"
    "port="    __stringify(CONFIG_PORT)               "\0"
    "adr="     __stringify(CONFIG_ADR)                "\0"
    "\0"
};

static int _do_env_set(int flag, int argc, char * const argv[]);
static int env_print(char *name, int flag);
cmd_tbl_t *find_cmd_tbl (const char *cmd, cmd_tbl_t *table, int table_len);
cmd_tbl_t *find_cmd (const char *cmd);
static int cmd_usage(const cmd_tbl_t *cmdtp);
static void set_default_env(const char *s);

static int hsearch_r(ENTRY item, ACTION action, ENTRY ** retval,
        struct hsearch_data *htab, int flag);
static int himport_r(struct hsearch_data *htab,
        const char *env, size_t size, const char sep, int flag,
        int nvars, char * const vars[]);

static int do_env_save(cmd_tbl_t *cmdtp, int flag, int argc,
               char * const argv[])
{
    printf("Saving Environment to %s...\n", env_name_spec);

    return saveenv() ? 1 : 0;
}

static int do_help(struct cmd_tbl_s *cmdtp, int flag, int argc, char * const argv[])
{
    (void)flag;
    int i;
    int rcode = 0;
    
    if (argc == 1)
    {
        for (cmdtp = &cmd_tbls[0]; cmdtp < &cmd_tbls[CONFIG_SYS_CMDCOUNT-1] ;cmdtp++)
        {
            const char *usage = cmdtp->usage;
            if (usage == NULL)
                continue;
            printf("%-*s- %s\n", CONFIG_SYS_HELP_CMD_WIDTH,
                cmdtp->name, usage);
        }
        return 0;
    }
    /*
       * command help (long version)
       */
    for (i = 1; i < argc; ++i) {
        if ((cmdtp = find_cmd_tbl (argv[i], cmd_tbls, CONFIG_SYS_CMDCOUNT )) != NULL) {
            rcode |= cmd_usage(cmdtp);
        } else {
            printf ("Unknown command '%s' - try 'help'"
                " without arguments for list of all"
                " known commands\n\n", argv[i]
                    );
            rcode = 1;
        }
    }
    return 0;
}
#ifdef MINISHELL_VERSION
static int do_version(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    printf("\n%s\n", version_string);
    return 0;
}
#endif
/***************************************************************************
 * find command table entry for a command
 */
cmd_tbl_t *find_cmd_tbl (const char *cmd, cmd_tbl_t *table, int table_len)
{
    cmd_tbl_t *cmdtp;
    cmd_tbl_t *cmdtp_temp = table;    /*Init value */
    const char *p;
    int len;
    int n_found = 0;

    if (!cmd)
    {
        return NULL;
    }
    /*
     * Some commands allow length modifiers (like "cp.b");
     * compare command name only until first dot.
     */
    len = ((p = strchr(cmd, '.')) == NULL) ? strlen (cmd) : (p - cmd);

    for (cmdtp = table; cmdtp != table + table_len; cmdtp++) 
    {
        if (strncmp (cmd, cmdtp->name, len) == 0) 
        {
            if (len == strlen (cmdtp->name))
            {
                return cmdtp;    /* full match */
            }
            cmdtp_temp = cmdtp;    /* abbreviated command ? */
            n_found++;
        }
    }
    if (n_found == 1) 
    {           /* exactly one match */
        return cmdtp_temp;
    }

    return NULL;    /* not found or ambiguous command */
}
#ifndef __GLIBC__
/* Duplicate S, returning an identical malloc'd string.  */
char *strdup (const char *s)
{
  size_t len = strlen (s) + 1;
  void *new = malloc (len);

  if (new == NULL)
  {
    return NULL;
  }

  return (char *) memcpy (new, s, len);
}
#endif
int uboot_cmd_register (cmd_tbl_t *cmd)
{
    int ret = 0;

    if (!cmd || !cmd->name)
    {
        ret = -1;
        return ret;
    }

    if (!find_cmd(cmd->name))
    {
        if (cmd_count < sizeof (cmd_tbls)/sizeof (cmd_tbl_t))
        {
            memcpy((void *)&cmd_tbls[cmd_count], cmd, sizeof(cmd_tbl_t));            
            cmd_count++;
            ret = 0;
        }
    }
    else
    {
        ret = -1;
    }
    return ret;
}

void prt_puts(const char *str)
{
    puts(str);
    //while (*str)
    //    putc(*str++, stdout);
}
int setenv(const char *varname, const char *varvalue)
{
    const char * const argv[4] = { "setenv", varname, varvalue, NULL };

    if (varvalue == NULL || varvalue[0] == '\0')
        return _do_env_set(0, 2, (char * const *)argv);
    else
        return _do_env_set(0, 3, (char * const *)argv);
}

/* pass 1 to disable ctrlc() checking, 0 to enable.
 * returns previous state
 */
int disable_ctrlc(int disable)
{
    int prev = ctrlc_disabled;/*  save previous state */

        ctrlc_disabled = disable;
    return prev;
}
uint32_t  crc32 (uint32_t seed, const char *buffer, uint32_t len)
{
    uint32_t crc = 0x0;
    uint32_t i = 0;

    if (!buffer)
    {
        printf("crc Calculation fail \n");
        return (crc = (uint32_t)-1);
    }
    for (i=0; i<len; i++)
    {
        crc ^= (uint32_t)buffer[i];
    }
    crc |= seed;
    return crc;    
}

/*
 * Pointer to initial global data area
 *
 * Here we initialize it.
 */
static gd_t *gd = NULL;/* global_data;(from uboot) */

/**
 * @brief in future, it will be hush. now it is array
 *
 * @return 
 */
int hush_init_var(void)
{   
#define MAX_VALUE_STR     64
    unsigned char buf[MAX_VALUE_STR]={0};
    unsigned int off;
    unsigned char *data = ((env_t *)env_buf)->data;
    unsigned int len;    
    int flags = 0;
    uint32_t crc = 0;
    uint32_t new_crc = 0;
#ifdef LEGACY_STYLE
    int i = 0;
    ENTRY e, *ep = NULL;
    int env_flag = H_INTERACTIVE;    
    
    for (i=0; names[i] != NULL; i++)
    {
        if (EnvRead(names[i], (char *)buf) < 0)
        {
            cmd_err("read from eeprom fail\n");
        }
        else
        {
            e.key = names[i];
            e.data = (char *)buf;
            hsearch_r(e, ENTER, &ep, &env_htab, env_flag);
            if (!ep) 
            {
                printf("## Error inserting \"%s\" variable, errno=%d\n",
                    e.key, errno);
                return 1;
            }
        }
    }
#endif    
    gd = &global_data;

    /* use eeprom 512 to store struct */
    gd->other_addr  =  CONFIG_OTHER_VAR_OFFSET; //joy
    /* use eeprom 1K to store var */

    /* read CRC */
    E2promBusRead(CONFIG_SYS_DEF_EEPROM_ADDR,
        CONFIG_ENV_OFFSET + offsetof(env_t, crc),
        (unsigned char *)&crc, sizeof(crc));

    len = ENV_SIZE;
    off = offsetof(env_t, data);
    /* read str data */
    while (len > 0)
    {
        int n = (len > sizeof(buf)) ? sizeof(buf) : len;
        E2promBusRead(CONFIG_SYS_DEF_EEPROM_ADDR,
            CONFIG_ENV_OFFSET + off, buf, n);
        memcpy(data + (off - offsetof(env_t, data)), buf, n);
        len -= n;
        off += n;
    }
    /* check it is str? crc */
    new_crc = crc32(0x1, (char *)data, ENV_SIZE);
    if (crc == (uint32_t)-1 ||  new_crc == (uint32_t)-1 || crc != new_crc)
    {
        gd->env_valid = 0;
        set_default_env("!bad CRC");
        return 0;
    }
    /* crc is ok */ 
    gd->env_valid   = 1;
    gd->env_addr    = offsetof(env_t, data);
    
    if (himport_r(&env_htab, (char *)env_buf + offsetof(env_t, data),
            sizeof(env_buf)-offsetof(env_t, data), '\0', flags,
            0, NULL) == 0)
    {
        cmd_err("Environment import failed: errno = %d\n", errno);
    }
    gd->flags |= GD_FLG_ENV_READY;
    return 0;
}

static char * delete_char (char *buffer, char *p, int *colp, int *np, int plen)
{
    char *s;

    if (*np == 0)
    {
        return (p);
    }

    if (*(--p) == '\t') /* will retype the whole line */
    {
        while (*colp > plen) 
        {
            prt_puts (erase_seq);
            (*colp)--;
        }
        for (s=buffer; s<p; ++s) 
        {
            if (*s == '\t') 
            {
                prt_puts (tab_seq+((*colp) & 07));
                *colp += 8 - ((*colp) & 07);
            } 
            else 
            {
                ++(*colp);
                putc (*s, stdout);
            }
        }
    }
    else
    {
        prt_puts(erase_seq);
        (*colp)--;
    }
    (*np)--;
    return (p);
}
int readline_into_buffer(const char *const prompt, char *buffer, int timeout)
{
    char *p = buffer;
    char * p_buf = p;
    int n = 0;/* buffer index*/
    int plen = 0;/* prompt length*/
    int col;/* output column cnt*/
    int ch = -1;
    char c;

    /*  print prompt */    
    if (prompt) 
    {
        plen = strlen (prompt);
        prt_puts (prompt);
    }
    col = plen;
    for (;;)
    {
        ch = getc(stdin);
        if (EOF == ch)
        {
            if (g_mini_shellcb)
            {
                g_mini_shellcb(NULL);
            }
            continue;
        }
        c = ch & 0xff;
        /* special char handling */
        switch (c)
        {
            case '\r':   /* Enter */
            case '\n':
                *p = '\0';
                prt_puts ("\r\n");
                return (p - p_buf);
            case '\0':  /* null */
                continue;
            case 0x03:  /* ^C-break */
                p_buf[0] = '\0';/* discard input */
                return (-1);
            case 0x15: /* ^U - erase line */
                while (col > plen)
                {
                    prt_puts(erase_seq);
                    --col;
                }
                p = p_buf;
                n = 0;
                continue;

            case 0x17:  /* todo... */ /* ^W erase word */
            case 0x08:   /* ^H backspace */
            case 0x7F:   /* DEL backspace */
                p=delete_char(p_buf, p, &col, &n, plen);
                continue;
            default:
                /* must be a normal char then */
                if (n < 256-2)
                {
                    if (c == '\t')
                    {
                        /* no auto complete */
                        prt_puts (tab_seq+(col&07));
                        col += 8 - (col&07);
                    }
                    else
                    {
                        char buf[2];
                        /* Echo input using prt_puts() to force am
                                        * LCD flush if we are using an LCD
                                        */
                        ++col;
                        buf[0] = c;
                        buf[1] = '\0';
                        prt_puts(buf);
                    }
                    *p++ = c;
                    ++n;
                }
                else
                {
                    putc('\a', stdout);
                }
        }
    }
}
/**
 * @brief Prompt for input and read a line.
 *
 * @param prompt
 *
 * @return number of read characters
 *         -1 if break
 *         -2 if timed out
 */
int readline (const char *const prompt)
{
    /*
     * If console_buffer isn't 0-length the user will be prompted to modify
     * it instead of entering it from scratch as desired.
     *           
     */
    console_buffer[0] = '\0';

    return readline_into_buffer(prompt, console_buffer, 0);
}
void clear_ctrlc(void)
{
    ctrlc_was_pressed = 0;
}

int parse_line (char *line, char *argv[])
{
    int nargs = 0;

    while (nargs < CONFIG_SYS_MAXARGS) {

        /*  skip any white space */
        while (isblank(*line))
            ++line;

        if (*line == '\0') {/*  end of line, no more args*/
            argv[nargs] = NULL;
            return (nargs);
        }

        argv[nargs++] = line;/*  begin of argument string*/

            /*  find end of string */
            while (*line && !isblank(*line))
                ++line;

        if (*line == '\0') {/*  end of line, no more args*/
            argv[nargs] = NULL;
            return (nargs);
        }

        *line++ = '\0';/*  terminate current argv */
    }

    printf ("** Too many args (max. %d) **\n", CONFIG_SYS_MAXARGS);

    return (nargs);
}
cmd_tbl_t *find_cmd (const char *cmd)
{
    int i = 0;
    int count = sizeof (cmd_tbls)/sizeof (cmd_tbl_t);
    cmd_tbl_t *ret = NULL;
    const char *p;
    int len;
//    cmd_tbl_t *start = ll_entry_start(cmd_tbl_t, cmd);
//    const int len = ll_entry_count(cmd_tbl_t, cmd);
    len = ((p = strchr(cmd, '.')) == NULL) ? strlen (cmd) : (p - cmd);

    for (i=0; i < count; i++)
    {
        if (cmd_tbls[i].name == NULL)
        {
            ret = NULL;
            break;
        }
        if (strncmp(cmd_tbls[i].name, cmd, len) == 0)
        {
            ret =  &cmd_tbls[i];
            break;
        }
    }
    return ret;
}
static int cmd_call(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int result;

    result = (cmdtp->cmd)(cmdtp, flag, argc, argv);
    if (result)
        printf("Command failed, result=%d \n", result);
    return result;
}

static int cmd_usage(const cmd_tbl_t *cmdtp)
{
    printf("%s - %s\r\n\n", cmdtp->name, cmdtp->usage);

#ifdef CONFIG_SYS_LONGHELP
    printf("Usage:\r\n%s ", cmdtp->name);

    if (!cmdtp->help) {
        prt_puts ("- No additional help available.\n");
        return 1;
    }

    prt_puts (cmdtp->help);
    putc ('\r', stdout);
    putc ('\n', stdout);
#endif /*  CONFIG_SYS_LONGHELP */
    return 1;
}
enum command_ret_t cmd_process(int flag, int argc, char * const argv[],
        int *repeatable, unsigned int/*ulong*/ *ticks)
{
    enum command_ret_t rc = CMD_RET_SUCCESS;
    cmd_tbl_t *cmdtp;
    (void)ticks;

    cmdtp = find_cmd(argv[0]);
    if (cmdtp == NULL) {
        printf("Unknown command '%s' - try 'help'\n", argv[0]);
        return CMD_RET_FAILURE;//1;
    }
    /*  found - check max args */
    if (argc > cmdtp->maxargs)
    {
        rc = CMD_RET_USAGE;
    }
    /* bootd???? */
    /* If OK so far, then do the command  */
    if (!rc)
    {
        rc = (enum command_ret_t)cmd_call(cmdtp, flag, argc, argv);
        *repeatable &= cmdtp->repeatable;
    }
    if (rc == CMD_RET_USAGE)
    {
        rc = (enum command_ret_t)cmd_usage(cmdtp);
    }
    return rc;
}
int had_ctrlc (void)
{
    return ctrlc_was_pressed;
}
static int builtin_run_command(const char *cmd, int flag)
{
    char cmdbuf[CONFIG_SYS_CBSIZE];    /* working copy of cmd        */
    char *token;    /* start of token in cmdbuf    */
    char *sep;      /* end of token (separator) in cmdbuf */
    char finaltoken[CONFIG_SYS_CBSIZE]={0};
    char *str = cmdbuf;
    char *argv[CONFIG_SYS_MAXARGS + 1];
    int argc, inquotes;
    int repeatable = 1;
    int rc = 0;

    clear_ctrlc(); /* forget any previous Control C */
    if (!cmd || !*cmd)
    {
        return -1; /* empty command */
    }

    if (strlen(cmd) >= CONFIG_SYS_CBSIZE) {
        prt_puts ("## Command too long!\n");
        return -1;
    }

    strcpy (cmdbuf, cmd);
    /* Process separators and check for invalid
     * repeatable commands
     */
    while (*str) {

        /*-----------------------------------------------------------------------------
         *  Find separator, or string end
         *  Allow simple escape of ';' by writing "\;"
         *-----------------------------------------------------------------------------*/
        for (inquotes = 0, sep = str; *sep; sep++) 
        {
            if ((*sep=='\'') &&
                    (*(sep-1) != '\\'))
                inquotes=!inquotes;
            if (!inquotes &&
                    (*sep == ';') &&
                    ( sep != str) && /* past string start */
                    (*(sep-1) != '\\'))/* and NOT escaped */
                break;
        }
        /* Limit the token to data between separators */
        token = str;
        if (*sep)
        {
            str = sep + 1; /* start of command for next pass  */
            *sep = '\0';
        }
        else
        {
            str = sep; /* no more commands for next pass */
        }
        /* not process macro 
             * !!!!!!!!!!!!!!!!!
             * !!!!!!!!!!!!!!!!!
             */
        memcpy(finaltoken, token, strlen(token));
        finaltoken[strlen(finaltoken)] = '\0';
        /* Extract arguments */
        if ((argc = parse_line (finaltoken, argv)) == 0) {
            rc = -1; /* no command at all */
            continue;
        }
        if (cmd_process(flag, argc, argv, &repeatable, NULL))
        {
            rc = -1;
        }
        /* Did the user stop this? */
        if (had_ctrlc ())
        {
            return -1;
        }

    }
    return rc ? rc : repeatable;
}
int run_command(const char *cmd, int flag)
{

    /*-----------------------------------------------------------------------------
     *  builtin_run_command can return 0 or 1 for success, so clean up
     *  its result.
     *-----------------------------------------------------------------------------*/
    if (builtin_run_command(cmd, flag) == -1)
        return 1;
    return 0;
}
void main_loop(void)
{
    static char lastcommand[CONFIG_SYS_CBSIZE] = { 0, };
    int len;
    int flag;
    int rc = 1;
    for (;;)
    {
        len = readline (CONFIG_SYS_PROMPT);
        flag = 0;/* assume no special flags for now */
        if (len > 0)
        {
            strcpy(lastcommand, console_buffer);
        }
        else if (len == 0)
        {
            flag |= CMD_FLAG_REPEAT;
        }    
        if (len == -1)
        {
            prt_puts ("<INTERRUPT>\n");
        }
        else
        {
            rc = run_command(lastcommand, flag);
        }
        if (rc <= 0)
        {
            /* invalid command or not repeatable, forget it */
            lastcommand[0] = 0;
        }
    }

}
/* set env code */
/**
 * @brief 
 *
 * @param item
 * @param action
 * @param retval
 * @param htab
 * @param flag
 *
 * @return hsearch_r() returns index, and -1 on error.
 */
static int hsearch_r(ENTRY item, ACTION action, ENTRY ** retval,
        struct hsearch_data *htab, int flag)
{
    unsigned int len = strlen(item.key);
    int idx;
    //unsigned int first_deleted = 0;
    int ret = -1;

    idx = 0;
    if (htab->table[idx].used) 
    {
        do {
            /*
                    ** Because SIZE is prime this guarantees to
                    ** step through all available indices.
                    **/
            if (strcmp(item.key, htab->table[idx].entry.key) == 0)
            {
                *retval = &htab->table[idx].entry;
                if (action == ENTER)
                {
                    free(htab->table[idx].entry.data);                
                    htab->table[idx].entry.data = strdup(item.data);
                    if (!htab->table[idx].entry.data) 
                    {
                        cmd_err("Out of memory \n");
                        *retval = NULL;
                        return 0;
                    }
                }
                ret = idx;
                return ret;
            }
            idx++;
        }
        while (htab->table[idx].used);
    }
    if (action == ENTER) {
        if (htab->filled == htab->size) {
            *retval = NULL;
            return -1;
        }
        for (idx = 0; htab->table[idx].used == 1; idx++)
        {
            ;
        }
        htab->table[idx].used = 1;//hval;
        htab->table[idx].entry.key = strdup(item.key);
        htab->table[idx].entry.data = strdup(item.data);
        if (!htab->table[idx].entry.key ||
                !htab->table[idx].entry.data) 
        {
            *retval = NULL;
            return -1;
        }
        ++htab->filled;
        *retval = &htab->table[idx].entry;
        return idx;
    }
    *retval = NULL;
    return ret;
}

int hdelete_r(const char *key, struct hsearch_data *htab,
        int flag)
{
    ENTRY e, *ep;
    int idx;

    cmd_debug("hdelete: DELETE key \"%s\"\n", key);

    e.key = (char *)key;
    idx = hsearch_r(e, FIND, &ep, htab, 0);
    if (idx < 0)
    {
        return 0;/* not found */
    }
    free((void *)ep->key);
    free(ep->data);
    ep->callback = NULL;
    ep->flags = 0;
    htab->table[idx].used = -1;
    --htab->filled;
    return 1;
}
/*
 * Look up variable from environment,
 * return address of storage for that variable,
 * or NULL if not found
 */
 
#ifdef CONFIG_MINISHELL
#ifndef __GLIBC__
char *getenv(const char *name)
{
    ENTRY e, *ep;
    e.key   = name;
    e.data  = NULL;
    hsearch_r(e, FIND, &ep, &env_htab, 0);
    return ep ? ep->data : NULL;
}
#endif
#endif
int do_setenv(const char *varname, const char *varvalue)
{
    const char * const argv[4] = { "setenv", varname, varvalue, NULL };

    if (varvalue == NULL || varvalue[0] == '\0')
        return _do_env_set(0, 2, (char * const *)argv);
    else
        return _do_env_set(0, 3, (char * const *)argv);
}
/*
 * Set a new environment variable,
 * or replace or delete an existing one.
 */
static int _do_env_set(int flag, int argc, char * const argv[])
{
    int   i, len;
    char  *name, *value, *s;
    ENTRY e, *ep;
    int env_flag = H_INTERACTIVE;

    cmd_debug("Initial value for argc=%d\n", argc);
    while (argc > 1 && **(argv + 1) == '-') {
        char *arg = *++argv;

        --argc;
        while (*++arg) {
            switch (*arg) {
            case 'f':        /* force */
                env_flag |= H_FORCE;
                break;
            default:
                return -1;//CMD_RET_USAGE;
            }
        }
    }
    cmd_debug("Final value for argc=%d\n", argc);
    name = argv[1];
    value = argv[2];

    if (strchr(name, '=')) {
        printf("## Error: illegal character '='"
               "in variable name \"%s\"\n", name);
        return 1;
    }

    env_id++;

    /* Delete only ? */
    if (argc < 3 || argv[2] == NULL) {
        /* Search and delete entry matching ITEM.key in internal hash table.  */
        int rc = hdelete_r(name, &env_htab, env_flag);
        return !rc;
    }

    /*
     * Insert / replace new value
     */
    for (i = 2, len = 0; i < argc; ++i)
        len += strlen(argv[i]) + 1;

    value = malloc(len);
    if (value == NULL) {
        printf("## Can't malloc %d bytes\n", len);
        return 1;
    }
    for (i = 2, s = value; i < argc; ++i) {
        char *v = argv[i];

        while ((*s++ = *v++) != '\0')
            ;
        *(s - 1) = ' ';
    }
    if (s != value)
        *--s = '\0';

    e.key    = name;
    e.data    = value;
    /*
     * Search for entry matching ITEM.key in internal hash table.  If
     * ACTION is `FIND' return found entry or signal error by returning
     * NULL.  If ACTION is `ENTER' replace existing data (if any) with
     * ITEM.data.
     */
    hsearch_r(e, ENTER, &ep, &env_htab, env_flag);
    free(value);
    if (!ep) {
        printf("## Error inserting \"%s\" variable, errno=%d\n",
            name, errno);
        return 1;
    }

    return 0;
}

ssize_t hexport_r(struct hsearch_data *htab, const char sep, int flag, char **resp, size_t size, int argc, char * const argv[])
{
    ENTRY *list[htab->size];
    char *res, *p;
    size_t totlen;
    int i, n;

    /*  Test for correct arguments.  */
    if ((resp == NULL) || (htab == NULL)) {
        return (-1);
    }
    /*
     ** Pass 1:
     ** search used entries,
     ** save addresses and compute total length
     */
    for (i = 0, n = 0, totlen = 0; i < htab->size; ++i) 
    {
        if (htab->table[i].used > 0) {
            ENTRY *ep = &htab->table[i].entry;
            int arg, found = 0;

            for (arg = 0; arg < argc; ++arg) {
                if (strcmp(argv[arg], ep->key) == 0) {
                    found = 1;
                    break;
                }
            }
            if ((argc > 0) && (found == 0))
                continue;

            if ((flag & H_HIDE_DOT) && ep->key[0] == '.')
                continue;

            list[n++] = ep;

            totlen += strlen(ep->key) + 2;

            if (sep == '\0') {
                totlen += strlen(ep->data);
            } else {/*  check if escapes are needed */
                char *s = ep->data;

                while (*s) {
                    ++totlen;
                    /*  add room for needed escape chars */
                    if ((*s == sep) || (*s == '\\'))
                        ++totlen;
                    ++s;
                }
            }
            totlen += 2;/*  for '=' and 'sep' char */
        }
    }
    /*  Check if the user supplied buffer size is sufficient */
    if (size) 
    {
        if (size < totlen + 1) 
        {  
            /*  provided buffer too small */
            printf("Env export buffer too small: %zu, "
                    "but need %zu\n", size, totlen + 1);
            return (-1);
        }
    } 
    else 
    {
        size = totlen + 1;
    }
    /*  Check if the user provided a buffer */
    if (*resp) 
    {
        /*  yes; clear it */
        res = *resp;
        memset(res, '\0', size);
    } 
    else 
    {
        /*  no, allocate and clear one */
        *resp = res = calloc(1, size);
        if (res == NULL) 
        {
            cmd_err("Out of memory size[%d]\n", size);
            return (-1);
        }
    }
    /*
     ** Pass 2:
     ** export sorted list of result data
     **
     */
    for (i = 0, p = res; i < n; ++i) {
        const char *s;

        s = list[i]->key;
        while (*s)
            *p++ = *s++;
        *p++ = '=';

        s = list[i]->data;

        while (*s) {
            if ((*s == sep) || (*s == '\\'))
                *p++ = '\\';/*  escape */
            *p++ = *s++;
        }
        *p++ = sep;
    }
    *p = '\0';/*  terminate result */

    return size;
}
/*
 * Check whether variable 'name' is amongst vars[],
 * and remove all instances by setting the pointer to NULL
 */
static int drop_var_from_set(const char *name, int nvars, char * vars[])
{
    int i = 0;
    int res = 0;

    /* No variables specified means process all of them */
    if (nvars == 0)
        return 1;

    for (i = 0; i < nvars; i++) {
        if (vars[i] == NULL)
            continue;
        /* If we found it, delete all of them */
        if (!strcmp(name, vars[i])) {
            vars[i] = NULL;
            res = 1;
        }
    }
    if (!res)
        cmd_debug("Skipping non-listed variable %s\n", name);

    return res;
}
/*
 * For the used double hash method the table size has to be a prime. To
 * correct the user given table size we need a prime test.  This trivial
 * algorithm is adequate because
 * a)  the code is (most probably) called a few times per program run and
 * b)  the number is small because the table must fit in the core
 * */
static int isprime(unsigned int number)
{
    /* no even number will be passed */
    unsigned int div = 3;

    while (div * div < number && number % div != 0)
        div += 2;

    return number % div != 0;
}

/*
 * Before using the hash table we must allocate memory for it.
 * Test for an existing table are done. We allocate one element
 * more as the found prime number says. This is done for more effective
 * indexing as explained in the comment for the hsearch function.
 * The contents of the table is zeroed, especially the field used
 * becomes zero.
 */

int hcreate_r(size_t nel, struct hsearch_data *htab)
{
    /* Test for correct arguments.  */
    if (htab == NULL) {
        __set_errno(EINVAL);
        return 0;
    }

    /* There is still another table active. Return with error. */
    if (htab->table != NULL)
        return 0;

    /* Change nel to the first prime number not smaller as nel. */
    nel |= 1;        /* make odd */
    while (!isprime(nel))
        nel += 2;

    htab->size = nel;
    htab->filled = 0;

    /* allocate memory and zero out */
    htab->table = (_ENTRY *) calloc(htab->size + 1, sizeof(_ENTRY));
    if (htab->table == NULL)
        return 0;

    /* everything went alright */
    return 1;
}

/*
 * hdestroy()
 */

/*
 * After using the hash table it has to be destroyed. The used memory can
 * be freed and the local static variable can be marked as not used.
 */

void hdestroy_r(struct hsearch_data *htab)
{
    int i;

    /* Test for correct arguments.  */
    if (htab == NULL) {
        __set_errno(EINVAL);
        return;
    }

    /* free used memory */
    for (i = 1; i <= htab->size; ++i) {
        if (htab->table[i].used > 0) {
            ENTRY *ep = &htab->table[i].entry;

            free((void *)ep->key);
            free(ep->data);
        }
    }
    free(htab->table);

    /* the sign for an existing table is an value != NULL in htable */
    htab->table = NULL;
}


/*
 * Import linearized data into hash table.
 *
 * This is the inverse function to hexport(): it takes a linear list
 * of "name=value" pairs and creates hash table entries from it.
 *
 * Entries without "value", i. e. consisting of only "name" or
 * "name=", will cause this entry to be deleted from the hash table.
 *
 * The "flag" argument can be used to control the behaviour: when the
 * H_NOCLEAR bit is set, then an existing hash table will kept, i. e.
 * new data will be added to an existing hash table; otherwise, old
 * data will be discarded and a new hash table will be created.
 *
 * The separator character for the "name=value" pairs can be selected,
 * so we both support importing from externally stored environment
 * data (separated by NUL characters) and from plain text files
 * (entries separated by newline characters).
 *
 * To allow for nicely formatted text input, leading white space
 * (sequences of SPACE and TAB chars) is ignored, and entries starting
 * (after removal of any leading white space) with a '#' character are
 * considered comments and ignored.
 *
 * [NOTE: this means that a variable name cannot start with a '#'
 * character.]
 *
 * When using a non-NUL separator character, backslash is used as
 * escape character in the value part, allowing for example for
 * multi-line values.
 *
 * In theory, arbitrary separator characters can be used, but only
 * '\0' and '\n' have really been tested.
 */

static int himport_r(struct hsearch_data *htab,
        const char *env, size_t size, const char sep, int flag,
        int nvars, char * const vars[])
{
    char *data, *sp, *dp, *name, *value;
    char *localvars[nvars];
    int i;

    /* Test for correct arguments.  */
    if (htab == NULL) {
        __set_errno(EINVAL);
        return 0;
    }

    /* we allocate new space to make sure we can write to the array */
    if ((data = malloc(size)) == NULL) {
        cmd_debug("himport_r: can't malloc %zu bytes\n", size);
        __set_errno(ENOMEM);
        return 0;
    }
    memcpy(data, env, size);
    dp = data;

    /* make a local copy of the list of variables */
    if (nvars)
        memcpy(localvars, vars, sizeof(vars[0]) * nvars);
#if 1   /* may be used in future (from uboot)*/
    if ((flag & H_NOCLEAR) == 0) {
        /* Destroy old hash table if one exists */
        cmd_debug("Destroy Hash Table: %p table = %p\n", htab,
               htab->table);
        if (htab->table)
            hdestroy_r(htab);
    }
#endif
    /*
     * Create new hash table (if needed).  The computation of the hash
     * table size is based on heuristics: in a sample of some 70+
     * existing systems we found an average size of 39+ bytes per entry
     * in the environment (for the whole key=value pair). Assuming a
     * size of 8 per entry (= safety factor of ~5) should provide enough
     * safety margin for any existing environment definitions and still
     * allow for more than enough dynamic additions. Note that the
     * "size" argument is supposed to give the maximum enviroment size
     * (CONFIG_ENV_SIZE).  This heuristics will result in
     * unreasonably large numbers (and thus memory footprint) for
     * big flash environments (>8,000 entries for 64 KB
     * envrionment size), so we clip it to a reasonable value.
     * On the other hand we need to add some more entries for free
     * space when importing very small buffers. Both boundaries can
     * be overwritten in the board config file if needed.
     */

    if (!htab->table) {
        int nent = CONFIG_ENV_MIN_ENTRIES + size / 8;

        if (nent > CONFIG_ENV_MAX_ENTRIES)
            nent = CONFIG_ENV_MAX_ENTRIES;

        cmd_debug("Create Hash Table: N=%d\n", nent);

        if (hcreate_r(nent, htab) == 0) {
            free(data);
            return 0;
        }
    }

    /* Parse environment; allow for '\0' and 'sep' as separators */
    do {
        ENTRY e, *rv;

        /* skip leading white space */
        while (isblank(*dp))
            ++dp;

        /* skip comment lines */
        if (*dp == '#') {
            while (*dp && (*dp != sep))
                ++dp;
            ++dp;
            continue;
        }

        /* parse name */
        for (name = dp; *dp != '=' && *dp && *dp != sep; ++dp)
            ;

        /* deal with "name" and "name=" entries (delete var) */
        if (*dp == '\0' || *(dp + 1) == '\0' ||
            *dp == sep || *(dp + 1) == sep) {
            if (*dp == '=')
                *dp++ = '\0';
            *dp++ = '\0';    /* terminate name */

            cmd_debug("DELETE CANDIDATE: \"%s\"\n", name);
            if (!drop_var_from_set(name, nvars, localvars))
                continue;

            if (hdelete_r(name, htab, flag) == 0)
                cmd_debug("DELETE ERROR ##############################\n");

            continue;
        }
        *dp++ = '\0';    /* terminate name */

        /* parse value; deal with escapes */
        for (value = sp = dp; *dp && (*dp != sep); ++dp) {
            if ((*dp == '\\') && *(dp + 1))
                ++dp;
            *sp++ = *dp;
        }
        *sp++ = '\0';    /* terminate value */
        ++dp;

        /* Skip variables which are not supposed to be processed */
        if (!drop_var_from_set(name, nvars, localvars))
            continue;

        /* enter into hash table */
        e.key = name;
        e.data = value;

        hsearch_r(e, ENTER, &rv, htab, flag);
        if (rv == NULL)
            printf("himport_r: can't insert \"%s=%s\" into hash table\n",
                name, value);

        cmd_debug("INSERT: table %p, filled %d/%d rv %p ==> name=\"%s\" value=\"%s\"\n",
            htab, htab->filled, htab->size,
            rv, name, value);
    } while ((dp < data + size) && *dp);    /* size check needed for text */
                        /* without '\0' termination */
    cmd_debug("INSERT: free(data = %p)\n", data);
    free(data);

    /* process variables which were not considered */
    for (i = 0; i < nvars; i++) {
        if (localvars[i] == NULL)
            continue;
        /*
         * All variables which were not deleted from the variable list
         * were not present in the imported env
         * This could mean two things:
         * a) if the variable was present in current env, we delete it
         * b) if the variable was not present in current env, we notify
         *    it might be a typo
         */
        if (hdelete_r(localvars[i], htab, flag) == 0)
            printf("WARNING: '%s' neither in running nor in imported env!\n", localvars[i]);
        else
            printf("WARNING: '%s' not in imported env, deleting it!\n", localvars[i]);
    }

    cmd_debug("INSERT: done\n");
    return 1;        /* everything OK */
}

static int env_print(char *name, int flag)
{
    char *res = NULL;
    size_t len;

    if (name) {        /* print a single name */
        ENTRY e, *ep;

        e.key = name;
        e.data = NULL;
        hsearch_r(e, FIND, &ep, &env_htab, flag);
        if (ep == NULL)
            return 0;
        len = printf("%s=%s\n", ep->key, ep->data);
        return len;
    }

    /* print whole list */
    len = hexport_r(&env_htab, '\n', flag, &res, 0, 0, NULL);

    if (len > 0) 
    {
        puts(res);
        free(res);
        return len;
    }

    /* should never happen */
    return 0;
}
int saveenv(void)
{
    char *res = NULL;
    ssize_t len;
#ifdef LEGACY_STYLE
    int i = 0;
#endif
    int    rc = 0;
    env_t    *env_new = (env_t *)env_buf;
    unsigned int off    = CONFIG_ENV_OFFSET;
    
    //env_new->crc = CRC_DEFAULT;
    res = (char *)env_new->data;
    len = hexport_r(&env_htab, '\0', 0, &res, ENV_SIZE, 0, NULL);
    if (len < 0) {
        cmd_err("Cannot export environment: errno = %d\n", errno);
        return 1;
    }

    env_new->crc = crc32(0x1, (char *)env_new->data, ENV_SIZE);//CRC_DEFAULT;
    
    printf("Erasing %s...\n", env_name_spec);
    rc = E2promBusErase(CONFIG_SYS_DEF_EEPROM_ADDR, off, CONFIG_ENV_SIZE);
    if (rc == 0)
    {
        printf("Erasing at 0x%x -- 100%% complete.\n", off);
    }
    else
    {
        printf("Erasing at 0x%x error.\n", off);
    }
    rc = E2promBusWrite(CONFIG_SYS_DEF_EEPROM_ADDR,
            off, (unsigned char *)env_new, CONFIG_ENV_SIZE);
#ifdef LEGACY_STYLE
    for (i=0; names[i] != NULL; i++)
    {
        ENTRY e, *ep;

        e.key = names[i];
        e.data = NULL;
        hsearch_r(e, FIND, &ep, &env_htab, 0);
        if (ep == NULL)
        {
            continue;
        }
        /*
               *  found it
               */
        if (EnvWrite (ep->key, ep->data) < 0)
        {
            cmd_err("write to eeprom fail\n");
        }
        printf("%s=%s\n", ep->key, ep->data);
    }
#endif    
#if PRINT_SAVEENV
    len = hexport_r(&env_htab, '\n', 0, &res, 0/*ENV_SIZE*/, 0, NULL);
    if (len < 0)
    {
        cmd_err("Cannot export environment\n");
        return 1;
    }
    else if (len > 0)
    {
        puts(res);        
        free(res);
    }
#endif
    if (rc == 0)
    {
        printf("Writing to %s... done\n", env_name_spec);
    }
    return rc;
}

static int do_env_print(cmd_tbl_t *cmdtp, int flag, int argc,
            char * const argv[])
{
    int i;
    int rcode = 0;
    int env_flag = H_HIDE_DOT;

    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'a') {
        argc--;
        argv++;
        env_flag &= ~H_HIDE_DOT;
    }

    if (argc == 1) {
        /* print all env vars */
        rcode = env_print(NULL, env_flag);
        if (!rcode)
            return 1;
        printf("\nEnvironment size: %d/%d bytes\n",
            rcode, (unsigned int)ENV_SIZE);
        return 0;
    }

    /* print selected env vars */
    env_flag &= ~H_HIDE_DOT;
    for (i = 1; i < argc; ++i) {
        int rc = env_print(argv[i], env_flag);
        if (!rc) {
            printf("## Error: \"%s\" not defined\n", argv[i]);
            ++rcode;
        }
    }

    return rcode;
}
static int do_env_set(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    if (argc < 2)
        return CMD_RET_USAGE;

    return _do_env_set(flag, argc, argv);
}
void set_default_env(const char *s)
{
    int flags = 0;

    if (sizeof(default_environment) > ENV_SIZE) {
        puts("*** Error - default environment is too large\n\n");
        return;
    }

    if (s) {
        if (*s == '!') {
            printf("*** Warning - %s, "
                "using default environment\n\n",
                s + 1);
        } else {
            flags = H_INTERACTIVE;
            puts(s);
        }
    } else {
        puts("Using default environment\n\n");
    }

    if (himport_r(&env_htab, (char *)default_environment,
            sizeof(default_environment), '\0', flags,
            0, NULL) == 0)
    {
        cmd_err("Environment import failed: errno = %d\n", errno);
    }

    gd->flags |= GD_FLG_ENV_READY;
}
/* [re]set individual variables to their value in the default environment */
int set_default_vars(int nvars, char * const vars[])
{
    /*
     * Special use-case: import from default environment
     * (and use \0 as a separator)
     */
    return himport_r(&env_htab, (const char *)default_environment,
                sizeof(default_environment), '\0',
                H_NOCLEAR | H_INTERACTIVE, nvars, vars);
}
static int do_env_default(cmd_tbl_t *cmdtp, int __flag,
              int argc, char * const argv[])
{
    int all = 0, flag = 0;

    cmd_debug("Initial value for argc=%d\n", argc);
    while (--argc > 0 && **++argv == '-') {
        char *arg = *argv;

        while (*++arg) {
            switch (*arg) {
            case 'a':        /* default all */
                all = 1;
                break;
            case 'f':        /* force */
                flag |= H_FORCE;
                break;
            default:
                return cmd_usage(cmdtp);
            }
        }
    }
    cmd_debug("Final value for argc=%d\n", argc);
    if (all && (argc == 0)) {
        /* Reset the whole environment */
        set_default_env("## Resetting to default environment\n");
        return 0;
    }
    if (!all && (argc > 0)) {
        /* Reset individual variables */
        set_default_vars(argc, argv);
        return 0;
    }

    return cmd_usage(cmdtp);
}
static inline void *map_sysmem(phys_addr_t paddr, unsigned long len)
{
    return (void *)(uintptr_t)paddr;
}
static inline void unmap_sysmem(const void *vaddr)
{
}

/*
 * Print data buffer in hex and ascii form to the terminal.
 *
 * data reads are buffered so that each memory address is only read once.
 * Useful when displaying the contents of volatile registers.
 *
 * parameters:
 *    addr: Starting address to display at start of line
 *    data: pointer to data buffer
 *    width: data value width.  May be 1, 2, or 4.
 *    count: number of values to display
 *    linelen: Number of values to print per line; specify 0 for default length
 */
#define MAX_LINE_LENGTH_BYTES (64)
#define DEFAULT_LINE_LENGTH_BYTES (16)
int print_buffer(ulong addr, const void *data, uint width, uint count,
         uint linelen)
{
    /* linebuf as a union causes proper alignment */
    union linebuf {
        uint32_t ui[MAX_LINE_LENGTH_BYTES/sizeof(uint32_t) + 1];
        uint16_t us[MAX_LINE_LENGTH_BYTES/sizeof(uint16_t) + 1];
        uint8_t  uc[MAX_LINE_LENGTH_BYTES/sizeof(uint8_t) + 1];
    } lb;
    int i;

    if (linelen*width > MAX_LINE_LENGTH_BYTES)
        linelen = MAX_LINE_LENGTH_BYTES / width;
    if (linelen < 1)
        linelen = DEFAULT_LINE_LENGTH_BYTES / width;

    while (count) {
        uint thislinelen = linelen;
        printf("%08lx:", addr);

        /* check for overflow condition */
        if (count < thislinelen)
            thislinelen = count;

        /* Copy from memory into linebuf and print hex values */
        for (i = 0; i < thislinelen; i++) {
            uint32_t x;
            if (width == 4)
                x = lb.ui[i] = *(volatile uint32_t *)data;
            else if (width == 2)
                x = lb.us[i] = *(volatile uint16_t *)data;
            else
                x = lb.uc[i] = *(volatile uint8_t *)data;
            printf(" %0*x", width * 2, x);
            //data += width;//joy 
            data = (unsigned char *)data + width;
        }

        while (thislinelen < linelen) {
            /* fill line with whitespace for nice ASCII print */
            for (i=0; i<width*2+1; i++)
                puts(" ");
            linelen--;
        }

        /* Print data in ASCII characters */
        for (i = 0; i < thislinelen * width; i++) {
            if (!isprint(lb.uc[i]) || lb.uc[i] >= 0x80)
                lb.uc[i] = '.';
        }
        lb.uc[i] = '\0';
        printf("    %s\n", lb.uc);

        /* update references */
        addr += thislinelen * width;
        count -= thislinelen;

        //if (ctrlc())
        //    return -1;
    }

    return 0;
}
int cmd_get_data_size(char* arg, int default_size)
{
    /* Check for a size specification .b, .w or .l.
     */
    int len = strlen(arg);
    if (len > 2 && arg[len-2] == '.') {
        switch(arg[len-1]) {
        case 'b':
            return 1;
        case 'w':
            return 2;
        case 'l':
            return 4;
        case 's':
            return -2;
        default:
            return -1;
        }
    }
    return default_size;
}

/* Display values from last command.
 * Memory modify remembered values are different from display memory.
 */
static uint    dp_last_addr, dp_last_size;
static uint    dp_last_length = 0x40;
//static uint    mm_last_addr, mm_last_size;

static    ulong    base_address = 0;

/* Memory Display
 *
 * Syntax:
 *    md{.b, .w, .l} {addr} {len}
 */
#define DISP_LINE_LEN    16
static int do_mem_md(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    ulong    addr, length;
#if defined(CONFIG_HAS_DATAFLASH)
    ulong    nbytes, linebytes;
#endif
    int    size;
    int rc = 0;

    /* We use the last specified parameters, unless new ones are
     * entered.
     */
    addr = dp_last_addr;
    size = dp_last_size;
    length = dp_last_length;

    if (argc < 2)
        return CMD_RET_USAGE;

    if ((flag & CMD_FLAG_REPEAT) == 0) {
        /* New command specified.  Check for a size specification.
         * Defaults to long if no or incorrect specification.
         */
        if ((size = cmd_get_data_size(argv[0], 4)) < 0)
            return 1;

        /* Address is specified since argc > 1
        */
        addr = SimpleStrtoul(argv[1], NULL, 16);
        addr += base_address;

        /* If another parameter, it is the length to display.
         * Length is the number of objects, not number of bytes.
         */
        if (argc > 2)
            length = SimpleStrtoul(argv[2], NULL, 16);
    }

#if defined(CONFIG_HAS_DATAFLASH)
    /* Print the lines.
     *
     * We buffer all read data, so we can make sure data is read only
     * once, and all accesses are with the specified bus width.
     */
    nbytes = length * size;
    do {
        char    linebuf[DISP_LINE_LEN];
        void* p;
        linebytes = (nbytes>DISP_LINE_LEN)?DISP_LINE_LEN:nbytes;

        rc = read_dataflash(addr, (linebytes/size)*size, linebuf);
        p = (rc == DATAFLASH_OK) ? linebuf : (void*)addr;
        print_buffer(addr, p, size, linebytes/size, DISP_LINE_LEN/size);

        nbytes -= linebytes;
        addr += linebytes;
        if (ctrlc()) {
            rc = 1;
            break;
        }
    } while (nbytes > 0);
#else

# if defined(CONFIG_BLACKFIN)
    /* See if we're trying to display L1 inst */
    if (addr_bfin_on_chip_mem(addr)) {
        char linebuf[DISP_LINE_LEN];
        ulong linebytes, nbytes = length * size;
        do {
            linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;
            memcpy(linebuf, (void *)addr, linebytes);
            print_buffer(addr, linebuf, size, linebytes/size, DISP_LINE_LEN/size);

            nbytes -= linebytes;
            addr += linebytes;
            if (ctrlc()) {
                rc = 1;
                break;
            }
        } while (nbytes > 0);
    } else
# endif

    {
        ulong bytes = size * length;
        const void *buf = map_sysmem(addr, bytes);

        /* Print the lines. */
        print_buffer(addr, buf, size, length, DISP_LINE_LEN / size);
        addr += bytes;
        unmap_sysmem(buf);
    }
#endif

    dp_last_addr = addr;
    dp_last_length = length;
    dp_last_size = size;
    return (rc);
}
int do_eeprom ( cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
    const char *const fmt =
        "\nEEPROM @0x%lX %s: addr %%08lx  off %04lx  count %ld ... \n";
    ulong dev_addr = CONFIG_SYS_DEF_EEPROM_ADDR;//SimpleStrtoul (argv[2], NULL, 16);
    ulong off  = 0;
    ulong cnt  = 0;
    int rcode  = 0;
    ulong addr = 0;
    
    if (argc < 3)
        return CMD_RET_USAGE;
    if (argc == 4) {
        //ulong addr = SimpleStrtoul (argv[2], NULL, 16);
        off  = SimpleStrtoul (argv[2], NULL, 16);
        cnt  = SimpleStrtoul (argv[3], NULL, 16); 
        if (strcmp(argv[1], "read") == 0)
        {
            printf (fmt, dev_addr, argv[1], off, cnt);
            {
                ulong bytes = cnt;
                uint width = 1;
                addr = 0x08080000;
                const void *buf = map_sysmem(addr+off, bytes);
                /* Print the lines. */
                print_buffer(addr, buf, width, cnt, DISP_LINE_LEN / width);
                addr += bytes;
                unmap_sysmem(buf);
            }
            return rcode;
        }
        else if (strcmp(argv[1], "erase") == 0)
        {
            printf (fmt, dev_addr, argv[1], off, cnt);
            rcode = E2promBusErase(CONFIG_SYS_DEF_EEPROM_ADDR, off, cnt);
            if (rcode == 0)
            {
                printf("Erasing at 0x%x -- 100%% complete.\n", off);
            }
            else
            {
                printf("Erasing at 0x%x error.\n", off);
            }
            puts ("done\n");
        }
    }
    if (argc == 3)
    {
        if (strcmp(argv[1], "erase") == 0 && strcmp(argv[2], "all") == 0)
        {
            off = 0;
            cnt = 0x1000;//4k
            printf (fmt, dev_addr, argv[1], 0, cnt);
            printf("Erasing %s...\n", "EEPROM");
            rcode = E2promBusErase(CONFIG_SYS_DEF_EEPROM_ADDR, off, cnt);
            if (rcode == 0)
            {
                printf("Erasing at 0x%x -- 100%% complete.\n", off);
            }
            else
            {
                printf("Erasing at 0x%x error.\n", off);
            }
            puts ("done\n");
        }
    }
    if (rcode != 0)
        return CMD_RET_USAGE;
    return rcode;   
}
void  run_main_loop(void (*callback)(void *), void *para)
{
    gd = &global_data;
    if (callback)
    {
        g_mini_shellcb = callback;
    }
    /*  main_loop() can return to retry autoboot, if so just run it again */
    for (;;)
    {
        main_loop();
    }
    
}
void read_phy_addr(void *value, unsigned int offset, size_t size)
{
    if (offset+size > 512)
    {
        cmd_err("you have no permissions\n");
        return;
    }
#ifdef CONFIG_NOEEPROM
    (void)value;
#else    
    memcpy(value, (unsigned char *)gd->other_addr+offset, size);
#endif
}
void write_phy_addr(unsigned int offset, void *value, size_t size)
{
    if (offset+size > 512)
    {
        cmd_err("you have no permissions\n");
        return;
    }
#ifdef CONFIG_NOEEPROM
    (void)value;
#else
    printf("write addr[0x%x] size[%d]\n", gd->other_addr+offset, size);
    memcpy((unsigned char *)gd->other_addr+offset, value, size);
#endif
}
