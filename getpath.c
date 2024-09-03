/*
=================================================================
getpath.c

Routine to get the path portion from a filespec string.

This source file is part of a computer program that is
(C) Copyright 1985-1988, 1990 by Ammon R. Campbell.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
=================================================================
*/

#include "getpath.h"

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
void
getpath(filespec)
        char    *filespec;
{
        int     found = -1;
        int     pos = 0;

        while (filespec[pos])
        {
                if (filespec[pos] == '\\')
                {
                        found = pos+1;
                }
                else if (filespec[pos] == ':' && filespec[pos+1] != '\\')
                {
                        found = pos+1;
                }
                pos++;
        }

        /*
        ** If separator wasn't found, then this filespec
        ** probably doesn't have a path prepended to
        ** it.
        */
        if (found == -1)
                *filespec = '\0';
        else
        {
                filespec[found] = '\0';
                if (filespec[found-1] != '\\' && filespec[found-1] != ':')
                {
                        filespec[found-1] = '\\';
                        filespec[found] = '\0';
                }
        }
}

