/*
======================================================================
makemac.c
Macro handling routines for make utility.

This source file is part of a computer program that is
(C) Copyright 1988 Ammon R. Campbell.  All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES

The routines in this module handle the parsing of macro definitions
from the makefile into the macro list used internally by the make
utility.  Only the parsing and list management is done here; the
invocation (expansion) of macros is handled elsewhere.

======================================================================
*/

/****************************** INCLUDES ****************************/

#ifdef WIN
# include <windows.h>
#endif /* WIN */
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <dos.h>
#include <direct.h>     /* For getcwd() */
#include <string.h>

#include "make.h"

/****************************** HEADERS *****************************/

/* Prototypes for functions local to this module: */
static int      add_macro(char *name, char *data);

/****************************** VARIABLES ***************************/

/* macro_list:  Linked list of defined macros. */
static MACRO *macro_list;

/*************************** LOCAL FUNCTIONS ************************/

/*
** add_macro:
** Adds a new macro description to the macro list.  If a macro
** already exists with the same name, it is overridden.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      name    Name of macro.
**      data    Data for macro's expansion.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      1       Successful.
**      0       Error occurred.
*/
static int
add_macro(name, data)
        char    *name;
        char    *data;
{
        MACRO   *mac;
        MACRO   *mptr;

        /* Allocate memory for macro descriptor. */
        mac = (MACRO *)mem_alloc(sizeof(MACRO));
        if (mac == (MACRO *)NULL)
        {
                /* Out of memory for macro definition. */
                errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
                return 0;
        }
        mac->mnext = (MACRO *)NULL;

        /* Allocate memory for macro name. */
        mac->mname = (char *)mem_alloc(strlen(name) + 1);
        if (mac->mname == (char *)NULL)
        {
                /* Out of memory for macro name. */
                errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
                mem_free(mac);
                return 0;
        }

        /* Save macro name in dscriptor. */
        strcpy(mac->mname, name);

        /* Allocate memory for macro data. */
        mac->mexp = (char *)mem_alloc(strlen(data) + 1);
        if (mac->mexp == (LINE *)NULL)
        {
                /* Out of memory for macro data. */
                errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
                mem_free(mac->mname);
                mem_free(mac);
                return 0;
        }

        /* Save macro data in dscriptor. */
        strcpy(mac->mexp, data);

        /* Add new macro descriptor to macro list. */
        if (macro_list == (MACRO *)NULL)
        {
                /* This macro is the head of the list. */
                macro_list = mac;
        }
        else
        {
                /* Place macro at end of list. */
                mptr = macro_list;
                while (mptr->mnext != (MACRO *)NULL)
                        mptr = mptr->mnext;
                mptr->mnext = mac;
        }

        return 1;
}

/****************************** FUNCTIONS ***************************/

/*
** init_macros:
** Initializes the macro list.  This function must be called
** prior to using any of the other macro list functions in
** this module.
**
** Parameters:
**      NONE
**
** Returns:
**      NONE
*/
void
init_macros(void)
{
        macro_list = (MACRO *)NULL;
}

/*
** predefine_macros:
** Adds make's predefined macros to the macro list.  This
** function gets called after the command line arguments
** have been parsed and after the environment has been
** read.
**
** Parameters:
**      NONE
**
** Returns:
**      Value   Meaning
**      -----   -------
**      1       Successful.
**      0       Error occurred.
*/
int
predefine_macros(void)
{
        char    tmp[MAXPATH];
        int     i;

        /*
        ** Add name of program to macro list as symbol "MAKE".
        */
        if (mkname[0] == '\0')
        {
                if (!add_macro("MAKE", "make"))
                        return 0;
        }
        else
        {
                if (!add_macro("MAKE", mkname))
                        return 0;
        }

        /*
        ** Add current directory to macro list as symbol "MAKEDIR".
        */
        getcwd(tmp, MAXPATH - 1);
        strlwr(tmp);                    /* Just for MS-DOS. */
        if (!add_macro("MAKEDIR", tmp))
                return 0;

        /*
        ** Add the command line flags as the symbol "MAKEFLAGS".
        */
        tmp[0] = '-';
        i = 1;
        if (cmdflags & FLAG_BUILD_ANYWAY)
                tmp[i++] = OPT_BUILD_ANYWAY;
        if (cmdflags & FLAG_DEBUG)
                tmp[i++] = OPT_DEBUG;
        if (cmdflags & FLAG_ENV_OVERRIDE)
                tmp[i++] = OPT_ENV_OVERRIDE;
        if (cmdflags & FLAG_IGNORE)
                tmp[i++] = OPT_IGNORE;
        if (cmdflags & FLAG_NOSPAWN)
                tmp[i++] = OPT_NOSPAWN;
        if (cmdflags & FLAG_SHOW_INFO)
                tmp[i++] = OPT_SHOW_INFO;
        if (cmdflags & FLAG_QUERY)
                tmp[i++] = OPT_QUERY;
        if (cmdflags & FLAG_NO_DEFAULTS)
                tmp[i++] = OPT_NO_DEFAULTS;
        if (cmdflags & FLAG_NO_SHOW)
                tmp[i++] = OPT_NO_SHOW;
        if (cmdflags & FLAG_TOUCH)
                tmp[i++] = OPT_TOUCH;
        if (cmdflags & FLAG_NEEDNEWER)
                tmp[i++] = OPT_NEEDNEWER;
        if (cmdflags & FLAG_SIGNON)
                tmp[i++] = OPT_SIGNON;
        if (i > 1)
                tmp[i] = '\0';
        else
                tmp[0] = '\0';
        if (!add_macro("MAKEFLAGS", tmp))
                return 0;
}

/*
** flush_macros:
** Empties and frees the macro list.
**
** Parameters:
**      NONE
**
** Returns:
**      NONE
*/
void
flush_macros(void)
{
        MACRO   *m;     /* Temporary macro descriptor pointers. */
        MACRO   *m2;

        /* Free each entry in the macro list. */
        m = macro_list;
        while (m != (MACRO *)NULL)
        {
                /* Free macro name. */
                mem_free(m->mname);

                /* Free macro expansion text. */
                mem_free(m->mexp);

                /* Free macro descriptor. */
                m2 = m->mnext;
                mem_free(m);
                m = m2;
        }

        macro_list = (MACRO *)NULL;
}

/*
** find_macro:
** Searches the macro list for a particular macro.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      mname   Name of macro to search for.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      NULL    Specified macro not found.
**      other   Pointer to buffer containing
**              text of macro's expansion.
*/
char *
find_macro(mname)
        char    *mname;
{
        MACRO   *mptr;
        char    *mfound;

        mfound = (char *)NULL;
        mptr = macro_list;
        while (mptr != (MACRO *)NULL)
        {
                if (strcmp(mname, mptr->mname) == 0)
                        mfound = mptr->mexp;
                mptr = mptr->mnext;
        }

        return mfound;
}

/*
** define_macro:
** Adds a macro to the macro list.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      line    First line of macro definition.
**      handle  File handle to read if macro definition is more
**              than one line long.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      1       Successful.
**      0       Error occurred (syntax error, out of memory, I/O error).
*/
int
define_macro(line, handle)
        char    *line;
        int     handle;
{
        int     pos;
        char    mname[MAXPATH]; /* Name of macro. */

        /* Get macro name. */
        pos = 0;
        while(line[pos] != '\0' &&
                line[pos] != ' ' &&
                line[pos] != '\t' &&
                line[pos] != '=')
        {
                /* Check for overflow on macro name. */
                if (pos >= MAXPATH - 1)
                {
                        /* Macro name is too long. */
                        errmsg(MSG_ERR_SYMTOOLONG, (char *)NULL, NOVAL);
                        return 0;
                }

                /* Save character. */
                mname[pos] = line[pos];
                pos++;
                mname[pos] = '\0';
        }

        /* Skip any whitespace between macro name and '='. */
        while (line[pos] == ' ' ||
                line[pos] == '\t')
                pos++;

        /* Check for '='. */
        if (line[pos] != '=')
        {
                /* Syntax error in macro definition. */
                errmsg(MSG_ERR_MACROSYNTAX, line, NOVAL);
                return 0;
        }

        /* Skip '='. */
        pos++;

        /* Skip any whitespace between '=' and macro data string. */
        while (line[pos] == ' ' ||
                line[pos] == '\t')
                pos++;

        /* Add the macro to the macro list. */
        if (!add_macro(mname, &line[pos]))
        {
                /* Error adding macro to list. */
                return 0;
        }

        return 1;
}

/*
** dump_macros:
** Outputs the macro list.  This function is used for
** debugging.
**
** Parameters:
**      NONE
**
** Returns:
**      NONE
*/
void
dump_macros(void)
{
        MACRO   *mptr;  /* Temporary macro pointer. */

        mptr = macro_list;
        if (mptr == (MACRO *)NULL)
                mputs(MSG_INFO_NOMACROS);
        while (mptr != (MACRO *)NULL)
        {
                mputs(MSG_INFO_MACRONAME);
                mputs(mptr->mname);
                mputs("\n");
                mputs(MSG_INFO_MACRODATA);
                mputs(mptr->mexp);
                mputs("\n");

                mptr = mptr->mnext;
        }
}

