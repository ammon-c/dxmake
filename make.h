/*
======================================================================
make.h
Include file for make utility

This source file is part of a computer program that is
(C) Copyright 1988 Ammon R. Campbell.  All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES

This include file contains constants, data types, variables,
and function prototypes which are considered global to the
make utility.  Constants, data types, variables, and function
prototypes that are local to a particular module are declared
in that module.

======================================================================
*/

/*
** The symbol 'MAIN' is defined in the main module, which causes the
** global variables in this file to be declared in the main module
** and declared as external in the other modules.
*/
#ifdef MAIN
#define EXT /* */
#else
#define EXT extern
#endif

/******************************* INCLUDES ***************************/

#include "makemsg.h"

/******************************* CONSTANTS **************************/

/*
** If BT_TARGETS is defined, the target list will be stored in a
** binary tree; otherwise, a linked list will be used.
*/
/*
#define BT_TARGETS
*/

/* Maximum length for filenames and macro names (including the ending null). */
#define MAXPATH		128

/* Maximum length for logical input lines (including the ending null). */
#define MAXLLINE	4096

/* Maximum length for filename suffixes (not including '.' or ending null). */
#define MAX_SUFFIX_STR	3

/* Value for errmsg() function to indicate no integer display. */
#define NOVAL		32767

/*
** Bit positions for makeflags.
*/

/* 1 = build all targets, even if not out-of-date. */
#define FLAG_BUILD_ANYWAY	1

/* 1 = enable debug output. */
#define FLAG_DEBUG		2

/* 1 = override defined macros with environment strings. */
#define FLAG_ENV_OVERRIDE	4

/* 1 = ignore return codes from subprocesses. */
#define FLAG_IGNORE		8

/* 1 = don't run subprocesses; just display them. */
#define FLAG_NOSPAWN		16

/* 1 = enable table information output. */
#define FLAG_SHOW_INFO		32

/* 1 = just test if target(s) are out of date. */
#define FLAG_QUERY		64

/* 1 = don't read 'make.inf' file. */
#define FLAG_NO_DEFAULTS	128

/* 1 = don't display subprocesses. */
#define FLAG_NO_SHOW		256

/* 1 = just touch out-of-date files; don't run subprocesses. */
#define FLAG_TOUCH		512

/* 1 = target time must be > dependents; 0 = target must be >= dependents. */
#define FLAG_NEEDNEWER		1024

/* 1 = display sign-on message. */
#define FLAG_SIGNON		2048

/******************************* MACROS *****************************/

#define SETFLAG(f)		(makeflags |= (f))
#define CHKFLAG(f)		(makeflags & (f))

/******************************* TYPES ******************************/

/* mstat_t data structure for file time/date stamps. */
struct mstat_t
{
	unsigned long	st_mtime; /* Time/date stamp of file. */
};

/* LINE data structure for linked lists of lines. */
struct line_s
{
	char		*ldata;	/* Text of line. */
	struct line_s	*lnext;	/* Pointer to next line of text. */
};
typedef struct line_s LINE;

/* MACRO data structure for linked list of macros. */
struct macro_s
{
	char		*mname;	/* Name of macro. */
	char		*mexp;	/* Text of macro expansion. */
	struct macro_s	*mnext;	/* Pointer to next macro in list. */
};
typedef struct macro_s MACRO;

/* RULE data structure for linked list of rules. */
struct rule_s
{
	char		*rsrc;		/* Source file extension of rule. */
	char		*rdest;		/* Dest file extension of rule. */
	LINE		*rcommands;	/* Commands to build target. */
	struct rule_s	*rnext;		/* Pointer to next rule in list. */
};
typedef struct rule_s RULE;

#ifdef BT_TARGETS
/* TARGET data structure for binary tree of targets. */
struct target_s
{
	char		*tname;		/* Name of target file. */
	char		*tdependents;	/* Target's dependent files. */
	LINE		*tcommands;	/* Commands to build the target. */
	struct target_s	*tleft;		/* Pointer to next target in tree. */
	struct target_s	*tright;	/* Pointer to next target in tree. */
};
typedef struct target_s TARGET;
#else
/* TARGET data structure for linked list of targets. */
struct target_s
{
	char		*tname;		/* Name of target file. */
	char		*tdependents;	/* Target's dependent files. */
	LINE		*tcommands;	/* Commands to build the target. */
	struct target_s	*tnext;		/* Pointer to next target in list. */
};
typedef struct target_s TARGET;
#endif /* BT_TARGETS */

/******************************* VARIABLES **************************/

/* inpline:  Buffer for logical lines read from the makefile. */
EXT char *inpline;

/* makeflags:  Flag values that tell make how to operate. */
EXT unsigned int makeflags;

/* cmdflags:  Flag values that were set on the command line. */
EXT unsigned int cmdflags;

/* mkenvp:  Pointer to the program's environment strings. */
EXT char **mkenvp;

/* mkname:  Pointer to program's name from argv[0]. */
EXT char *mkname;

/******************************* HEADERS ****************************/

/* Functions that are 'exported' from modules are declared here. */

/* From make.c: */
#ifdef WIN
int	dmain(int argc, char *argv[], char *envp[]);
#else
int	main(int argc, char *argv[], char *envp[]);
#endif /* WIN */

/* From makeutil.c: */
int	get_part_filename(int part, char *src, char *dest);
char	*mutoa(short unsigned int val);
char	*multoa(long unsigned int val);
int	touch_file(char *);
void	mputs(char *s);
#ifndef WIN
int	mchdir(char *dname);
#else
int	mchdir(LPSTR dname);
#endif /* WIN */
int	mopen_r(char *);
int	mclose(int);
int	mread(int, char *, int);
int	mstat(char *fname, struct mstat_t *tstat);
char	*enumpath(int flag, char *envvar);
void	errmsg(char *msg, char *sval, int dval);
void	free_lines(LINE *);
LINE	*append_line(LINE *lhead, char *s);
LINE	*create_line(char *s);
LINE	*dup_lines(LINE *lorg);
int	cindex(char *s, char c);

/* From makein.c: */
int	read_logical_line(int handle, char *str, int maxlen);
int	unread_logical_line(char *str);
int	process_makefile(char *mf);

/* From makexpnd.c: */
int	expand_named_macros(char *src, char *dest, int maxlen);
LINE	*expand_dependents(TARGET *tar);
LINE	*expand_wildcard(char *fspec);
int	expand_tarspecial(char *src, char *dest, int maxlen,
			char *tfile, char *sfile);

/* From makebld.c: */
int	make_target(char *tname, int level, time_t *hitime);

/* From maketar.c: */
void	init_targets(void);
void	flush_targets(void);
void	dump_targets(void);
int	define_target(char *line, int handle);
TARGET	*find_target(char *tname);
char	*default_target(void);

/* From makerul.c: */
void	init_rules(void);
void	flush_rules(void);
void	dump_rules(void);
int	define_rule(char *line, int handle);
int	check_rules(char *suf);
RULE	*lookup_rule(char *src, char *dest);

/* From makemac.c: */
void	init_macros(void);
void	flush_macros(void);
void	dump_macros(void);
int	predefine_macros(void);
int	define_macro(char *line, int handle);
char	*find_macro(char *mname);

/* From makemem.c: */
int	mem_init(void);
int	mem_deinit(void);
void	*mem_alloc(size_t bytes);
void	mem_free(void *ptr);
void	mem_heapmin(void);

/* From makeprec.c: */
void	init_precious(void);
void	flush_precious(void);
void	dump_precious(void);
int	do_precious(char *line);
int	is_precious(char *tname);

/* From makesuf.c: */
void	init_suffixes(void);
void	flush_suffixes(void);
void	dump_suffixes(void);
int	do_suffixes(char *line);
char	*enum_suffix(int index);

#ifdef WIN
/* From makew.c: */
void	wputc(unsigned char ch);
int	wrun(unsigned char *cmd);
int	check_abort(void);
#endif /* WIN */

