/*
======================================================================
makemem.c
Memory handling routines for make utility.

This source file is part of a computer program that is
(C) Copyright 1985-1988, 1990, 1992 Ammon R. Campbell.
All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES

The routines in this module insulate the application from the
environment specific details of memory handling.

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
#include <malloc.h>

#include "make.h"

/****************************** FUNCTIONS ***************************/

/*
** mem_init:
** Initializes the memory management module.
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
int
mem_init(void)
{
	return 1;
}

/*
** mem_deinit:
** Shuts down the memory management module.
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
int
mem_deinit(void)
{
	return 1;
}

/*
** mem_alloc:
** Allocates a block of memory.
**
** Parameters:
**	Name	Description
**	----	-----------
**	bytes	Number of bytes of memory to allocate.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	NULL	An error occurred (i.e. not enough memory left).
**	other	Pointer to allocated memory.
*/
void *
mem_alloc(size_t bytes)
{
#ifndef WIN
	return malloc(bytes);
#else
	/*
	** The memory type LPTR causes LocalAlloc to return
	** a fixed near pointer instead of a memory handle.
	*/
	return (void *)LocalAlloc(LPTR, bytes);
#endif /* WIN */
}

/*
** mem_free:
** Frees a block of memory previously allocated by mem_alloc().
**
** Parameters:
**	Name	Description
**	----	-----------
**	ptr	Pointer to memory block to be freed.
**
** Returns:
**	NONE
*/
void
mem_free(void *ptr)
{
#ifndef WIN
	free(ptr);
#else
	/*
	** Free a pointer allocated as type "LPTR" by LocalAlloc.
	*/
	LocalFree((HLOCAL)ptr);
#endif /* WIN */
}

/*
** mem_heapmin:
** Releases any unused heap memory back to the system.
** This is called before spawning a subprocess to give
** it the maximum amount of free memory.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
void
mem_heapmin(void)
{
#ifndef WIN
	/*
	** Before we execute the subprocess, minimize our heap so
	** the subprocess will have as much free memory as we can
	** afford.  The _heapmin() function is documented abysmally
	** in the Microsoft C documentation, but it looks like the
	** heap gets extended back out if we try to allocate memory
	** again later, which is what we want; it would be nice to
	** know if that's the way it really works though, otherwise
	** we could run into trouble with memory someday.
	*/
	_heapmin();
#endif /* WIN */
}

