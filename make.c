/*
======================================================================
make.c
Main module for 'make', an automated build utility similar to the
UNIX make command.

This source file is part of a computer program that is
(C) Copyright 1985-1988, 1990, 1992 Ammon R. Campbell.
All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES

This module contains the main() function and miscellaneous utility
functions for the make utility.  Most of the actual work is done
by the other modules in the program.

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
#include <string.h>

#define MAIN
#include "make.h"

#include "cvtslash.h"

/****************************** HEADERS *****************************/

/* Functions that are local to this file: */
static int      load_env(char *menvp[]);
static int      find_inifile(char *str);
static void     usage(void);
static int      initialize(void);
static void     deinitialize(void);
static void     errstop(void);

/****************************** CONSTANTS ***************************/

/* Default filenames. */
#define DEFAULT_MAKEFILE "Makefile"
#define DEFAULT_INIFILE "make.inf"

/* For "copynote.h", so it won't use printf() which is big. */
#define COPYNOTE_PRINT_FUNC     mputs

/****************************** VARIABLES ***************************/

/*
** makefile_name:  The filename of the makefile (normally 'Makefile').
*/
static char makefile_name[MAXPATH];

/*
** cwd_name:  Name of working directory (empty if current directory).
*/
static char cwd_name[MAXPATH];

/*************************** LOCAL FUNCTIONS ************************/

/*
** load_env:
** Loads environment strings into the macro table.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      menvp   Array of environment strings.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      1       Successful.
**      0       Error occurred.
*/
static int
load_env(menvp)
        char    *menvp[];
{
        int     pos;

        while (*menvp != (char *)NULL)
        {
                pos = cindex(*menvp, '=');
                if (pos < 1)
                {
                        errmsg(MSG_ERR_ENVSYNTAX, *menvp, NOVAL);
                        return 0;
                }
                if (!define_macro(*menvp, -1))
                {
                        return 0;
                }
                menvp++;
        }
        return 1;
}

/*
** find_inifile:
** Searches the current directory and the directories listed in
** the PATH environment variable for the program's INI file.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      str     Buffer to return pathname of INI file in.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      1       Successful.
**      0       INI file not found.
*/
static int
find_inifile(str)
        char    *str;
{
        char    *cp;            /* Temporary character pointer. */

        /* Check in current directory first. */
        if (access(DEFAULT_INIFILE, 444) == 0)
        {
                /* INI file exists. */
                strcpy(str, DEFAULT_INIFILE);
                return 1;
        }

        /* Enumerate each directory in the PATH environment variable. */
        enumpath(0, "PATH");
        while((cp = enumpath(1, "PATH")) != (char *)NULL)
        {
                /* Build full pathname of possible INI file. */
                strcpy(str, cp);
                if (str[strlen(str) - 1] != '\\')
                        strcat(str, "\\");
                strcat(str, DEFAULT_INIFILE);

                /* See if it exists. */
                if (access(str, 444) == 0)
                {
                        /* INI file exists. */
                        return 1;
                }
        }

        /* INI file not found. */
        return 0;
}

/*
** usage:
** Displays information about the program.
**
** Parameters:
**      NONE
**
** Returns:
**      NONE
*/
static void
usage(void)
{
        mputs(MSG_SIGNON);
        mputs(MSG_COPYRIGHT);
        mputs(MSG_USAGE);
}

/*
** initialize:
** Performs one-time initializations.
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
static int
initialize(void)
{
        /* Set defaults. */
        strcpy(makefile_name, DEFAULT_MAKEFILE);
        strcpy(cwd_name, "");
        makeflags = 0;

        /* Initialize memory handler. */
        if (!mem_init())
        {
                /* Couldn't initialize memory handler. */
                errmsg(MSG_ERR_MEMINIT, (char *)NULL, NOVAL);
                return 0;
        }

        /* Set up lists. */
        init_macros();                  /* Macro list is empty. */
        init_rules();                   /* Rule list is empty. */
        init_targets();                 /* Target list is empty. */
        init_precious();                /* Precious names list is empty. */
        init_suffixes();                /* Suffixes list is empty. */

        return 1;
}

/*
** deinitialize:
** Performs one-time deinitializations.
**
** Parameters:
**      NONE
**
** Returns:
**      NONE
*/
static void
deinitialize(void)
{
        /* Free macro list. */
        flush_macros();

        /* Free rule list. */
        flush_rules();

        /* Free target list. */
        flush_targets();

        /* Free the precious filenames list. */
        flush_precious();

        /* Free the suffixes list. */
        flush_suffixes();

        /* Shut down memory handler. */
        mem_deinit();

}

/*
** errstop:
** Displays the error message trailer that is displayed when
** make exits with an error.
**
** Parameters:
**      NONE
**
** Returns:
**      NONE
*/
static void
errstop(void)
{
#ifndef WIN
        /* Display message. */
        mputs(MSG_ERRSTOP);
#else
        MSG     msg;

        /* Display message. */
        mputs(MSG_ERRSTOP);
        mputs(MSG_PRESSAKEY);

        /* Pause until user presses a key. */
        msg.message = 0;
        while (msg.message != WM_KEYDOWN && GetMessage(&msg, NULL, 0, 0))
        {
                TranslateMessage((LPMSG)&msg);
                DispatchMessage((LPMSG)&msg);
        }
#endif /* WIN */
}

/****************************** FUNCTIONS ***************************/

/*
** main:
** C application entry point.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      argc    Number of command line arguments.
**      argv    Array of command line argument strings.
**      envp    Array of environment setting strings.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      0       No errors.
**      1       Error(s) occurred.
*/
int
#ifdef WIN
dmain(argc, argv, envp)
#else
main(argc, argv, envp)
#endif /* WIN */
        int     argc;
        char    *argv[];
        char    *envp[];
{
        int     i;              /* Loop index. */
        int     j;              /* Loop index. */
        int     itmp;           /* Temporary integer. */
        time_t  hitime = 0L;    /* Temporary time value. */
        int     result;         /* Function return code. */
        int     query_result = 0; /* Up-to-date flag. */
        char    inifile_name[MAXPATH]; /* Pathname of INI file. */

        /* See if user wants help. */
        if (argv[1][0] == '-' && argv[1][1] == '?')
        {
                usage();
                return 1;
        }

        /* Save environment pointer. */
        mkenvp = envp;

        /* Save pointer to argv[0]. */
        mkname = argv[0];

        /* Perform one-time setup. */
        if (!initialize())
        {
                errstop();
                return 1;
        }

        /* Parse command line options. */
        i = 1;
        while (i < argc)
        {
                if (argv[i][0] == '-')
                {
                        j = 1;
                        while (argv[i][j])
                        {
                                switch(argv[i][j])
                                {
                                        case OPT_BUILD_ANYWAY:
                                                /*
                                                ** Build targets even if
                                                ** not out-of-date.
                                                */
                                                SETFLAG(FLAG_BUILD_ANYWAY);
                                                break;

                                        case OPT_SIGNON:
                                                /* Show signon. */
                                                SETFLAG(FLAG_SIGNON);
                                                break;

                                        case OPT_DEBUG:
                                                /* Enable debug output. */
                                                SETFLAG(FLAG_DEBUG);
                                                break;

                                        case OPT_ENV_OVERRIDE:
                                                /* Environment override. */
                                                SETFLAG(FLAG_ENV_OVERRIDE);
                                                break;

                                        case OPT_WORKINGDIR:
                                                /* Set working directory. */
                                                if (argv[i][j + 1])
                                                {
                                                        strcpy(cwd_name,
                                                        &argv[i][j + 1]);
                                                        j = strlen(argv[i]);
                                                        j--;
                                                }
                                                else
                                                {
                                                        /* No dirname. */
                                                        errmsg(
                                                        MSG_ERR_FNODIRNAME,
                                                                (char *)NULL,
                                                                NOVAL);
                                                        errstop();
                                                        return 1;
                                                }
                                                break;

                                        case OPT_MAKEFILE:
                                                /* Use alternate makefile. */
                                                if (argv[i][j + 1])
                                                {
                                                        strcpy(makefile_name,
                                                        &argv[i][j + 1]);
                                                        j = strlen(argv[i]);
                                                        j--;
                                                }
                                                else
                                                {
                                                        /* No filename. */
                                                        errmsg(
                                                        MSG_ERR_FNOMAKEFILE,
                                                                (char *)NULL,
                                                                NOVAL);
                                                        errstop();
                                                        return 1;
                                                }
                                                break;

                                        case OPT_IGNORE:
                                                /* Ignore return codes. */
                                                SETFLAG(FLAG_IGNORE);
                                                break;

                                        case OPT_NOSPAWN:
                                                /* Don't execute commands. */
                                                SETFLAG(FLAG_NOSPAWN);
                                                break;

                                        case OPT_SHOW_INFO:
                                                /*
                                                ** Show info about macros
                                                ** and targets.
                                                */
                                                SETFLAG(FLAG_SHOW_INFO);
                                                break;

                                        case OPT_QUERY:
                                                /* Query mode. */
                                                SETFLAG(FLAG_QUERY);
                                                SETFLAG(FLAG_NOSPAWN);
                                                break;

                                        case OPT_NO_DEFAULTS:
                                                /* No default rules. */
                                                SETFLAG(FLAG_NO_DEFAULTS);
                                                break;

                                        case OPT_NO_SHOW:
                                                /* Don't show commands. */
                                                SETFLAG(FLAG_NO_SHOW);
                                                break;

                                        case OPT_TOUCH:
                                                /*
                                                ** Touch targets instead
                                                ** of building them.
                                                */
                                                SETFLAG(FLAG_TOUCH);
                                                break;

                                        case OPT_NEEDNEWER:
                                                /*
                                                ** Targets must be newer
                                                ** than dependents to be
                                                ** considered up to date.
                                                */
                                                SETFLAG(FLAG_NEEDNEWER);
                                                break;

                                        default:
                                                errmsg(MSG_ERR_BADOPTION,
                                                        &argv[i][j], NOVAL);
                                                errstop();
                                                return 1;
                                } /* End switch(...) */

                                j++;

                        } /* End while(...) */

                } /* End if(...) */

                i++;

        } /* End while(...) */

        /* Save flags from command line. */
        cmdflags = makeflags;

        if (CHKFLAG(FLAG_SIGNON))
        {
                mputs(MSG_SIGNON);
                mputs(MSG_COPYRIGHT);
                mputs("\n");
        }
        if (CHKFLAG(FLAG_DEBUG))
        {
                mputs(MSG_DBG_ENABLED);
        }
        if (CHKFLAG(FLAG_SHOW_INFO))
        {
                mputs(MSG_INFO_ENABLED);
        }

        /* If directory was specified, change to it. */
        if (cwd_name[0] != '\0')
        {
                /* Change directory. */
                if (!mchdir(cwd_name))
                {
                        /* Couldn't change directory. */
                        errmsg(MSG_ERR_CHDIR, cwd_name, NOVAL);
                        errstop();
                        return 1;
                }
        }

        /* Parse macro definitions listed on command line. */
        i = 1;
        while (i < argc)
        {
                if ((itmp = cindex(argv[i], '=')) > 0)
                {
                        if (CHKFLAG(FLAG_SHOW_INFO))
                        {
                                mputs(MSG_INFO_CMDMACRO);
                                mputs("'");
                                mputs(argv[i]);
                                mputs("'\n");
                        }

                        /* Argument contains a macro definition. */
                        if (!define_macro(argv[i], -1))
                        {
                                errstop();
                                return 1;
                        }
                }
                i++;
        }

        /*
        ** If environment override is not enabled, then
        ** read the environment before the makefile is read.
        */
        if (!CHKFLAG(FLAG_ENV_OVERRIDE))
        {
                if (!load_env(envp))
                {
                        deinitialize();
                        errstop();
                        return 1;
                }
        }

        /* Initialize predefined macros. */
        if (!predefine_macros())
        {
                deinitialize();
                return 1;
        }

        /*
        ** If defaults are not disabled, read the default
        ** macros and rules from the initialization file.
        */
        if (!CHKFLAG(FLAG_NO_DEFAULTS))
        {
                /* See if make initialization file exists. */
                if (find_inifile(inifile_name))
                {
                        /* Process the make initialization file. */
                        if (!process_makefile(inifile_name))
                        {
                                deinitialize();
                                errstop();
                                return 1;
                        }
                }
        }

        /* Parse the contents of the makefile. */
        cvt_slash(makefile_name);
        if (!process_makefile(makefile_name))
        {
                deinitialize();
                errstop();
                return 1;
        }

        /*
        ** If environment override is enabled, read the
        ** environment now.
        */
        if (CHKFLAG(FLAG_ENV_OVERRIDE))
        {
                if (!load_env(envp))
                {
                        deinitialize();
                        errstop();
                        return 1;
                }
        }

        /* Make sure at least one target was found in the makefile. */
        if (default_target() == (char *)NULL)
        {
                errmsg(MSG_ERR_NOTARGETS, makefile_name, NOVAL);
                deinitialize();
                errstop();
                return 1;
        }

        /* If info mode is enabled, then display collected information. */
        if (CHKFLAG(FLAG_SHOW_INFO))
        {
                dump_macros();
                dump_precious();
                dump_suffixes();
                dump_rules();
                dump_targets();
        }

        /* Build targets listed on command line. */
        i = 1;
        itmp = 0;
        while (i < argc)
        {
                if (argv[i][0] != '-' &&
                        cindex(argv[i], '=') < 0)
                {
                        if (CHKFLAG(FLAG_SHOW_INFO))
                        {
                                /* Display target from command line. */
                                mputs(MSG_INFO_CMDTARGETS);
                                mputs("'");
                                mputs(argv[i]);
                                mputs("'\n");
                        }

                        /* Argument contains a target name. */
                        if (!(result = make_target(argv[i], 0, &hitime)))
                        {
                                deinitialize();
                                if (!CHKFLAG(FLAG_QUERY))
                                        errstop();
                                return 1;
                        }
                        if (result == 2)
                        {
                                /*
                                ** Tell user that target is already up to
                                ** date.
                                */
                                mputs(MSG_UPTODATE);
                                mputs("'");
                                mputs(argv[i]);
                                mputs("'\n");
                        }
                        else
                                query_result++;
                        itmp++;
                }
                i++;
        }

        if (!itmp)
        {
                if (CHKFLAG(FLAG_SHOW_INFO))
                {
                        /* Tell user that default target name is being used. */
                        mputs(MSG_INFO_ASSUMETARGET);
                        mputs("'");
                        mputs(default_target());
                        mputs("'\n");
                }

                /*
                ** No targets were given on command line, so
                ** build the default target, which is the first
                ** target listed in the makefile.
                */
                if (!(result = make_target(default_target(), 0, &hitime)))
                {
                        deinitialize();
                        if (!CHKFLAG(FLAG_QUERY))
                                errstop();
                        return 1;
                }
                if (result == 2)
                {
                        /* Tell user that the target is already up to date. */
                        mputs(MSG_UPTODATE);
                        mputs("'");
                        mputs(default_target());
                        mputs("'\n");
                }
                else
                        query_result++;
        }

        /* Shut down. */
        deinitialize();

        /* If in query mode, return query result. */
        if (CHKFLAG(FLAG_QUERY))
                return query_result;

        /* No errors. */
        return 0;
}

