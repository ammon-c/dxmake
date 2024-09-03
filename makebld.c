/*
======================================================================
makebld.c
Target building routines for make utility.

This source file is part of a computer program that is
(C) Copyright 1988 Ammon R. Campbell.  All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES

This module contains the routines that do the actual testing and
building of files for the make utility.  Once the makefile has
been completely parsed and closed and the make utility's internal
data structures have been constructed, the make_target() function
in this module is called to build the primary target.  Any other
files that must be built to satisfy the request are handled here
by recursive calls to make_target() through the make_dependents()
function.

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
#include <process.h>

#include "make.h"

/****************************** HEADERS *****************************/

/* Functions local to this file: */
//static int    expand_smacros(char *src, char *dest, char *tfile, char *sfile);
static int      run_command(char *cmd);
static int      run_commands(char *tname, TARGET *tar, RULE *rul);
static RULE     *find_rule(char *tname, char *srcname);
static int      timestamp_up_to_date(time_t t1, time_t t2);
static int      make_dependents(TARGET *tar, int level, time_t *hitime);
static int      expand_dspecial(TARGET *tar, char *cmd);
//static int    expand_special(char *tname, char *src, char *cmd);

/****************************** CONSTANTS ***************************/

/* Maximum number of arguments per subprocess command line. */
#define MAXARGS         20

/*************************** LOCAL FUNCTIONS ************************/

/*
** find_rule:
** Attempts to find a rule that can be used to build a particular
** undescribed target file.  Also, verifies the existance of the
** source file necessary to use the rule.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      tname   Name of target file.
**      srcname Buffer to return name of source file to go with
**              rule in.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      NULL    No rule was found that can be used to build the
**              specified target file.
**      other   Pointer to rule to build target file.
*/
static RULE *
find_rule(tname, srcname)
        char    *tname;
        char    *srcname;
{
        int     sindex = 0;                     /* Index into suffixes list. */
        char    tsuffix[MAX_SUFFIX_STR + 1];    /* Target file's suffix. */
        char    *suffix;                        /* Pointer to source suffix. */
        RULE    *rptr;                          /* Pointer to rule. */

        /* Extract the suffix portion of the target filename. */
        get_part_filename(4, tname, tsuffix);
        if (tsuffix[0] == '\0')
        {
                /*
                ** The target file has no suffix, so it can't be
                ** matched against a rule.
                */
                return (RULE *)NULL;
        }

        /*
        ** Check if it's worth our time to test all the suffixes
        ** with the rule list.
        */
        if (!check_rules(tsuffix))
        {
                /* Don't bother. */
                return (RULE *)NULL;
        }

        /*
        ** Try each possible suffix for a rule until we find
        ** one that works or run out of suffixes.
        */
        while ((suffix = enum_suffix(sindex)) != (char *)NULL)
        {
                /*
                ** Make sure suffix isn't the same as target's
                ** suffix before looking for a rule.
                */
                if (strcmp(suffix, tsuffix) != 0)
                {
                        /* Look for a rule for this suffix. */
                        if ((rptr = lookup_rule(suffix, tsuffix)) !=
                                (RULE *)NULL)
                        {
                                if (CHKFLAG(FLAG_DEBUG))
                                {
                                        /*
                                        ** Tell the user we have a
                                        ** rule that might work.
                                        */
                                        mputs(MSG_DBG_POSSIBLERULE);
                                        mputs(".");
                                        mputs(rptr->rsrc);
                                        mputs(".");
                                        mputs(rptr->rdest);
                                        mputs("\n");
                                }

                                /*
                                ** We have a matching rule, so see if
                                ** there is a source file we can use
                                ** with it.
                                */

                                /* Build name of potential source file. */
                                get_part_filename(5, tname, srcname);
                                strcat(srcname, ".");
                                strcat(srcname, rptr->rsrc);

                                /* Check if source file exists. */
                                if (access(srcname, 444) == 0)
                                {
                                        /*
                                        ** The source file exists, so
                                        ** return the rule to the caller.
                                        */
                                        if (CHKFLAG(FLAG_DEBUG))
                                        {
                                                mputs(MSG_DBG_HAVEMATCH);
                                                mputs(srcname);
                                                mputs("\n");
                                        }
                                        return rptr;
                                }
                        }
                }

                /* Try next suffix in suffixes list. */
                sindex++;
        }

        /* No matching rules. */
        return (RULE *)NULL;
}

/*
** timestamp_up_to_date:
** Checks to see if a timestamp is up to date with respect to another
** timestamp.  If the 'CHKFLAG(FLAG_NEEDNEWER)' flag is enabled, the first
** timestamp must be newer than the second timestamp to be considered
** up to date; otherwise the first timestamp must be the same as or
** newer than the second timestamp to be considered up to date.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      t1      First timestamp to check.
**      t2      Second tiemstamp to check.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      1       First timestamp is up-to-date with respect to the
**              second.
**      0       First timestamp is not up-to-date with respect to
**              the second.
*/
static int
timestamp_up_to_date(t1, t2)
        time_t  t1;
        time_t  t2;
{
        if (CHKFLAG(FLAG_NEEDNEWER))
        {
                if (t1 > t2)
                        return 1;
        }
        else
        {
                if (t1 >= t2)
                        return 1;
        }
        return 0;
}

/*
** make_dependents:
** Makes the dependent files of the specified target.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      tar     Pointer to target descriptor of target
**              whose dependents should be built.
**      level   Recursion level of call.
**      hitime  Pointer to time_t value to receive
**              timestamp of newest dependent file.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      1       Successful.
**      0       Error(s) occurred.
*/
static int
make_dependents(tar, level, hitime)
        TARGET  *tar;
        int     level;
        time_t  *hitime;
{
        LINE    *dptr;
        LINE    *lptr;
        time_t  tmptime;

        *hitime = 0L;

        /* Expand the list of dependent files. */
        dptr = expand_dependents(tar);
        if (dptr == (LINE *)NULL)
        {
                /* Error expanding dependent list. */
                return 0;
        }

        if (CHKFLAG(FLAG_DEBUG))
        {
                mputs(MSG_DBG_CHKDEPS);
                mputs(tar->tname);
                mputs("\n");
                lptr = dptr;
                while (lptr != (LINE *)NULL)
                {
                        mputs(MSG_DBG_DEPNAME);
                        mputs(lptr->ldata);
                        mputs("\n");
                        lptr = lptr->lnext;
                }
        }

        /* Process each dependent file in the list. */
        lptr = dptr;
        while (lptr != (LINE *)NULL)
        {
                if (!make_target(lptr->ldata, level + 1, &tmptime))
                {
                        /* Error making dependent file. */
                        free_lines(dptr);
                        return 0;
                }
                if (tmptime > *hitime)
                        *hitime = tmptime;

                lptr = lptr->lnext;
        }

        free_lines(dptr);
        return 1;
}

/*
** expand_dspecial:
** Expands the special macros "$**" and "$?" that appear in
** the specified command line.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      tar     Pointer to target descriptor of target file.
**      cmd     Command line to be expanded.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      1       Successful.
**      0       Error occurred (undefined macro, command too long).
*/
static int
expand_dspecial(tar, cmd)
        TARGET  *tar;
        char    *cmd;
{
        char    *tname = tar->tname;
        char    *tmp;
        int     tpos = 0;
        int     cpos = 0;
        int     i = 0;
        LINE    *dptr;
        LINE    *lptr;

        if (cindex(cmd, '$') < 0)
        {
                /* No dspecial macros to expand; quick success. */
                return 1;
        }

        /* Allocate memory for temporary buffer. */
        tmp = (char *)mem_alloc(MAXLLINE);
        if (tmp == (char *)NULL)
        {
                /* Not enough memory. */
                errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
                return 0;
        }

        if (tar->tdependents == (char *)NULL)
        {
                /* Target has no dependents. */
                mem_free(tmp);
                return 1;
        }
        dptr = expand_dependents(tar);
        if (dptr == (LINE *)NULL)
        {
                /* Error expanding dependents list. */
                mem_free(tmp);
                return 0;
        }

        while (cmd[cpos] != '\0')
        {
                if (cmd[cpos] == '$')
                {
                        cpos++;
                        if (cmd[cpos] == '?')
                        {
                                cpos++;
                                lptr = dptr;
                                while (lptr != (LINE *)NULL)
                                {
                                        if (tpos + strlen(lptr->ldata) + 1 >=
                                                MAXLLINE)
                                        {
                                                errmsg(MSG_ERR_EXPTOOLONG,
                                                        cmd, NOVAL);
                                                free_lines(dptr);
                                                mem_free(tmp);
                                                return 0;
                                        }
                                        tmp[tpos] = '\0';
                                        strcat(tmp, lptr->ldata);
                                        strcat(tmp, " ");
                                        tpos = strlen(tmp);
                                        lptr = lptr->lnext;
                                }
                        }
                        else if (cmd[cpos] == '*')
                        {
                                cpos++;
                                if (cmd[cpos] == '*')
                                {
                                        cpos++;
                                        lptr = dptr;
                                        while (lptr != (LINE *)NULL)
                                        {
                                                if (tpos + strlen(lptr->ldata)
                                                        + 1 >= MAXLLINE)
                                                {
                                                        errmsg(
                                                        MSG_ERR_EXPTOOLONG,
                                                                cmd, NOVAL);
                                                        free_lines(dptr);
                                                        mem_free(tmp);
                                                        return 0;
                                                }
                                                tmp[tpos] = '\0';
                                                strcat(tmp, lptr->ldata);
                                                strcat(tmp, " ");
                                                tpos = strlen(tmp);
                                                lptr = lptr->lnext;
                                        }
                                }
                                else
                                {
                                        if (tpos >= MAXLLINE - 1)
                                        {
                                                errmsg(MSG_ERR_EXPTOOLONG,
                                                        cmd, NOVAL);
                                                free_lines(dptr);
                                                mem_free(tmp);
                                                return 0;
                                        }
                                        tmp[tpos++] = '$';
                                        tmp[tpos++] = '*';
                                }
                        }
                        else
                        {
                                if (tpos >= MAXLLINE - 1)
                                {
                                        errmsg(MSG_ERR_EXPTOOLONG,
                                                cmd, NOVAL);
                                        free_lines(dptr);
                                        mem_free(tmp);
                                        return 0;
                                }
                                tmp[tpos++] = '$';
                                tmp[tpos++] = cmd[cpos++];
                        }
                }
                else
                {
                        if (tpos >= MAXLLINE)
                        {
                                errmsg(MSG_ERR_EXPTOOLONG,
                                        cmd, NOVAL);
                                free_lines(dptr);
                                mem_free(tmp);
                                return 0;
                        }
                        tmp[tpos++] = cmd[cpos++];
                }
        }

        tmp[tpos] = '\0';

        if (strlen(tmp) > MAXPATH)
        {
                /*
                ** The expanded command line exceeds the maximum length
                ** for a command line.
                */
                errmsg(MSG_ERR_CMDMACROLEN, tmp, NOVAL);
                mem_free(tmp);
                return 0;
        }

        /* Release dependent list memory. */
        free_lines(dptr);

        /* Return expanded command line to caller. */
        strcpy(cmd, tmp);

        mem_free(tmp);
        return 1;
}

/*
** run_command:
** Executes a command and tests the return code.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      cmd     String containing command to be executed.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      1       Successful (or return code was ignored).
**      0       Command could not be executed or return code
**              was non-zero.
*/
static int
run_command(cmd)
        char    *cmd;
{
        char    margc;
        char    argbfr[MAXPATH];
        char    *margv[MAXARGS];
#ifdef WIN
        char    stmp[MAXPATH];
#endif /* WIN */
        int     i;
        int     result;

        /* Make sure the command line isn't too long. */
        if (strlen(cmd) > MAXPATH)
        {
                errmsg(MSG_ERR_CMDTOOLONG, cmd, NOVAL);
                return 0;
        }

        /* Zero arguments. */
        for (i = 0; i < MAXARGS; i++)
        {
                margv[i] = (char *)NULL;
        }

        /*
        ** If user put '!' before the command, then complain.  This is
        ** what Microsoft NMAKE uses to reexecute the command once for
        ** each of the target's dependent files.
        */
        if (cmd[0] == '!')
        {
                /* We don't implement this, so complain to the user. */
                errmsg(MSG_ERR_NOBANG, cmd, NOVAL);
                return 0;
        }

        /*
        ** Copy the command string to a local buffer where we
        ** can munge it for spawnvp().
        */
        if (cmd[0] == '@' || cmd[0] == '-' || cmd[0] == '!')
                strcpy(argbfr, &cmd[1]);
        else
                strcpy(argbfr, &cmd[0]);

        /* Display the command unless display is disabled. */
        if (!CHKFLAG(FLAG_NO_SHOW) && cmd[0] != '@')
        {
                mputs(argbfr);
                mputs("\n");
        }

        /*
        ** If make is running in display only mode, then don't actually
        ** run the command.
        */
        if (CHKFLAG(FLAG_NOSPAWN))
        {
                return 1;
        }

        /* Release unused memory to system before calling subprocess. */
        mem_heapmin();

#ifdef WIN
        result = wrun(argbfr);
        if (result != 0 && cmd[0] != '-')
        {
                if (result == -1)
                {
                        /* Can't execute the command.  Complain. */
                        errmsg(MSG_ERR_CANTEXEC, argbfr, (int)errno);
                        return 0;
                }
                else
                {
                        /*
                        ** Command had non-zero return code.
                        ** Complain to the user.
                        */
                        errmsg(MSG_ERR_RETCODE, (char *)NULL, result);
                        return 0;
                }
        }
#else /* !WIN */
        /*
        ** If command contains I/O redirection, then use system() instead
        ** of spawnvp().
        */
        if (cindex(argbfr, '<') >= 0 ||
                cindex(argbfr, '>') >= 0 ||
                cindex(argbfr, '|') >= 0)
        {
                /* Run command using system() to handle redirection. */
                result = system(argbfr);

                /*
                ** If the process return code was nonzero, generate an
                ** error, unless we were told to ignore the return code.
                */
                if (result != 0 && !CHKFLAG(FLAG_IGNORE) && cmd[0] != '-')
                {
                        if (result == -1)
                        {
                                /* Can't execute the command.  Complain. */
                                errmsg(MSG_ERR_CANTEXEC, margv[0], (int)errno);
                                return 0;
                        }
                        else
                        {
                                /*
                                ** Command had non-zero return code.
                                ** Complain to the user.
                                */
                                errmsg(MSG_ERR_RETCODE, (char *)NULL, result);
                                return 0;
                        }
                }

                return 1;
        }

        /* Split the command into individual arguments for spawnvp(). */
        i = 0;
        margv[0] = &argbfr[0];
        margc = 0;
        while (argbfr[i] != '\0')
        {
                margc++;
                while (argbfr[i] != ' ' &&
                        argbfr[i] != '\t' &&
                        argbfr[i] != '\0')
                {
                        i++;
                }
                if (argbfr[i] == '\0')
                        break;
                argbfr[i++] = '\0';

                while (argbfr[i] == ' ' ||
                        argbfr[i] == '\t')
                {
                        i++;
                }
                margv[margc] = &argbfr[i];

                if (margc >= MAXARGS)
                {
                        /* Too many arguments in command.  Complain. */
                        errmsg(MSG_ERR_MAXARGS, cmd, MAXARGS);
                        return 0;
                }
        }

        /* Try to run the command by spawning. */
        result = spawnvp(P_WAIT, margv[0], margv);
        if (result == -1)
        {
                /* Couldn't spawn directly, so try it as a DOS command. */
                for (i = margc - 1; i >= 0; i--)
                {
                        margv[i + 2] = margv[i];
                }
                margv[0] = "command.com";
                margv[1] = "/c";
                result = spawnvp(P_WAIT, margv[0], margv);
        }
        if (result != 0)
        {
                /*
                ** If the process return code was nonzero, generate an
                ** error, unless we were told to ignore the return code.
                */
                if (!CHKFLAG(FLAG_IGNORE) && cmd[0] != '-')
                {
                        if (result == -1)
                        {
                                /* Couldn't execute the command.  Complain. */
                                errmsg(MSG_ERR_CANTEXEC, margv[0], errno);
                                return 0;
                        }
                        else
                        {
                                /*
                                ** Command had non-zero return code.
                                ** Complain to the user.
                                */
                                errmsg(MSG_ERR_RETCODE, (char *)NULL, result);
                                return 0;
                        }
                }
        }
#endif /* WIN */

        return 1;
}

/*
** run_commands:
** Runs a list of subprocesses.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      tname   Name of target being built.
**      tar     Target descriptor for target being built (if any).
**      rul     Rule descriptor for rule being used (if any).
**
** Either tar or rul may be NULL, but not both.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      1       Successful.
**      0       Error occurred.
*/
static int
run_commands(tname, tar, rul)
        char    *tname;
        TARGET  *tar;
        RULE    *rul;
{
        LINE    *lptr;
        char    cmd[MAXPATH];
        char    rfile[MAXPATH];
        char    *rsrc = (char *)NULL;

        /* Assume we will use commands from target descriptor. */
        if (tar != (TARGET *)NULL)
                lptr = tar->tcommands;

        /* Check if rule was specified. */
        if (rul != (RULE *)NULL)
        {
                /* Build source filename for rule. */
                get_part_filename(5, tname, rfile);
                strcat(rfile, ".");
                strcat(rfile, rul->rsrc);

                /* Change pointer to source from NULL to filename. */
                rsrc = rfile;

                /* Use commands list from rule. */
                lptr = rul->rcommands;
        }

        /* Process each command in the list. */
        while (lptr != (LINE *)NULL)
        {
#ifdef WIN
                /* Check if user aborted. */
                if (check_abort())
                        return 1;
#endif /* WIN */

                /* Make sure command isn't too long. */
                if (strlen(lptr->ldata) > MAXPATH)
                {
                        /*
                        ** The command line exceeds the maximum length
                        ** for a command line.
                        */
                        errmsg(MSG_ERR_CMDTOOLONG, lptr->ldata, NOVAL);
                        return 0;
                }

                /* Expand any special macros. */
                if (!expand_tarspecial(lptr->ldata, cmd, MAXPATH, tname, rsrc))
                {
                        /* Macro expansion error. */
                        return 0;
                }

                /* Expand any dspecial macros. */
                if (tar != (TARGET *)NULL)
                {
                        if (!expand_dspecial(tar, cmd))
                        {
                                /* Macro expansion error. */
                                return 0;
                        }
                }

                /* Run the command. */
                if (!run_command(cmd))
                {
                        /* Couldn't run the command. */
                        if (!is_precious(tname))
                        {
                                /*
                                ** Remove the potentially
                                ** incorrect target file.
                                */
                                unlink(tname);
                        }
                        return 0;
                }

                /* Step to next command in list. */
                lptr = lptr->lnext;
        }

        return 1;
}

/****************************** FUNCTIONS ***************************/

/*
** make_target:
** Makes the specified target file up to date, including building
** of the target's dependent files and inferring build commands
** when necessary.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      tname   Name of target file to build.
**      level   Recursion level of call.
**      hitime  Pointer to time_t value to receive
**              timestamp of newest dependent file.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      2       Specified target was already up to date.
**      1       Successful, target is now up to date.
**      0       Error occurred, couldn't make file up to date.
*/
int
make_target(tname, level, hitime)
        char    *tname;
        int     level;
        time_t  *hitime;
{
        TARGET  *tar;           /* Pointer to target's descriptor. */
        LINE    *lptr;          /* Temporary line pointer. */
        char    cmd[MAXPATH];   /* Temporary command buffer. */
        RULE    *rul;           /* Temporary rule pointer. */
        int     i;              /* Temporary integer/loop index. */
        int     pos;            /* Temporary index into string. */
        int     exists = 0;     /* Flag, nonzero if target file exists. */
        struct mstat_t tstat;   /* File statistics for target file. */
        struct mstat_t dstat;   /* File statistics for dependent file. */

#ifdef WIN
        /* Check if user aborted. */
        if (check_abort())
                return 2;
#endif /* WIN */

        if (CHKFLAG(FLAG_DEBUG))
        {
                /* Tell the user what target we're about to try to make. */
                mputs(MSG_DBG_WANTTOMAKE);
                mputs(tname);
                mputs("\n");
        }

        /* See if target file exists, and if it does, get its timestamp. */
        *hitime = 0L;
        if (access(tname, 444) == 0)
        {
                if (mstat(tname, &tstat) != 0)
                {
                        /* Couldn't access file.  Complain to the user. */
                        errmsg(MSG_ERR_FACCESS, tname, NOVAL);
                        return 0;
                }
                *hitime = tstat.st_mtime;
                exists = 1;
        }

        /* Find the descriptor for the specified target. */
        tar = find_target(tname);

        /* Do the right thing depending on if the target was found. */
        if (tar == (TARGET *)NULL)
        {
                /*
                ** There is no descriptor for the specified target.
                ** See if we can infer how to build the target by
                ** applying a rule from the rule list to a file
                ** with an appropriate basename and extension in
                ** the current directory.  For example, if the
                ** target is "test.obj", and we have a ".c.obj" rule,
                ** and there happens to be a file called "test.c"
                ** in the current directory, we would use the rule
                ** to build "test.obj" from "test.c".
                */

                /* Get the extension portion of the targetname. */
                i = 0;
                pos = -1;
                while(tname[i] != '\0' && tname[i] != '\\')
                {
                        if (tname[i] == '.')
                                pos = i + 1;
                        i++;
                }
                if (pos == -1 || tname[pos] == '\0')
                {
                        /*
                        ** Target has no extension, so we can't use a
                        ** rule on it.  If the target already exists,
                        ** then all is well, otherwise we're stuck.
                        */
                        if (exists)
                        {
                                /* The target already exists; good. */
                                if (CHKFLAG(FLAG_DEBUG))
                                {
                                        /*
                                        ** Undescribed target exists,
                                        ** so tell the user.
                                        */
                                        mputs(MSG_DBG_UTEXISTS);
                                        mputs(tname);
                                        mputs("\n");
                                }
                                *hitime = tstat.st_mtime;
                                return 1;
                        }

                        /* Don't know how to make the specified target. */
                        errmsg(MSG_ERR_CANTMAKE, tname, NOVAL);
                        return 0;
                }

                /* Search for a rule we can use. */
                rul = find_rule(tname, cmd);

                if (rul == (RULE *)NULL)
                {
                        /*
                        ** We couldn't find a rule to build the
                        ** file from.  If the file already exists,
                        ** then it's OK, otherwise we're stuck.
                        */
                        if (exists)
                        {
                                /* The target already exists; good. */
                                if (CHKFLAG(FLAG_DEBUG))
                                {
                                        /*
                                        ** Tell the user that the undescribed
                                        ** target file exists.
                                        */
                                        mputs(MSG_DBG_UTEXISTS);
                                        mputs(tname);
                                        mputs("\n");
                                }
                                *hitime = tstat.st_mtime;
                                return 1;
                        }

                        /* Don't know how to make the specified target file. */
                        errmsg(MSG_ERR_CANTMAKE, tname, NOVAL);
                        return 0;
                }

                /*
                ** If target is newer than inferred dependent file,
                ** then don't build it.
                */

                /* Get timestamp of inferred dependent file. */
                if (mstat(cmd, &dstat) != 0)
                {
                        /* Couldn't access the file.  Complain. */
                        errmsg(MSG_ERR_FACCESS, cmd, NOVAL);
                        return 0;
                }

                /* Check target's timestamp against inferred depedent. */
                if (!CHKFLAG(FLAG_BUILD_ANYWAY) && exists &&
                        timestamp_up_to_date(tstat.st_mtime, dstat.st_mtime))
                {
                        /*
                        ** Target file is newer.  Don't build it.
                        */
                        if (CHKFLAG(FLAG_DEBUG))
                        {
                                /*
                                ** Tell the user that the target is up to
                                ** date with the dependent.
                                */
                                mputs(MSG_DBG_SHOWUPTODATE);
                                mputs(tname);
                                mputs(", ");
                                mputs(cmd);
                                mputs("\n");
                        }
                        *hitime = tstat.st_mtime;
                        return 2;
                }

                if (!exists)
                {
                        /*
                        ** Warn the user that the target about to be built
                        ** doesn't exist yet.
                        */
                        mputs(MSG_TARGETNOTEXIST);
                        mputs("'");
                        mputs(tname);
                        mputs("'\n");
                }

                if (CHKFLAG(FLAG_DEBUG))
                {
                        /* Tell the user we're building the target. */
                        mputs(MSG_DBG_BUILDING);
                        mputs(tname);
                        mputs("\n");
                }
                if (CHKFLAG(FLAG_TOUCH))
                {
                        if (!touch_file(tname))
                        {
                                /* Couldn't touch the file. */
                                errmsg(MSG_ERR_FACCESS, tname, NOVAL);
                                return 0;
                        }
                        if (mstat(tname, &tstat) != 0)
                        {
                                /* Error accessing the file. */
                                errmsg(MSG_ERR_FACCESS, tname, NOVAL);
                                return 0;
                        }
                        *hitime = tstat.st_mtime;
                        return 1;
                }

                /* Execute the commands specified in the rule. */
                lptr = rul->rcommands;
                if (lptr == (LINE *)NULL)
                {
                        /*
                        ** The rule doesn't have any commands associated
                        ** with it.  Complain to the user.
                        */
                        cmd[0] = '.';
                        cmd[1] = '\0';
                        strcat(cmd, rul->rsrc);
                        strcat(cmd, ".");
                        strcat(cmd, rul->rdest);
                        errmsg(MSG_ERR_EMPTYRULE, cmd, NOVAL);
                        return 0;
                }
                if (!run_commands(tname, (TARGET *)NULL, rul))
                {
                        return 0;
                }

                /* Get the timestamp of the just built target file. */
                if (mstat(tname, &tstat) == 0)
                        *hitime = tstat.st_mtime;

                /* No errors. */
                return 1;
        }

        /*
        ** If execution reaches here, the target was listed in
        ** the makefile, and we have a valid target descriptor.
        */

        /*
        ** If the target has dependents, make them before attempting
        ** to make the target itself.
        */
        if (tar->tdependents != (char *)NULL)
        {
                /* Make sure the target's dependents are up to date. */
                if (!make_dependents(tar, level, hitime))
                {
                        /* Error making dependents. */
                        return 0;
                }
        }

        /*
        ** Build the target file.  If the target has a command list,
        ** it will be used; otherwise, we will try to infer how to
        ** build the target file.
        */
        if (tar->tcommands != (LINE *)NULL)
        {
                /*
                ** We have commands to build the target.
                ** See if target file is already up to date.
                */
                if (!CHKFLAG(FLAG_BUILD_ANYWAY) && exists &&
                        timestamp_up_to_date(tstat.st_mtime, *hitime))
                {
                        /*
                        ** Target file is newer.  Don't build it.
                        */
                        if (CHKFLAG(FLAG_DEBUG))
                        {
                                /* Tell user that file is up to date. */
                                mputs(MSG_DBG_ISUPTODATE);
                                mputs(tname);
                                mputs("\n");
                        }
                        *hitime = tstat.st_mtime;
                        return 2;
                }

                if (!exists && cindex(tname, '.') > 0)
                {
                        /* Warn user that target file doesn't exist yet. */
                        mputs(MSG_TARGETNOTEXIST);
                        mputs("'");
                        mputs(tname);
                        mputs("'\n");
                }

                if (CHKFLAG(FLAG_DEBUG))
                {
                        /* Tell the user we're building the target now. */
                        mputs(MSG_DBG_BUILDING);
                        mputs(tname);
                        mputs("\n");
                }
                if (CHKFLAG(FLAG_TOUCH))
                {
                        if (!touch_file(tname))
                        {
                                /* Error touching file. */
                                errmsg(MSG_ERR_FACCESS, tname, NOVAL);
                                return 0;
                        }
                        if (mstat(tname, &tstat) != 0)
                        {
                                /* Error accessing file. */
                                errmsg(MSG_ERR_FACCESS, tname, NOVAL);
                                return 0;
                        }
                        *hitime = tstat.st_mtime;
                        return 1;
                }

                /*
                ** There are commands in the target's command
                ** list.  Use them to build the targetfile.
                */
                if (!run_commands(tname, tar, (RULE *)NULL))
                {
                        return 0;
                }

                /* Get the timestamp of the just built target file. */
                if (mstat(tname, &tstat) == 0)
                        *hitime = tstat.st_mtime;
        }
        else
        {
                /*
                ** There are no commands to build the target.
                ** See if there is an inference rule we can
                ** use.  Each inference rule's extensions
                ** will be checked against each file
                ** with the same basename in the current
                ** directory, until there is a rule that can
                ** be used or we run out of rules.
                ** For example, if the target is "test.obj"
                ** and we happen to have a ".c.obj" rule,
                ** and there happens to be a file called "test.c"
                ** in the current directory, we would use the rule
                ** to build "test.obj" from "test.c".
                */

                /* Get the extension portion of the targetname. */
                i = 0;
                pos = -1;
                while(tname[i] != '\0' && tname[i] != '\\')
                {
                        if (tname[i] == '.')
                                pos = i + 1;
                        i++;
                }
                if (pos == -1 || tname[pos] == '\0')
                {
                        /*
                        ** Target has no extension, so we can't use a
                        ** rule on it.  If the target already exists,
                        ** then all is well.  If the target had dependents,
                        ** then all is well.  Otherwise, we're stuck.
                        */
                        if (exists)
                        {
                                /* The target already exists; good. */
                                if (CHKFLAG(FLAG_DEBUG))
                                {
                                        /*
                                        ** Tell the user that the undescribed
                                        ** target file exists.
                                        */
                                        mputs(MSG_DBG_UTEXISTS);
                                        mputs(tname);
                                        mputs("\n");
                                }
                                *hitime = tstat.st_mtime;
                                return 1;
                        }
                        if (tar->tdependents != (char *)NULL)
                        {
                                /* The target had dependents. */
                                if (CHKFLAG(FLAG_DEBUG))
                                {
                                        /*
                                        ** Tell the user that we're assuming
                                        ** the extensionless target is really
                                        ** a dummy target.
                                        */
                                        mputs(MSG_DBG_ASSUMEDUMMY);
                                        mputs(tname);
                                        mputs("\n");
                                }
                                return 1;
                        }

                        /* We don't know how to build the specified target. */
                        errmsg(MSG_ERR_CANTMAKE, tname, NOVAL);
                        return 0;
                }

                /* Search for a rule we can use. */
                rul = find_rule(tname, cmd);

                if (rul == (RULE *)NULL)
                {
                        /*
                        ** We couldn't find a rule to build the
                        ** file from.  If the file already exists,
                        ** then it's OK, otherwise we're stuck.
                        */
                        if (exists)
                        {
                                /* The target already exists; good. */
                                if (CHKFLAG(FLAG_DEBUG))
                                {
                                        /*
                                        ** Tell the user that the
                                        ** undescribed target file
                                        ** exists.
                                        */
                                        mputs(MSG_DBG_UTEXISTS);
                                        mputs(tname);
                                        mputs("\n");
                                }
                                *hitime = tstat.st_mtime;
                                return 1;
                        }

                        /* We don't know how to make the specified target. */
                        errmsg(MSG_ERR_CANTMAKE, tname, NOVAL);
                        return 0;
                }

                /*
                ** If target is newer than inferred dependent file,
                ** then don't build it, unless the target's dependents
                ** are newer.
                */

                /* Get timestamp of inferred dependent file. */
                if (mstat(cmd, &dstat) != 0)
                {
                        /* Error accessing file. */
                        errmsg(MSG_ERR_FACCESS, cmd, NOVAL);
                        return 0;
                }

                /* Check target's timestamp against inferred depedent. */
                if (!CHKFLAG(FLAG_BUILD_ANYWAY) && exists &&
                        timestamp_up_to_date(tstat.st_mtime, dstat.st_mtime) &&
                        timestamp_up_to_date(tstat.st_mtime, *hitime))
                {
                        /*
                        ** Target file is newer.  Don't build it.
                        */
                        if (CHKFLAG(FLAG_DEBUG))
                        {
                                /*
                                ** Tell the user that the target file is
                                ** up to date with respect to the dependent
                                ** file.
                                */
                                mputs(MSG_DBG_SHOWUPTODATE);
                                mputs(tname);
                                mputs(", ");
                                mputs(cmd);
                                mputs("\n");
                        }
                        *hitime = tstat.st_mtime;
                        return 2;
                }

                if (!exists)
                {
                        /* Warn the user that the target doesn't exist yet. */
                        mputs(MSG_TARGETNOTEXIST);
                        mputs("'");
                        mputs(tname);
                        mputs("'\n");
                }

                if (CHKFLAG(FLAG_DEBUG))
                {
                        /* Tell user we're building the target. */
                        mputs(MSG_DBG_BUILDING);
                        mputs(tname);
                        mputs("\n");
                }
                if (CHKFLAG(FLAG_TOUCH))
                {
                        if (!touch_file(tname))
                        {
                                /* Error touching file. */
                                errmsg(MSG_ERR_FACCESS, tname, NOVAL);
                                return 0;
                        }
                        if (mstat(tname, &tstat) != 0)
                        {
                                /* Error accessing file. */
                                errmsg(MSG_ERR_FACCESS, tname, NOVAL);
                                return 0;
                        }
                        *hitime = tstat.st_mtime;
                        return 1;
                }

                /* Execute the commands specified in the rule. */
                lptr = rul->rcommands;
                if (lptr == (LINE *)NULL)
                {
                        /*
                        ** The rule doesn't have any commands associated
                        ** with it.  Complain to the user.
                        */
                        cmd[0] = '.';
                        cmd[1] = '\0';
                        strcat(cmd, rul->rsrc);
                        strcat(cmd, ".");
                        strcat(cmd, rul->rdest);
                        errmsg(MSG_ERR_EMPTYRULE, cmd, NOVAL);
                        return 0;
                }
                if (!run_commands(tname, tar, rul))
                {
                        return 0;
                }

                /* Get the timestamp of the just built target file. */
                if (mstat(tname, &tstat) == 0)
                        *hitime = tstat.st_mtime;
        }

        return 1;
}

