/*
======================================================================
makein.c
Makefile reading/parsing routines for make utility.

This source file is part of a computer program that is
(C) Copyright 1988 Ammon R. Campbell.  All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES


Lines:

The make utility treats the makefile as a sequence of logical
lines.  Each logical line may be made up of one or more physical
lines.  If a physical line ends with a backslash, the next
physical line is assumed to be part of the same logical line.  A
double backslash may be placed at the end of a physical line to
indicate that the line really ends with a backslash.

A physical line is a sequence of characters terminated by a line
feed or a null character.

The maximum length of a logical line is defined by the symbol
'MAXLLINE' which is defined in the "make.h" file.


Macro Expansion:

The input routines handle expansion of named macros of the form
"$(macroname)".  Only the special target-name related macros
should need to be handled elsewhere in the make utility.


Buffering:

Raw data from the makefile is blocked into a buffer called 'iobfr'
rather than reading one byte at a time from the file.  The
read_logical_line() routine then reads from 'iobfr' a byte at a
time as it needs, refilling 'iobfr' whenever it becomes empty.

Since it is necessary for the make utility to sometimes back up
the input, a mechanism has been provided for backing up one
logical line in the input through the unread_logical_line()
routine.  This routine allocates a buffer large enough to hold the
data being unread, called 'unread_lbfr'.  The next call to
read_logical_line() will check 'unread_lbfr', and use its contents
before processing more data from 'iobfr'.

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

/* Functions that are local to this file: */
static int do_psuedo(char *line);
static int line_type(char *line);
static int init_input_buffers(void);
static void deinit_input_buffers(void);
static int local_read_logical_line(int handle, char *str, int maxlen);

/****************************** CONSTANTS ***************************/

#define MAXIO	1024	/* Size of I/O buffer. */
#define MAXIFS	8	/* Maximum nesting level of !IFs. */

/****************************** VARIABLES ***************************/

/* iobfr:  Buffer for data read from makefile. */
static char	*iobfr;
static int	iosize;		/* Number of bytes in iobfr. */
static int	iopos;		/* Current position in iobfr. */

/* unread_lbfr:  Buffer for pushing logical line back into the input stream. */
static char *unread_lbfr;

/* ifmode:  Flag, 0=normal, 1=doing inside !IF, 2=skipping inside !IF */
static char	ifmode[MAXIFS];
static int	iflevel;

/*************************** LOCAL FUNCTIONS ************************/

/*
** do_psuedo:
** Parses psuedo-target lines.  Handles ".SUFFIXES", ".IGNORE",
** ".SILENT", and ".PRECIOUS".
**
** Parameters:
**	Name	Description
**	----	-----------
**	line	String containing psuedo-target to parse.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Successful.
**	0	Error occurred.
*/
static int
do_psuedo(line)
	char	*line;
{
	if (strncmp(line, ".SUFFIXES", 9) == 0)
	{
		if (!do_suffixes(line))
		{
			/* Failed handling ".SUFFIXES" */
			return 0;
		}
	}
	else if (strncmp(line, ".PRECIOUS", 9) == 0)
	{
		if (!do_precious(line))
		{
			/* Failed handling ".PRECIOUS" */
			return 0;
		}
	}
	else if (strncmp(line, ".IGNORE", 7) == 0)
	{
		SETFLAG(FLAG_IGNORE);
	}
	else if (strncmp(line, ".SILENT", 7) == 0)
	{
		SETFLAG(FLAG_NO_SHOW);
	}
	else
	{
		/* Unrecognized psuedo-target in makefile. */
		errmsg(MSG_ERR_BADPSUEDO, line, NOVAL);
		return 0;
	}

	return 1;
}

/*
** line_type:
** Determines what type of makefile section the specified
** line is the beginning of.
**
** Parameters:
**	Name	Description
**	----	-----------
**	line	Line to be tested.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	0	Line is unrecognized.
**	1	Line is blank or a comment.
**	2	Line is the start of a macro definition.
**	3	Line is the start of a psuedo-target (i.e. ".SUFFIXES").
**	4	Line is the start of a rule.
**	5	Line is the start of a target.
*/
static int
line_type(line)
	char	*line;
{
	int	i;		/* Loop index. */
	int	allwhite;	/* Flag. */

	/*
	** Check for the different types of makefile sections:
	**
	** A blank line.
	**
	** A comment, which starts with a '#'.
	**
	** A macro definition of the form:
	**	|
	**	|macro=string
	**
	** A psuedo-target of the form:
	**	|
	**	|.opname
	**
	**  or
	**	|
	**	|.opname: parameters
	**
	** A rule of the form:
	**	|
	**	|.ext.ext:
	**	|	[commands]
	**
	** A target of the form:
	**	|
	**	|target.trg: [dependents]
	**	|	[commands]
	*/

	/* Check for blank line. */
	if (line[0] == '\0')
	{
		/* Line is blank. */
		return 1;
	}

	/*
	** Check for a line that is all whitespace.
	** This is treated as a blank line.
	*/
	allwhite = 1;	/* Assume it's all whitespace. */
	i = 0;
	while (line[i] != '\0')
	{
		if (line[i] != ' ' && line[i] != '\t')
		{
			/* Found a non-whitespace character. */
			allwhite = 0;
			break;
		}
		i++;
	}
	if (allwhite)
	{
		/* This line is all whitespace. */
		return 1;
	}

	/* Check for comment. */
	if (line[0] == '#')
	{
		/* Line contains a comment. */
		return 1;
	}

	/*
	** Check for macro definition.  If the second
	** non-whitespace token on the line is '=',
	** we assume it is a macro definition.
	*/
	i = 0;
	while (line[i] != '\0' &&
		line[i] != ' ' &&
		line[i] != '\t' &&
		line[i] != '=')
		i++;
	while (line[i] == ' ' || line[i] == '\t')
		i++;
	if (line[i] == '=')
	{
		/* Line is a macro definition. */
		return 2;
	}

	/*
	** Check for a rule definition.  If the
	** first five tokens on the line are
	** dot-identifier-dot-identifier-colon,
	** we assume it is a rule.
	*/
	i = 0;
	if (line[i] == '.')
	{
		i++;
		while (line[i] != '.' &&
			line[i] != ':' &&
			line[i] != ' ' &&
			line[i] != '\t' &&
			line[i] != '\0')
		{
			i++;
		}
		if (line[i] == '.')
		{
			i++;
			while (line[i] != '.' &&
				line[i] != ':' &&
				line[i] != ' ' &&
				line[i] != '\t' &&
				line[i] != '\0')
			{
				i++;
			}
			if (line[i] == ':')
			{
				/* Line is a rule. */
				return 4;
			}
		}
	}

	/*
	** Since we've already checked for rules, any other
	** line that starts with a '.' is assumed to be a
	** psuedo-target.
	*/
	if (line[0] == '.')
	{
		/* Line is a psuedo-target. */
		return 3;
	}

	/*
	** Since we've already screened out all the other
	** valid possibilities, the line must be either
	** a target line or an error.  If the line doesn't
	** start with whitespace, we assume it's a target.
	*/
	if (line[0] != ' ' && line[0] != '\t')
	{
		/* Line is a target. */
		return 5;
	}

	/* Unrecognized line. */
	return 0;
}

/*
** init_input_buffers:
** Allocates memory for and initializes the input buffers
** used when processing data from the makefile.
**
** Parameters:
**	NONE
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Successful.
**	0	Error occurred.
*/
static int
init_input_buffers(void)
{
	/* Mark unread buffer as empty. */
	unread_lbfr = (char *)NULL;

	/* Allocate raw input buffer. */
	iobfr = (char *)mem_alloc(MAXIO);
	if (iobfr == (char *)NULL)
	{
		errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
		return 0;
	}
	iosize = 0;
	iopos = 0;

	/* Allocate line input buffer. */
	inpline = (char *)mem_alloc(MAXLLINE);
	if (inpline == (char *)NULL)
	{
		errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
		return 0;
	}

	iflevel = -1;

	return 1;
}

/*
** deinit_input_buffers:
** Frees memory used by the input buffers used when
** processing the makefile.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
static void
deinit_input_buffers(void)
{
	/* Free the raw input buffer. */
	if (iobfr != (char *)NULL)
		mem_free(iobfr);
	iobfr = (char *)NULL;
	iosize = 0;
	iopos = 0;

	/* Free the line input buffer. */
	if (inpline != (char *)NULL)
		mem_free(inpline);
	unread_lbfr = (char *)NULL;

	/* Free the unread buffer. */
	if (unread_lbfr != (char *)NULL)
		mem_free(unread_lbfr);
	unread_lbfr = (char *)NULL;

	iflevel = -1;
}

/*
** local_read_logical_line:
** Reads a logical line of text from the makefile.  A logical
** line is one line of text which is a combination of the next
** physical line from the makefile plus any physical continuation
** lines indicated by a trailing backslash on the previous line.
** Called by read_logical_line().
**
** Parameters:
**	Name	Description
**	----	-----------
**	handle	Open file handle to read from.
**	str	Buffer to read into.
**	maxlen  Maximum number of bytes to read.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	If successful.
**	0	If end-of-file or I/O error occurs.
**	-1	If line exceeds maximum length or fatal error
**		occurs.
*/
static int
local_read_logical_line(handle, str, maxlen)
	int	handle;
	char	*str;
	int	maxlen;
{
	char	*sstart = str;	/* Copy of original string pointer. */
	int	bytes = 0;	/* Number of bytes in line so far. */
	char	last = '\0';	/* Last character placed in buffer. */
	char	last2 = '\0';	/* Second to last char placed in buffer. */
	char	*tmp;		/* Buffer for macro expansion. */
	int	eresult;	/* Result of last macro expansion. */
	char	has_dollar = 0;	/* Flag, nonzero if line contains a '$'. */

	/* Check if the unread buffer has something in it. */
	if (unread_lbfr != (char *)NULL)
	{
		if (strlen(unread_lbfr) > (unsigned)maxlen)
		{
			/* Unread buffer is too long for caller's buffer. */
			errmsg(MSG_ERR_UNREADLEN, (char *)NULL, NOVAL);
			return -1;
		}
		strcpy(str, unread_lbfr);
		mem_free(unread_lbfr);
		unread_lbfr = (char *)NULL;
		return 1;
	}

	/*
	** Read characters from the input file until the
	** end of a line is found or an error occurs.
	*/
	*str = '\0';
	while (1)
	{
		/* See if we need to read a new buffer full of data. */
		if (iopos >= iosize)
		{
			iosize = mread(handle, iobfr, MAXIO);
			if (iosize < 0)
			{
				/* Read failure. */
				errmsg(MSG_ERR_IOREAD, (char *)NULL, NOVAL);
				return -1;
			}
			iopos = 0;
		}

		/* Check for end of file. */
		if (iosize == 0)
		{
			/* End of file. */
			*str = '\0';
			if (str != sstart)
			{
				/* We got some data before EOF. */
				return 1;
			}

			/* There's no data to return. */
			return 0;
		}

		/* Copy byte from input buffer to caller's buffer. */
		*str = iobfr[iopos++];
		if (*str == '$')
			has_dollar = 1;

		/* Do the right thing depending on what byte it is. */
		if (*str == '\n' || *str == '\0')
		{
			/* Check for continuation line. */
			if (last == '\\' && last2 != '\\')
			{
				/* Continuation line. */
				last = '\0';
				last2 = '\0';
				str--;
				*str = '\0';
			}
			else
			{
				/* This is the end of the line. */
				break;
			}
		}
		if ((*str != '\r') && (*str != '\n') && (*str != '\0'))
		{
			last2 = last;
			last = *str;
			str++;
			bytes++;
		}
		*str = '\0';

		if (bytes >= maxlen)
		{
			/* Line too long. */
			errmsg(MSG_ERR_LINETOOLONG, (char *)NULL, NOVAL);
			return -1;
		}
	}

	/* Mark end of line with null. */
	*str = '\0';

	/* Check if we should try expanding named macros. */
	if (has_dollar && (sstart[0] != '#'))
	{
		/*
		** The line contains at least one '$', so we
		** need to try expanding named macros.
		*/

		/* Allocate memory for macro expansion. */
		tmp = (char *)mem_alloc(maxlen);
		if (tmp == (char *)NULL)
		{
			/* Not enough memory. */
			errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
			return -1;
		}

		/*
		** Perform macro expansions on named macros.
		*/
		do
		{
			eresult = expand_named_macros(sstart, tmp, maxlen);
			if (eresult > 0)
				strcpy(sstart, tmp);
		}
		while (eresult == 2);
		if (eresult < 1)
		{
			/* Error expanding macros. */
			mem_free(tmp);
			return -1;
		}

		/* Free macro expansion buffer. */
		mem_free(tmp);
	}

	return 1;
}

/****************************** FUNCTIONS ***************************/

/*
** unread_logical_line:
** Puts a line of text back into the input stream, to be read
** the next time read_logical_line() is called.  The input can
** only be backed up one line.
**
** Parameters:
**	Name	Description
**	----	-----------
**	str	String to unread.
**
** Returns:
**	Name	Description
**	----	-----------
**	1	Successful.
**	0	The unread buffer already has something in it
**		or a memory error occurred.
*/
int
unread_logical_line(str)
	char	*str;
{
	/* Check if unread buffer already has something in it. */
	if (unread_lbfr != (char *)NULL)
	{
		/* Unread buffer already has something in it. */
		errmsg(MSG_ERR_UNREADFULL, (char *)NULL, NOVAL);
		return 0;
	}

	/* Allocate memory for unread buffer. */
	unread_lbfr = (char *)mem_alloc(strlen(str) + 1);
	if (unread_lbfr == (char *)NULL)
	{
		/* Not enough memory. */
		errmsg(MSG_ERR_OUTOFMEMORY, (char *)NULL, NOVAL);
		return 0;
	}

	/* Copy line to unread buffer. */
	strcpy(unread_lbfr, str);

	return 1;
}

/*
** read_logical_line:
** Reads a logical line of text from the makefile.  A logical
** line is one line of text which is a combination of the next
** physical line from the makefile plus any physical continuation
** lines indicated by a trailing backslash on the previous line.
** Named macros in the input text are expanded, and comments
** are eliminated.
**
** Parameters:
**	Name	Description
**	----	-----------
**	handle	Open file handle to read from.
**	str	Buffer to read into.
**	maxlen  Maximum number of bytes to read.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	If successful.
**	0	If end-of-file or I/O error occurs.
**	-1	If line exceeds maximum length or fatal error
**		occurs.
*/
int
read_logical_line(handle, str, maxlen)
	int	handle;
	char	*str;
	int	maxlen;
{
	int	haveline = 0;
	int	result;
	int	i;
	int	j;
	int	k;
	char	token[20];
	char	mname[MAXPATH];

	/*
	** Read a line of input, skipping comments.
	*/
	do
	{
		/* Get a line of input. */
		result = local_read_logical_line(handle, str, maxlen);
		if (result != 1)
		{
			if (result == 0 && iflevel > -1)
			{
				/*
				** End-of-file.  If we're inside an IF,
				** then complain.
				*/
				errmsg(MSG_ERR_EOF, (char *)NULL, NOVAL);
				return -1;
			}

			/* End-of-file or read error. */
			return result;
		}
		haveline = 1;

		/* Check for special cases. */
		if (str[0] == '#')
		{
			/* Line contains a comment. */
			haveline = 0;
		}
		else if (str[0] == '!')
		{
			/* Skip any whitespace following '!' */
			i = 1;
			while (str[i] == ' ' || str[i] == '\t')
				i++;

			/* Get token following '!'. */
			j = 0;
			while (j < 19 &&
				str[i] != '\0' &&
				str[i] != ' ' &&
				str[i] != '\t')
				token[j++] = str[i++];
			token[j] = '\0';
			if (str[i] != ' ' && str[i] != '\t' && str[i] != '\0')
				break;

			/* Handle '!' directives. */
			if (strcmp(token, "ERROR") == 0)
			{
				/* See if we're skipping right now. */
				k = 0;
				if (iflevel > -1)
				{
					/* Search the level of nested ifs for a skip. */
					k = 0;
					j = iflevel;
					while (j >= 0)
					{
						/* Is this level in skip mode? */
						if (ifmode[j] == 2)
							k = 1;
						j--;
					}
				}

				if (k == 0)
				{
					/* Output forced error message. */
					if (str[i] == ' ' || str[i] == '\t')
						i++;
					mputs(&str[i]);
					mputs("\n");
					return -1;
				}
				haveline = 0;
			}
			else if (strcmp(token, "MESSAGE") == 0)
			{
				/* See if we're skipping right now. */
				k = 0;
				if (iflevel > -1)
				{
					/* Search the level of nested ifs for a skip. */
					k = 0;
					j = iflevel;
					while (j >= 0)
					{
						/* Is this level in skip mode? */
						if (ifmode[j] == 2)
							k = 1;
						j--;
					}
				}

				if (k == 0)
				{
					/* Output message. */
					if (str[i] == ' ' || str[i] == '\t')
						i++;
					mputs(&str[i]);
					mputs("\n");
				}
				haveline = 0;
			}
			else if (strcmp(token, "IFDEF") == 0)
			{
				if (iflevel >= MAXIFS - 1)
				{
					/* Nesting too deep in ifs. */
					errmsg(MSG_ERR_TOOMANYIFS, str, NOVAL);
					return -1;
				}
				while (str[i] == ' ' || str[i] == '\t')
					i++;
				j = 0;
				while (j < MAXPATH - 1 && str[i] != '\0' &&
					str[i] != ' ' &&
					str[i] != '\t')
					mname[j++] = str[i++];
				mname[j] = '\0';
				if (j >= MAXPATH - 1)
				{
					/* Macro name too long. */
					errmsg(MSG_ERR_SYMTOOLONG, str, NOVAL);
					return -1;
				}
				iflevel++;
				if (find_macro(mname) != (char *)NULL)
					ifmode[iflevel] = 1;
				else
					ifmode[iflevel] = 2;
				haveline = 0;
			}
			else if (strcmp(token, "IFNDEF") == 0)
			{
				if (iflevel >= MAXIFS - 1)
				{
					/* Nesting too deep in ifs. */
					errmsg(MSG_ERR_TOOMANYIFS, str, NOVAL);
					return -1;
				}
				while (str[i] == ' ' || str[i] == '\t')
					i++;
				j = 0;
				while (j < MAXPATH - 1 && str[i] != '\0' &&
					str[i] != ' ' &&
					str[i] != '\t')
					mname[j++] = str[i++];
				mname[j] = '\0';
				if (j >= MAXPATH - 1)
				{
					/* Macro name too long. */
					errmsg(MSG_ERR_SYMTOOLONG, str, NOVAL);
					return -1;
				}
				iflevel++;
				if (find_macro(mname) != (char *)NULL)
					ifmode[iflevel] = 2;
				else
					ifmode[iflevel] = 1;
				haveline = 0;
			}
			else if (strcmp(token, "ELSE") == 0)
			{
				if (iflevel < 0 || ifmode[iflevel] == 0)
				{
					/* Unexpected "!ELSE" */
					errmsg(MSG_ERR_BANGUNEXP, str, NOVAL);
					return -1;
				}
				if (ifmode[iflevel] == 1)
					ifmode[iflevel] = 2;
				else if (ifmode[iflevel] == 2)
					ifmode[iflevel] = 1;
				haveline = 0;
			}
			else if (strcmp(token, "ENDIF") == 0)
			{
				if (iflevel > -1 && ifmode[iflevel] != 0)
				{
					iflevel--;
				}
				else
				{
					/* Unexpected "!ENDIF" */
					errmsg(MSG_ERR_BANGUNEXP, str, NOVAL);
					return -1;
				}
				haveline = 0;
			}
			else if (strcmp(token, "IF") == 0)
			{
				/* NOT IMPLEMENTED YET. */
				haveline = 0;
			}
			else if (strcmp(token, "INCLUDE") == 0)
			{
				/* NOT IMPLEMENTED YET. */
				haveline = 0;
			}
			else if (strcmp(token, "UNDEF") == 0)
			{
				/* NOT IMPLEMENTED YET. */
				haveline = 0;
			}
		}

		/*
		** Don't return to caller until lines inside of
		** if-else-endif are skipped.
		*/
		if (iflevel > -1)
		{
			/* Search the level of nested ifs for a skip. */
			i = 0;
			j = iflevel;
			while (j >= 0)
			{
				/* Is this level in skip mode? */
				if (ifmode[j] == 2)
					i = 1;
				j--;
			}

			/* We are in skip mode at some level. */
			if (i == 1)
				haveline = 0;
		}
	}
	while (!haveline);

	return result;
}

/*
** process_makefile:
** Reads and parses the contents of the makefile.
**
** Parameters:
**	Name	Description
**	----	-----------
**	mf	Name of makefile to process.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Successful.
**	0	Error occurred.
*/
int
process_makefile(mf)
	char	*mf;
{
	int	mh;		/* File handle to makefile. */
	int	result;		/* Result of last read. */

	/* Open the makefile. */
//	mh = open(mf, O_RDONLY | O_BINARY);
	mh = mopen_r(mf);
	if (mh < 0)
	{
		/* Error opening makefile. */
		errmsg(MSG_ERR_CANTOPEN, mf, NOVAL);
		return 0;
	}

	/* Initialize the input buffers. */
	if (!init_input_buffers())
	{
		/* Couldn't initialize buffers. */
		return 0;
	}

	if (CHKFLAG(FLAG_DEBUG))
	{
		/* Tell which makefile we're reading from. */
		mputs(MSG_DBG_MAKENAME);
		mputs(mf);
		mputs("\n");
	}

	/* Process each section of the makefile. */
	while ((result = read_logical_line(mh, inpline, MAXLLINE)) == 1)
	{
		/*
		** Determine what type of section this is.
		** The possibilities are:
		*/
		switch(line_type(inpline))
		{
			case 1: /* Blank or comment. */
				/* Ignore it. */
				break;

			case 2: /* Macro. */
				if (!define_macro(inpline, mh))
				{
					/* Error defining macro. */
					return 0;
				}
				break;

			case 3: /* Psuedo-target. */
				if (!do_psuedo(inpline))
				{
					/* Error in psuedo-target. */
					deinit_input_buffers();
					return 0;
				}
				break;

			case 4: /* Rule. */
				if (!define_rule(inpline, mh))
				{
					/* Error defining rule. */
					deinit_input_buffers();
					return 0;
				}
				break;

			case 5: /* Target. */
				if (!define_target(inpline, mh))
				{
					/* Error defining target. */
					deinit_input_buffers();
					return 0;
				}
				break;

			case 0:
			default:
				/* Syntax error in makefile. */
				errmsg(MSG_ERR_SYNTAX, inpline, NOVAL);
				deinit_input_buffers();
				return 0;

		} /* End switch() */

	} /* End while(...) */

	if (result == -1)
	{
		/* A fatal read error occurred. */
		deinit_input_buffers();
		return 0;
	}

	mclose(mh);
	deinit_input_buffers();

	return 1;
}

