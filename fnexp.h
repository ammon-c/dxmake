/*
=========================================================================
fnexp.h
Include file for "fnexp.c" and calling applications.
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

        \n      Matches the character `n', where `n' may be a normal
                character, or may be one of the special characters
                `*', `?', or `['.

Any other character matches itself.
=========================================================================
*/

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
int     check_rexp(char *str, char *rexp);

