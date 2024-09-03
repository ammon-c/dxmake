/*
=================================================================
getpath.h

Include file for "getpath.c" and calling applications.
Routine to get the path portion of a filespec.

This source file is part of a computer program that is
(C) Copyright 1985-1988, 1990 by Ammon R. Campbell.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
=================================================================
*/

/*
** getpath:
** Deletes the filename and extension from a filespec.
**
** Parameters:
**      Value           Meaning
**      -----           -------
**      filespec        Filespec to be modified.
**
** Returns:
**      NONE
*/
void getpath(char *filespec);

