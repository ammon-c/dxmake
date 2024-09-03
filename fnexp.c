/*
=========================================================================
fnexp.c
Routine for parsing limited regular expressions for filenames.

This source file is part of a computer program that is
Copyright 1988 by Ammon R. Campbell.  All rights reserved.
Unauthorized duplication, distribution, or use is prohibited
by applicable laws.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
=========================================================================
NOTES

Rules for limited regular expressions:

        *       Matches any string of characters.

        ?       Matches any single character.

        [nnn]   Matches any single character out of the set `nnn'.

        [n-m]   Matches any single character between `n' and `m',
                inclusive.

//      \n      Matches the character `n', where `n' may be a normal
//              character, or may be one of the special characters
//              `*', `?', or `['.

Any other character matches itself.
=========================================================================
*/

#include <string.h>
#include "fnexp.h"

/*
** check_rexp:
** Checks to see if the specified string matches the specified
** limited regular expression.
**
** Parameters:
**      Value   Meaning
**      -----   -------
**      str     String to be tested.
**      rexp    Limited regular expression to test with.
**
**      Value   Meaning
**      -----   -------
**      1       String matched specified expression.
**      0       String doesn't match specified expression.
**      -1      Error in expression.
*/
int
check_rexp(str, rexp)
        char    *str;
        char    *rexp;
{
        int     s = 0;          /* Current position in str. */
        int     r = 0;          /* Current position in rexp. */
        int     tmp, tmp2;      /* Temporary variable. */

        /* In MS-DOS, case doesn't matter in filenames. */
        strlwr(str);
        strlwr(rexp);

        /* Compare each token of rexp to str. */
        while (str[s] || rexp[r])
        {
                if (rexp[r] == '[')
                {
                        /* `[' is beginning of a group of characters. */

                        /* Skip `[' */
                        r++;

                        tmp = 0;
                        while (rexp[r] != ']' && rexp[r])
                        {
                                if (rexp[r+1] == '-')
                                {
                                        if (str[s] >= rexp[r] &&
                                                str[s] <= rexp[r+2])
                                        {
                                                tmp = 1;
                                        }
                                        r += 3;
                                }
                                else if (rexp[r] == str[s])
                                {
                                        tmp = 1;
                                        r++;
                                }
                                else
                                {
                                        r++;
                                }
                        }

                        if (rexp[r] != ']')
                        {
                                /* No ']' at end of expression. */
                                return -1;
                        }
                        r++;

                        if (!tmp)
                        {
                                /* String char didn't match group. */
                                return 0;
                        }

                        if (str[s])
                                s++;
                }
                else if (rexp[r] == '?')
                {
                        /* `?' means we don't care what this character is. */

                        /* Skip `[' */
                        r++;

                        if (str[s])
                                s++;
                }
                else if (rexp[r] == '*')
                {
                        /*
                        ** `*' means we don't care what str is up until
                        ** the next matching character.
                        */

                        /* Skip `*' */
                        r++;

                        if (rexp[r] == '*' || rexp[r] == '?')
                        {
                                /* Can't have multiple wildcards in a row. */
                                return -1;
                        }

#if 0
                        /*
                        ** If no characters are left in string, then
                        ** error, because * must match something.
                        */
                        if (!str[s])
                                return 0;
#endif /* 0 */

                        /*
                        ** If no characters follow the `*' in the
                        ** expression, then the string automatically
                        ** matches.
                        */
                        if (!rexp[r])
                        {
                                /* Force match `*' */
                                return 1;
                        }

                        if (str[s]
#if 1
                                && str[s] != rexp[r]
#endif
                                )
                        {
                                /* `*' must skip at least one character. */
                                s++;
                        }
#if 0
                        else
                        {
                                /* End of string, but not end of rexp. */
                                return 0;
                        }
#endif

                        /*
                        ** Search for last match of character following
                        ** the `*'.
                        */
                        tmp2 = -1;
                        tmp = s;
                        while (str[tmp])
                        {
                                if (str[tmp] == rexp[r])
                                        tmp2 = tmp;
                                tmp++;
                        }

                        if (tmp2 != -1)
                        {
                                s = tmp2;
                        }

                        if (str[s] != rexp[r])
                                return 0;

                        if (str[s])
                                s++;
                        if (rexp[r])
                                r++;
                }
                else
                {
                        /* Regular character. */
                        if (str[s] != rexp[r])
                        {
                                /* Current characters don't match. */
                                return 0;
                        }
                        if (rexp[r])
                                r++;
                        if (str[s])
                                s++;
                }
        }

        return 1;
}

