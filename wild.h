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

/*
** w_findfirst:
** Finds the first matching file for the specified filespec.
**
** Parameters:
**	Name		Description
**	----		-----------
**	filespec	Filespec to match.
**	findbfr		Pointer to 'find_t' structure as defined
**			in Microsoft C "dos.h" file.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	0	Match found; data returned in findbfr.
**	1	No match.
*/
int	w_findfirst(char *filespec, struct find_t *findbfr);

/*
** w_findnext:
** Finds the next matching file for the findbfr from a previous call
** to w_findfirst().
**
** Parameters:
**	Name		Description
**	----		-----------
**	filespec	Filespec to match.
**	findbfr		Pointer to 'find_t' structure as defined
**			in Microsoft C "dos.h" file, and processed
**			by an initial call to w_findfirst().
**
** Returns:
**	Value	Meaning
**	-----	-------
**	0	Match found; data returned in findbfr.
**	1	No match.
*/
int	w_findnext(char *filespec, struct find_t *findbfr);

