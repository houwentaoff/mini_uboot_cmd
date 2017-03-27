/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Jz.
 *       Filename:  uboot_cmd.c
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

#include "uboot_cmd.h"

/*  Flags for himport_r(), hexport_r(), hdelete_r(), and hsearch_r() */
#define H_NOCLEAR       (1 << 0) /*  do not clear hash table before importing */
#define H_FORCE         (1 << 1) /*  overwrite read-only/write-once variables */
#define H_INTERACTIVE   (1 << 2) /*  indicate that an import is user directed */
#define H_HIDE_DOT      (1 << 3) /*  don't print env vars that begin with '.' */ 
#define ENV_SIZE        (0x1000)

#define CMD_FLAG_REPEAT  0x0001/*  repeat last command*/
#define CMD_FLAG_BOOTD   0x0002/*  command is from bootd*/ 
#define CONFIG_SYS_HELP_CMD_WIDTH   8
#define CONFIG_SYS_MAXARGS     8            /*  */
#define CONFIG_SYS_CBSIZE      256            /*  */
#define CONFIG_SYS_CMDCOUNT    20
#define CONFIG_SYS_VARCOUNT    20            /*  */
#define CONFIG_SYS_PROMPT      "lorawan>" /*  */
#define U_BOOT_VERSION_STRING  "minishell 2016.11.18 joy"
#define cmd_debug(...)  printf(__VA_ARGS__)           /*  */
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
 *   * have reentrant counterparts ending with _r.  The non-reentrant
 *    * functions all work on a signle internal hashing table.
 *     */

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


callback g_mini_shellcb = NULL;

extern int32_t EnvWrite(const char *Name, const char *Value);
extern int32_t EnvRead(const char *Name, char *Value);


static int do_help(struct cmd_tbl_s *cmdtp, int flag, int argc, char * const argv[]);
static int do_version(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
static int do_env_print(cmd_tbl_t *cmdtp, int flag, int argc,
            char * const argv[]);
static int do_env_set(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
static int do_env_save(cmd_tbl_t *cmdtp, int flag, int argc,
               char * const argv[]);

cmd_tbl_t cmd_tbls[CONFIG_SYS_CMDCOUNT] = {
    {
        .name = "help",
        .maxargs = CONFIG_SYS_MAXARGS,
        .repeatable = 1,
        .cmd   = do_help,
        .usage = "print command description/usage", //"help [cmd]",
        .help  = "\r\n"
                 "   - print brief description of all commands\r\n"
                 "help command ...\r\n"
                 "   - print detailed usage of 'command'",
    },
    {
        .name = "version",
        .maxargs = 1,
        .repeatable = 1,
        .cmd = do_version,
        .usage = "print monitor, compiler and linker version",
        .help  = "",
    },
    {
        .name = "printenv",
        .maxargs = CONFIG_SYS_MAXARGS,
        .repeatable = 1,
        .cmd   = do_env_print,
        .usage = "print environment variables",
        .help  = "[-a]\r\n    - print [all] values of all environment variables\r\n"
                 "printenv name ...\r\n"
                 "    - print value of environment variable 'name'",
    },
    {
        .name  = "setenv",
        .maxargs = CONFIG_SYS_MAXARGS,
        .repeatable = 0,
        .cmd   = do_env_set,
        .usage = "set environment variables",
        .help  = "name value ...\r\n"
                 "set environment variable 'name' to 'value ...'\r\n"
                 "setenv name\r\n"
                 "delete environment variable 'name'"
    }, 
    {
        .name  = "saveenv",
        .maxargs = 1,
        .repeatable = 0,
        .cmd   = do_env_save,
        .usage = "save environment variables to persistent storage",
        .help  = ""
    },     
};
const char __weak version_string[] = U_BOOT_VERSION_STRING;
const char *env_name_spec = "EEPROM";
static int cmd_count = 5;
/*  test if ctrl-c was pressed */
static int ctrlc_disabled = 0;/*  see disable_ctrl() */
static int ctrlc_was_pressed = 0;
char        console_buffer[CONFIG_SYS_CBSIZE + 1]; /*  console I/O buffer*/
static const char erase_seq[] = "\b \b";/* erase sequence */
static const char   tab_seq[] = "        ";/* used to expand TABs*/
static const char *names[] = {"DEVADDR", "DEVEUI", "APPEUI", "APPKEY", "NETID", "NWKSKEY", "APPSKEY", NULL};

_ENTRY entry[CONFIG_SYS_VARCOUNT];

struct hsearch_data env_htab = {
    .table     = entry,
    .size      = CONFIG_SYS_VARCOUNT,
    .filled    = 0,
    .change_ok = NULL,//env_flags_validate,
};
int errno = 0;
static int env_id = 1;

static int _do_env_set(int flag, int argc, char * const argv[]);
static int env_print(char *name, int flag);
cmd_tbl_t *find_cmd_tbl (const char *cmd, cmd_tbl_t *table, int table_len);
cmd_tbl_t *find_cmd (const char *cmd);
int cmd_usage(const cmd_tbl_t *cmdtp);
static int saveenv(void);
int hsearch_r(ENTRY item, ACTION action, ENTRY ** retval,
        struct hsearch_data *htab, int flag);

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
    //cmd_debug("exec help cmd .....\n");
    return 0;
}
static int do_version(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    printf("\n%s\n", version_string);
    return 0;
}
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
/**
 * @brief read conf from e2
 *
 * @return 
 */
static int ivm_read_eeprom(void)
{
    return 0;
}

int hush_init_var(void)
{   
#define MAX_VALUE_STR     50
    int i = 0;
    char buf[MAX_VALUE_STR]={0};
    ENTRY e, *ep = NULL;
    int env_flag = H_INTERACTIVE;
    for (i=0; names[i] != NULL; i++)
    {
        if (EnvRead(names[i], buf) < 0)
        {
            cmd_err("read from eeprom fail\n");
        }
        else
        {
            e.key = names[i];
            e.data = buf;
            hsearch_r(e, ENTER, &ep, &env_htab, env_flag);
            if (!ep) 
            {
                printf("## Error inserting \"%s\" variable, errno=%d\n",
                    e.key, errno);
                return 1;
            }
        }
    }
    ivm_read_eeprom();
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
        c = getc(stdin);
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
    int len = sizeof (cmd_tbls)/sizeof (cmd_tbl_t);
    cmd_tbl_t *ret = NULL;
//    cmd_tbl_t *start = ll_entry_start(cmd_tbl_t, cmd);
//    const int len = ll_entry_count(cmd_tbl_t, cmd);
    for (i=0; i < len; i++)
    {
        if (cmd_tbls[i].name == NULL)
        {
            ret = NULL;
            break;
        }
        if (strcmp(cmd_tbls[i].name, cmd) == 0)
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

int cmd_usage(const cmd_tbl_t *cmdtp)
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
int hsearch_r(ENTRY item, ACTION action, ENTRY ** retval,
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

    printf("hdelete: DELETE key \"%s\"\n", key);

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

//    cmd_debug("Initial value for argc=%d\n", argc);
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
//    cmd_debug("Final value for argc=%d\n", argc);
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

static int saveenv(void)
{
    //char *res = NULL;
    //ssize_t len;
    int    rc = 0;
    int i = 0;
    
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
#if 0
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

int run_main_loop(void (*callback)(void *), void *para)
{
    if (callback)
    {
        g_mini_shellcb = callback;
    }
    /*  main_loop() can return to retry autoboot, if so just run it again */
    for (;;)
    {
        main_loop();
    }
    return 0;
}
