/*
======================================================================
makew.c
Main module for Make utility for Microsoft Windows.

This source file is part of a computer program that is
(C) Copyright 1985-1988, 1990, 1992 Ammon R. Campbell.
All rights reserved.

This source file contains trade secrets of the author and
may not be disclosed without the express written consent
of the author.
======================================================================

NOTES

This module contains the WinMain() function and miscellaneous utility
functions for the make utility.  Most of the actual work is done
by the other modules in the program.

======================================================================
*/

/*************************** INCLUDES *******************************/

#include <windows.h>
#include <toolhelp.h>
#include <time.h>
#include <string.h>

#include "make.h"
#include "makecid.h"

/*************************** HEADERS ********************************/

int PASCAL      WinMain(HANDLE hInstance, HANDLE hPrevInstance,
                        LPSTR lpszCmdLine, int nCmdShow);
long FAR PASCAL main_window_proc(HWND hwnd, unsigned message,
                        WPARAM wparam, LPARAM lparam);
BOOL FAR PASCAL notify_proc(WORD wid, DWORD dwdata);
BOOL FAR PASCAL startup_dlg_proc(HWND hdlg, unsigned message,
                        WPARAM wparam, LPARAM lparam);
BOOL FAR PASCAL options_dlg_proc(HWND hdlg, unsigned message,
                        WPARAM wparam, LPARAM lparam);
BOOL FAR PASCAL about_dlg_proc(HWND hdlg, unsigned message,
                        WPARAM wparam, LPARAM lparam);
int             show_help(HWND hwnd, WORD type, DWORD data);

/*************************** CONSTANTS ******************************/

#define MODULE_NAME     "MAKE"
#define HELPFILE_NAME   "dxmake.hlp"

#define MAXARGS 20
#define MAX_ENV_SIZE    2048
#define MAX_ENV_STRINGS 256

#define DBGMSG(s)       MessageBox(0, (LPSTR)(s), \
                                (LPSTR)"debug", MB_OK | MB_TASKMODAL);

/*************************** VARIABLES ******************************/

/* hmainwnd:  Window handle of the application's main window. */
static HWND     hmainwnd;

/* hinstance:  Instance handle of the application. */
static HANDLE   hinstance;

/* charx:  Current horizontal position to draw character at. */
static int      charx;

/* chary:  Current vertical position to draw character at. */
static int      chary;

/* task_spawned_id:  Instance handle of the last task we spawned. */
static HTASK    task_spawned_id;

/* task_exit_id:  Instance handle of the last task to exit. */
static HINSTANCE task_exit_id;

/* task_exit_code:  Exit code of the last task to exit. */
static WORD     task_exit_code;

/* aborted:  This flag is zero unless the user aborts the make. */
static int      aborted;

/*
** cmd_args:  String containing the command line arguments passed
** to us.
*/
unsigned char cmd_args[MAXPATH];

/*
** Pointers to TOOLHELP.DLL entry points.
*/
BOOL (FAR PASCAL *fp_notify_register)(HTASK, LPFNNOTIFYCALLBACK, WORD);
BOOL (FAR PASCAL *fp_notify_unregister)(HTASK);
BOOL (FAR PASCAL *fp_task_find_handle)(TASKENTRY FAR *, HTASK);

/*
** Enabled options.
*/
static char     flag_build_anyway;
static char     flag_signon;
static char     flag_debug;
static char     flag_env_override;
static char     flag_ignore;
static char     flag_nospawn;
static char     flag_show_info;
static char     flag_query;
static char     flag_no_defaults;
static char     flag_no_show;
static char     flag_touch;
static char     flag_neednewer;

/*
** Buffer for environment strings.
*/
static char     menvbfr[MAX_ENV_SIZE];
static char     *menvp[MAX_ENV_STRINGS];

/*************************** FUNCTIONS ******************************/

/*
** WinMain:
** Microsoft Windows application entry point.
*/
int PASCAL
WinMain(HANDLE hInstance, HANDLE hPrevInstance,
        LPSTR lpszCmdLine, int nCmdShow)
{
        char            margc;
        char            argbfr[MAXPATH];
        char            *margv[MAXARGS];
        int             i;
        int             result;
        MSG             msg;
        HWND            hwnd;
        HMENU           hmenu;
        WNDCLASS        wc;
        FARPROC         notify_proc_inst;
        int             toolhelp_available;
        OFSTRUCT        ofs;
        HANDLE          toolhelp_library;
        FARPROC         dlg_proc;
        LPSTR           env_ptr;
        char            *char_ptr;

        hinstance = hInstance;

        /* Trim leading whitespace from command arguments. */
        while (*lpszCmdLine == ' ' || *lpszCmdLine == '\t')
                lpszCmdLine++;

        /*
        ** If there are no command line arguments, then
        ** ask the user what to do.
        */
        lstrcpy((LPSTR)cmd_args, (LPSTR)lpszCmdLine);
        if (lpszCmdLine[0] == '\0')
        {
                /* Set default option values. */
                flag_build_anyway = 0;
                flag_signon = 0;
                flag_debug = 0;
                flag_env_override = 0;
                flag_ignore = 0;
                flag_nospawn = 0;
                flag_show_info = 0;
                flag_query = 0;
                flag_no_defaults = 0;
                flag_no_show = 0;
                flag_touch = 0;
                flag_neednewer = 0;

                /* Let user change options from dialog. */
                dlg_proc = MakeProcInstance(
                                startup_dlg_proc,
                                hinstance);
                if ((i = DialogBox(hinstance,
                        "STARTUP_DLG_TEMPLATE", /* Resource. */
                        NULL,                   /* Parent. */
                        dlg_proc))              /* Address. */
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

        /* Link to the tool helper library if it is available. */
        toolhelp_available = 0;
        if (OpenFile((LPSTR)"TOOLHELP.DLL", (LPOFSTRUCT)&ofs, OF_EXIST) >= 0)
        {
                /* TOOLHELP.DLL exists, so load it. */
                toolhelp_available = 1;
                toolhelp_library = LoadLibrary((LPSTR)"TOOLHELP.DLL");
                if (toolhelp_library < 32)
                        toolhelp_available = 0;
        }
        if (toolhelp_available)
        {
                fp_notify_register = (void FAR *)GetProcAddress(
                                                toolhelp_library,
                                                (LPSTR)"NotifyRegister");
                fp_notify_unregister = (void FAR *)GetProcAddress(
                                                toolhelp_library,
                                                (LPSTR)"NotifyUnRegister");
                fp_task_find_handle = (void FAR *)GetProcAddress(
                                                toolhelp_library,
                                                (LPSTR)"TaskFindHandle");
                if (fp_notify_register == (void FAR *)NULL ||
                        fp_notify_unregister == (void FAR *)NULL ||
                        fp_task_find_handle == (void FAR *)NULL)
                {
                        /* Couldn't get function entry points from TOOLHELP. */
                        FreeLibrary(toolhelp_library);
                        toolhelp_available = 0;
                }
                else
                {
                        /* Register our notification function. */
                        notify_proc_inst = MakeProcInstance(
                                                notify_proc, hInstance);
                        (*fp_notify_register)(NULL,
                                        (void FAR *)notify_proc_inst,
                                        NF_NORMAL);
                }
        }

        /* Save the window handle for later use. */
        hmainwnd = hwnd;

        /* Initial drawing position is upper left. */
        charx = 0;
        chary = 0;

        /* User has not aborted (yet). */
        aborted = 0;

        /* Warn user if we can't check subprocess return codes. */
        if (!toolhelp_available)
                mputs(MSG_TOOLHELPWARN);

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
        margv[0] = "";          /* Windows doesn't give us an argv[0]. */
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

        /* Make a local copy of the DOS environment. */
        i = 0;
        env_ptr = GetDOSEnvironment();
        while (*env_ptr != '\0')
        {
                i += lstrlen((LPSTR)env_ptr) + 1;
                env_ptr += lstrlen((LPSTR)env_ptr) + 1;
        }
        i++;
        if (i >= MAX_ENV_SIZE)
        {
                /*
                ** The DOS environment is too large.
                ** Warn the user it will be truncated.
                */
                mputs(MSG_ENVTOOLARGE);
                i = MAX_ENV_SIZE - 1;
        }
        _fmemcpy((void FAR *)menvbfr, (void FAR *)GetDOSEnvironment(), i);
        menvbfr[i] = '\0';

        /* Make an array of pointers to the environment strings. */
        i = 0;
        char_ptr = menvbfr;
        while (*char_ptr != '\0')
        {
                menvp[i] = char_ptr;
                i++;
                char_ptr += lstrlen((LPSTR)char_ptr) + 1;
        }
        menvp[i] = (char *)NULL;

        /* Run the main() function. */
        result = dmain(margc, margv, menvp);

#if 0
        /* Pause until user presses a key. */
        mputs(MSG_PRESSAKEY);
        msg.message = 0;
        while (!aborted &&
                msg.message != WM_KEYDOWN && GetMessage(&msg, NULL, 0, 0))
        {
                TranslateMessage((LPMSG)&msg);
                DispatchMessage((LPMSG)&msg);
        }
#endif

        if (toolhelp_available)
        {
                /* Unregister our notification function. */
                (*fp_notify_unregister)(NULL);
                FreeProcInstance(notify_proc_inst);
                FreeLibrary(toolhelp_library);
        }

        /* Process remaining messages until we're all finished. */
        while (!aborted && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
                TranslateMessage((LPMSG)&msg);
                DispatchMessage((LPMSG)&msg);
        }

        return result;
}

/*
** MainWndProc:
** Main message handler for this application.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      hwnd    Window handle of recipient window.
**      message Type of message.
**      wparam  Message parameters.
**      lparam  Additional message parameters.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      FALSE   Message was used.
**      other   Message was passed on to DefWindowProc().
*/
long FAR PASCAL
main_window_proc(hwnd, message, wparam, lparam)
        HWND            hwnd;
        unsigned        message;
        WPARAM          wparam;
        LPARAM          lparam;
{
        FARPROC         dlg_proc;

        /* Do the right thing depending on what message we get. */
        switch (message)
        {
                case WM_DESTROY:
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
                                        "ABOUT_DLG_TEMPLATE",   /* Resource. */
                                        hwnd,                   /* Parent. */
                                        dlg_proc)               /* Address. */
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
        }

        return NULL;
}

/*
** wputc:
** Outputs a character to the application's window as if it
** were a dumb teletype output device.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      ch      Character to be output.
**
** Returns:
**      NONE
*/
void
wputc(unsigned char ch)
{
        HDC     dc;             /* Display context of app's window. */
        HFONT   oldfont;        /* Handle of original font. */
        HFONT   ourfont;        /* Handle of the font we use. */
        TEXTMETRIC tm;          /* Metrics of our font. */
        int     cwidth;         /* Width of one character in pixels. */
        int     cheight;        /* Height of one character in pixels. */
        RECT    crect;          /* Client area of our window. */
        char    stmp[2];        /* String buffer for character. */

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
}

/*
** wrun:
** Runs a command using WinExec.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      cmd     String containing the command line to be run.
**
** Returns:
**      Value   Meaning
**      -----   -------
**      -1      Could not run specified command (i.e. WinExec failed).
**      other   Return code of command (this is not implemented yet,
**              so zero is always returned).
*/
int
wrun(unsigned char *cmd)
{
        char    stmp[MAXPATH * 2];      /* Temporary string buffer. */
        char    stmp2[MAXPATH];         /* Temporary string buffer. */
        int     result;                 /* Return code from WinExec. */
        MSG     msg;                    /* Message struct for PeekMessage(). */
        int     has_redirect = 0;       /* Flag, 1=command has redirectrion. */

        /*
        ** If command contains I/O redirection, then use COMMAND.COM
        ** to run it instead of trying to run it directly.
        */
        lstrcpy((LPSTR)stmp, (LPSTR)cmd);
        if (cindex(stmp, '<') >= 0 ||
                cindex(stmp, '>') >= 0 ||
                cindex(stmp, '|') >= 0)
        {
                lstrcpy((LPSTR)stmp, (LPSTR)"COMMAND.COM /C ");
                lstrcat((LPSTR)stmp, (LPSTR)cmd);
                has_redirect = 1;
        }

        task_spawned_id = 0;
        task_exit_code = 0;
        task_exit_id = 0;

        /* Run command using WinExec(). */
        result = WinExec((LPSTR)stmp, SW_NORMAL);
        if (result < 32)
        {
                /*
                ** We couldn't run the command, so try
                ** running it with COMMAND.COM, becuase
                ** it might be a DOS command.
                */
                if (!has_redirect)
                {
                        lstrcpy((LPSTR)stmp, (LPSTR)"COMMAND.COM /C ");
                        lstrcat((LPSTR)stmp, (LPSTR)cmd);
                        result = WinExec((LPSTR)stmp, SW_NORMAL);
                        if (result < 32)
                        {
                                /* Failed executing the command. */
                                return -1;
                        }
                }
                else
                {
                        /* Failed executing the command. */
                        return -1;
                }
        }
        task_spawned_id = result;

        /*
        ** Wait for the command to finish.
        ** GetModuleFileName() is a cheap way to see if an
        ** instance handle is still valid.
        */
        while (GetModuleFileName(result, (LPSTR)stmp2, MAXPATH - 1) > 0)
        {
                Yield();

                /* Flush our message queue. */
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                        TranslateMessage((LPMSG)&msg);
                        DispatchMessage((LPMSG)&msg);
                        Yield();
                }
        }

        /*
        ** If we got a task end notification for the program we
        ** just ran, use its return code.
        */
        task_spawned_id = 0;
        if (task_exit_id == (HINSTANCE)result)
        {
                return (int)task_exit_code;
        }

        /* Assume the command executed OK. */
        return 0;
}

/*
** notify_proc:
** System notification callback routine.  We use this to
** receive task exit notifications from TOOLHELP.DLL, so
** we can get the return codes of DOS programs in Windows
** 3.1.  When control is transferred to this routine, the
** current task handle is the task handle of the task
** that caused the notification to happen.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      wid     Type of notification
**      dwdata  Notification associated data.
*/
BOOL FAR PASCAL
notify_proc(WORD wid, DWORD dwdata)
{
        TASKENTRY       taskinfo;
        HTASK           this_task;

        switch(wid)
        {
                case NFY_EXITTASK:
                        /*
                        ** A task just ended.  Get the task
                        ** information so we can see if this
                        ** is the task we're waiting for.
                        */
                        this_task = GetCurrentTask();
                        taskinfo.dwSize = sizeof(TASKENTRY);
                        if (!(*fp_task_find_handle)(
                                (TASKENTRY FAR *)&taskinfo,
                                this_task))
                        {
                                /* Error getting task information. */
                                break;
                        }

                        /*
                        ** See if this is the task we're waiting
                        ** for.
                        */
                        if (taskinfo.hInst == task_spawned_id)
                        {
                                /*
                                ** This is the task we're waiting
                                ** for, so save the instance handle
                                ** and the return code in global
                                ** variables.  The main routine
                                ** will check these values.
                                */
                                task_exit_id = taskinfo.hInst;
                                task_exit_code = LOWORD(dwdata);
                        }
                        break;

                default:
                        ;
        }

        /* Return zero to indicate that we didn't handle the notification. */
        return 0;
}

/*
** check_abort:
** Checks to see if the program has been aborted by the user.
**
** Parameters:
**      NONE
**
** Returns:
**      Value   Meaning
**      -----   -------
**      1       The user has aborted the make.
**      0       The user has not aborted the make.
*/
int
check_abort(void)
{
        if (aborted)
                return 1;
        return 0;
}

/*
** startup_dlg_proc:
** Message handling callback routine for the primary dialog.
**
** Parameters:
**      Standard dialog callback parameters.
**
** Returns:
**      Standard dialog callback return value.
*/
BOOL FAR PASCAL
startup_dlg_proc(HWND hdlg, unsigned message, WPARAM wparam, LPARAM lparam)
{
        FARPROC dlg_proc;
        HMENU   hmenu;
        char    stmp[MAXPATH];
        int     i;

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
                                        "ABOUT_DLG_TEMPLATE",   /* Resource. */
                                        hdlg,                   /* Parent. */
                                        dlg_proc)               /* Address. */
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
                                /* Get name of makefile, if any. */
                                if (GetDlgItemText(hdlg, IDE_MAKEFILENAME,
                                        (LPSTR)stmp, MAXPATH - 1) > 0)
                                {
                                        i = lstrlen((LPSTR)cmd_args);
                                        cmd_args[i] = ' ';
                                        cmd_args[i + 1] = '-';
                                        cmd_args[i + 2] = OPT_MAKEFILE;
                                        cmd_args[i + 3] = '\0';
                                        lstrcat((LPSTR)cmd_args, (LPSTR)stmp);
                                }

                                /* Get name of source directory, if any. */
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

                                /* Get list of target names, if any. */
                                if (GetDlgItemText(hdlg, IDE_TARGETNAMES,
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
                        else if (wparam == IDB_OPTIONS)
                        {
                                dlg_proc = MakeProcInstance(
                                                options_dlg_proc,
                                                hinstance);
                                if (DialogBox(hinstance,
                                        "OPTIONS_DLG_TEMPLATE", /* Resource. */
                                        hdlg,                   /* Parent. */
                                        dlg_proc)               /* Address. */
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
                        /* Ignore this message. */
                        ;
        }

        return FALSE;
}

/*
** options_dlg_proc:
** Message handling callback routine for the options dialog.
**
** Parameters:
**      Standard dialog callback parameters.
**
** Returns:
**      Standard dialog callback return value.
*/
BOOL FAR PASCAL
options_dlg_proc(HWND hdlg, unsigned message, WPARAM wparam, LPARAM lparam)
{
        int     i;
        switch (message)
        {
                case WM_INITDIALOG:
                        if (flag_build_anyway)
                                CheckDlgButton(hdlg, IDB_OPTA, 1);
                        if (flag_signon)
                                CheckDlgButton(hdlg, IDB_OPTC, 1);
                        if (flag_debug)
                                CheckDlgButton(hdlg, IDB_OPTD, 1);
                        if (flag_env_override)
                                CheckDlgButton(hdlg, IDB_OPTE, 1);
                        if (flag_ignore)
                                CheckDlgButton(hdlg, IDB_OPTI, 1);
                        if (flag_nospawn)
                                CheckDlgButton(hdlg, IDB_OPTN, 1);
                        if (flag_show_info)
                                CheckDlgButton(hdlg, IDB_OPTP, 1);
                        if (flag_query)
                                CheckDlgButton(hdlg, IDB_OPTQ, 1);
                        if (flag_no_defaults)
                                CheckDlgButton(hdlg, IDB_OPTR, 1);
                        if (flag_no_show)
                                CheckDlgButton(hdlg, IDB_OPTS, 1);
                        if (flag_touch)
                                CheckDlgButton(hdlg, IDB_OPTT, 1);
                        if (flag_neednewer)
                                CheckDlgButton(hdlg, IDB_OPTY, 1);
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
                                flag_build_anyway = 0;
                                flag_signon = 0;
                                flag_debug = 0;
                                flag_env_override = 0;
                                flag_ignore = 0;
                                flag_nospawn = 0;
                                flag_show_info = 0;
                                flag_query = 0;
                                flag_no_defaults = 0;
                                flag_no_show = 0;
                                flag_touch = 0;
                                flag_neednewer = 0;
                                cmd_args[0] = '\0';
                                i = lstrlen((LPSTR)cmd_args);
                                if (IsDlgButtonChecked(hdlg, IDB_OPTA))
                                {
                                        flag_build_anyway = 1;
                                        cmd_args[i] = ' ';
                                        cmd_args[i + 1] = '-';
                                        cmd_args[i + 2] = OPT_BUILD_ANYWAY;
                                        cmd_args[i + 3] = '\0';
                                        i = lstrlen((LPSTR)cmd_args);
                                }
                                if (IsDlgButtonChecked(hdlg, IDB_OPTC))
                                {
                                        flag_signon = 1;
                                        cmd_args[i] = ' ';
                                        cmd_args[i + 1] = '-';
                                        cmd_args[i + 2] = OPT_SIGNON;
                                        cmd_args[i + 3] = '\0';
                                        i = lstrlen((LPSTR)cmd_args);
                                }
                                if (IsDlgButtonChecked(hdlg, IDB_OPTD))
                                {
                                        flag_debug = 1;
                                        cmd_args[i] = ' ';
                                        cmd_args[i + 1] = '-';
                                        cmd_args[i + 2] = OPT_DEBUG;
                                        cmd_args[i + 3] = '\0';
                                        i = lstrlen((LPSTR)cmd_args);
                                }
                                if (IsDlgButtonChecked(hdlg, IDB_OPTE))
                                {
                                        flag_env_override = 1;
                                        cmd_args[i] = ' ';
                                        cmd_args[i + 1] = '-';
                                        cmd_args[i + 2] = OPT_ENV_OVERRIDE;
                                        cmd_args[i + 3] = '\0';
                                        i = lstrlen((LPSTR)cmd_args);
                                }
                                if (IsDlgButtonChecked(hdlg, IDB_OPTI))
                                {
                                        flag_ignore = 1;
                                        cmd_args[i] = ' ';
                                        cmd_args[i + 1] = '-';
                                        cmd_args[i + 2] = OPT_IGNORE;
                                        cmd_args[i + 3] = '\0';
                                        i = lstrlen((LPSTR)cmd_args);
                                }
                                if (IsDlgButtonChecked(hdlg, IDB_OPTN))
                                {
                                        flag_nospawn = 1;
                                        cmd_args[i] = ' ';
                                        cmd_args[i + 1] = '-';
                                        cmd_args[i + 2] = OPT_NOSPAWN;
                                        cmd_args[i + 3] = '\0';
                                        i = lstrlen((LPSTR)cmd_args);
                                }
                                if (IsDlgButtonChecked(hdlg, IDB_OPTP))
                                {
                                        flag_show_info = 1;
                                        cmd_args[i] = ' ';
                                        cmd_args[i + 1] = '-';
                                        cmd_args[i + 2] = OPT_SHOW_INFO;
                                        cmd_args[i + 3] = '\0';
                                        i = lstrlen((LPSTR)cmd_args);
                                }
                                if (IsDlgButtonChecked(hdlg, IDB_OPTQ))
                                {
                                        flag_query = 1;
                                        cmd_args[i] = ' ';
                                        cmd_args[i + 1] = '-';
                                        cmd_args[i + 2] = OPT_QUERY;
                                        cmd_args[i + 3] = '\0';
                                        i = lstrlen((LPSTR)cmd_args);
                                }
                                if (IsDlgButtonChecked(hdlg, IDB_OPTR))
                                {
                                        flag_no_defaults = 1;
                                        cmd_args[i] = ' ';
                                        cmd_args[i + 1] = '-';
                                        cmd_args[i + 2] = OPT_NO_DEFAULTS;
                                        cmd_args[i + 3] = '\0';
                                        i = lstrlen((LPSTR)cmd_args);
                                }
                                if (IsDlgButtonChecked(hdlg, IDB_OPTS))
                                {
                                        flag_no_show = 1;
                                        cmd_args[i] = ' ';
                                        cmd_args[i + 1] = '-';
                                        cmd_args[i + 2] = OPT_NO_SHOW;
                                        cmd_args[i + 3] = '\0';
                                        i = lstrlen((LPSTR)cmd_args);
                                }
                                if (IsDlgButtonChecked(hdlg, IDB_OPTT))
                                {
                                        flag_touch = 1;
                                        cmd_args[i] = ' ';
                                        cmd_args[i + 1] = '-';
                                        cmd_args[i + 2] = OPT_TOUCH;
                                        cmd_args[i + 3] = '\0';
                                        i = lstrlen((LPSTR)cmd_args);
                                }
                                if (IsDlgButtonChecked(hdlg, IDB_OPTY))
                                {
                                        flag_neednewer = 1;
                                        cmd_args[i] = ' ';
                                        cmd_args[i + 1] = '-';
                                        cmd_args[i + 2] = OPT_NEEDNEWER;
                                        cmd_args[i + 3] = '\0';
                                        i = lstrlen((LPSTR)cmd_args);
                                }
                                EndDialog(hdlg, IDOK);
                        }
                        else if (wparam == IDCANCEL)
                        {
                                EndDialog(hdlg, IDCANCEL);
                        }
                        else if (wparam == IDB_HELP)
                        {
                                show_help(hmainwnd, HELP_INDEX, 0L);
                        }
                        break;

                default:
                        /* Ignore this message. */
                        ;
        }

        return FALSE;
}

/*
** about_dlg_proc:
** Message handling callback routine for the "About" dialog.
**
** Parameters:
**      Standard dialog callback parameters.
**
** Returns:
**      Standard dialog callback return value.
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
}

/*
** show_help:
** Invokes the Windows help engine to display information
** from the application's help file.
**
** Parameters:
**      Name    Description
**      ----    -----------
**      hwnd    Window handle of window requesting help.
**      type    Type of help requested (see the wCommand
**              parameter of the WinHelp function in the
**              Windows SDK reference for more info).
**      data    Context/key identifier (see the dwData
**              parameter of the WinHelp function in the
**              Windows SDK reference for more info).
**
** Returns:
**      Value   Meaning
**      -----   -------
**      1       Successful.
**      0       Error occurred.
*/
int
show_help(HWND hwnd, WORD type, DWORD data)
{
        HANDLE  htmp;           /* Temporary module handle. */
        int     i;              /* Loop index. */
        char    stmp[MAXPATH];  /* Temporary pathname. */

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
}

