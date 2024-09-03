/*
======================================================================
maketar.c
Target list handling routines for make utility.

This source file is part of a computer program that is
(C) Copyright 1988 Ammon R. Campbell.  All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES

The routines in this module handle the parsing of target
description data from the makefile into the target list used by
the make utility.  Only the parsing and list management is done
here; the actual testing and building of targets is done
elsewhere.

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

/* Prototypes of functions local to this module: */
#ifdef BT_TARGETS
void	free_target_tree(TARGET *t);
#endif /* BT_TARGETS */
static TARGET	*dup_target(TARGET *tar, char *tname);
static int	add_target(TARGET *tar);

/****************************** CONSTANTS ***************************/

	/* See "make.h" file. */

/****************************** VARIABLES ***************************/

/* target_list:  Linked list of defined targets. */
static TARGET *target_list;

	/* Also see "make.h" file. */

/*************************** LOCAL FUNCTIONS ************************/

#ifdef BT_TARGETS
/*
** free_target_tree:
** Frees a binary tree of target nodes.
**
** Parameters:
**	Name	Description
**	----	-----------
**	t	Pointer to tree to be freed.
**
** Returns:
**	NONE
*/
void
free_target_tree(t)
	TARGET	*t;
{
	/* Check if this is the end of a branch. */
	if (t == (TARGET *)NULL)
		return;

	/* Free children first. */
	free_target_tree(t->tleft);
	free_target_tree(t->tright);

	/* Free strings. */
	mem_free(t->tname);
	if (t->tdependents != (char *)NULL)
		mem_free(t->tdependents);
	if (t->tcommands != (LINE *)NULL)
		free_lines(t->tcommands);

	/* Free the target descriptor structure. */
	mem_free(t);
} /* End free_target_tree() */
#endif /* BT_TARGETS */

/*
** add_target:
** Adds a target descriptor to the targets list.
**
** Parameters:
**	Name	Description
**	----	-----------
**	tar	Pointer to target descriptor to be added.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Successful.
**	0	Error occurred (i.e. duplicate target name).
*/
int
add_target(tar)
	TARGET	*tar;
{
	TARGET	*tptr;
	int	comparison;

	if (target_list == (TARGET *)NULL)
	{
		/* This is the first entry in the targets list. */
		target_list = tar;
#ifdef BT_TARGETS
		target_list->tleft = (TARGET *)NULL;
		target_list->tright = (TARGET *)NULL;
#else
		target_list->tnext = (TARGET *)NULL;
#endif /* BT_TARGETS */
	}
	else
	{
#ifdef BT_TARGETS
		/*
		** This is not the first entry; find the appropriate
		** leaf on the tree to add the new entry.
		*/
		tptr = target_list;
		while (1)
		{
			comparison = strcmp(tar->tname, tptr->tname);
			if (comparison == 0)
			{
				/* Same target is already defined. */
				errmsg(MSG_ERR_SAMETARGET, tar->tname, NOVAL);
				return 0;
			}
			else if (comparison < 0)
			{
				/* Check if left branch is empty. */
				if (tptr->tleft == (TARGET *)NULL)
				{
					tptr->tleft = tar;
					tptr->tleft->tleft = (TARGET *)NULL;
					tptr->tleft->tright = (TARGET *)NULL;
					break;
				}

				/* Move down left side of tree. */
				tptr = tptr->tleft;
			}
			else /* comparison > 0 */
			{
				/* Check if right branch is empty. */
				if (tptr->tright == (TARGET *)NULL)
				{
					tptr->tright = tar;
					tptr->tright->tleft = (TARGET *)NULL;
					tptr->tright->tright = (TARGET *)NULL;
					break;
				}

				/* Move down right side of tree. */
				tptr = tptr->tright;
			}
		}
#else
		/* This is not the first entry; insert at the end. */
		tptr = target_list;
		while (tptr->tnext != (TARGET *)NULL)
		{
			if (strcmp(tptr->tname, tar->tname) == 0)
			{
				/* Same target is already defined. */
				errmsg(MSG_ERR_SAMETARGET, tar->tname, NOVAL);
				return 0;
			}
			tptr = tptr->tnext;
		}
		tptr->tnext = tar;
		tptr->tnext->tnext = (TARGET *)NULL;
#endif /* BT_TARGETS */
	}

	return 1;
} /* End add_target() */

/*
** dup_target:
** Makes a duplicate of an existing target description,
** but with a different target name.
**
** Parameters:
**	Name	Description
**	----	-----------
**	tar	Pointer to target descriptor to duplicate.
**	tname	Name to use for new target descriptor.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	NULL	Error occurred.
**	other	Pointer to new target descriptor.
*/
static TARGET *
dup_target(tar, tname)
	TARGET	*tar;
	char	*tname;
{
	TARGET	*newtar;

	/* Allocate memory for target descriptor. */
	newtar = (TARGET *)mem_alloc(sizeof(TARGET));
	if (newtar == (TARGET *)NULL)
	{
		/* Out of memory for target definition. */
		errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
		return (TARGET *)NULL;
	}
#ifdef BT_TARGETS
	newtar->tleft = (TARGET *)NULL;
	newtar->tright = (TARGET *)NULL;
#else
	newtar->tnext = (TARGET *)NULL;
#endif /* BT_TARGETS */
	newtar->tdependents = (char *)NULL;
	newtar->tcommands = (LINE *)NULL;

	/* Allocate memory for target name. */
	newtar->tname = (char *)mem_alloc(strlen(tname) + 1);
	if (newtar->tname == (char *)NULL)
	{
		/* Out of memory for target name. */
		errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
		mem_free(newtar);
		return (TARGET *)NULL;
	}

	/* Save target name in descriptor. */
	strcpy(newtar->tname, tname);

	/* Copy dependents info. */
	/* Allocate memory for dependent line (if any). */
	if (tar->tdependents != (char *)NULL)
	{
		newtar->tdependents =
			mem_alloc(strlen(tar->tdependents) + 1);
		if (newtar->tdependents == (char *)NULL)
		{
			/* Out of memory for dependent list. */
			errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
			mem_free(newtar->tname);
			mem_free(newtar);
			return (TARGET *)NULL;
		}
		strcpy(newtar->tdependents, tar->tdependents);
	}

	/* Copy command info. */
	if (tar->tcommands != (LINE *)NULL)
	{
		newtar->tcommands = dup_lines(tar->tcommands);
		if (newtar->tcommands == (LINE *)NULL)
		{
			/* Out of memory for dependent list. */
			errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
			mem_free(newtar->tname);
			mem_free(newtar->tdependents);
			mem_free(newtar);
			return (TARGET *)NULL;
		}
	}

	/* Success! */
	return newtar;
} /* End dup_target() */

/****************************** FUNCTIONS ***************************/

/*
** init_targets:
** Initializes the target list.  This function must be called
** prior to using any of the other target list functions in
** this module.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
void
init_targets(void)
{
	target_list = (TARGET *)NULL;
} /* End init_targets() */

/*
** default_target:
** Returns the filename of the default target file.
**
** Parameters:
**	NONE
**
** Returns:
**	Value	Meaning
**	-----	-------
**	NULL	There are no targets defined.
**	other	Pointer to character string
**		containing target filename.
*/
char *
default_target(void)
{
	if (target_list == (TARGET *)NULL)
		return (char *)NULL;

	return target_list->tname;
} /* End default_target() */

/*
** find_target:
** Finds the target descriptor for a particular target.
**
** Parameters:
**	Name	Description
**	----	-----------
**	tname	Name of target to find.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	NULL	Specified target not found.
**	other	Pointer to target descriptor.
*/
TARGET *
find_target(tname)
	char	*tname;
{
	TARGET	*tar;		/* Pointer to target's descriptor. */
	int	comparison;	/* Result of target name comparison. */

	/* Find target descriptor for specified target (if any). */
	tar = target_list;
#ifdef BT_TARGETS
	while (tar != (TARGET *)NULL)
	{
		comparison = strcmp(tname, tar->tname);
		if (comparison == 0)
		{
			/* Found it. */
			return tar;
		}
		else if (comparison < 0)
		{
			/* Move down left side of tree. */
			tar = tar->tleft;
		}
		else /* comparison > 0 */
		{
			/* Move down right side of tree. */
			tar = tar->tright;
		}
	}
#else
	while (tar != (TARGET *)NULL)
	{
		/* Is this descriptor for the target we want? */
		if (strcmp(tname, tar->tname) == 0)
		{
			/* Found the right descriptor. */
			return tar;
		}

		tar = tar->tnext;
	}
#endif /* BT_TARGETS */

	return (TARGET *)NULL;
} /* End find_target() */

/*
** flush_targets:
** Empties and frees the target list.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
void
flush_targets(void)
{
#ifdef BT_TARGETS
	free_target_tree(target_list);
#else
	TARGET	*t;	/* Temporary target descriptor pointers. */
	TARGET	*t2;

	/* Free each entry in the target list. */
	t = target_list;
	while (t != (TARGET *)NULL)
	{
		/* Free target name. */
		mem_free(t->tname);

		/* Free target dependent files. */
		if (t->tdependents != (char *)NULL)
			mem_free(t->tdependents);

		/* Free target command lines. */
		if (t->tcommands != (LINE *)NULL)
			free_lines(t->tcommands);

		/* Free target descriptor. */
		t2 = t->tnext;
		mem_free(t);
		t = t2;
	}
#endif

	/* Mark target list as empty. */
	target_list = (TARGET *)NULL;
} /* End flush_targets() */

/*
** define_target:
** Parses a target description block from the makefile, adding the
** target description information to the internal target list.
**
** Parameters:
**	Name	Description
**	----	-----------
**	line	First line of target definition.
**	handle	File handle to read if target definition is more
**		than one line long.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Successful.
**	0	Error occurred (syntax error, out of memory, I/O error, etc).
*/
int
define_target(line, handle)
	char	*line;
	int	handle;
{
	int	pos;
	int	tpos;
	char	tname[MAXPATH];	/* Name of target. */
	TARGET	*tar;		/* Temporary target descriptor pointer. */
	TARGET	*tar2;		/* Temporary target descriptor pointer. */
	LINE	*nptr;		/* Temporary line descriptor pointer. */
	int	didcmds = 0;	/* Flag, nonzero after first command read. */
	LINE	*names;		/* Temporary list of target names. */
	int	result;		/* Result of last read. */

	/* Get target name(s) from input line. */
	names = (LINE *)NULL;
	pos = 0;
	while (line[pos] != '\0' &&
		!(line[pos] == ':' && line[pos + 1] != '\\'))
	{
		/* Get next target name from line. */
		tpos = 0;
		while (line[pos] != '\0' &&
			line[pos] != ' ' &&
			line[pos] != '\t' &&
			!(line[pos] == ':' && line[pos + 1] != '\\'))
		{
			/* Check for overflow. */
			if (pos >= MAXPATH)
			{
				/* Target name too long. */
				errmsg(MSG_ERR_PATHTOOLONG, line, NOVAL);
				return 0;
			}

			/* Copy character for target name. */
			tname[tpos++] = line[pos++];
		}
		tname[tpos] = '\0';

		/* Add target name to temporary list of target names. */
		nptr = append_line(names, tname);
		if (nptr == (LINE *)NULL)
		{
			/* Out of memory for dependent list. */
			errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
			free_lines(names);
			return 0;
		}
		names = nptr;

		/* Skip any whitespace after target name. */
		while (line[pos] == ' ' ||
			line[pos] == '\t')
			pos++;
	}

	/* Make sure at least one target name was given. */
	if (names == (LINE *)NULL)
	{
		/* No targets given; syntax error. */
		errmsg(MSG_ERR_TARGETSYNTAX, line, NOVAL);
		return 0;
	}

	/* Check for ':'. */
	if (line[pos] != ':')
	{
		/* Syntax error in target definition. */
		errmsg(MSG_ERR_TARGETSYNTAX, line, NOVAL);
		free_lines(names);
		return 0;
	}

	/* Skip ':'. */
	pos++;

	/* Skip any whitespace between ':' and dependents list. */
	while (line[pos] == ' ' ||
		line[pos] == '\t')
		pos++;

	/*
	** Build new target descriptor.
	*/

	/* Allocate memory for target descriptor. */
	tar = (TARGET *)mem_alloc(sizeof(TARGET));
	if (tar == (TARGET *)NULL)
	{
		/* Out of memory for target definition. */
		errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
		free_lines(names);
		return 0;
	}
#ifdef BT_TARGETS
	tar->tleft = (TARGET *)NULL;
	tar->tright = (TARGET *)NULL;
#else
	tar->tnext = (TARGET *)NULL;
#endif /* BT_TARGETS */
	tar->tdependents = (char *)NULL;
	tar->tcommands = (LINE *)NULL;

	/* Allocate memory for target name. */
	tar->tname = (char *)mem_alloc(strlen(names->ldata) + 1);
	if (tar->tname == (char *)NULL)
	{
		/* Out of memory for target name. */
		errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
		mem_free(tar);
		free_lines(names);
		return 0;
	}

	/* Save target name in descriptor. */
	strcpy(tar->tname, names->ldata);

	/* Allocate memory for dependent line (if any). */
	if (line[pos] != '\0')
	{
		tar->tdependents = (char *)mem_alloc(strlen(&line[pos]) + 1);
		if (tar->tdependents == (char *)NULL)
		{
			/* Out of memory for dependent list. */
			errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
			mem_free(tar->tname);
			mem_free(tar);
			free_lines(names);
			return 0;
		}
		strcpy(tar->tdependents, &line[pos]);
	}

	/* Add new target descriptor to target list. */
	if (!add_target(tar))
	{
		/* Error adding target to list. */
		return 0;
	}

	/*
	** Build target's command list.
	*/
	while (1)
	{
		result = read_logical_line(handle, inpline, MAXLLINE);
		if (result == -1)
		{
			/* Fatal read error. */
			return 0;
		}
		if (result < 1)
		{
			if (didcmds)
			{
				/*
				** End of makefile after commands.
				** This is not an error.  Make used
				** to barf if there weren't at least
				** two lines after the commands.
				** This is the hack to fix it.
				*/
				break;
			}

			/* Unexpected end-of-file. */
			errmsg(MSG_ERR_EOF, (char *)NULL, NOVAL);
			free_lines(names);
			return 0;
		}

		if (inpline[0] == '#')
		{
			/*
			** The input line contains a comment, so go back
			** and try again.
			*/
			continue;
		}

		didcmds = 1;

		/*
		** Check for end of target's commands.  A line that
		** is not indented is not part of the target's commands.
		*/
		if (inpline[0] != ' ' && inpline[0] != '\t')
		{
			/* End of target commands definition. */
			unread_logical_line(inpline);
			break;
		}

		/* Find first non-whitespace character. */
		pos = 0;
		while (inpline[pos] == ' ' || inpline[pos] == '\t')
			pos++;

		/* Add line to target commands. */
		nptr = append_line(tar->tcommands, &inpline[pos]);
		if (nptr == (LINE *)NULL)
		{
			/* Out of memory for target commands. */
			errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
			free_lines(names);
			free_lines(tar->tcommands);
			return 0;
		}
		tar->tcommands = nptr;
	} /* End while(1) */

	/*
	** Duplicate target description for each target that was named
	** on this line.
	*/
	nptr = names->lnext;
	while (nptr != (LINE *)NULL)
	{
		/* Duplicate descriptor info. */
		tar2 = dup_target(tar, nptr->ldata);
		if (tar2 == (TARGET *)NULL)
		{
			/* Error duplicating target. */
			free_lines(names);
			return 0;
		}

		/* Add new target descriptor to target list. */
		if (!add_target(tar2))
		{
			/* Error adding target to list. */
			mem_free(tar2->tname);
			mem_free(tar2->tdependents);
			free_lines(tar2->tcommands);
			mem_free(tar2);
			free_lines(names);
			return 0;
		}

		/* Step to next name in list. */
		nptr = nptr->lnext;
	}

	/* Success! */
	free_lines(names);
	return 1;
} /* End define_target() */

/*
** dump_targets:
** Outputs the target list.  This function is used for
** debugging.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
void
dump_targets(void)
{
	LINE	*lptr;	/* Temporary line pointer. */
	TARGET	*tptr;	/* Temporary target pointer. */

#ifdef BT_TARGETS
	mputs("********** TARGET DUMP NOT IMPLEMENTED *********\n");
#else
	tptr = target_list;
	if (tptr == (TARGET *)NULL)
		mputs(MSG_INFO_NOTARGETS);
	while (tptr != (TARGET *)NULL)
	{
		mputs(MSG_INFO_TARGETNAME);
		mputs(tptr->tname);
		mputs("\n");
		if (tptr->tdependents != (char *)NULL)
		{
			mputs(MSG_INFO_TDEPHDR);
			mputs(MSG_INFO_TDEPNAME);
			mputs(tptr->tdependents);
			mputs("\n");
		}
		lptr = tptr->tcommands;
		if (lptr != (LINE *)NULL)
			mputs(MSG_INFO_TCMDHDR);
		while (lptr != (LINE *)NULL)
		{
			mputs(MSG_INFO_TCMDNAME);
			mputs(lptr->ldata);
			mputs("\n");

			lptr = lptr->lnext;
		}

		tptr = tptr->tnext;
	}
#endif /* BT_TARGETS */
} /* End dump_targets() */

/*
======================================================================
End maketar.c
======================================================================
*/
