/*
======================================================================
makeprec.c
"Precious filenames" list handling routines for make utility.

This source file is part of a computer program that is
(C) Copyright 1988 Ammon R. Campbell.  All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES

By default, the make utility deletes targetfiles that failed to
build correctly.  By placing a ".PRECIOUS" psuedo-target in the
makefile, the user can specify that certain files are not to be
deleted if they fail to build correctly.  These are called
'precious files'.

The ".PRECIOUS" psuedo-target is followed by a colon, and then
by a list of the filenames to be treated as 'precious files'.

For example:  ".PRECIOUS: myfile.obj myfile.exe"

The precious filenames list is implemented as a linked list of
lines, of the data type LINE, which is defined in the "make.h"
file, where each line contains one filename.

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

#include "make.h"

/****************************** HEADERS *****************************/

	/* See "make.h" */

/****************************** CONSTANTS ***************************/

	/* See "make.h" */

/****************************** VARIABLES ***************************/

/* precious_list:  Linked list of precious filenames. */
static LINE *precious_list;

	/* Also see "make.h" */

/*************************** LOCAL FUNCTIONS ************************/

	/* NONE */

/****************************** FUNCTIONS ***************************/

/*
** init_precious:
** Initializes the precious filenames list.  This function
** gets called before any other actions are performed on
** the precious filenames list.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
void
init_precious(void)
{
	precious_list = (LINE *)NULL;
} /* End init_precious() */

/*
** flush_precious:
** Flushes the contents of the precious filenames list.
** This function gets called before the program terminates
** to free up memory used by the precious filenames list.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
void
flush_precious(void)
{
	free_lines(precious_list);
	precious_list = (LINE *)NULL;
} /* End flush_precious() */

/*
** do_precious:
** Parses ".PRECIOUS" psuedo-target lines from the makefile.
** Any precious filenames are added to the precious filenames
** list.
**
** Parameters:
**	Name	Description
**	----	-----------
**	line	String containing ".PRECIOUS" psuedo-target
**		to be parsed.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Successful.
**	0	Error occurred.
*/
int
do_precious(line)
	char	*line;
{
	int	i = 0;			/* Line position index. */
	int	j;			/* Loop index. */
	int	num_precious;		/* Number of filenames extracted. */
	char	precname[MAXPATH];	/* Precious filename. */
	LINE	*tmp;			/* Temporary line pointer. */

	/* Skip the psuedo-target name. */
	while (line[i] != '\0' &&
		line[i] != ' ' &&
		line[i] != '\t')
	{
		i++;
	}

	/* Skip leading whitespace. */
	while (line[i] == ' ' || line[i] == '\t')
		i++;

	if (strncmp(line, ".PRECIOUS", 9) != 0)
	{
		/*
		** Line isn't a ".PRECIOUS" line.
		** This should never happen, since the primary
		** input dispatching code calls us.
		*/
		return 0;
	}

	/* Extract each precious filename from input line. */
	num_precious = 0;
	while (line[i] != '\0')
	{
		/* Extract next filename from input line. */
		j = 0;
		while (line[i] != ' ' && line[i] != '\t' &&
			line[i] != '\0' && j < MAXPATH - 1)
		{
			precname[j++] = line[i++];
		}
		precname[j] = '\0';
		if (line[i] != ' ' && line[i] != '\t' &&
			line[i] != '\0')
		{
			/* Error in ".PRECIOUS:" */
			errmsg(MSG_ERR_PATHTOOLONG, line, NOVAL);
			return 0;
		}

		/* Add the filename to the precious list. */
		tmp = append_line(precious_list, precname);
		if (tmp == (LINE *)NULL)
		{
			errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
			return 0;
		}
		precious_list = tmp;
		num_precious++;

		/* Skip whitespace before next filename. */
		while (line[i] == ' ' || line[i] == '\t')
			i++;
	} /* End while() */

	/* Success! */
	return 1;
} /* End do_precious() */

/*
** is_precious:
** Determines if the specified filename is in the list
** of precious filenames.
**
** Parameters:
**	Name	Description
**	----	-----------
**	tname	Name of file to check.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Specified file is a 'precious' file.
**	0	Specified file is not a 'precious' file.
*/
int
is_precious(tname)
	char	*tname;
{
	LINE	*lptr;

	lptr = precious_list;
	while (lptr != (LINE *)NULL)
	{
		if (stricmp(lptr->ldata, tname) == 0)
		{
			/* Found a match. */
			return 1;
		}
		lptr = lptr->lnext;
	}

	/* Filename wasn't found in the list. */
	return 0;
} /* End is_precious() */

/*
** dump_precious:
** Outputs the contents of the precious filenames list.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
void
dump_precious(void)
{
	LINE	*lptr;

	mputs(MSG_INFO_PRECIOUS);
	lptr = precious_list;
	if (lptr == (LINE *)NULL)
	{
		mputs(MSG_INFO_NOPRECIOUS);
		return;
	}
	while (lptr != (LINE *)NULL)
	{
		mputs(MSG_INFO_SHOWPRECIOUS);
		mputs(lptr->ldata);
		mputs("\n");

		lptr = lptr->lnext;
	}
} /* End dump_precious() */

/*
======================================================================
End makeprec.c
======================================================================
*/
