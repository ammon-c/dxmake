/*
======================================================================
makeutil.c
Miscellaneous utility routines for make utility.

This source file is part of a computer program that is
(C) Copyright 1988 Ammon R. Campbell.  All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================
*/

/****************************** INCLUDES ****************************/

#ifdef WIN
# include <windows.h>
#endif /* WIN */
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <dos.h>
#include <string.h>

#include "make.h"

/****************************** HEADERS *****************************/

	/* Also see "make.h" file. */

/****************************** CONSTANTS ***************************/

	/* See "make.h" file. */

/****************************** VARIABLES ***************************/

/*
** Temporary pathname buffer used by enumpath().
*/
static char locpath[MAXPATH];

/*
** Temporary pathname pointer used by enumpath().
*/
static char *pathptr;

	/* Also see "make.h" file. */

/****************************** FUNCTIONS ***************************/

/*
** get_part_filename:
** Extracts a particular portion of a full pathname.
**
** Parameters:
**	Name	Description
**	----	-----------
**	part	Specifies which part of the pathname
**		to extract:
**			1 = Drive/directory only.
**			2 = Basename/suffix only.
**			3 = Basename only.
**			4 = Suffix only.
**			5 = Drive/directory/basename only.
**	src	Buffer containing pathname to parse.
**	dest	Buffer to place parsed info into (may be
**		the same buffer as 'src'.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Successful.
**	0	Error occurred.
*/
int
get_part_filename(part, src, dest)
	int	part;
	char	*src;
	char	*dest;
{
	char	src2[MAXPATH];	/* Copy of 'src'. */
	char	*s;
	int	dpos = 0;	/* Position in destination string. */
	int	i;
	int	j;

	/* Make a local copy of 'src'. */
	strcpy(src2, src);
	s = src2;

	/* Range check 'part'. */
	if (part < 1 || part > 5)
	{
		/* Unrecognized 'part' option. */
		return 0;
	}

	/* Check if we need drive letter. */
	if (part == 1 || part == 5)
	{
		/* Get drive letter (if any). */
		if (s[1] == ':')
		{
			/* Copy drive letter to dest. */
			dest[dpos++] = *s;
			dest[dpos++] = ':';

			/* Skip drive letter and colon. */
			s++;
			s++;
		}
	}

	/*
	** Determine where the directory portion of the filename
	** ends.
	*/
	i = strlen(s);
	while (i > 0 && s[i] != '\\')
		i--;

	/* Check if we need the directory part. */
	if (part == 1 || part == 5)
	{
		/* Copy directory name to dest. */
		j = 0;
		while (j < i && (*s != '\0'))
		{
			dest[dpos++] = s[j++];
		}
	}

	/* Check if we need trailing backslash. */
	if (part == 5)
	{
		if (s[i] == '\\')
			dest[dpos++] = '\\';
	}
	if (s[i] == '\\')
		i++;

	/* Skip directory part of src. */
	j = 0;
	while (j < i && (*s != '\0'))
	{
		j++;
		s++;
	}

	/* Find end of basename. */
	i = strlen(s);
	while (i > 0 && s[i] != '.')
		i--;
	if (s[i] != '.')
		i = strlen(s);

	/* Check if we need the basename. */
	if (part == 2 || part == 3 || part == 5)
	{
		/* Copy basename to dest. */
		j = 0;
		while (j < i && (*s != '\0'))
		{
			dest[dpos++] = s[j++];
		}
	}

	/* Check if we need the trailing '.' */
	if (part == 2)
	{
		if (s[i] == '.')
			dest[dpos++] = '.';
	}
	if (s[i] == '.')
		i++;

	/* Skip the basename. */
	j = 0;
	while (j < i && *s != '\0')
	{
		j++;
		s++;
	}

	/* Check if we need the suffix. */
	if (part == 2 || part == 4)
	{
		/* Copy suffix to dest. */
		while (*s != '\0')
			dest[dpos++] = *s++;
	}

	/* Add terminating null to dest. */
	dest[dpos] = '\0';

	/* Success! */
	return 1;
} /* End get_part_filename() */

/*
** touch_file:
** Changes the timestamp of a file to the current time/date.
** This is implemented using direct interrupt to MS-DOS
** because we don't have to call open() and close() which
** take up alot of code space.  If code size was not an issue,
** we could just as easily call open() and close() on the
** file to touch it.
**
** Parameters:
**	Name	Description
**	----	-----------
**	name	Name of file to timestamp.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Successful.
**	0	Error occurred.
*/
int
touch_file(name)
	char	*name;
{
	int	handle;
	union REGS dosregs;

	/* Open file for writing. */
	dosregs.h.ah = 0x3D;		/* DOS open file function. */
	dosregs.x.dx = (int)name;	/* DX gets offset of filename. */
	dosregs.h.al = 2;		/* 2 = open for read/write. */
	intdos(&dosregs, &dosregs);
	handle = dosregs.x.ax;

	/* Check for error. */
	if (dosregs.x.cflag)
		return 0;	/* Error occurred. */

	/* Close the file. */
	dosregs.h.ah = 0x3E;	/* DOS close file function. */
	dosregs.x.bx = handle;	/* BX gets file handle. */
	intdos(&dosregs, &dosregs);

	/* Check for error. */
	if (dosregs.x.cflag)
		return 0;	/* Error occurred. */

	/* Success! */
	return 1;
} /* End touch_file() */

/*
** mutoa:
** Converts a short unsigned integer value to ASCII text.
**
** Parameters:
**	Name	Description
**	----	-----------
**	val	Value to be converted.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	any	Pointer to buffer containing converted string.
*/
char *
mutoa(val)
	short unsigned int	val;
{
	static char	s[7];
	int		pos = 5;

	if (val == 0)
	{
		s[0] = '0';
		s[1] = '\0';
		return s;
	}

	s[6] = '\0';
	while (val > 0)
	{
		s[pos--] = (char)(val % 10) + (char)'0';
		val /= 10;
	}

	return &s[pos + 1];
} /* End mutoa() */

#ifdef NOTUSED
/*
** multoa:
** Converts a long unsigned integer value to ASCII text.
**
** Parameters:
**	Name	Description
**	----	-----------
**	val	Value to be converted.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	any	Pointer to buffer containing converted string.
*/
char *
multoa(val)
	long unsigned int	val;
{
	static char	s[15];
	int		pos = 13;

	if (val == 0L)
	{
		s[0] = '0';
		s[1] = '\0';
		return s;
	}

	s[14] = '\0';
	while (val > 0L)
	{
		s[pos--] = (char)(val % 10L) + (char)'0';
		val /= 10L;
	}

	return &s[pos + 1];
} /* End multoa() */
#endif

/*
** mputs:
** Small put string routine.  By using this instead of
** printf() we save about 4K off the EXE size.  If code
** space were not an issue, we could just as easily use
** printf() or one of the other library output routines.
**
** Parameters:
**	Name	Description
**	----	-----------
**	s	String to output.
**
** Returns:
**	NONE
*/
void
mputs(s)
	char	*s;
{
#ifndef WIN
	union REGS dosregs;

	while (*s)
	{
		dosregs.h.ah = 2;	/* DOS output character function. */
		dosregs.h.dl = *s++;
		intdos(&dosregs, &dosregs);
		if (*(s - 1) == '\n')
		{
			dosregs.h.ah = 2;
			dosregs.h.dl = '\r';
			intdos(&dosregs, &dosregs);
		}
	}
#else
	while (*s)
	{
		wputc(*s++);
	}
#endif /* WIN */
} /* End mputs() */

/*
** mchdir:
** Changes the current directory to the specified
** drive and directory.
**
** Parameters:
**	Name	Description
**	----	-----------
**	dname	Name of drive/directory to change to.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Successful.
**	0	Error occurred.
*/
int
#ifndef WIN
mchdir(char *dname)
#else
mchdir(LPSTR dname)
#endif /* WIN */
{
	char	stmp[MAXPATH];

	/* Make local copy of directory name. */
#ifndef WIN
	strcpy(stmp, dname);
#else
	lstrcpy((LPSTR)stmp, (LPSTR)dname);
#endif /* WIN */

	/* Change to directory. */
#ifdef WIN
	SetErrorMode(1);
#endif /* WIN */
	if (chdir(stmp))
	{
#ifdef WIN
		SetErrorMode(0);
#endif /* WIN */
		return 0;
	}

	/* Change to drive where EXE file is. */
	if (stmp[1] == ':')
	{
		if (stmp[0] >= 'a' && stmp[0] <= 'z')
			stmp[0] = stmp[0] - (char)'a' + (char)'A';
		if (_chdrive(stmp[0] - 'A' + 1))
		{
#ifdef WIN
			SetErrorMode(0);
#endif /* WIN */
			return 0;
		}
	}

	/* Success! */
#ifdef WIN
	SetErrorMode(0);
#endif /* WIN */
	return 1;
} /* End mchdir() */

/*
** mopen_r:
** Opens a file for reading in binary mode.  This is
** similar to "open(fname, O_RDONLY | O_BINARY)", but
** takes up alot less code space.  If code space were
** not an issue, we could just as easily use open()
** instead.
**
** Parameters:
**	Name	Description
**	----	-----------
**	fname	Name of file to open.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	-1	Error occurred.
**	other	File handle of open file.
*/
int
mopen_r(fname)
	char	*fname;
{
	union REGS dosregs;

	dosregs.h.ah = 0x3D;		/* DOS open file function. */
	dosregs.x.dx = (int)fname;	/* DX gets offset of filename. */
	dosregs.h.al = 0;		/* 0 = open for reading. */
	intdos(&dosregs, &dosregs);

	/* Check for error. */
	if (dosregs.x.cflag)
		return -1;	/* Error occurred. */

	/* Return file handle to caller. */
	return (int)dosregs.x.ax;
} /* End mopen_r() */

/*
** mclose:
** Closes a file that was opened with mopen_r().  This
** does basically the same thing as close() but takes
** up alot less code space.  If code space were not an
** issue, we could just as easily use close() instead.
**
** Parameters:
**	Name	Description
**	----	-----------
**
** Returns:
**	Value	Meaning
**	-----	-------
**	-1	Error occurred.
**	0	Successful.
*/
int
mclose(fh)
	int	fh;
{
	union REGS dosregs;

	dosregs.h.ah = 0x3E;	/* DOS close file function. */
	dosregs.x.bx = fh;	/* BX gets file handle. */
	intdos(&dosregs, &dosregs);

	if (dosregs.x.cflag)
		return -1;
	return 0;
} /* End mclose() */

/*
** mread:
** Reads data from a file that was opened with mopen_r().
** This is basically the same as read() but takes up alot
** less code space.  If code space were not an issue, we
** could just as easily use read() instead.
**
** Parameters:
**	Name	Description
**	----	-----------
**	fh	File handle of file to read from.
**	bfr	Pointer to buffer to read into.
**	count	Number of bytes to read.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	-1	Error occurred.
**	other	Number of bytes read.
*/
int
mread(fh, bfr, count)
	int	fh;
	char	*bfr;
	int	count;
{
	union REGS dosregs;

	dosregs.h.ah = 0x3F;		/* DOS read file function. */
	dosregs.x.bx = fh;		/* BX gets file handle. */
	dosregs.x.dx = (int)bfr;	/* DX gets buffer offset. */
	dosregs.x.cx = count;		/* CX gets number of bytes to read. */
	intdos(&dosregs, &dosregs);

	/* Check for error. */
	if (dosregs.x.cflag)
		return -1;

	/* Return number of bytes read. */
	return (int)dosregs.x.ax;
} /* End mread() */

/*
** mstat:
** Retrieves the time/date stamp of a file.
**
** Parameters:
**	Name	Description
**	----	-----------
**	fname	Name of file to get status of.
**	tstat	Pointer to mstat_t structure to
**		put time/date stamp of file into.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	0	Successful.
**	other	Error occurred.
*/
int
mstat(fname, tstat)
	char		*fname;
	struct mstat_t	*tstat;
{
	int		handle;
	union REGS	dosregs;

	/* Open the file. */
	dosregs.h.ah = 0x3D;		/* DOS open file function. */
	dosregs.x.dx = (int)fname;	/* DX gets offset of filename. */
	dosregs.h.al = 0;		/* 0 = open for reading. */
	intdos(&dosregs, &dosregs);

	/* Check for error. */
	if (dosregs.x.cflag)
		return -1;	/* Error occurred. */

	/* Save file handle. */
	handle = (int)dosregs.x.ax;

	/* Get time/date of file. */
	dosregs.h.ah = 0x57;	/* DOS get/set time/date stamp function. */
	dosregs.h.al = 0;	/* subfunction 0 = get time/date stamp. */
	dosregs.x.bx = handle;	/* BX gets file handle. */
	intdos(&dosregs, &dosregs);

	/* Check for error. */
	if (dosregs.x.cflag)
	{
		/* Close the file. */
		dosregs.h.ah = 0x3E;	/* DOS close file function. */
		dosregs.x.bx = handle;	/* BX gets file handle. */
		intdos(&dosregs, &dosregs);

		return -1;	/* Error occurred. */
	}

	/* Save time/date stamp in caller's structure. */
	tstat->st_mtime =
		(0xFFFF0000L & ((unsigned long)dosregs.x.dx << 16)) +
			(0x0000FFFFL & (unsigned long)dosregs.x.cx);

	/* Close the file. */
	dosregs.h.ah = 0x3E;	/* DOS close file function. */
	dosregs.x.bx = handle;	/* BX gets file handle. */
	intdos(&dosregs, &dosregs);

	/* Check for error. */
	if (dosregs.x.cflag)
		return -1;	/* Error occurred. */

	/* Success! */
	return 0;
} /* End mstat() */

/*
** enumpath:
** Enumerates the pathnames from the specified environment
** variable.
**
** Parameters:
**	Name	Description
**	----	-----------
**	flag	This integer flag determines what the function
**		does.
**		0 = Start at beginning of PATH.
**		1 = Enumerate next pathname.
**	envvar	This character pointer specifies the name of
**		the environment variable containing the path
**		to be searched.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	NULL	'envvar' is not defined or no more pathnames are
**		available in 'envvar'.
**	other	Pointer to pathname.
**
** NOTE:  Return value is meaningless when flag is zero.
*/
char *
enumpath(flag, envvar)
	int	flag;
	char	*envvar;
{
	int	pos;		/* Current position in return string. */
#if 0
	char	**tmpenvp;	/* Pointer into environment. */
#endif

	if (flag)
	{
		/* Return next pathname from PATH. */

		/* If PATH is NULL, return NULL. */
		if (pathptr == (char *)NULL)
			return (char *)NULL;

		/* Copy next string out of path. */
		pos = 0;
		while (*pathptr != ';' && *pathptr != '\0')
		{
			locpath[pos++] = *pathptr++;
		}
		while (*pathptr == ';' || *pathptr == ' ')
			++pathptr;
		locpath[pos] = '\0';
		if (locpath[0] == '\0' || pos < 1)
			return (char *)NULL;

		/* If pathname doesn't have backslash on it then add it. */
		if (locpath[pos-1] != '\\')
		{
			locpath[pos++] = '\\';
			locpath[pos] = '\0';
		}

		return (char *)locpath;
	}
	else
	{
		/* Get user's PATH environment variable. */
#if 0
		tmpenvp = mkenvp;
		while (tmpenvp != (char **)NULL &&
			**tmpenvp &&
			strncmp(*tmpenvp, "PATH=", 5) != 0)
		{
			++tmpenvp;
		}
		pathptr = *tmpenvp;
#else
		pathptr = getenv("PATH");
#endif
		return (char *)NULL;
	}
} /* End enumpath() */

/*
** errmsg:
** Outputs an error message.
**
** Parameters:
**	Name	Description
**	----	-----------
**	msg	Error message string.
**	sval	Optional string to display with error message.
**		If NULL, no string will display.
**	dval	Optional number to display with error message.
**		if NOVAL, no number will display.
**
** Returns:
**	NONE
*/
void
errmsg(msg, sval, dval)
	char	*msg;
	char	*sval;
	int	dval;
{
	mputs(MSG_ERRMSG);
	mputs(msg);
	if (sval != (char *)NULL || dval != NOVAL)
		mputs(": ");
	if (sval != (char *)NULL)
	{
		mputs(" '");
		mputs(sval);
		mputs("'");
	}
	if (dval != NOVAL)
	{
		mputs(" ");
		mputs(mutoa(dval));
	}
	mputs("\n");
} /* End errmsg() */

/*
** free_lines:
** Releases memory used by a linked list of line descriptors.
**
** Parameters:
**	Name	Description
**	----	-----------
**	lptr	Pointer to first line descriptor to free.
**
** Returns:
**	NONE
*/
void
free_lines(lptr)
	LINE	*lptr;
{
	LINE	*ltmp;

	while (lptr != (LINE *)NULL)
	{
		mem_free(lptr->ldata);
		ltmp = lptr->lnext;
		mem_free(lptr);
		lptr = ltmp;
	}
} /* End free_lines() */

/*
** create_line:
** Allocates and builds a line descriptor for the specified
** line of text.
**
** Parameters:
**	Name	Description
**	----	-----------
**	s	Text to build line descriptor for.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	NULL	Out of memory.
**	other	Pointer to new line descriptor.
*/
LINE *
create_line(s)
	char	*s;
{
	LINE	*l;

	/* Allocate memory for descriptor. */
	l = (LINE *)mem_alloc(sizeof(LINE));
	if (l == (LINE *)NULL)
	{
		/* Not enough memory. */
		return NULL;
	}

	/* Allocate memory for text. */
	l->ldata = (char *)mem_alloc(strlen(s) + 1);
	if (l->ldata == (char *)NULL)
	{
		/* Not enough memory. */
		mem_free(l);
		return NULL;
	}

	/* Set descriptor fields. */
	strcpy(l->ldata, s);
	l->lnext = (LINE *)NULL;

	return l;
} /* End create_line() */

/*
** append_line:
** Appends a string to the end of a linked list of lines.
**
** Parameters:
**	Name	Description
**	----	-----------
**	lhead	Pointer to first line in list of lines to
**		be appended to.
**	s	Pointer to buffer containing null terminated
**		string to be appended to list of lines.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	NULL	Error occurred.
**	other	Pointer to new head of line list.
*/
LINE *
append_line(lhead, s)
	LINE	*lhead;
	char	*s;
{
	LINE	*lptr;

	if (lhead == (LINE *)NULL)
	{
		lhead = create_line(s);
		if (lhead == (LINE *)NULL)
			return (LINE *)NULL;
		lhead->lnext = (LINE *)NULL;
	}
	else
	{
		lptr = lhead;
		while (lptr->lnext != (LINE *)NULL)
			lptr = lptr->lnext;
		lptr->lnext = create_line(s);
		if (lptr->lnext == (LINE *)NULL)
			return (LINE *)NULL;
		lptr->lnext->lnext = (LINE *)NULL;
	}

	/* Success! */
	return lhead;
} /* End append_line() */

/*
** dup_lines:
** Makes a duplicate of a list of lines.
**
** Parameters:
**	Name	Description
**	----	-----------
**	lorg	Pointer to first LINE to duplicate.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	NULL	Error occurred (i.e. out of memory).
**	other	Pointer to first LINE in duplicate list.
*/
LINE *
dup_lines(lorg)
	LINE	*lorg;
{
	LINE	*lnew;
	LINE	*lorgnext;
	LINE	*lnewnext = (LINE *)NULL;

	/* Check for empty list. */
	if (lorg == (LINE *)NULL)
		return (LINE *)NULL;

	/* Process each line in the original list. */
	lorgnext = lorg;
	while (lorgnext != (LINE *)NULL)
	{
		if (lnewnext == (LINE *)NULL)
		{
			/* This is first item in new list. */
			lnew = create_line(lorgnext->ldata);
			if (lnew == (LINE *)NULL)
				return (LINE *)NULL;
			lnewnext = lnew;
		}
		else
		{
			/* This is next item in new list. */
			lnewnext->lnext = create_line(lorgnext->ldata);
			if (lnewnext->lnext == (LINE *)NULL)
			{
				free_lines(lnew);
				return (LINE *)NULL;
			}
			lnewnext = lnewnext->lnext;
		}

		/* Step to next line in original list. */
		lorgnext = lorgnext->lnext;
	}

	return lnew;
} /* End dup_lines() */

/*
** cindex:
** Finds the first occurrence of a character in a string.
**
** Parameters:
**	Name	Description
**	----	-----------
**	s	String to search.
**	c	Character to search for.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	-1	Specified character not found in string.
**	other	Index of character in string.
*/
int
cindex(s, c)
	char	*s;
	char	c;
{
	int	i = 0;

	while (s[i])
	{
		if (s[i] == c)
			return i;
		i++;
	}

	return -1;
} /* End cindex() */

/*
======================================================================
End makeutil.c
======================================================================
*/
