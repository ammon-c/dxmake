/*
==========================================================================
cvtslash.c
C function to replace slashes with backslashes.  Useful for changing
UNIX-style filespecs to MS-DOS-style filespecs.

By Ammon R. Campbell.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
==========================================================================
*/

#include "cvtslash.h"

/*
** cvt_slash:
** Converts all occurrences of a slash ('/') in a string to a
** backslash ('\').
**
** Parameters:
**	Name	Description
**	----	-----------
**	str	String to be converted.
**
** Returns:
**	NONE
*/
void
cvt_slash(str)
	char	*str;
{
	while (*str)
	{
		if (*str == '/')
		{
			*str = '\\';
		}
		str++;
	}
}

