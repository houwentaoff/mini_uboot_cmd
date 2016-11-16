/*
 * =====================================================================================
 *       Copyright (c), 2013-2020, Jz.
 *       Filename:  test_setenv.c
 *
 *    Description:  
 *         Others:
 *
 *        Version:  1.0
 *        Created:  11/15/2016 03:22:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Joy. Hou (hwt), houwentaoff@gmail.com
 *   Organization:  Jz
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define debug(...)  printf(__VA_ARGS__)           /*  */

/*  Flags for himport_r(), hexport_r(), hdelete_r(), and hsearch_r() */
#define H_NOCLEAR       (1 << 0) /*  do not clear hash table before importing */
#define H_FORCE         (1 << 1) /*  overwrite read-only/write-once variables */
#define H_INTERACTIVE   (1 << 2) /*  indicate that an import is user directed */
#define H_HIDE_DOT      (1 << 3) /*  don't print env vars that begin with '.' */ 
#define ENV_SIZE        (0x1000)

enum env_op {
    env_op_create,
    env_op_delete,
    env_op_overwrite,
};

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

_ENTRY entry[20];
struct hsearch_data env_htab = {
    .table     = entry,
    .size      = 15,
    .filled    = 0,
    .change_ok = NULL,//env_flags_validate,
};
int errno = 0;
static int env_id = 1;


static int _do_env_set(int flag, int argc, char * const argv[]);
static int env_print(char *name, int flag);

/**
 * @brief 
 *
 * @param item
 * @param action
 * @param retval
 * @param htab
 * @param flag
 *
 * @return hsearch_r() returns nonzero on success, and -1 on error.
 */
int hsearch_r(ENTRY item, ACTION action, ENTRY ** retval,
        struct hsearch_data *htab, int flag)
{
    unsigned int len = strlen(item.key);
    int idx;
    unsigned int first_deleted = 0;
    int ret = -1;

    idx = 0;
    if (htab->table[idx].used) 
    {
        do {
            /*
             ** Because SIZE is prime this guarantees to
             ** step through all available indices.
             **/
#if 0
            if (idx <= hval2)
                idx = htab->size + idx - hval2;
            else
                idx -= hval2;
#endif
            /*
             ** If we visited all entries leave the loop
             ** unsuccessfully.
             **/
#if 0
            if (idx == hval)
                break;
#endif
#if 0
            /*  If entry is found use it. */
            ret = _compare_and_overwrite_entry(item, action, retval,
                    htab, flag, hval, idx);
            if (ret != -1)
                return ret;
#endif
            if (strcmp(item.key, htab->table[idx].entry.key) == 0)
            {
                *retval = &htab->table[idx].entry;
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

//	debug("Initial value for argc=%d\n", argc);
	while (argc > 1 && **(argv + 1) == '-') {
		char *arg = *++argv;

		--argc;
		while (*++arg) {
			switch (*arg) {
			case 'f':		/* force */
				env_flag |= H_FORCE;
				break;
			default:
				return -1;//CMD_RET_USAGE;
			}
		}
	}
//	debug("Final value for argc=%d\n", argc);
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

	e.key	= name;
	e.data	= value;
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

static int do_env_print(/* cmd_tbl_t *cmdtp, */ int flag, int argc,
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
		printf("\nEnvironment size: %d/%ld bytes\n",
			rcode, (ulong)ENV_SIZE);
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
    for (i = 0, n = 0, totlen = 0; i <= htab->size; ++i) 
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

	if (name) {		/* print a single name */
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

	if (len > 0) {
		puts(res);
		free(res);
		return len;
	}

	/* should never happen */
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main ( int argc, char *argv[] )
{
    do_setenv("mac", "08:00:27:c2:d1:04");
    do_setenv("baudrate", "115200");
    do_setenv("ipaddr", "192.168.1.1");
    env_print(NULL, H_HIDE_DOT);
    env_print("mac", H_HIDE_DOT);
    //env_print("mac", H_HIDE_DOT);
    return EXIT_SUCCESS;
}

