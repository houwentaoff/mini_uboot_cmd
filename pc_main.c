/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, xxx.
 *       Filename:  minishell.c
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  05/13/2020 03:56:10 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Joy. Hou (hwt), 544088192@qq.com
 *   Organization:  xxx
 *
 * =====================================================================================
 */

 
#include <stdlib.h>
#include <stdio.h>
//#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>

#define CMD_FLAG_REPEAT  0x0001/*  repeat last command*/
#define CMD_FLAG_BOOTD   0x0002/*  command is from bootd*/ 
#define CONFIG_SYS_MAXARGS     8            /*  */
#define CONFIG_SYS_CBSIZE      128            /*  */
#define CONFIG_SYS_PROMPT      "jlq>"      /*  */
 
 enum command_ret_t {
     CMD_RET_SUCCESS,/*  0 = Success */
     CMD_RET_FAILURE,/*  1 = Failure */
     CMD_RET_USAGE = -1,/*  Failure, please report 'usage' error */
 };
 
 struct cmd_tbl_s {
     char  *name;/*  Command Name*/
     int  maxargs;/*  maximum number of arguments*/
     int repeatable;/*  autorepeat allowed?autorepeat*/
     /*  Implementation function*/
     int (*cmd)(struct cmd_tbl_s *, int, char * const []);
     char *usage;/*  Usage message(short)short*/
#ifdef CONFIG_SYS_LONGHELP
     char *help;/*  Help  message(long)long*/
#endif
#ifdef CONFIG_AUTO_COMPLETE
     /*  do auto completion on the arguments */
     int (*complete)(int argc, char * const argv[], char last_char, int maxv, char *cmdv[]);
#endif
 };
 
 typedef struct cmd_tbl_s cmd_tbl_t;
 
 int help_cmd(struct cmd_tbl_s *cmdtp, int argc, char * const argv[]);
 int go_cmd(struct cmd_tbl_s *cmdtp, int argc, char * const argv[]);
 int cmd_usage(const cmd_tbl_t *cmdtp);
 
 cmd_tbl_t cmd_tbls[] = {
     {
         .name = "help",
         .maxargs = 1,
         .repeatable = 0,
         .cmd = help_cmd,
         .usage = "help cmd",
     },
     {
         .name = "go",
         .maxargs = 2,
         .repeatable = 0,
         .cmd = go_cmd,
         .usage = "go 0x000000",
     },
 };
#define CONFIG_SYS_CMDCOUNT   3
#define CONFIG_SYS_HELP_CMD_WIDTH   8       
 /*  test if ctrl-c was pressed */
 static int ctrlc_disabled = 0;/*  see disable_ctrl() */
 static int ctrlc_was_pressed = 0;
 char        console_buffer[CONFIG_SYS_CBSIZE + 1]; /*  console I/O buffer*/
 static const char erase_seq[] = "\b \b";/* erase sequence */
 static const char   tab_seq[] = "        ";/* used to expand TABs*/

 /*-----------------------------------------------------------------------------
 *  weak api (override sys api)
 *-----------------------------------------------------------------------------*/
 
extern void uart_putc_polled(char c);
extern char uart_getchar_polled(void);

int puts(const char *s)
{
    while (*s)
    {
        if ('\n' == *s)
        {
            uart_putc_polled('\r');
        }
        uart_putc_polled(*s);
        s++;
    }
    return 0;
}
#if 0
int fputc(int ch, FILE *f)
{
    if ('\n' == ch&0xff)
    {
        uart_putc_polled('\r');
    }
    uart_putc_polled(ch & 0xff);
    return(ch);
}
#endif
int putc(int ch, FILE *f)
{
    uart_putc_polled(ch & 0xff);
    return(ch);
}
int getc(FILE *stream)
{
    int ch = EOF;
    ch = uart_getchar_polled();
    return ch;
}



//#define MY_PRINT
#ifdef MY_PRINT

int my_printf(const char *fmt, ...)
{
#if 1
    va_list args;
    uint32_t i;
    char printbuffer[256];

    va_start(args, fmt);

    /* For this to work, printbuffer must be larger than
     * anything we ever want to print.
     */
    i = vsnprintf(printbuffer, sizeof(printbuffer), fmt, args);
    va_end(args);

    /* Print the string */
    puts(printbuffer);
    return i;
#else
    return 0;
#endif
}
#else
#define PAD_RIGHT 1
#define PAD_ZERO 2
static void printchar(char **str, int c, char *buflimit)
{
	//extern int putchar(int c);

	if (str) {
		if( buflimit == ( char * ) 0 ) {
			/* Limit of buffer not known, write charater to buffer. */
			**str = (char)c;
			++(*str);
		}
		else if( ( ( unsigned long ) *str ) < ( ( unsigned long ) buflimit ) ) {
			/* Withing known limit of buffer, write character. */
			**str = (char)c;
			++(*str);
		}
	}
	else
	{
	    if (c == '\n'){
            (void)uart_putc_polled('\r');
        }
		(void)uart_putc_polled(c);
	}
}

static int prints(char **out, const char *string, int width, int pad, char *buflimit)
{
	register int pc = 0, padchar = ' ';

	if (width > 0) {
		register int len = 0;
		register const char *ptr;
		for (ptr = string; *ptr; ++ptr) ++len;
		if (len >= width) width = 0;
		else width -= len;
		if (pad & PAD_ZERO) padchar = '0';
	}
	if (!(pad & PAD_RIGHT)) {
		for ( ; width > 0; --width) {
			printchar (out, padchar, buflimit);
			++pc;
		}
	}
	for ( ; *string ; ++string) {
		printchar (out, *string, buflimit);
		++pc;
	}
	for ( ; width > 0; --width) {
		printchar (out, padchar, buflimit);
		++pc;
	}

	return pc;
}
/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static int printi(char **out, int i, int b, int sg, int width, int pad, int letbase, char *buflimit)
{
	char print_buf[PRINT_BUF_LEN];
	register char *s;
	register int t, neg = 0, pc = 0;
	register unsigned int u = (unsigned int)i;

	if (i == 0) {
		print_buf[0] = '0';
		print_buf[1] = '\0';
		return prints (out, print_buf, width, pad, buflimit);
	}

	if (sg && b == 10 && i < 0) {
		neg = 1;
		u = (unsigned int)-i;
	}

	s = print_buf + PRINT_BUF_LEN-1;
	*s = '\0';

	while (u) {
		t = (unsigned int)u % b;
		if( t >= 10 )
			t += letbase - '0' - 10;
		*--s = (char)(t + '0');
		u /= b;
	}

	if (neg) {
		if( width && (pad & PAD_ZERO) ) {
			printchar (out, '-', buflimit);
			++pc;
			--width;
		}
		else {
			*--s = '-';
		}
	}

	return pc + prints (out, s, width, pad, buflimit);
}

static int tiny_print( char **out, const char *format, va_list args, unsigned int buflen )
{
	register int width, pad;
	register int pc = 0;
	char scr[2], *buflimit;

	if( buflen == 0 ){
		buflimit = ( char * ) 0;
	}
	else {
		/* Calculate the last valid buffer space, leaving space for the NULL
		terminator. */
		buflimit = ( *out ) + ( buflen - 1 );
	}

	for (; *format != 0; ++format) {
		if (*format == '%') {
			++format;
			width = pad = 0;
			if (*format == '\0') break;
			if (*format == '%') goto out;
			if (*format == '-') {
				++format;
				pad = PAD_RIGHT;
			}
			while (*format == '0') {
				++format;
				pad |= PAD_ZERO;
			}
			for ( ; *format >= '0' && *format <= '9'; ++format) {
				width *= 10;
				width += *format - '0';
			}
			if( *format == 's' ) {
				register char *s = (char *)(long)va_arg( args, unsigned int );
				pc += prints (out, s?s:"(null)", width, pad, buflimit);
				continue;
			}
			if( *format == 'd' ) {
				pc += printi (out, va_arg( args, int ), 10, 1, width, pad, 'a', buflimit);
				continue;
			}
			if( *format == 'x' ) {
				pc += printi (out, va_arg( args, int ), 16, 0, width, pad, 'a', buflimit);
				continue;
			}
			if( *format == 'X' ) {
				pc += printi (out, va_arg( args, int ), 16, 0, width, pad, 'A', buflimit);
				continue;
			}
			if( *format == 'u' ) {
				pc += printi (out, va_arg( args, int ), 10, 0, width, pad, 'a', buflimit);
				continue;
			}
			if( *format == 'c' ) {
				/* char are converted to int then pushed on the stack */
				scr[0] = (char)va_arg( args, int );
				scr[1] = '\0';
				pc += prints (out, scr, width, pad, buflimit);
				continue;
			}
		}
		else {
		out:
			printchar (out, *format, buflimit);
			++pc;
		}
	}
	if (out) **out = '\0';
	va_end( args );
	return pc;
}
int my_printf(const char *format, ...)
{
    va_list args;
    va_start( args, format );
    return tiny_print( 0, format, args, 0 );
}

#if 0
int fputc(int c, FILE *stream)
{
    /* Only output console data to tube */
        if (c == 0) {
                return feof(stream);
        } else {
        if (stream == stdout || stream == stderr) {
            uart_putc_polled(c & 0xff);
        }
        return c;
        }
}
#endif
#endif
 int my_strncmp(const char *cs, const char *ct, size_t count)
 {
     unsigned char c1, c2;

     while (count) {
         c1 = *cs++;
         c2 = *ct++;
         if (c1 != c2)
             return c1 < c2 ? -1 : 1;
         if (!c1)
             break;
         count--;
     }
     return 0;
 }
 char *my_strchr(const char *s, int c)
 {
     for (; *s != (char)c; ++s)
         if (*s == '\0')
             return NULL;
     return (char *)s;
 }
 size_t my_strlen(const char *s)
 {
     const char *sc;

     for (sc = s; *sc != '\0'; ++sc)
         /* nothing */;
     return sc - s;
 }
 int my_strcmp(const char *cs, const char *ct)
 {
     unsigned char c1, c2;

     while (1) {
         c1 = *cs++;
         c2 = *ct++;
         if (c1 != c2)
             return c1 < c2 ? -1 : 1;
         if (!c1)
             break;
     }
     return 0;
 }
char *my_strcpy(char *dest, const char *src)
{
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
		/* nothing */;
	return tmp;
}
void *my_memcpy(void *dest, const void *src, size_t count)
{
    char *tmp = dest;
    const char *s = src;

    while (count--)
        *tmp++ = *s++;
    return dest;
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
     len = ((p = my_strchr(cmd, '.')) == NULL) ? my_strlen (cmd) : (p - cmd);
 
     for (cmdtp = table; cmdtp != table + table_len; cmdtp++) 
     {
         if (my_strncmp (cmd, cmdtp->name, len) == 0)
         {
             if (len == my_strlen (cmdtp->name))
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
 
 int help_cmd(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
 {
     int i;
     int rcode = 0;
     
     if (argc == 1)
     {
         for (cmdtp = &cmd_tbls[0]; cmdtp < &cmd_tbls[CONFIG_SYS_CMDCOUNT-1] ;cmdtp++)
         {
             const char *usage = cmdtp->usage;
             if (usage == NULL)
                 continue;
             my_printf("%s- %s\n",
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
             my_printf ("Unknown command '%s' - try 'help'"
                 " without arguments for list of all"
                 " known commands\n\n", argv[i]
                     );
             rcode = 1;
         }
     }
 
     return 0;
 }
 
 static int hex(char ch)
 {
     if ((ch >= '0') && (ch <= '9'))
         return ch - '0';
     if ((ch >= 'a') && (ch <= 'f'))
         return ch - 'a' + 10;
     if ((ch >= 'A') && (ch <= 'F'))
         return ch - 'A' + 10;
     return -1;
 }
 
 /*
  * While we find nice hex chars, build a long_val.
  * Return number of chars processed.
  */
 int hex2u64(const char *ptr, unsigned long long *long_val)
 {
     const char *p = ptr;
     *long_val = 0;
 
     while (*p) {
         const int hex_val = hex(*p);
 
         if (hex_val < 0)
             break;
 
         *long_val = (*long_val << 4) | hex_val;
         p++;
     }
 
     return p - ptr;
 }


 int go_cmd(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
 {
     int i = 0;
     unsigned long long addr = 0;
     const unsigned char *p = NULL;
     if (argc != 2)
     {
         my_printf("Usage: %s\n",  cmdtp->usage);
         return (-1);
     }
     if (!cmdtp)
     {
         my_printf("error\n");
         return -1;
     }
     //sscanf(argv[1], "%x", &addr);
     //"0x123"  --> &addr
     //my_printf("==>%s: addr [0x%x]\n", __func__, addr);
     p = argv[1];
     if ( (p[0] == '0') && (p[1] == 'x'||p[1] == 'X' ) )
     {
        p += 2;
        hex2u64(p, &addr);
     }
     my_printf("go addr [0x%x]\n", addr);
     longjump(addr);
     return 0;
 }
 
 void prt_puts(const char *str)
 {
     while (*str)
         putc(*str++, stdout);
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
         plen = my_strlen (prompt);
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
 
     while (nargs < 16) {
 
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
 
     my_printf ("** Too many args (max. %d) **\n", CONFIG_SYS_MAXARGS);
 
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
         if (my_strcmp(cmd_tbls[i].name, cmd) == 0)
         {
             ret =  &cmd_tbls[i];
             break;
         }
     }
     return ret;
 }
 static int cmd_call(cmd_tbl_t *cmdtp, int argc, char * const argv[])
 {
     int result;
 
     result = (cmdtp->cmd)(cmdtp, argc, argv);
     if (result)
         my_printf("Command failed, result=%d", result);
     return result;
 }
 
 int cmd_usage(const cmd_tbl_t *cmdtp)
 {
     my_printf("%s - %s\n\n", cmdtp->name, cmdtp->usage);
 
#ifdef CONFIG_SYS_LONGHELP
     my_printf("Usage:\n%s ", cmdtp->name);
 
     if (!cmdtp->help) {
         prt_puts ("- No additional help available.\n");
         return 1;
     }
 
     prt_puts (cmdtp->help);
     putc ('\n');
#endif /*  CONFIG_SYS_LONGHELP */
     return 1;
 }
 enum command_ret_t cmd_process(int flag, int argc, char * const argv[],
         int *repeatable, unsigned long *ticks)
 {
     enum command_ret_t rc = CMD_RET_SUCCESS;
     cmd_tbl_t *cmdtp;
     (void)ticks;
 
     cmdtp = find_cmd(argv[0]);
     if (cmdtp == NULL) {
         my_printf("Unknown command '%s' - try 'help'\n", argv[0]);
         return 1;
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
         rc = cmd_call(cmdtp, argc, argv);
         *repeatable &= cmdtp->repeatable;
     }
     if (rc == CMD_RET_USAGE)
     {
         rc = cmd_usage(cmdtp);
     }
     return rc;
 }
 int had_ctrlc (void)
 {
     return ctrlc_was_pressed;
 }
 static int builtin_run_command(const char *cmd, int flag)
 {
     char cmdbuf[CONFIG_SYS_CBSIZE];
     char *token;
     char *sep;
     char finaltoken[CONFIG_SYS_CBSIZE]={0};
     char *str = cmdbuf;
     char *argv[16 + 1];
     int argc, inquotes;
     int repeatable = 1;
     int rc = 0;
 
     clear_ctrlc(); /* forget any previous Control C */
     if (!cmd || !*cmd)
     {
         return -1; /* empty command */
     }
 
     if (my_strlen(cmd) >= CONFIG_SYS_CBSIZE) {
         prt_puts ("## Command too long!\n");
         return -1;
     }
 
     my_strcpy (cmdbuf, cmd);
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
         my_memcpy(finaltoken, token, my_strlen(token));
         finaltoken[my_strlen(finaltoken)] = '\0';
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
     static char lastcommand[CONFIG_SYS_CBSIZE];
     int len;
     int flag;
     int rc = 1;
 #if 0
     // init env
     hush_init_var();
 #endif

     for(rc =0; rc < sizeof(lastcommand);rc++)
         lastcommand[rc]=0;

     for (;;)
     {
         len = readline (CONFIG_SYS_PROMPT);
         flag = 0;/* assume no special flags for now */
         if (len > 0)
         {
             my_strcpy(lastcommand, console_buffer);
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
 
 static int run_main_loop(void)
 {
     /*  main_loop() can return to retry autoboot, if so just run it again */
     for (;;)
         main_loop();
     return 0;
 }
 /* 
  * ===  FUNCTION  ======================================================================
  *         Name:  shell_main
  *  Description:  
  * =====================================================================================
  */
 int shell_main ()
 {
     run_main_loop();
     return 0;
 }

