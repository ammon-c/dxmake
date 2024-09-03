/*
======================================================================
touchmsg.h
Message strings include file for touch utility

This source file is part of a computer program that is
(C) Copyright 1985-1988, 1990, 1992 Ammon R. Campbell.
All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES

This include file contains definitions for all of the text messages
that are output by the touch utility, including the sign-on text,
error messages, status messages, and debug messages.

======================================================================
*/

/* Option switch characters. */
#define OPT_NOCREATE            'c'
#define OPT_DATE                'd'
#define OPT_TIME                't'
#define OPT_WORKINGDIR          'w'

/* Dialog/control/menu messages. */
#define MSG_APPNAME             "DXTouch"
#define MSG_STARTUPTITLE        "DXTouch"
#define MSG_OPTIONSTITLE        "DXTouch Options"
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
#define MSG_SRCDIR              "&Working Directory:"
#define MSG_FILENAMES           "&Filename(s):"
#define MSG_NOCREAT             "[-&c]  Don't force creation of missing files"
#define MSG_DATE                "[-&d]  Date:"
#define MSG_TIME                "[-&t]  Time:"

/* Messages for usage(). */
# define MSG_SIGNON     "\
DXTouch - Timestamp update utility - version 1.5\n"
#define MSG_COPYRIGHT   "\
(C) Copyright 1985-1988, 1990, 1992 Ammon R. Campbell.  This package\n\
may be distributed freely for non-commerical use according to the\n\
terms in the accompanying documentation.  Commerical use of this\n\
software is prohibited.  All other rights reserved.\n"
#define MSG_USAGE       "\n\
usage:  touch [options] file ...\n\
\noptions:\n\
   -c       Disable file creation.\n\
   -dDATE   Specifies date to use instead of current date, where\n\
            DATE specifies the date in the format MMDDYY.\n\
   -tTIME   Specifies time to use instead of current time, where\n\
            TIME specifies the time in 24-hour HHMM format.\n\
   -wDIR    Specifies that working directory should be changed to\n\
            DIR instead of default directory.\n\
"

/* Status messages. */
#define MSG_WARN_DIRECTORY      "Ignoring directory"
#define MSG_PRESSAKEY           "touch:  Press a key to continue\n"

/* Error messages. */
#define MSG_ERRMSG              "touch:  "
#ifdef WIN
# define MSG_ERR_REGCLASS       "Error registering window class"
# define MSG_ERR_CREATEWIN      "Error creating window"
# define MSG_ERR_CREATEDLG      "Error creating dialog box"
# define MSG_ERR_MODULENAME     "Error retrieving module handle"
# define MSG_ERR_NOHELP         "Error accessing on-line help"
#endif /* WIN */
#define MSG_ERR_CHDIR           "Error changing current drive/directory"
#define MSG_ERR_CANTCREAT       "Can't create"
#define MSG_ERR_CANTACCESS      "Can't access"
#define MSG_ERR_CANTTOUCH       "Can't touch"
#define MSG_ERR_BADTIME         "Invalid time"
#define MSG_ERR_BADDATE         "Invalid date"
#define MSG_ERR_BADOPTION       "Unrecognized option"
#define MSG_ERR_NOFILES         "No files specified"
#define MSG_ERR_NODIR           "'-w' requires directory name"
#define MSG_ERR_MAXARGS         "\
Maximum number of arguments exceeded in spawned command"

