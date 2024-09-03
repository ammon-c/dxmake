/*
======================================================================
makexpnd.c
Macro expansion routines for make utility.

This source file is part of a computer program that is
(C) Copyright 1988 Ammon R. Campbell.  All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES

This module contains routines for expanding macro invocations in
text strings.  Only macro expansion is done here; the macro list
magagement and parsing of macro definitions from the makefile is
handled elsewhere.

Special Macros:

Make's 'special macros' include the following:

        $*      Expands to the basename of the current target.
                This may be used in the commands list of a
                target or a rule.

        $@      Expands to the full name of the current target.
                This may be used in the commands list of a
                target or a rule.

        $<      Expands to the full name of the dependent file
                for a rule.  This may only be used in the
                commands list of a rule.

        $**     Expands to a list of all the dependents of the
                current target.  This may be used in the
                commands list of a target or a rule.

        $?      Expands to a list of all out-of-date dependents
                for the current target.  This may be used in the
                commands list of a target or a rule.

======================================================================
*/

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

#include "make.h"
#include "getpath.h"
#include "fnexp.h"
#include "wild.h"

/*
** expand_named_macros:
** Expands any named macros in a given string.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      src     Buffer containing the string to be expanded.
**      dest    Buffer to placed the expanded string in.
**      maxlen  Maximum length of expanded string.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      2       Successful, with macros expanded.
**      1       Successful, but there were no macros to expand.
**      0       Error occurred (undefined macro used, expansion
**              too long, etc).
*/
int
expand_named_macros(src, dest, maxlen)
        char    *src;
        char    *dest;
        int     maxlen;
{
        int     spos = 0;       /* Position in src string. */
        int     dpos = 0;       /* Position in dest string. */
        int     mpos;           /* Position in mname string. */
        char    mname[MAXPATH]; /* Name of macro. */
        char    *mptr;          /* Pointer to macro expansion. */
        int     did = 0;        /* Flag, nonzero if macro gets expanded. */

        /* Process each character in the source string. */
        while (src[spos] != '\0')
        {
                /* Check for destination string overflow. */
                if (dpos >= maxlen - 1)
                {
                        /* Expansion caused string to become too long. */
                        errmsg(MSG_ERR_EXPTOOLONG, src, NOVAL);
                        return 0;
                }

                /*
                ** Do the right thing depending on what character is
                ** encountered in the source string.
                */
                if (src[spos] == '\\')
                {
                        /*
                        ** We got a backslash, which is sometimes
                        ** used to escape various special characters,
                        ** such as the '$' which normally preceeds a
                        ** macro invocation.
                        */

                        /* Check for escaped '$' */
                        if (src[spos + 1] == '$')
                        {
                                /* It's a real '$' */
                                dest[dpos++] = '$';
                                spos += 2;              /* Skip "\$" */
                        }
                        else
                        {
                                /* Copy the backslash. */
                                dest[dpos++] = src[spos++];

                                /* Copy the escaped character. */
                                if (src[spos] != '\0')
                                        dest[dpos++] = src[spos++];
                        }
                }
                else if (src[spos] == '$')
                {
                        /*
                        ** We hit a '$', so this is an invocation of
                        ** a macro.
                        */
                        spos++;         /* Skip '$' */
                        mpos = 0;       /* Assume no macro. */

                        /*
                        ** Do the right thing depending on what character
                        ** follows the '$'.
                        */
                        if (src[spos] == '*' || src[spos] == '<' ||
                                src[spos] == '@' || src[spos] == '?')
                        {
                                /* Check for dest buffer overflow. */
                                if (dpos >= maxlen - 2)
                                {
                                        /*
                                        ** Expansion caused string
                                        ** to become too long.
                                        */
                                        errmsg(MSG_ERR_EXPTOOLONG, src, NOVAL);
                                        return 0;
                                }

                                /*
                                ** This is a special macro, which is
                                ** handled elsewhere.
                                */
                                dest[dpos++] = '$';
                                dest[dpos++] = src[spos++];
                        }
                        else if (src[spos] == '$')
                        {
                                /* Check for dest buffer overflow. */
                                if (dpos >= maxlen - 2)
                                {
                                        /*
                                        ** Expansion caused string
                                        ** to become too long.
                                        */
                                        errmsg(MSG_ERR_EXPTOOLONG, src, NOVAL);
                                        return 0;

                                }

                                /* Got double '$', which we ignore. */
                                dest[dpos++] = '$';
                                dest[dpos++] = src[spos++];
                        }
                        else if (src[spos] == '(')
                        {
                                /* Macro name is enclosed in parenthesis. */
                                /* Extract macro name from source string. */
                                spos++;         /* Skip '(' */
                                mpos = 0;
                                while (src[spos] != ')' &&
                                        mpos < MAXPATH - 1 &&
                                        src[spos] != '\0')
                                {
                                        mname[mpos++] = src[spos++];
                                }
                                mname[mpos] = '\0';
                                if (src[spos] != ')')
                                {
                                        /* Expected a ')' after macro name. */
                                        errmsg(MSG_ERR_NORPAREN, src, NOVAL);
                                        return 0;
                                }
                                spos++;         /* Skip ')' */

                                /*
                                ** If it's a special macro, ignore it for
                                ** now, because they are handled elsewhere.
                                */
                                if (mpos > 0 &&
                                        mname[0] == '*' ||
                                        mname[0] == '<' ||
                                        mname[0] == '@' ||
                                        mname[0] == '?')
                                {
                                        /* Check for dest buffer overflow. */
                                        if (dpos >= maxlen - 4 -
                                                        strlen(mname))
                                        {
                                                /*
                                                ** Expansion caused string
                                                ** to become too long.
                                                */
                                                errmsg(MSG_ERR_EXPTOOLONG,
                                                        src, NOVAL);
                                                return 0;
                                        }

                                        /*
                                        ** Copy special macro invocation
                                        ** back to dest.
                                        */
                                        dest[dpos++] = '$';
                                        dest[dpos++] = '(';
                                        mpos = 0;
                                        while (mname[mpos] != '\0')
                                                dest[dpos++] = mname[mpos++];
                                        dest[dpos++] = ')';

                                        /* Don't expand macro below. */
                                        mpos = 0;
                                }
                        }
                        else
                        {
                                /* Macro name is only one letter. */
                                mname[0] = src[spos++]; /* Skip letter. */
                                mname[1] = '\0';
                        }

                        /*
                        ** If a macro name was generated, that means we
                        ** need to expand it.
                        */
                        if (mpos > 0)
                        {
                                /* Get macro expansion. */
                                mptr = find_macro(mname);
                                if (mptr == (char *)NULL)
                                {
                                        /* Macro not defined. */
                                        errmsg(MSG_ERR_NOMACRO, mname, NOVAL);
                                        return 0;
                                }

                                /* Place macro expansion in dest buffer. */
                                mpos = 0;
                                while (mptr[mpos] != '\0')
                                {
                                        /* Check for dest buffer overflow. */
                                        if (dpos >= maxlen - 1)
                                        {
                                                /*
                                                ** Expansion caused string
                                                ** to become too long.
                                                */
                                                errmsg(MSG_ERR_EXPTOOLONG,
                                                        src, NOVAL);
                                                return 0;
                                        }

                                        /* Copy macro character to dest. */
                                        dest[dpos++] = mptr[mpos++];
                                }

                                did = 1;
                        }
                }
                else
                {
                        /* Copy character from src to dest. */
                        dest[dpos++] = src[spos++];
                }
        }

        /* Place null at end of expanded string. */
        dest[dpos] = '\0';

        if (did)
                return 2;
        return 1;
}

/*
** expand_wildcard:
** Expands a wildcard filespec into individual filenames.
** Directories, hidden files, and system files are not
** included.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      fspec   Wildcard filespec to be expanded.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      -1      Error occurred.
**      NULL    No files matched the wildcard.
**      other   Pointer to list of lines containing the
**              filenames.
*/
LINE *
expand_wildcard(fspec)
        char    *fspec;
{
        LINE            *names = (LINE *)NULL;
        LINE            *ltmp;
        struct find_t   findbfr;        /* Buffer for w_find...() */
        int             result;         /* Result of last w_find...() */
        char            fname[MAXPATH]; /* Name of matched file. */

        /* Process each matching filespec. */
        result = w_findfirst(fspec, &findbfr);
        while (!result)
        {
                /* Check if matched file is the right kind. */
                if (!(findbfr.attrib & _A_SUBDIR) &&
                        !(findbfr.attrib & _A_HIDDEN) &&
                        !(findbfr.attrib & _A_SYSTEM))
                {
                        /* Build full pathname of matched file. */
                        strcpy(fname, fspec);
                        get_part_filename(1, fname, fname);
                        if (fname[0] != '\0' &&
                                fname[strlen(fname) - 1] != '\\' &&
                                fname[strlen(fname) - 1] != ':')
                        {
                                strcat(fname, "\\");
                        }
                        strcat(fname, findbfr.name);
                        strlwr(fname);                  /* Just for MS-DOS. */

                        /* Add filename to list. */
                        ltmp = append_line(names, fname);
                        if (ltmp == (LINE *)NULL)
                        {
                                /* Error adding to list. */
                                errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
                                free_lines(names);
                                return (LINE *)(-1);
                        }
                        names = ltmp;
                }

                /* Find next match (if any). */
                result = w_findnext(fspec, &findbfr);
        }

        /* Check if any files matched. */
        if (names == (LINE *)NULL)
                return (LINE *)NULL;

        return names;
}

/*
** expand_dependents:
** Expands the dependent files list for a particular target
** by taking the text stored in the 'tdependents' field of
** the specified target and creating a list of the individual
** dependent filenames found there.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      tar     Pointer to descriptor for target to expand
**              dependents for.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      NULL    Error expanding dependents.
**      other   Pointer to first line descriptor of expanded
**              dependent list.
*/
LINE *
expand_dependents(tar)
        TARGET  *tar;
{
        char    tmp[MAXPATH];           /* Temporary dependent filename. */
        LINE    *dlhead = (LINE *)NULL; /* Head of dependent filenames list. */
        int     tpos;                   /* Position in temporary buffer. */
        int     dpos;                   /* Position in dependent info line. */
        LINE    *ltmp;                  /* Temporary line pointer. */
        LINE    *matches;               /* Temporary line pointer. */
        LINE    *ltmp2;                 /* Temporary line pointer. */

        /*
        ** Extract individual filenames from dependent info line.
        ** "tar->tdependents" contains a line of text from the
        ** makefile with one or more dependent filenames.  These
        ** are extracted one at a time into the buffer tmp and
        ** then appended to the linked list of lines starting at
        ** "dlhead".
        */
        dpos = 0;
        while (tar->tdependents[dpos] != '\0')
        {
                /* Skip any leading whitespace. */
                while (tar->tdependents[dpos] == ' ' ||
                        tar->tdependents[dpos] == '\t')
                {
                        dpos++;
                }

                /* Copy filename to temporary buffer. */
                tpos = 0;
                while (tar->tdependents[dpos] != ' ' &&
                        tar->tdependents[dpos] != '\t' &&
                        tar->tdependents[dpos] != '\0')
                {
                        /* Check for overflow. */
                        if (tpos >= MAXPATH)
                        {
                                /* Filename exceeds maximum length. */
                                errmsg(MSG_ERR_PATHTOOLONG,
                                        tar->tdependents, NOVAL);
                                free_lines(dlhead);
                                return (LINE *)NULL;
                        }

                        /* Copy character to buffer. */
                        tmp[tpos++] = tar->tdependents[dpos++];
                }
                tmp[tpos] = '\0';

                /* Check if it's a wildcard or not. */
                if (cindex(tmp, '*') >= 0 || cindex(tmp, '?') >= 0)
                {
                        /* Get list of matching files. */
                        matches = expand_wildcard(tmp);
                        if (matches == (LINE *)(-1))
                        {
                                /* Error expanding wildcard. */
                                free_lines(dlhead);
                                return (LINE *)NULL;
                        }

                        /* Complain if wildcard didn't match anything. */
                        if (matches == (LINE *)NULL)
                        {
                                errmsg(MSG_ERR_NOWILDDEP, tmp, NOVAL);
                                return (LINE *)NULL;
                        }

                        /* Add matches to dependent list. */
                        ltmp2 = matches;
                        while (ltmp2 != (LINE *)NULL)
                        {
                                ltmp = append_line(dlhead, ltmp2->ldata);
                                if (ltmp == (LINE *)NULL)
                                {
                                        /* Out of memory. */
                                        errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
                                        free_lines(dlhead);
                                        return (LINE *)NULL;
                                }
                                dlhead = ltmp;
                                ltmp2 = ltmp2->lnext;
                        }
                        free_lines(matches);
                }
                else
                {
                        /* Add the filename to the new dependent list. */
                        if (tpos > 0)
                        {
                                ltmp = append_line(dlhead, tmp);
                                if (ltmp == (LINE *)NULL)
                                {
                                        /* Out of memory. */
                                        errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
                                        free_lines(dlhead);
                                        return (LINE *)NULL;
                                }
                                dlhead = ltmp;
                        }
                }
        }

        return dlhead;
}

/*
** expand_tarspecial:
** Expands the '$<', '$*', and '$@' special macros.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      src     String to be expanded.
**      dest    String to place expanded text in.
**      maxlen  Maximum length of expanded text.
**      tfile   Target filename.
**      sfile   Source filename for rule (if rule is not being
**              used, this should be NULL).
**
** Returns:
**      Value   Meaning
**      -----   -------
**      1       Successful.
**      0       Error occurred.
*/
int
expand_tarspecial(src, dest, maxlen, tfile, sfile)
        char    *src;
        char    *dest;
        int     maxlen;
        char    *tfile;
        char    *sfile;
{
        int     spos = 0;       /* Position in src string. */
        int     dpos = 0;       /* Position in dest string. */
        int     i;              /* Temporary loop index. */
        int     j;              /* Temporary loop index. */
        int     did_paren;      /* Flag, nonzero if macro in parenthesis. */

        while (src[spos] != '\0')
        {
                did_paren = 0;

                /* Check for destination string overflow. */
                if (dpos >= maxlen - 1)
                {
                        /* Expansion caused string to become too long. */
                        errmsg(MSG_ERR_EXPTOOLONG, src, NOVAL);
                        return 0;
                }

                /*
                ** Do the right thing depending on what the next
                ** character in the source string is.
                */
                if (src[spos] == '\\')
                {
                        /*
                        ** Character is a backslash, so treat the
                        ** next character literally if it is a '$'.
                        */

                        /* Check for overflow. */
                        if (dpos >= maxlen - 2)
                        {
                                /* Expansion caused string to be too long. */
                                errmsg(MSG_ERR_EXPTOOLONG, src, NOVAL);
                                return 0;
                        }

                        if (src[spos + 1] == '$')
                        {
                                spos++;                         /* Skip '\\' */
                                dest[dpos++] = src[spos++];     /* Get '$' */
                        }
                        else
                        {
                                dest[dpos++] = src[spos++];     /* Get '\\' */
                                if (src[spos] != '\0')          /* Get char */
                                        dest[dpos++] = src[spos++];
                        }
                }
                if (src[spos] == '$')
                {
                        /*
                        ** We got a '$', so see if it's the start of
                        ** a special macro.  If not, we treat it as
                        ** a literal '$'.
                        ** Also, the macro may be enclosed in '()'.
                        */
                        spos++;                         /* Skip '$' */
                        if (src[spos] == '(')
                        {
                                did_paren = 1;
                                spos++;                 /* Skip '(' */
                        }

                        if (src[spos] == '<')
                        {
                                /*
                                ** Handle '$<'
                                */
                                spos++;                 /* Skip '<' */
                                if (did_paren)
                                {
                                        if (src[spos] != ')')
                                        {
                                                errmsg(MSG_ERR_NORPAREN,
                                                        src, NOVAL);
                                                return 0;
                                        }
                                        spos++;         /* Skip ')' */
                                }
                                if (sfile == (char *)NULL ||
                                        sfile[0] == '\0')
                                {
                                        /* We're not in a rule, so complain. */
                                        errmsg(MSG_ERR_LTNORULE, src, NOVAL);
                                        return 0;
                                }
                                else
                                {
                                        /* Check for overflow. */
                                        if (dpos + strlen(sfile) + 1 >= maxlen)
                                        {
                                                /* Expansion caused string to be too long. */
                                                errmsg(MSG_ERR_EXPTOOLONG, src, NOVAL);
                                                return 0;
                                        }

                                        /* Copy rule sourcefile into dest. */
                                        i = 0;
                                        while (sfile[i] != '\0')
                                                dest[dpos++] = sfile[i++];
                                }
                        }
                        else if (src[spos] == '*')
                        {
                                /*
                                ** Handle '$*', making sure to leave '$**'.
                                */
                                spos++;                 /* Skip '*' */
                                if (src[spos] == '*')
                                {
                                        /* Leave '$**' alone. */
                                        dest[dpos++] = '$';
                                        dest[dpos++] = '*';
                                        dest[dpos++] = src[spos++];
                                }
                                else
                                {
                                        /*
                                        ** It really is '$*' so extract
                                        ** the basename of the target into
                                        ** the dest.
                                        */

                                        /* Find end of basename. */
                                        i = strlen(tfile);
                                        while (i > 0 &&
                                                tfile[i] != '.' &&
                                                tfile[i] != '\\')
                                                i--;
                                        if (i == 0 || tfile[i] == '\\')
                                                i = strlen(tfile);

                                        /* Check for overflow. */
                                        if (dpos + i + 1 >= maxlen)
                                        {
                                                /* Expansion caused string to be too long. */
                                                errmsg(MSG_ERR_EXPTOOLONG, src, NOVAL);
                                                return 0;
                                        }

                                        /* Copy basename to dest. */
                                        j = 0;
                                        while (j < i)
                                                dest[dpos++] = tfile[j++];
                                }
                                if (did_paren)
                                {
                                        if (src[spos] != ')')
                                        {
                                                errmsg(MSG_ERR_NORPAREN,
                                                        src, NOVAL);
                                                return 0;
                                        }
                                        spos++;         /* Skip ')' */
                                }
                        }
                        else if (src[spos] == '@')
                        {
                                /*
                                ** Handle '$@'
                                */
                                spos++;                 /* Skip '@' */
                                if (did_paren)
                                {
                                        if (src[spos] != ')')
                                        {
                                                errmsg(MSG_ERR_NORPAREN,
                                                        src, NOVAL);
                                                return 0;
                                        }
                                        spos++;         /* Skip ')' */
                                }

                                /* Check for overflow. */
                                if (dpos + strlen(tfile) + 1 >= maxlen)
                                {
                                        /* Expansion caused string to be too long. */
                                        errmsg(MSG_ERR_EXPTOOLONG, src, NOVAL);
                                        return 0;
                                }

                                /* Copy target name to dest. */
                                i = 0;
                                while (tfile[i] != '\0')
                                        dest[dpos++] = tfile[i++];
                        }
                        else
                        {
                                /*
                                ** Character was a '$' with nothing
                                ** special, so just copy the '$'.
                                */

                                /* Check for overflow. */
                                if (dpos >= maxlen - 2)
                                {
                                        /* Expansion caused string to be too long. */
                                        errmsg(MSG_ERR_EXPTOOLONG, src, NOVAL);
                                        return 0;
                                }

                                /* Just copy the '$'. */
                                dest[dpos++] = '$';
                                if (did_paren)
                                        dest[dpos++] = '(';
                        }
                }
                else
                {
                        /*
                        ** The source character is nothing special,
                        ** so just copy it.
                        */
                        dest[dpos++] = src[spos++];
                }
        } /* End while() */

        dest[dpos] = '\0';

        return 1;
}

