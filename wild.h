/*
=========================================================================
wild.h
Include file for "wild.c" and calling applications.
Routines for handling wildcard filenames for MS-DOS.

This source file is part of a computer program that is
Copyright 1988 by Ammon R. Campbell.  All rights reserved.
Unauthorized duplication, distribution, or use is prohibited
by applicable laws.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
=========================================================================
*/

/****************************** HEADERS *******************************/

int	w_findfirst(char *filespec, struct find_t *findbfr);
int	w_findnext(char *filespec, struct find_t *findbfr);

/*
=========================================================================
End wild.h
=========================================================================
*/
