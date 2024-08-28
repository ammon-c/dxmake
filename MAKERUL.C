/*
======================================================================
makerul.c
Rule handling routines for make utility.

This source file is part of a computer program that is
(C) Copyright 1988 Ammon R. Campbell.  All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES

This module contains routines for parsing rule definitions from the
makefile into the rule list used internally by the make utility.
Only the parsing and list management is handled here; the matching
of rules to files during the build process is handled elsewhere.

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

/****************************** CONSTANTS ***************************/

	/* See 'make.h' file. */

/****************************** VARIABLES ***************************/

/* rule_list:  Linked list of defined rules. */
static RULE *rule_list;

	/* Also see 'make.h' file. */

/****************************** FUNCTIONS ***************************/

/*
** init_rules:
** Initializes the rules list.  This function must be called
** prior to using any of the other rule list functions in
** this module.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
void
init_rules(void)
{
	rule_list = (RULE *)NULL;
} /* End init_rules() */

/*
** flush_rules:
** Empties and frees the rule list.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
void
flush_rules(void)
{
	RULE	*r;	/* Temporary rule descriptor pointers. */
	RULE	*r2;

	/* Free each entry in the rule list. */
	r = rule_list;
	while (r != (RULE *)NULL)
	{
		/* Free rule source. */
		mem_free(r->rsrc);

		/* Free rule dest. */
		mem_free(r->rdest);

		/* Free rule command lines. */
		free_lines(r->rcommands);

		/* Free rule descriptor. */
		r2 = r->rnext;
		mem_free(r);
		r = r2;
	}

	/* Mark rule list as empty. */
	rule_list = (RULE *)NULL;
} /* End flush_rules() */

/*
** define_rule:
** Adds a rule to the rule list.
**
** Parameters:
**	Name	Description
**	----	-----------
**	line	First line of rule definition.
**	handle	File handle to read for remaining lines of rule.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Successful.
**	0	Error occurred (syntax error, out of memory, I/O error).
*/
int
define_rule(line, handle)
	char	*line;
	int	handle;
{
	int	pos;
	int	itmp;
	int	result;		/* Return code from last read. */
	char	rsrc[MAX_SUFFIX_STR + 2]; /* Source file extension. */
	char	rdest[MAX_SUFFIX_STR + 2];/* Dest file extension. */
	RULE	*rul;		/* Temporary rule descriptor pointer. */
	RULE	*rptr;		/* Temporary rule descriptor pointer. */
	LINE	*ltmp;		/* Temporary line pointer. */

	/* Check for starting dot. */
	pos = 0;
	if (line[pos] != '.')
	{
		/* Syntax error in rule definition. */
		errmsg(MSG_ERR_RULESYNTAX, line, NOVAL);
		return 0;
	}
	pos++;

	/* Get rule source name. */
	itmp = 0;
	while(line[pos] != '\0' &&
		line[pos] != '.' &&
		line[pos] != ':' &&
		line[pos] != ' ' &&
		line[pos] != '\t')
	{
		/* Check for overflow. */
		if (itmp >= MAX_SUFFIX_STR)
		{
			/* Suffix is too long. */
			errmsg(MSG_ERR_SUFFIXTOOLONG, line, NOVAL);
			return 0;
		}

		/* Copy character from input to source suffix name. */
		rsrc[itmp++] = line[pos++];
	}
	rsrc[itmp] = '\0';

	/* Check for second dot. */
	if (line[pos] != '.')
	{
		/* Syntax error in rule definition. */
		errmsg(MSG_ERR_RULESYNTAX, line, NOVAL);
		return 0;
	}
	pos++;

	/* Get rule dest name. */
	itmp = 0;
	while(line[pos] != '\0' &&
		line[pos] != '.' &&
		line[pos] != ':' &&
		line[pos] != ' ' &&
		line[pos] != '\t')
	{
		/* Check for overflow. */
		if (itmp >= MAX_SUFFIX_STR)
		{
			/* Suffix is too long. */
			errmsg(MSG_ERR_SUFFIXTOOLONG, line, NOVAL);
			return 0;
		}

		/* Copy character from input to destination suffix name. */
		rdest[itmp++] = line[pos++];
	}
	rdest[itmp] = '\0';

	/* Skip any whitespace before the colon. */
	while (line[pos] == ' ' || line[pos] == '\t')
		pos++;

	/* Check for colon. */
	if (line[pos] != ':')
	{
		/* Syntax error in rule definition. */
		errmsg(MSG_ERR_RULESYNTAX, line, NOVAL);
		return 0;
	}
	pos++;	/* Skip the colon. */

	/* Make sure both of the suffixes aren't zero length. */
	if (strlen(rdest) < 1 || strlen(rsrc) < 1)
	{
		/* Syntax error in rule definition. */
		errmsg(MSG_ERR_RULESYNTAX, line, NOVAL);
		return 0;
	}

	/*
	** Build new rule descriptor.
	*/

	/* Allocate memory for rule descriptor. */
	rul = (RULE *)mem_alloc(sizeof(RULE));
	if (rul == (RULE *)NULL)
	{
		/* Out of memory for rule definition. */
		errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
		return 0;
	}
	rul->rnext = (RULE *)NULL;

	/* Allocate memory for rule dest extension. */
	rul->rdest = (char *)mem_alloc(strlen(rdest) + 1);
	if (rul->rdest == (char *)NULL)
	{
		/* Out of memory for rule name. */
		errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
		mem_free(rul);
		return 0;
	}

	/* Save dest extension in rule descriptor. */
	strcpy(rul->rdest, rdest);

	/* Allocate memory for rule source extension. */
	rul->rsrc = (char *)mem_alloc(strlen(rsrc) + 1);
	if (rul->rsrc == (char *)NULL)
	{
		/* Out of memory for rule name. */
		errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
		mem_free(rul->rdest);
		mem_free(rul);
		return 0;
	}

	/* Save source extension in rule descriptor. */
	strcpy(rul->rsrc, rsrc);

	/* Assume rule has no commands. */
	rul->rcommands = (LINE *)NULL;

	if (rule_list == (RULE *)NULL)
	{
		/* This rule is the head of the list. */
		rule_list = rul;
	}
	else
	{
		/* Insert rule at head of list. */
		rptr = rule_list;
		rule_list = rul;
		rul->rnext = rptr;
	}

	/*
	** Get the command lines for the rule.
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
			/* Unexpected end of file. */
			errmsg(MSG_ERR_EOF, (char *)NULL, NOVAL);
			return 0;
		}

		/* Check for a comment line. */
		if (inpline[0] == '#')
		{
			/*
			** The line contains a comment, so go back
			** and try again.
			*/
			continue;
		}

		/*
		** Check for end of rule's commands.  A line that
		** isn't indented isn't part of the rule's commands.
		*/
		if (inpline[0] != ' ' && inpline[0] != '\t')
		{
			/*
			** End of rule definition.  This line is
			** part of whatever comes next in the
			** makefile, so push it back into the
			** input stream.
			*/
			unread_logical_line(inpline);
			break;
		}

		/* Find first non-whitespace character. */
		pos = 0;
		while (inpline[pos] == ' ' || inpline[pos] == '\t')
			pos++;

		/* Add line to rule. */
		ltmp = append_line(rul->rcommands, &inpline[pos]);
		if (ltmp == (LINE *)NULL)
		{
			/* Out of memory for rule data. */
			errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
			return 0;
		}
		rul->rcommands = ltmp;
	} /* End while(1) */

	/* Success! */
	return 1;
} /* End define_rule() */

/*
** dump_rules:
** Outputs the rule list.  This function is used for
** debugging.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
void
dump_rules(void)
{
	LINE	*lptr;	/* Temporary line pointer. */
	RULE	*rptr;	/* Temporary rule pointer. */

	rptr = rule_list;
	if (rptr == (RULE *)NULL)
		mputs(MSG_INFO_NORULES);
	while (rptr != (RULE *)NULL)
	{
		mputs(MSG_INFO_RULENAME);
		mputs(".");
		mputs(rptr->rsrc);
		mputs(".");
		mputs(rptr->rdest);
		mputs("\n");
		lptr = rptr->rcommands;
		while (lptr != (LINE *)NULL)
		{
			mputs(MSG_INFO_RULECMD);
			mputs(lptr->ldata);
			mputs("\n");

			lptr = lptr->lnext;
		}

		rptr = rptr->rnext;
	}
} /* End dump_rules() */

/*
** check_rules:
** Checks to see if the rule list contains any rules for which
** the destination extension is a particular extension.  This
** is useful if the caller wishes to find out if there are any
** rules that might be of interest before calling lookup_rule
** repeatedly to match each possible suffix to.
**
** Parameters:
**	Name	Description
**	----	-----------
**	suf	Suffix to check for in rule list.  The suffix
**		should not have a leading '.'.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	The rule list contains at least one rule for which
**		the destination suffix is the specified suffix.
**	0	The rule list doesn't contain any rules for which
**		the destination suffix is the specified suffix.
*/
int
check_rules(suf)
	char	*suf;
{
	RULE	*rptr;

	rptr = rule_list;
	while (rptr != (RULE *)NULL)
	{
		if (strcmp(rptr->rdest, suf) == 0)
			return 1;
		rptr = rptr->rnext;
	}

	return 0;
} /* End check_rules() */

/*
** lookup_rule:
** Looks in the rule list for a rule for building a file
** with a given suffix from a file with another given suffix.
**
** Parameters:
**	Name	Description
**	----	-----------
**	src	Filename suffix of source file.  Note that
**		the leading '.' of the suffix should not be
**		present.
**	dest	Filename suffix of destination file.  Note
**		that the leading '.' of the suffix should
**		not be present.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	NULL	No rule was found for the specified suffixes.
**	other	Pointer to RULE for specified suffixes.
*/
RULE *
lookup_rule(src, dest)
	char	*src;
	char	*dest;
{
	RULE	*rptr;

	/* Start at head of rule list. */
	rptr = rule_list;

	/*
	** Look through each rule until we find the one we want or
	** run out of rules.
	*/
	while (rptr != (RULE *)NULL)
	{
		/* Is this the rule we're looking for? */
		if (strcmp(rptr->rsrc, src) == 0 &&
			strcmp(rptr->rdest, dest) == 0)
		{
			/* Found the rule we're looking for. */
			return rptr;
		}

		/* Step to next rule in list. */
		rptr = rptr->rnext;
	}

	/* Didn't find the rule we're looking for. */
	return (RULE *)NULL;
} /* End lookup_rule() */

/*
======================================================================
End makerul.c
======================================================================
*/
