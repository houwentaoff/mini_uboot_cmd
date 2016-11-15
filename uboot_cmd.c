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
//#include <search.h>

#include "uboot_cmd.h"

#define CMD_FLAG_REPEAT  0x0001/*  repeat last command*/
#define CMD_FLAG_BOOTD   0x0002/*  command is from bootd*/ 
#define CONFIG_SYS_MAXARGS     8            /*  */
#define CONFIG_SYS_CBSIZE      256            /*  */
#define CONFIG_SYS_PROMPT      "lorawan>" /*  */
#define U_BOOT_VERSION_STRING  "minishell 2016.11.15 joy"

enum command_ret_t {
    CMD_RET_SUCCESS,/*  0 = Success */
    CMD_RET_FAILURE,/*  1 = Failure */
    CMD_RET_USAGE = -1,/*  Failure, please report 'usage' error */
};

/* search */


callback g_mini_shellcb = NULL;

cmd_tbl_t *find_cmd (const char *cmd);

static int do_help(struct cmd_tbl_s *cmdtp, int flag, int argc, char * const argv[]);
static int do_version(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);

cmd_tbl_t cmd_tbls[10] = {
    {
        .name = "help",
        .maxargs = CONFIG_SYS_MAXARGS,
        .repeatable = 1,
        .cmd = do_help,
        .usage = "print command description/usage", //"help [cmd]",
        .help = "\r\n"
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
        .help = "",
    },
};
const char __weak version_string[] = U_BOOT_VERSION_STRING;

static int cmd_count = 1;
/*  test if ctrl-c was pressed */
static int ctrlc_disabled = 0;/*  see disable_ctrl() */
static int ctrlc_was_pressed = 0;
char        console_buffer[CONFIG_SYS_CBSIZE + 1]; /*  console I/O buffer*/
static const char erase_seq[] = "\b \b";/* erase sequence */
static const char   tab_seq[] = "        ";/* used to expand TABs*/

static int do_help(struct cmd_tbl_s *cmdtp, int flag, int argc, char * const argv[])
{
    (void)flag;
    
    if (argc != 2)
    {
#if 0
        printf("Usage: %s\n",  cmdtp->usage);
        if (cmdtp->help)
        {
            printf("%s\n",  cmdtp->help);
        }
#endif
        return (-1);
    }
    if (!cmdtp)
    {
        printf("error\n");
        return -1;
    }
    char * p = malloc(100);
    strcpy(p, "asdfghjkl;1234567890");
    printf("%s\n", p);
    free(p);
    printf("exec help cmd .....\n");
    return 0;
}
static int do_version(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("\n%s\n", version_string);
    return 0;
}
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
int ivm_read_eeprom(void)
{
    return 0;
}
int hush_init_var(void)
{
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
    char cmdbuf[CONFIG_SYS_CBSIZE];	/* working copy of cmd		*/
    char *token;    /* start of token in cmdbuf	*/
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

    // init env
    hush_init_var();
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
