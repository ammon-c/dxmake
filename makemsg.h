/*
======================================================================
makemsg.h
Message strings include file for make utility

This source file is part of a computer program that is
(C) Copyright 1985-1988, 1990, 1992 Ammon R. Campbell.
All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES

This include file contains definitions for all of the text messages
that are output by the make utility, including the sign-on text,
error messages, status messages, and debug messages.

======================================================================
*/

/* Option switch characters. */
#define OPT_MAKEFILE            'f'
#define OPT_WORKINGDIR          'w'
#define OPT_BUILD_ANYWAY        'a'
#define OPT_SIGNON              'c'
#define OPT_DEBUG               'd'
#define OPT_ENV_OVERRIDE        'e'
#define OPT_IGNORE              'i'
#define OPT_NOSPAWN             'n'
#define OPT_SHOW_INFO           'p'
#define OPT_QUERY               'q'
#define OPT_NO_DEFAULTS         'r'
#define OPT_NO_SHOW             's'
#define OPT_TOUCH               't'
#define OPT_NEEDNEWER           'y'

/* Sign on message. */
#define MSG_APPNAME     "DXMake"
# define MSG_SIGNON     "\
DXMake - Script-driven build utility - version 1.5\n"
#define MSG_COPYRIGHT   "\
(C) Copyright 1985-1988, 1990, 1992 Ammon R. Campbell.  This package\n\
may be distributed freely for non-commerical use according to the\n\
terms in the accompanying documentation.  Commerical use of this\n\
software is prohibited.  All other rights reserved.\n"

/* Dialog/control/menu messages. */
#define MSG_STARTUPTITLE        "DXMake"
#define MSG_OPTIONSTITLE        "DXMake Options"
#define MSG_MABOUT              "About..."
#define MSG_ABOUTTITLE          "About"
#define MSG_VERSIONSTRING       "Version 1.5"
#define MSG_ABOUTCOPYRIGHT      "\
Copyright 1985-1988, 1990, 1992 Ammon R. Campbell."
#define MSG_ABOUTCOPYRIGHT2     "\
This package \
may be distributed freely for non-commerical use according to the \
terms in the accompanying documentation.  Commerical use of this \
software is prohibited.  All other rights reserved."
#define MSG_BOK                 "OK"
#define MSG_BCANCEL             "Cancel"
#define MSG_BHELP               "&Help..."
#define MSG_BOPTIONS            "&Options..."
#define MSG_SRCDIR              "&Source Directory:"
#define MSG_MAKEFILENAME        "&Makefile:"
#define MSG_TARGETNAMES         "&Target(s):"
#define MSG_OPTA                "[-&a]  Build targets even if not out of date"
#define MSG_OPTC                "[-&c]  Display sign-on message"
#define MSG_OPTD                "[-&d]  Enable debug output"
#define MSG_OPTE                "\
[-&e]  Override macros with environment strings"
#define MSG_OPTI                "[-&i]  Ignore exit codes of commands"
#define MSG_OPTN                "\
[-&n]  Display commands without executing them"
#define MSG_OPTP                "\
[-&p]  Display macros, rules, and targets found in makefile"
#define MSG_OPTQ                "\
[-&q]  Query:  return 0 if targets are up-to-date, nonzero otherwise"
#define MSG_OPTR                "\
[-&r]  Don't use the default rules and macros in 'make.inf'"
#define MSG_OPTS                "\
[-&s]  Suppress display of commands that are executed"
#define MSG_OPTT                "\
[-&t]  Touch all out-of-date targets without building them"
#define MSG_OPTY                "\
[-&y]  Targets must be newer than dependents to be up-to-date"

/* Messages for usage(). */
#define MSG_USAGE       "\n\
usage:  make [-fMakefile] [-wDirectory] [options] [macro=text...] [target...]\n\
\noptions:\n\
   -a   Build targets even if not out of date.\n\
   -c   Display name/version message.\n\
   -d   Enable debug output.\n\
   -e   Override macros with environment strings.\n\
   -i   Ignore exit codes of commands.\n\
   -n   Display commands without executing them.\n\
   -p   Display macros, rules, and targets.\n\
   -q   Query:  return 0 if targets are up-to-date, nonzero otherwise.\n\
   -r   Don't use the default rules and macros in 'make.inf'\n\
   -s   Suppress display of commands that are executed.\n\
   -t   Touch all out-of-date targets without building them.\n\
   -y   Targets must be newer than dependents to be up-to-date.\n"

/* Status messages. */
#define MSG_UPTODATE            "make:  Target already up to date:  "
#define MSG_TARGETNOTEXIST      "make:  Warning, target does not exist:  "
#define MSG_TOOLHELPWARN        "\
make:  Warning, TOOLHELP.DLL not installed; can't check exit codes\n"
#define MSG_PRESSAKEY           "make:  Press a key to continue\n"
#define MSG_ENVTOOLARGE         "\
make:  Warning, environment length exceeds buffer size; truncating\n"

/* Error messages. */
#define MSG_ERRMSG              "make:  "
#define MSG_ERRSTOP             "Stop.\n"
#ifdef WIN
# define MSG_ERR_REGCLASS       "Error registering window class"
# define MSG_ERR_CREATEWIN      "Error creating window"
# define MSG_ERR_CREATEDLG      "Error creating dialog box"
# define MSG_ERR_MODULENAME     "Error retrieving module handle"
# define MSG_ERR_NOHELP         "Error accessing on-line help"
#endif /* WIN */
#define MSG_ERR_CHDIR           "Error changing current drive/directory"
#define MSG_ERR_MEMINIT         "Error initializing memory handler"
#define MSG_ERR_ENVSYNTAX       "Error in environment string"
#define MSG_ERR_FNOMAKEFILE     "'-f' requires filename"
#define MSG_ERR_FNODIRNAME      "'-w' requires directory name"
#define MSG_ERR_BADOPTION       "Unrecognized option"
#define MSG_ERR_NOTARGETS       "No targets defined in makefile"
#define MSG_ERR_BADSUFFIX       "Invalid suffix in suffix list"
#define MSG_ERR_IOREAD          "Read failure in input file"
#define MSG_ERR_NOMACRO         "Macro not defined"
#define MSG_ERR_NORPAREN        "Missing right parenthesis ')' "
#define MSG_ERR_UNREADFULL      "Fatal scanner error - multiple unreads"
#define MSG_ERR_UNREADLEN       "Fatal scanner error - unread data too long"
#define MSG_ERR_NOWILDDEP       "Wildcard dependent has no match"
#define MSG_ERR_PATHTOOLONG     "Filespec exceeds maximum length"
#define MSG_ERR_SYMTOOLONG      "Token for macro name exceeds maximum length"
#define MSG_ERR_LINETOOLONG     "Input line exceeds maximum length"
#define MSG_ERR_CMDMACROLEN     "\
Macro expansion causes command line to exceed maximum length"
#define MSG_ERR_LTNORULE        "'$<' can only be used in inference rules"
#define MSG_ERR_NOBANG          "'!' not supported"
#define MSG_ERR_CANTEXEC        "Exec failed"
#define MSG_ERR_RETCODE         "Non-zero return code"
#define MSG_ERR_MAXARGS         "\
Maximum number of arguments exceeded in spawned command"
#define MSG_ERR_FACCESS         "Error accessing file"
#define MSG_ERR_CANTMAKE        "Don't know how to make"
#define MSG_ERR_EMPTYRULE       "No commands specified in rule"
#define MSG_ERR_SUFFIXTOOLONG   "Suffix too long"
#define MSG_ERR_BADPSUEDO       "Unrecognized psuedo-target"
#define MSG_ERR_CANTOPEN        "Can't open file"
#define MSG_ERR_SYNTAX          "Syntax error"
#define MSG_ERR_MACROSYNTAX     "Syntax error in macro definition"
#define MSG_ERR_EOF             "Unexpected end of input file"
#define MSG_ERR_RULESYNTAX      "Syntax error in rule definition"
#define MSG_ERR_TARGETSYNTAX    "Syntax error in target definition"
#define MSG_ERR_SAMETARGET      "Target defined more than once"
#define MSG_ERR_CMDTOOLONG      "Command line exceeds maximum allowable length"
#define MSG_ERR_EXPTOOLONG      "\
Macro expansion causes string to exceed maximum allowable length"
#define MSG_ERR_OUTOFMEMORY     "Out of memory"
#define MSG_ERR_BANGUNEXP       "Unexpected directive"
#define MSG_ERR_TOOMANYIFS      "!IFs nested too deeply"

/* Debug mode messages. */
#define MSG_DBG_ENABLED         "debug:  Debugging output enabled.\n"
#define MSG_DBG_MAKENAME        "debug:  Processing make input:  "
#define MSG_DBG_CHKDEPS         "debug:  Checking dependents for:  "
#define MSG_DBG_DEPNAME         "debug:    Dependent:  "
#define MSG_DBG_WANTTOMAKE      "debug:  Want to make:  "
#define MSG_DBG_UTEXISTS        "debug:  Undescribed target exists:  "
#define MSG_DBG_POSSIBLERULE    "debug:    Possible rule:  "
#define MSG_DBG_HAVEMATCH       "debug:    Matching file:  "
#define MSG_DBG_SHOWUPTODATE    "debug:  Target is up to date with dependent:  "
#define MSG_DBG_ISUPTODATE      "debug:  Target is up to date:  "
#define MSG_DBG_BUILDING        "debug:  Building:  "
#define MSG_DBG_ASSUMEDUMMY     "debug:  Assuming target is dummy:  "

/* Info mode messages. */
#define MSG_INFO_ENABLED        "info:  Table information output enabled.\n"
#define MSG_INFO_CMDMACRO       "info:  Macro defined on command line: "
#define MSG_INFO_CMDTARGETS     "info:  Target listed on command line: "
#define MSG_INFO_ASSUMETARGET   "\
info:  No targets on command line; assuming "
#define MSG_INFO_SUFFIXES       "info:  SUFFIXES:\n"
#define MSG_INFO_NOSUFFIXES     "info:    <none>\n"
#define MSG_INFO_SHOWSUFFIX     "info:    "
#define MSG_INFO_PRECIOUS       "info:  PRECIOUS filenames:\n"
#define MSG_INFO_NOPRECIOUS     "info:    <none>\n"
#define MSG_INFO_SHOWPRECIOUS   "info:    "
#define MSG_INFO_NOMACROS       "info:  Macro list is empty.\n"
#define MSG_INFO_MACRONAME      "info:  Macro:  "
#define MSG_INFO_MACRODATA      "info:        = "
#define MSG_INFO_NORULES        "info:  Rule list is empty.\n"
#define MSG_INFO_RULENAME       "info:  Rule:  "
#define MSG_INFO_RULECMD        "info:      "
#define MSG_INFO_NOTARGETS      "info:  Target list is empty.\n"
#define MSG_INFO_TARGETNAME     "info:  Target:  "
#define MSG_INFO_TDEPHDR        "info:     Dependents:\n"
#define MSG_INFO_TDEPNAME       "info:        "
#define MSG_INFO_TCMDHDR        "info:     Commands:\n"
#define MSG_INFO_TCMDNAME       "info:        "

