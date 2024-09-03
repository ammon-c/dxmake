/*
====================================================================
touch.c
Change file timestamp utility

(C) Copyright 1985-1988, 1990, 1992 Ammon R. Campbell.
All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
====================================================================
Description:

This program changes the timestamp on a file or files.
====================================================================
*/

/***************************** INCLUDES ***************************/

#ifdef WIN
# include <windows.h>
#endif /* WIN */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <dos.h>
#include <io.h>
#include <string.h>
#include <direct.h>

#include "touchmsg.h"
#include "touchcid.h"
#include "cvtslash.h"
#include "getpath.h"
#include "fnexp.h"
#include "wild.h"

/**************************** HEADERS *****************************/

#ifdef WIN
int PASCAL	WinMain(HANDLE hInstance, HANDLE hPrevInstance,
			LPSTR lpszCmdLine, int nCmdShow);
long FAR PASCAL	main_window_proc(HWND hwnd, unsigned message,
			WPARAM wparam, LPARAM lparam);
BOOL FAR PASCAL	startup_dlg_proc(HWND hdlg, unsigned message,
			WPARAM wparam, LPARAM lparam);
BOOL FAR PASCAL	about_dlg_proc(HWND hdlg, unsigned message,
			WPARAM wparam, LPARAM lparam);
int		check_abort(void);
int		show_help(HWND hwnd, WORD type, DWORD data);
#endif /* WIN */

static void	errpause(void);
static void	errmsg(char *msg, char *sval);
static void	mputs(char *s);
#ifndef WIN
static int	mchdir(char *dname);
#else
static int	mchdir(LPSTR dname);
#endif /* WIN */
static int	newtime(char *str);
static int	newdate(char *str);
static int	touch_file(char *filespec,
		unsigned short int date, unsigned short int ttime);
static void	usage(void);
#ifdef WIN
int	dmain(int argc, char **argv);
#else
int	main(int argc, char **argv);
#endif /* WIN */

/**************************** CONSTANTS ***************************/

#define MODULE_NAME	"TOUCH"
#define HELPFILE_NAME	"DXMAKE.HLP"

#ifdef WIN
# define MAXARGS	20
# define DBGMSG(s)	MessageBox(0, (LPSTR)(s), \
				(LPSTR)"debug", MB_OK | MB_TASKMODAL);
#endif /* WIN */

#define MAXPATH		128	/* Maximum length for path/filename. */

/**************************** VARIABLES ***************************/

#ifdef WIN

/* hmainwnd:  Window handle of the application's main window. */
static HWND	hmainwnd;

/* hinstance:  Instance handle of the application. */
static HANDLE	hinstance;

/* charx:  Current horizontal position to draw character at. */
static int	charx;

/* chary:  Current vertical position to draw character at. */
static int	chary;

/* aborted:  This flag is zero unless the user aborts the make. */
static int	aborted;

/*
** cmd_args:  String containing the command line arguments passed
** to us.
*/
unsigned char cmd_args[MAXPATH];

#endif /* WIN */

unsigned short	hour, minute, second, year, month, day;
unsigned short	date, ttime;
char		flag_nocreate;

/**************************** FUNCTIONS ***************************/

#ifdef WIN

/*
** WinMain:
** Microsoft Windows application entry point.
*/
int PASCAL
WinMain(HANDLE hInstance, HANDLE hPrevInstance,
	LPSTR lpszCmdLine, int nCmdShow)
{
	char		margc;
	char		argbfr[MAXPATH];
	char		*margv[MAXARGS];
	int		i;
	int		result;
	MSG		msg;
	HWND		hwnd;
	HMENU		hmenu;
	WNDCLASS	wc;
	FARPROC		dlg_proc;

	/* Save the instance handle. */
	hinstance = hInstance;

	/* Trim whitespace from command arguments. */
	while (*lpszCmdLine == ' ' || *lpszCmdLine == '\t')
		lpszCmdLine++;

	/*
	** If there are no command line arguments, then
	** ask the user what to do.
	*/
	lstrcpy((LPSTR)cmd_args, (LPSTR)lpszCmdLine);
	if (lpszCmdLine[0] == '\0')
	{
		/* Let user change options from dialog. */
		dlg_proc = MakeProcInstance(
				startup_dlg_proc,
				hinstance);
		if ((i = DialogBox(hinstance,
			"STARTUP_DLG_TEMPLATE",	/* Resource. */
			NULL,			/* Parent. */
			dlg_proc))		/* Address. */
				== -1)
		{
			MessageBox(0, (LPSTR)MSG_ERR_CREATEDLG,
				(LPSTR)MSG_APPNAME, MB_OK | MB_TASKMODAL);
		}
		FreeProcInstance(dlg_proc);
		if (i != IDOK)
		{
			/* User aborted startup dialog or error occurred. */
			return 1;
		}
	}

	/* Check if we need to do one-time initializations. */
	if (hPrevInstance == NULL)
	{
		/* Create our application's window class. */
		wc.style = 0;
		wc.lpfnWndProc = main_window_proc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(hInstance, MSG_APPNAME);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = GetStockObject(WHITE_BRUSH);
		wc.lpszMenuName = (LPSTR)NULL;
		wc.lpszClassName = (LPSTR)MSG_APPNAME;
		if (!RegisterClass(&wc))
		{
			/* Couldn't register window class. */
			MessageBox(0, (LPSTR)MSG_ERR_REGCLASS,
				(LPSTR)MSG_APPNAME, MB_OK | MB_TASKMODAL);
			return 0;
		}
	}

	/* Create our application's window. */
	hwnd = CreateWindow(
			(LPSTR)MSG_APPNAME,
			(LPSTR)MSG_APPNAME,
			WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION |
			WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX |
			WS_MAXIMIZEBOX,
			CW_USEDEFAULT,
			nCmdShow,
			GetSystemMetrics(SM_CXSCREEN) -
				GetSystemMetrics(SM_CXSCREEN) / 16,
			GetSystemMetrics(SM_CYSCREEN) / 4,
			NULL,
			NULL,
			hInstance,
			NULL);
	if (hwnd == NULL)
	{
		/* Couldn't create main window. */
		MessageBox(0, (LPSTR)MSG_ERR_CREATEWIN,
			(LPSTR)MSG_APPNAME, MB_OK | MB_TASKMODAL);
		return 0;
	}

	/* Add the "About" item to the control-menu. */
	hmenu = GetSystemMenu(hwnd, FALSE);
	AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hmenu, MF_STRING, IDM_ABOUT, MSG_MABOUT);

	/* Save the window handle for later use. */
	hmainwnd = hwnd;

	/* Initial drawing position is upper left. */
	charx = 0;
	chary = 0;

	/* User has not aborted (yet). */
	aborted = 0;

	/*
	** Flush any messages waiting in our queue before we tie
	** up the system.
	*/
	while (!aborted && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage((LPMSG)&msg);
		DispatchMessage((LPMSG)&msg);
	}

	/* Split command line into individual arguments for main(). */
	i = 0;
	while (cmd_args[i] == ' ' || cmd_args[i] == '\t')
		i++;
	lstrcpy((LPSTR)argbfr, (LPSTR)&cmd_args[i]);
	i = 0;
	margv[0] = "";		/* Windows doesn't give us an argv[0]. */
	margv[1] = &argbfr[0];
	margc = 1;
	while (argbfr[i] != '\0')
	{
		margc++;
		while (argbfr[i] != ' ' &&
			argbfr[i] != '\t' &&
			argbfr[i] != '\0')
		{
			i++;
		}
		if (argbfr[i] == '\0')
			break;
		argbfr[i++] = '\0';

		while (argbfr[i] == ' ' ||
			argbfr[i] == '\t')
		{
			i++;
		}
		margv[margc] = &argbfr[i];

		if (margc >= MAXARGS)
		{
			/* Too many arguments in command.  Complain. */
			MessageBox(0, (LPSTR)MSG_ERR_MAXARGS,
				(LPSTR)MSG_APPNAME, MB_OK | MB_TASKMODAL);
			return 0;
		}
	}

	/* Run the main() function. */
	result = dmain(margc, margv);

	/* Process remaining messages until we're all finished. */
	while (!aborted && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage((LPMSG)&msg);
		DispatchMessage((LPMSG)&msg);
	}

	return result;
} /* End WinMain() */

/*
** MainWndProc:
** Main message handler for this application.
**
** Parameters:
**	Name	Description
**	----	-----------
**	hwnd	Window handle of recipient window.
**	message	Type of message.
**	wparam	Message parameters.
**	lparam	Additional message parameters.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	FALSE	Message was used.
**	other	Message was passed on to DefWindowProc().
*/
long FAR PASCAL
main_window_proc(hwnd, message, wparam, lparam)
	HWND		hwnd;
	unsigned	message;
	WPARAM		wparam;
	LPARAM		lparam;
{
	FARPROC		dlg_proc;

	/* Do the right thing depending on what message we get. */
	switch (message)
	{
		case WM_DESTROY:
			/*
			** The window is being destroyed.
			** Post a quit message to our message dispatcher.
			*/
			aborted = 1;
			PostQuitMessage(0);
			break;

		case WM_COMMAND:
		case WM_SYSCOMMAND:
			if (wparam == IDM_ABOUT)
			{
				/*
				** The user selected the "About"
				** menu item.
				*/
				dlg_proc = MakeProcInstance(
						about_dlg_proc,
						hinstance);
				if (DialogBox(hinstance,
					"ABOUT_DLG_TEMPLATE",	/* Resource. */
					hwnd,			/* Parent. */
					dlg_proc)		/* Address. */
						== -1)
				{
					MessageBox(0, (LPSTR)MSG_ERR_CREATEDLG,
						(LPSTR)MSG_APPNAME,
						MB_OK | MB_TASKMODAL);
				}
				FreeProcInstance(dlg_proc);
			}
			break;

		default:
			/*
			** We got a message we aren't interested
			** in, so let Windows handle it for us.
			*/
			return (DefWindowProc(hwnd, message, wparam, lparam));
	} /* End switch(...) */

	return NULL;
} /* End MainWndProc() */

/*
** wputc:
** Outputs a character to the application's window as if it
** were a dumb teletype output device.
**
** Parameters:
**	Name	Description
**	----	-----------
**	ch	Character to be output.
**
** Returns:
**	NONE
*/
void
wputc(unsigned char ch)
{
	HDC	dc;		/* Display context of app's window. */
	HFONT	oldfont;	/* Handle of original font. */
	HFONT	ourfont;	/* Handle of the font we use. */
	TEXTMETRIC tm;		/* Metrics of our font. */
	int	cwidth;		/* Width of one character in pixels. */
	int	cheight;	/* Height of one character in pixels. */
	RECT	crect;		/* Client area of our window. */
	char	stmp[2];	/* String buffer for character. */

	/* Set up to draw with fixed-pitch font in our window. */
	ourfont = GetStockObject(OEM_FIXED_FONT);
	dc = GetDC(hmainwnd);
	oldfont = SelectObject(dc, ourfont);
	GetTextMetrics(dc, &tm);
	cwidth = tm.tmMaxCharWidth;
	cheight = tm.tmHeight;

	/* Rationalize the drawing position. */
	GetClientRect(hmainwnd, &crect);
	if (charx >= (crect.right - crect.left + 1) / cwidth)
	{
		charx = 0;
		chary++;
	}
	if (chary >= (crect.bottom - crect.top + 1) / cheight)
	{
		ScrollWindow(hmainwnd, 0, -cheight, NULL, NULL);
		UpdateWindow(hmainwnd);
		chary = (crect.bottom - crect.top + 1) / cheight - 1;
	}

	/* Draw the character. */
	if (ch == 0x0A)
	{
		/* Line feed. */
/* Enable this code if a line-feed generates a carriage-return/line-feed. */
#if 1
		charx = 0;
#endif
		chary++;
	}
	else if (ch == 0x0D)
	{
		/* Carriage return. */
		charx = 0;
	}
	else if (ch == 9)
	{
		/* Tab. */
		do
		{
			charx++;
		}
		while (charx % 8);
	}
	else
	{
		/* Normal character. */
		stmp[0] = ch;
		stmp[1] = '\0';
		TextOut(dc, charx * cwidth, chary * cheight, (LPSTR)stmp, 1);
		charx++;
	}

	/* Put the window back the way it was. */
	SelectObject(dc, oldfont);
	ReleaseDC(hmainwnd, dc);
} /* End wputc() */

/*
** check_abort:
** Checks to see if the program has been aborted by the user.
**
** Parameters:
**	NONE
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	The user has aborted the make.
**	0	The user has not aborted the make.
*/
int
check_abort(void)
{
	if (aborted)
		return 1;
	return 0;
} /* End check_abort() */

/*
** startup_dlg_proc:
** Message handling callback routine for the primary dialog.
**
** Parameters:
**	Standard dialog callback parameters.
**
** Returns:
**	Standard dialog callback return value.
*/
BOOL FAR PASCAL
startup_dlg_proc(HWND hdlg, unsigned message, WPARAM wparam, LPARAM lparam)
{
	HMENU	hmenu;
	FARPROC	dlg_proc;
	char	stmp[MAXPATH];
	int	i;

	switch (message)
	{
		case WM_INITDIALOG:
			/* Add the "About" item to the control-menu. */
			hmenu = GetSystemMenu(hdlg, FALSE);
			AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
			AppendMenu(hmenu, MF_STRING, IDM_ABOUT, MSG_MABOUT);
			break;

		case WM_SYSCOMMAND:
			if (wparam == IDM_ABOUT)
			{
				/*
				** The user selected the "About"
				** menu item.
				*/
				dlg_proc = MakeProcInstance(
						about_dlg_proc,
						hinstance);
				if (DialogBox(hinstance,
					"ABOUT_DLG_TEMPLATE",	/* Resource. */
					hdlg,			/* Parent. */
					dlg_proc)		/* Address. */
						== -1)
				{
					MessageBox(0, (LPSTR)MSG_ERR_CREATEDLG,
						(LPSTR)MSG_APPNAME,
						MB_OK | MB_TASKMODAL);
				}
				FreeProcInstance(dlg_proc);
			}
			break;

		case WM_COMMAND:
			/*
			** User selected a control.  IDOK indicates
			** that the user clicked the "OK" button.
			** IDCANCEL indicates that the user clicked
			** the system menu close command.
			*/
			if (wparam == IDOK)
			{
				cmd_args[0] = '\0';

				/* Check if 'don't force create' is enabled. */
				if (IsDlgButtonChecked(hdlg, IDB_NOCREAT))
				{
					cmd_args[i] = ' ';
					cmd_args[i + 1] = '-';
					cmd_args[i + 2] = OPT_NOCREATE;
					cmd_args[i + 3] = '\0';
				}

				/* Get time, if any. */
				if (GetDlgItemText(hdlg, IDE_TIME,
					(LPSTR)stmp, MAXPATH - 1) > 0)
				{
					i = lstrlen((LPSTR)cmd_args);
					cmd_args[i] = ' ';
					cmd_args[i + 1] = '-';
					cmd_args[i + 2] = OPT_TIME;
					cmd_args[i + 3] = '\0';
					lstrcat((LPSTR)cmd_args, (LPSTR)stmp);
				}

				/* Get date, if any. */
				if (GetDlgItemText(hdlg, IDE_DATE,
					(LPSTR)stmp, MAXPATH - 1) > 0)
				{
					i = lstrlen((LPSTR)cmd_args);
					cmd_args[i] = ' ';
					cmd_args[i + 1] = '-';
					cmd_args[i + 2] = OPT_DATE;
					cmd_args[i + 3] = '\0';
					lstrcat((LPSTR)cmd_args, (LPSTR)stmp);
				}

				/* Get name of working directory, if any. */
				if (GetDlgItemText(hdlg, IDE_SRCDIR,
					(LPSTR)stmp, MAXPATH - 1) > 0)
				{
					i = lstrlen((LPSTR)cmd_args);
					cmd_args[i] = ' ';
					cmd_args[i + 1] = '-';
					cmd_args[i + 2] = OPT_WORKINGDIR;
					cmd_args[i + 3] = '\0';
					lstrcat((LPSTR)cmd_args, (LPSTR)stmp);
				}

				/* Get list of filenames, if any. */
				if (GetDlgItemText(hdlg, IDE_FILENAMES,
					(LPSTR)stmp, MAXPATH - 1) > 0)
				{
					lstrcat((LPSTR)cmd_args, (LPSTR)" ");
					lstrcat((LPSTR)cmd_args, (LPSTR)stmp);
				}
				EndDialog(hdlg, IDOK);
			}
			else if (wparam == IDCANCEL)
			{
				EndDialog(hdlg, IDCANCEL);
			}
			else if (wparam == IDB_HELP)
			{
				show_help(hdlg, HELP_INDEX, 0L);
			}
			break;

		default:
			/* Ignore this message. */
			;
	}

	return FALSE;
} /* End startup_dlg_proc() */

/*
** about_dlg_proc:
** Message handling callback routine for the "About" dialog.
**
** Parameters:
**	Standard dialog callback parameters.
**
** Returns:
**	Standard dialog callback return value.
*/
BOOL FAR PASCAL
about_dlg_proc(HWND hdlg, unsigned message, WPARAM wparam, LPARAM lparam)
{
	switch (message)
	{
		case WM_COMMAND:
			/*
			** User selected a control.  IDOK indicates
			** that the user clicked the "OK" button.
			** IDCANCEL indicates that the user clicked
			** the system menu close command.
			*/
			if (wparam == IDOK)
			{
				EndDialog(hdlg, IDOK);
			}
			else if (wparam == IDCANCEL)
			{
				EndDialog(hdlg, IDCANCEL);
			}
			break;

		default:
			/* Ignore this message. */
			;
	}

	return FALSE;
} /* End about_dlg_proc() */

/*
** show_help:
** Invokes the Windows help engine to display information
** from the application's help file.
**
** Parameters:
**	Name	Description
**	----	-----------
**	hwnd	Window handle of window requesting help.
**	type	Type of help requested (see the wCommand
**		parameter of the WinHelp function in the
**		Windows SDK reference for more info).
**	data	Context/key identifier (see the dwData
**		parameter of the WinHelp function in the
**		Windows SDK reference for more info).
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Successful.
**	0	Error occurred.
*/
int
show_help(HWND hwnd, WORD type, DWORD data)
{
	HANDLE	htmp;		/* Temporary module handle. */
	int	i;		/* Loop index. */
	char	stmp[MAXPATH];	/* Temporary pathname. */

	/*
	** Build the pathname of the help file.  The help file must
	** be in the same directory as the application's EXE file.
	*/

	/* Get module handle of this module. */
	htmp = GetModuleHandle(MODULE_NAME);
	if (htmp == NULL)
	{
		/* Couldn't get module handle. */
		MessageBox(0, (LPSTR)MSG_ERR_MODULENAME,
			(LPSTR)MSG_APPNAME, MB_OK | MB_TASKMODAL);
		return 0;
	}

	/* Get pathname of this module's EXE file. */
	GetModuleFileName(htmp, (LPSTR)stmp, MAXPATH - 1);

	/* Remove filename from module's path. */
	i = lstrlen((LPSTR)stmp);
	while (i > 0)
	{
		if (stmp[i] == '\\')
		{
			stmp[i] = '\0';
			break;
		}
		else if (stmp[i] == ':')
		{
			stmp[i + 1] = '\0';
			break;
		}
		i--;
	}
	stmp[i] = '\0';
	if (i > 0 && stmp[i - 1] != '\\' && stmp[i - 1] != ':')
	{
		stmp[i] = '\\';
		stmp[i + 1] = '\0';
	}

	/* Add helpfile name to directory name. */
	lstrcat((LPSTR)stmp, (LPSTR)HELPFILE_NAME);

	/* Invoke the help engine. */
	if (!WinHelp(hwnd, (LPSTR)stmp, (WORD)type, (DWORD)data))
	{
		/* Couldn't get help. */
		MessageBox(0, (LPSTR)MSG_ERR_NOHELP,
			(LPSTR)MSG_APPNAME, MB_OK | MB_TASKMODAL);
		return 0;
	}

	return 1;
} /* End show_help() */

#endif /* WIN */

/*
** errpause:
** Tells the user to press a key and then waits for the
** user to press a key.  In the MS-DOS version of the
** program, this does nothing.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
static void
errpause(void)
{
#ifdef WIN
	MSG	msg;

	/* Display message. */
	mputs(MSG_PRESSAKEY);

	/* Pause until user presses a key. */
	msg.message = 0;
	while (msg.message != WM_KEYDOWN && GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage((LPMSG)&msg);
		DispatchMessage((LPMSG)&msg);
	}
#endif /* WIN */
} /* End errpause() */

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
**
** Returns:
**	NONE
*/
void
errmsg(msg, sval)
	char	*msg;
	char	*sval;
{
	mputs(MSG_ERRMSG);
	mputs(msg);
	if (sval != (char *)NULL)
	{
		mputs(": ");
		mputs(" '");
		mputs(sval);
		mputs("'");
	}
	mputs("\n");
} /* End errmsg() */

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
** newtime:
** Parses time specified on command line into our internal
** format.
**
** Parameters:
**	str	String from command line containing 4-digit
**		time value.
**
** Returns:
**	1	If successful.
**	0	If time value is invalid.
*/
int
newtime(str)
	char *str;
{
	int	digits;

	/* Extract hour from string. */
	hour = 0;
	digits = 0;
	while (digits < 2 && *str >= '0' && *str <= '9')
	{
		hour = hour * 10 + (unsigned short)((*str) - '0');
		str++;
		digits++;
	}

	if (digits < 1)
	{
		/* Not enough or invalid digits encountered. */
		return 0;
	}

	if (*str == ':')
		str++;

	/* Extract minutes from string. */
	minute = 0;
	digits = 0;
	while (digits < 2 && *str >= '0' && *str <= '9')
	{
		minute = minute * 10 + (unsigned short)((*str) - '0');
		str++;
		digits++;
	}

	if (digits < 1)
	{
		/* Not enough or invalid digits encountered. */
		return 0;
	}

	if (*str != '\0')
	{
		/* Unexpected characters remaining in string. */
		return 0;
	}

	/* Validate quanitities. */
	if (hour > 23)
		return 0;
	if (minute > 59)
		return 0;

	/* Convert the time into the format required for files. */
	ttime = (hour << 11) | (minute << 5);

	return 1;
} /* End newtime() */

/*
** newdate:
** Parses date specified on command line into our internal
** format.
**
** Parameters:
**	str	String from command line containing 6-digit
**		date value.
**
** Returns:
**	1	If successful.
**	0	If time value is invalid.
*/
int
newdate(str)
	char *str;
{
	int	digits;

	/* Extract month from string. */
	month = 0;
	digits = 0;
	while (digits < 2 && *str >= '0' && *str <= '9')
	{
		month = month * 10 + (unsigned short)((*str) - '0');
		str++;
		digits++;
	}

	if (digits < 1)
	{
		/* Not enough or invalid digits encountered. */
		return 0;
	}

	if (*str == '-' || *str == '/' || *str == '\\')
		str++;

	/* Extract day from string. */
	day = 0;
	digits = 0;
	while (digits < 2 && *str >= '0' && *str <= '9')
	{
		day = day * 10 + (unsigned short)((*str) - '0');
		str++;
		digits++;
	}

	if (digits < 1)
	{
		/* Not enough or invalid digits encountered. */
		return 0;
	}

	if (*str == '-' || *str == '/' || *str == '\\')
		str++;

	/* Extract year from string. */
	year = 0;
	digits = 0;
	while (digits < 4 && *str >= '0' && *str <= '9')
	{
		year = year * 10 + (unsigned short)((*str) - '0');
		str++;
		digits++;
	}

	if (digits < 1)
	{
		/* Not enough or invalid digits encountered. */
		return 0;
	}

	/* Remove century from year. */
	if (year >= 1900 && year <= 2079)
		year -= 1900;

	if (*str != '\0')
	{
		/* Unexpected characters remaining in string. */
		return 0;
	}

	/* Validate quanitities. */
	if (month < 1 || month > 12)
		return 0;
	if (day > 31)
		return 0;
	if (year < 80 || year > 127)
		return 0;

	/* Convert the date into the format required for files. */
	date = ((year-80) << 9) | (month << 5) | day;

	return 1;
} /* End newdate() */

/*
** touch_file:
** Sets the time and date of a file to the
** specified time and date.
**
** Parameters:
**	Name		Description
**	----		-----------
**	filespec	Pathname of file.
**	date		Date to set.
**	ttime		Time to set.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	1	Successful.
**	0	Error occured.
*/
int
touch_file(filespec, date, ttime)
	char	*filespec;
	unsigned short int date;
	unsigned short int ttime;
{
	int	handle;

	/* Attempt to open specified file. */
	handle = open(filespec, O_RDWR);
	if (handle < 0)
	{
		if (!flag_nocreate)
		{
			/* Attempt to create the file. */
			handle = open(filespec, O_RDWR | O_BINARY |
					O_CREAT | O_TRUNC,
					S_IREAD | S_IWRITE);
			if (handle < 0)
			{
				errmsg(MSG_ERR_CANTCREAT, filespec);
				errpause();
				return 0;
			}
		}
		else
		{
			/* Couldn't open file. */
			errmsg(MSG_ERR_CANTACCESS, filespec);
			errpause();
			return 0;
		}
	}

	/* Attempt to change timestamp of file. */
	if (_dos_setftime(handle, date, ttime))
	{
		/* Couldn't set timestamp. */
		errmsg(MSG_ERR_CANTTOUCH, filespec);
		errpause();
		close(handle);
		return 0;
	}

	/* Timestamp was set OK; close file. */
	close(handle);
	return 1;
} /* End touch_file() */

/*
** usage:
** Displays information about the program.
**
** Parameters:
**	NONE
**
** Returns:
**	NONE
*/
static void
usage(void)
{
	mputs(MSG_SIGNON);
	mputs(MSG_COPYRIGHT);
	mputs(MSG_USAGE);
} /* End usage() */

/*
** main:
** C application entry point.
**
** Parameters:
**	Name	Description
**	----	-----------
**	argc	Number of command line arguments.
**	argv	Array of pointers to command line arguments.
**
** Returns:
**	Value	Meaning
**	-----	-------
**	0	No errors occured.
**	1	Error(s) occured.
*/
int
#ifdef WIN
dmain(argc, argv)
#else
main(argc, argv)
#endif /* WIN */
	int	argc;
	char	**argv;
{
	int			i;
	int			did;
	struct find_t		findbfr;
	char			pathspec[MAXPATH];
	char			filespec[MAXPATH];
	struct dosdate_t	systemdate;
	struct dostime_t	systemtime;

	/* If command line is empty or contains '-?', display usage info. */
	if (argv[1][0] == '-' && argv[1][1] == '?')
	{
		/* User needs help. */
		usage();
		return 1;
	}

	/* Set defaults. */
	flag_nocreate = 0;

	/* Get the current date and time from MS-DOS. */

	_dos_getdate(&systemdate);

	day = (unsigned int)systemdate.day;
	month = (unsigned int)systemdate.month;
	year = (unsigned int)systemdate.year;

	_dos_gettime(&systemtime);

	second = (unsigned int)systemtime.second;
	minute = (unsigned int)systemtime.minute;
	hour = (unsigned int)systemtime.hour;

	/* Convert the date into the format required for files. */
	date = ((year-1980) << 9) | (month << 5) | day;

	/* Convert the time into the format required for files. */
	ttime = (hour << 11) | (minute << 5);

	/* Parse arguments. */
	did = 0;
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			/* Command line parameter is an option. */
			switch(argv[i][1])
			{
				case OPT_NOCREATE: /* Don't create. */
					flag_nocreate = 1;
					break;

				case OPT_TIME: /* Specify time. */
					if (!newtime(&argv[i][2]))
					{
						errmsg(MSG_ERR_BADTIME,
							&argv[i][2]);
						errpause();
						return 1;
					}
					break;

				case OPT_DATE: /* Specify date. */
					if (!newdate(&argv[i][2]))
					{
						errmsg(MSG_ERR_BADDATE,
							&argv[i][2]);
						errpause();
						return 1;
					}
					break;

				case OPT_WORKINGDIR:
					/* Specify working directory. */
					if (strlen(&argv[i][2]) < 1)
					{
						/* No directory specified. */
						errmsg(MSG_ERR_NODIR,
							(char *)NULL);
						errpause();
						return 1;
					}
					else
					{
						/* Change directory. */
						if (!mchdir(&argv[i][2]))
						{
							/* Couldn't change. */
							errmsg(MSG_ERR_CHDIR,
								&argv[i][2]);
							errpause();
							return 1;
						}
					}
					break;

				default:
					/* Unrecognized option. */
					errmsg(MSG_ERR_BADOPTION,
						argv[i]);
					errpause();
					return 1;
			}
		}
		else
		{
			/* Command line parameter is a filespec. */
			strcpy(pathspec, argv[i]);
			cvt_slash(pathspec);
			getpath(pathspec);
			did++;

			if (w_findfirst(argv[i], &findbfr))
			{
				/*
				** There is no match for the specified
				** file, so try touching it anyway.
				** This will create an empty file with
				** the specified name if the filename
				** is valid.
				*/
				if (!touch_file(argv[i], date, ttime))
				{
					/* Couldn't touch specified file. */
					; /* Do nothing. */
				}
			}
			else
			{
				do
				{
					/* Touch a file. */
					strcpy(filespec, pathspec);
					strcat(filespec, findbfr.name);
					strlwr(filespec);
					if (findbfr.attrib & _A_SUBDIR)
					{
						errmsg(MSG_WARN_DIRECTORY,
							filespec);
					}
					else
					{
						touch_file(filespec,
							date, ttime);
					}
				}
				while (!w_findnext(argv[i], &findbfr));
			}
		}
	}

	if (did < 1)
	{
		/* There were no files named on the command line. */
		errmsg(MSG_ERR_NOFILES, (char *)NULL);
		errpause();
		return 1;
	}

	return 0;
} /* End main() */

/*
====================================================================
End touch.c
====================================================================
*/
