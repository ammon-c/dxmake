/*
======================================================================
makesuf.c
Suffixes list handling routines for make utility.

This source file is part of a computer program that is
(C) Copyright 1988 Ammon R. Campbell.  All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES

The make utility looks up rules based on the contents of the
suffixes list.  By placing a ".SUFFIXES" psuedo-target in the
makefile, the user can specify which filename suffixes and in
what order the suffixes should be looked at when looking up a
rule for building targets.

The ".SUFFIXES" psuedo-target is followed by a colon, and then
optionally by a list of the filename suffixes to be placed in
the suffixes list.

For example:  ".SUFFIXES: .obj .c"

If no filename suffixes follow the colon, the current suffixes
list is flushed.

The suffixes list is implemented as a linked list of lines, of the
data type LINE, which is defined in the "make.h" file, where each
line contains one filename suffix.

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

/****************************** VARIABLES ***************************/

/* suffixes_list:  Linked list of suffix (file extension) names. */
static LINE *suffixes_list;

/****************************** FUNCTIONS ***************************/

/*
** init_suffixes:
** Initializes the suffixes list.  This function gets
** called before any other actions are performed on
** the suffixes list.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
void
init_suffixes(void)
{
	suffixes_list = (LINE *)NULL;
}

/*
** flush_suffixes:
** Flushes the contents of the suffixes list.
** This function gets called before the program terminates
** to free up memory used by the suffixes list, and each
** time the suffixes list is emptied by an empty ".SUFFIXES"
** list in the makefile.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
void
flush_suffixes(void)
{
	free_lines(suffixes_list);
	suffixes_list = (LINE *)NULL;
}

/*
** do_suffixes:
** Parses a ".SUFFIXES" psuedo-target line from the makefile.
** Any suffixes listed are added to the suffixes list.  If no
** suffixes are listed in the line from the makefile, the
** suffixes list will be flushed.
**
** Parameters:
**	Name	Description
**	----	-----------
**	line	Line from makefile containing ".SUFFIXES"
**		statement to parse.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Successful.
**	0	Error occurred.
*/
int
do_suffixes(line)
	char	*line;
{
	int	i = 0;
	int	j;
	int	num_suffixes;
	char	suffix[MAX_SUFFIX_STR + 2];
	LINE	*tmp;

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

	if (strncmp(line, ".SUFFIXES", 9) != 0)
	{
		/*
		** The line does not contain a ".SUFFIXES"
		** statement.  This should never happen
		** because the input dispatcher only calls
		** us if ".SUFFIXES" is encountered.
		*/
		return 0;
	}

	/* Extract each suffix from input line. */
	num_suffixes = 0;
	while (line[i] != '\0')
	{
		/* Extract next suffix from input line. */
		j = 0;
		while (line[i] != ' ' && line[i] != '\t' &&
			line[i] != '\0' && j < MAX_SUFFIX_STR + 1)
		{
			suffix[j++] = line[i++];
		}
		suffix[j] = '\0';
		if (line[i] != ' ' && line[i] != '\t' &&
			line[i] != '\0')
		{
			/* Error in ".SUFFIXES:" */
			errmsg(MSG_ERR_BADSUFFIX, line, NOVAL);
			return 0;
		}

		/* Add the suffix to the suffixes list. */
		if (suffix[0] == '.')
			tmp = append_line(suffixes_list, &suffix[1]);
		else
			tmp = append_line(suffixes_list, suffix);
		if (tmp == (LINE *)NULL)
		{
			errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
			return 0;
		}
		suffixes_list = tmp;
		num_suffixes++;

		/* Skip whitespace before next suffix. */
		while (line[i] == ' ' || line[i] == '\t')
			i++;
	}

	/*
	** If no suffixes were specified, then the suffix list
	** should be erased.
	*/
	if (num_suffixes < 1)
	{
		/* Erase the suffixes list. */
		flush_suffixes();
	}

	return 1;
}

/*
** enum_suffix:
** Enumerates a filename suffix from the suffixes list.
**
** Parameters:
**	Name	Description
**	----	-----------
**	index	Index in list of suffix to enumerate.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	NULL	Specified index is out of range.
**	other	Pointer to null terminated character
**		string containing filename suffix.
*/
char *
enum_suffix(index)
	int	index;
{
	LINE	*lptr;
	int	i = 0;

	/* Start at head of suffixes list. */
	lptr = suffixes_list;

	/*
	** Step through suffixes until we get to the one
	** we want or run out of suffixes.
	*/
	while (i < index && lptr != (LINE *)NULL)
	{
		lptr = lptr->lnext;
		i++;
	}

	if (i != index || lptr == (LINE *)NULL)
	{
		/* Index out of range. */
		return (char *)NULL;
	}

	/* Return pointer to suffix name. */
	return (char *)lptr->ldata;
}

/*
** dump_suffixes:
** Outputs the contents of the suffix list.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
void
dump_suffixes(void)
{
	LINE	*lptr;

	mputs(MSG_INFO_SUFFIXES);
	lptr = suffixes_list;
	if (lptr == (LINE *)NULL)
	{
		mputs(MSG_INFO_NOSUFFIXES);
		return;
	}
	while (lptr != (LINE *)NULL)
	{
		mputs(MSG_INFO_SHOWSUFFIX);
		mputs(lptr->ldata);
		mputs("\n");
		lptr = lptr->lnext;
	}
}

