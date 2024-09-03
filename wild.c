/*
=========================================================================
wild.c
Routines for handling wildcard filenames for MS-DOS.

The external routines getpath() and check_rexp() are used.

This source file is part of a computer program that is
Copyright 1988 by Ammon R. Campbell.  All rights reserved.
Unauthorized duplication, distribution, or use is prohibited
by applicable laws.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
=========================================================================
*/

/****************************** INCLUDES ******************************/

#include <dos.h>
#include <string.h>

#include "getpath.h"
#include "fnexp.h"
#include "wild.h"

/****************************** FUNCTIONS *****************************/

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
int
w_findfirst(filespec, findbfr)
	char	*filespec;
	struct find_t *findbfr;
{
	int	result;
	char	str[128];

	strcpy(str, filespec);
	getpath(str);
	strcat(str, "*.*");
	if ((result = _dos_findfirst(str,
			_A_NORMAL | _A_ARCH | _A_SUBDIR |
			_A_HIDDEN | _A_SYSTEM | _A_VOLID | _A_RDONLY,
			findbfr)))
	{
		/* No match. */
		return 1;
	}

	while (!result)
	{
		/* Build filename. */
		strcpy(str, filespec);
		getpath(str);
		strcat(str, findbfr->name);
		strlwr(str);

		/* See if this file matches wildcard expression. */
		if (check_rexp(str, filespec) == 1)
		{
			/* Found a match. */
			return 0;
		}

		/* Get next file. */
		result = _dos_findnext(findbfr);
	}

	/* No match. */
	return 1;
}

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
int
w_findnext(filespec, findbfr)
	char		*filespec;
	struct find_t	*findbfr;
{
	int	result;
	char	str[128];

	if ((result = _dos_findnext(findbfr)))
	{
		/* No match. */
		return 1;
	}

	while (!result)
	{
		/* Build filename. */
		strcpy(str, filespec);
		getpath(str);
		strcat(str, findbfr->name);
		strlwr(str);

		/* See if this file matches wildcard expression. */
		if (check_rexp(str, filespec) == 1)
		{
			/* Found a match. */
			return 0;
		}

		/* Get next file. */
		result = _dos_findnext(findbfr);
	}

	return 1;
}

