/*
 *  tuio_socket_proxy.c
 *
 *  Tuio protocol proxying (forwarding from udp to server on tcp port)
 *
 *  Author : Goran Medakovic
 *  Email : avion.bg@gmail.com
 *
 *  Copyright (c) 2008 Goran Medakovic <avion.bg@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */


#include <windows.h>
#include <windowsx.h>
#include <winsock.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include "lo/lo.h"
#include "resource.h"
#include <commdlg.h> //4 browse 4 file
#include <commctrl.h> //4 common controls
#include <shlwapi.h> //4 PathFileExistsA
#define QLEN 5
#define SHUT_RDWR SD_BOTH
  #define TA_FAILED 0
   #define TA_SUCCESS_CLEAN 1
   #define TA_SUCCESS_KILL 2
   #define TA_SUCCESS_16 3

struct protoent *ptrp;
struct sockaddr_in address;
char port[6], osc[256];
int servsock, clientsock, fwdport, aosc, fwdport;
int done = 0;
int debug = 0;
int toggle = 1;
int connected = -1;

lo_server st;

typedef struct
{
    char path[12];
    char name[8];
    int sessionID;
    float x,y,X,Y,m,w,h;
} set;

typedef struct
{
    char path[12];
    char name[8];
    int framenum;
} fseq;

void error(int num, const char *m, const char *path);
int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
                 void *data, void *user_data);
int fwd_handler(const char *path, const char *types, lo_arg **argv,
                int argc, void *data, void *user_data);
void initlo();
void initcp();

/* ::::::::: GUI STUFF ::::::::: */

#define THIS_CLASSNAME      "ToXy tuio socket proxy Window"
#define THIS_TITLE          "ToXy Tuio sOcket proXy tray"
static BOOL g_bModalState       = FALSE;
HWND hWndtray = NULL;
HINSTANCE   g_hInst;
//HWND hWndlg = NULL;

enum
{
    WM_ICON         = 100,
    ID_TRAYICON         = 101,
    ID_TRAYICONR         = 102,
    ID_TRAYICONB         = 103,
    APPWM_TRAYICON      = WM_APP,
    APPWM_NOP           = WM_APP + 1,
    ID_ABOUT            = 2000,
    ID_TOGGLE             = 3000,
    ID_CONF              = 4000,
    ID_EXIT,
};

void    AddTrayIcon( HWND hWnd, UINT uID, UINT uCallbackMsg, UINT uIcon,
                     LPSTR pszToolTip );
void    RemoveTrayIcon( HWND hWnd, UINT uID);
void    ModifyTrayIcon( HWND hWnd, UINT uID, UINT uIcon, LPSTR pszToolTip );
HICON   LoadSmallIcon( HINSTANCE hInstance, UINT uID );
BOOL    ShowPopupMenu( HWND hWnd, POINT *curpos, int wDefaultItem );
void    OnInitMenuPopup( HWND hWnd, HMENU hMenu, UINT uID );
BOOL    OnCommand( HWND hWnd, WORD wID, HWND hCtl );
void    OnTrayIconMouseMove( HWND hWnd );
void    OnTrayIconRBtnUp( HWND hWnd );
void    OnTrayIconLBtnDblClick( HWND hWnd );
void    OnClose( HWND hWnd );
void    RegisterMainWndClass( HINSTANCE hInstance );
void    UnregisterMainWndClass( HINSTANCE hInstance );
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK Configure( HWND , UINT , WPARAM , LPARAM );
LRESULT CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

int WINAPI _WinMain( HINSTANCE hInst, HINSTANCE prev, LPSTR cmdline, int show )
{
    HMENU   hSysMenu    = NULL;
    HWND    hWnd        = NULL;
    HWND    hPrev       = NULL;
    MSG     msg;
    g_hInst = hInst;
    InitCommonControls();
    if ( hPrev = FindWindow( THIS_CLASSNAME, THIS_TITLE ) )
        return 0;
    RegisterMainWndClass( hInst );
    hWnd = CreateWindow( THIS_CLASSNAME, THIS_TITLE,
                         0, 0, 0, 100, 100, NULL, NULL, hInst, NULL );
    if ( ! hWnd )
    {
        MessageBox( NULL, "Ack! I can't create the window!", THIS_TITLE,
                    MB_ICONERROR | MB_OK | MB_TOPMOST );
        return 1;
    }
    hWndtray = hWnd;
    while ( GetMessage( &msg, NULL, 0, 0 ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    UnregisterMainWndClass( hInst );
    return msg.wParam;
}

void AddTrayIcon( HWND hWnd, UINT uID, UINT uCallbackMsg, UINT uIcon,
                  LPSTR pszToolTip )
{
    NOTIFYICONDATA  nid;
    memset( &nid, 0, sizeof( nid ) );

    nid.cbSize              = sizeof( nid );
    nid.hWnd                = hWnd;
    nid.uID                 = uID;
    nid.uFlags              = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage    = uCallbackMsg;
    nid.hIcon               = LoadSmallIcon( GetModuleHandle( NULL ), ID_TRAYICON );
    strcpy( nid.szTip, pszToolTip );

    Shell_NotifyIcon( NIM_ADD, &nid );
}

void ModifyTrayIcon( HWND hWnd, UINT uID, UINT uIcon, LPSTR pszToolTip )
{
    NOTIFYICONDATA  nid;

    memset( &nid, 0, sizeof( nid ) );

    nid.cbSize  = sizeof( nid );
    nid.hWnd    = hWnd;
    nid.uID     = uID;

    if ( uIcon != (UINT)-1 )
    {
        nid.hIcon   = LoadSmallIcon( GetModuleHandle( NULL ), uIcon );
        nid.uFlags  |= NIF_ICON;
    }

    if ( pszToolTip )
    {
        strcpy( nid.szTip, pszToolTip );
        nid.uFlags  |= NIF_TIP;
    }

    if ( uIcon != (UINT)-1 || pszToolTip )
    {
        Shell_NotifyIcon( NIM_MODIFY, &nid );
    }
}

void RemoveTrayIcon( HWND hWnd, UINT uID )
{
    NOTIFYICONDATA  nid;

    memset( &nid, 0, sizeof( nid ) );

    nid.cbSize  = sizeof( nid );
    nid.hWnd    = hWnd;
    nid.uID     = uID;

    Shell_NotifyIcon( NIM_DELETE, &nid );
}

static LRESULT CALLBACK WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam )
{
    switch ( uMsg )
    {
    case WM_CREATE:
        AddTrayIcon( hWnd, ID_TRAYICON, APPWM_TRAYICON, 0, THIS_TITLE );
        return 0;
    case APPWM_NOP:
        return 0;
    case APPWM_TRAYICON:
        SetForegroundWindow( hWnd );

        switch ( lParam )
        {
        case WM_MOUSEMOVE:
            OnTrayIconMouseMove( hWnd );
            return 0;

        case WM_RBUTTONUP:
            OnTrayIconRBtnUp( hWnd );
            return 0;

        case WM_LBUTTONDBLCLK:
            OnTrayIconLBtnDblClick( hWnd );
            return 0;
        }
        return 0;

    case WM_COMMAND:
        return OnCommand( hWnd, LOWORD(wParam), (HWND)lParam );

    case WM_INITMENUPOPUP:
        OnInitMenuPopup( hWnd, (HMENU)wParam, lParam );
        return 0;

    case WM_CLOSE:
        OnClose( hWnd );
        return DefWindowProc( hWnd, uMsg, wParam, lParam );

    default:
        return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }
}

void OnClose( HWND hWnd )
{
    RemoveTrayIcon( hWnd, ID_TRAYICON );
    lo_address t = lo_address_new(NULL, "3333");
    lo_send(t, "/quit", NULL);
    PostQuitMessage( 0 );
}

BOOL ShowPopupMenu( HWND hWnd, POINT *curpos, int wDefaultItem )
{
    HMENU   hPop        = NULL;
    int     i           = 0;
    WORD    cmd;
    POINT   pt;
    if ( g_bModalState )
        return FALSE;
    hPop = CreatePopupMenu();
    if ( ! curpos )
    {
        GetCursorPos( &pt );
        curpos = &pt;
    }
    InsertMenu( hPop, i++, MF_BYPOSITION | MF_STRING, ID_ABOUT, "About..." );
    InsertMenu( hPop, i++, MF_BYPOSITION | MF_STRING, ID_CONF, "Configure..." );
    InsertMenu( hPop, i++, MF_BYPOSITION | MF_STRING, ID_TOGGLE, "Tuio on/off..." );
    InsertMenu( hPop, i++, MF_BYPOSITION | MF_STRING, ID_EXIT, "Exit" );
    SetMenuDefaultItem( hPop, ID_ABOUT, FALSE );
    SetFocus( hWnd );
    SendMessage( hWnd, WM_INITMENUPOPUP, (WPARAM)hPop, 0 );
    cmd = TrackPopupMenu( hPop, TPM_LEFTALIGN | TPM_RIGHTBUTTON
                          | TPM_RETURNCMD | TPM_NONOTIFY,
                          curpos->x, curpos->y, 0, hWnd, NULL );

    SendMessage( hWnd, WM_COMMAND, cmd, 0 );
    DestroyMenu( hPop );
    return cmd;
}

BOOL OnCommand( HWND hWnd, WORD wID, HWND hCtl )
{
    if ( g_bModalState )
        return 1;
    switch ( wID )
    {
    case ID_ABOUT:
        DialogBox( g_hInst, MAKEINTRESOURCE(IDD_ABOUT_DIALOG), hWnd, About );
        return 0;
    case ID_CONF:
        DialogBox( g_hInst, MAKEINTRESOURCE(IDD_CONF_DIALOG), hWnd, Configure );
        return 0;
    case ID_TOGGLE:
    {
        lo_address t = lo_address_new(NULL, "3333");
        lo_send(t, "/toggle", NULL);
        return 0;
    }
    case ID_EXIT:
        PostMessage( hWnd, WM_CLOSE, 0, 0 );
        return 0;
    }

}

INT_PTR CALLBACK About( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
    switch (uMessage)
    {
    case WM_INITDIALOG:
        {
        HWND control;
        control = GetDlgItem(hWnd, IDD_APIC);
        HBITMAP bitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(100), IMAGE_ICON,0,0,0);
        SendMessage(control, STM_SETIMAGE,  (WPARAM)IMAGE_ICON, (LPARAM)bitmap);
        return TRUE;
        }
    case WM_COMMAND:
        switch (wParam)
        {
        case IDOK:
            EndDialog(hWnd, IDOK);
            return TRUE;

        case IDCANCEL:
            EndDialog(hWnd, IDCANCEL);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK Configure( HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam )
{
    switch (uMessage)
    {
    case WM_INITDIALOG:
        {
        //hWndlg = hWnd;
        HWND control;

        control = GetDlgItem(hWnd, IDD_PORT);
        SetWindowText(control, port);

        control = GetDlgItem(hWnd, IDD_FWDP);
        char tmp[6];
        sprintf(tmp,"%d",fwdport);
        SetWindowText(control, (char*) tmp);
        control = GetDlgItem(hWnd, IDD_PATH);
        SetWindowText(control, osc);
        if (aosc == 1)
        {
            control = GetDlgItem(hWnd, IDD_CHKB);
            PostMessage(control, BM_SETCHECK,BST_CHECKED,0);
        }
        return TRUE;
        }
    case WM_COMMAND:
        switch (wParam)
        {
        case 1:
            {
            HWND control;
            control = GetDlgItem(hWnd, IDD_PORT);
            char tmp[6] = {0};
            GetWindowText(control, (LPWSTR)port, sizeof(port));
            control = GetDlgItem(hWnd, IDD_FWDP);
            GetWindowText(control, (LPWSTR)tmp, sizeof(tmp));
            sscanf(tmp, "%d", &fwdport);
            control = GetDlgItem(hWnd, IDD_PATH);
            GetWindowText(control, (LPWSTR)osc, sizeof(osc));
            control = GetDlgItem(hWnd, IDD_CHKB);
            aosc = SendMessage(control, BM_GETCHECK, NULL, NULL);
            EndDialog(hWnd, COK);
            //hWndlg = NULL;
            return TRUE;
        }
        case 2:
            EndDialog(hWnd, CCAN);
            return TRUE;
        case IDD_BROW:
        {
            OPENFILENAME ofn;
            char szFileName[500];
            szFileName[0] = '\0';
            memset(&ofn, 0, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = NULL;
            ofn.lpstrFile = szFileName;
            ofn.nMaxFile = sizeof(szFileName);
            ofn.hInstance = NULL;
            ofn.lpstrFilter = TEXT("Applications *.exe\0*.exe\0All Files *.*\0*.*\0\0");
            ofn.nFilterIndex = 1;
            ofn.Flags= OFN_FILEMUSTEXIST;
            int bRes;
            bRes = GetOpenFileName(&ofn);
            HWND control;
            control = GetDlgItem(hWnd, IDD_PATH);
            SetWindowText(control, szFileName);
        }
        }
        break;
    }

    return FALSE;
}

void OnTrayIconMouseMove( HWND hWnd )
{
    //  stub
}

void OnTrayIconRBtnUp( HWND hWnd )
{
    SetForegroundWindow( hWnd );
    ShowPopupMenu( hWnd, NULL, -1 );
    PostMessage( hWnd, APPWM_NOP, 0, 0 );
}

void OnTrayIconLBtnDblClick( HWND hWnd )
{
    SendMessage( hWnd, WM_COMMAND, ID_ABOUT, 0 );
}

void OnInitMenuPopup( HWND hWnd, HMENU hPop, UINT uID )
{
    //  stub
}
void RegisterMainWndClass( HINSTANCE hInstance )
{
    WNDCLASSEX wclx;
    memset( &wclx, 0, sizeof( wclx ) );

    wclx.cbSize         = sizeof( wclx );
    wclx.style          = 0;
    wclx.lpfnWndProc    = &WindowProc;
    wclx.cbClsExtra     = 0;
    wclx.cbWndExtra     = 0;
    wclx.hInstance      = hInstance;
    //wclx.hIcon        = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_TRAYICON ) );
    //wclx.hIconSm      = LoadSmallIcon( hInstance, IDI_TRAYICON );
    wclx.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wclx.hbrBackground  = (HBRUSH)( COLOR_BTNFACE + 1 );    //  COLOR_* + 1 is
    //  special magic.
    wclx.lpszMenuName   = NULL;
    wclx.lpszClassName  = THIS_CLASSNAME;

    RegisterClassEx( &wclx );
}

void UnregisterMainWndClass( HINSTANCE hInstance )
{
    UnregisterClass( THIS_CLASSNAME, hInstance );
}

HICON LoadSmallIcon( HINSTANCE hInstance, UINT uID )
{
    return LoadImage( hInstance, MAKEINTRESOURCE( uID ), IMAGE_ICON,
                      16, 16, 0 );
}

/* ::::::::: END OF GUI STUFF ::::::::: */

void LoadPrefsEx()
{
    TCHAR acPath[256];
    GetModuleFileName(NULL,acPath,MAX_PATH);
    PathRemoveFileSpec(acPath);
    char ConfigFile[256];
    sprintf (ConfigFile,"%s\\toxy_win.ini",acPath);
    char buffer[256];
    GetPrivateProfileString("ports", "UDP", "3333", buffer, 256, ConfigFile);
    strncpy(port, (char *) buffer, 6);
    GetPrivateProfileString("ports", "TCP", "3000", buffer, 256, ConfigFile);
    sscanf(buffer, "%d", &fwdport);
    GetPrivateProfileString("misc", "debug", "0", buffer, 256, ConfigFile);
    debug = (0 != atoi(buffer));
    GetPrivateProfileString("osc", "path", "./osc.exe", buffer, 256, ConfigFile);
    strcpy(osc,(char *) buffer);
    GetPrivateProfileString("osc", "autostart", "0", buffer, 256, ConfigFile);
    aosc = (0 != atoi(buffer));
}

void SavePrefsEx()
{
    TCHAR acPath[256];
    GetModuleFileName(NULL,acPath,MAX_PATH);
    PathRemoveFileSpec(acPath);
    char ConfigFile[256];
    sprintf (ConfigFile,"%s\\toxy_win.ini",acPath);
    char buff[256];
    WritePrivateProfileString("ports", "UDP",   port,  ConfigFile);
    sprintf(buff, "%d", fwdport);
    WritePrivateProfileString("ports", "TCP", buff, ConfigFile);
    WritePrivateProfileString("osc", "path",   osc, ConfigFile);
    sprintf(buff, "%d", aosc);
    WritePrivateProfileString("osc", "autostart",   buff, ConfigFile);
    sprintf(buff, "%d", debug);
    WritePrivateProfileString("misc", "debug",    buff,  ConfigFile);
}

static void
set_errno(int winsock_err)
{
    switch (winsock_err)
    {
    case WSAEWOULDBLOCK:
        errno = EAGAIN;
        break;
    default:
        errno = winsock_err;
        break;
    }
}

int mingw_setnonblocking (SOCKET fd, int nonblocking)
{
    int rc;

    unsigned long mode = 1;
    rc = ioctlsocket(fd, FIONBIO, &mode);
    if (rc != 0)
    {
        set_errno(WSAGetLastError());
    }
    return (rc == 0 ? 0 : -1);
}

void initcp()
{
    WSADATA wsaData;
    WSAStartup(0x0101, &wsaData);
    char *opt;
    memset((char *)&address,0,sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((u_short)fwdport);

    if ( ((int)(ptrp = getprotobyname("tcp"))) == 0)
    {
        fprintf(stderr, "cannot map \"tcp\" to protocol number");
        exit(1);
    }
    servsock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    setsockopt(servsock,SOL_SOCKET,SO_REUSEADDR,(void *)&opt, sizeof(opt));
    mingw_setnonblocking(servsock, 1);
    if (servsock < 0)
    {
        fprintf(stderr, "socket creation failed\n");
        exit(1);
    }
    if (bind(servsock, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        fprintf(stderr,"bind failed\n");
        exit(1);
    }
    if (listen(servsock, QLEN) < 0)
    {
        fprintf(stderr,"listen failed\n");
        exit(1);
    }

}

void initlo()
{
    st = lo_server_new((char *) port, error);
    lo_server_add_method(st, NULL, NULL, quit_handler, NULL);
    lo_server_add_method(st, NULL, NULL, fwd_handler, NULL);
}

BOOL CALLBACK TerminateAppEnum( HWND hwnd, LPARAM lParam )
{
  DWORD dwID ;

  GetWindowThreadProcessId(hwnd, &dwID) ;

  if(dwID == (DWORD)lParam)
  {
     PostMessage(hwnd, WM_CLOSE, 0, 0) ;
  }

  return TRUE ;
}

DWORD WINAPI TerminateApp( DWORD dwPID, DWORD dwTimeout )
{
  HANDLE   hProc ;
  DWORD   dwRet ;

  // If we can't open the process with PROCESS_TERMINATE rights,
  // then we give up immediately.
  hProc = OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, FALSE,
     dwPID);

  if(hProc == NULL)
  {
     return TA_FAILED ;
  }

  // TerminateAppEnum() posts WM_CLOSE to all windows whose PID
  // matches your process's.
  EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM) dwPID) ;

  // Wait on the handle. If it signals, great. If it times out,
  // then you kill it.
  if(WaitForSingleObject(hProc, dwTimeout)!=WAIT_OBJECT_0)
     dwRet=(TerminateProcess(hProc,0)?TA_SUCCESS_KILL:TA_FAILED);
  else
     dwRet = TA_SUCCESS_CLEAN ;

  CloseHandle(hProc) ;

  return dwRet ;
}

int main(int argc, char *argv[])
{
    void *status;
    LoadPrefsEx();
    int pid = NULL;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    if (PathFileExistsA (osc) && aosc)
{
    memset(&StartupInfo, 0, sizeof(StartupInfo));
	StartupInfo.cb = sizeof(STARTUPINFO);
	StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	StartupInfo.wShowWindow = SW_HIDE;
	
    if (!CreateProcess( NULL, osc, NULL, NULL, FALSE,
		DETACHED_PROCESS, //CREATE_NEW_CONSOLE creates new console DETACHED_PROCESS does not automatically create a new console
		NULL, 
		NULL,
		&StartupInfo,
		&ProcessInfo))
	{
		return GetLastError();		
	}
	pid = ProcessInfo.hProcess;
}
    int lo_fd;
    int retval;
    fd_set rfds;
    initcp();
    initlo();
    lo_fd = lo_server_get_socket_fd(st);
    status = CreateThread(NULL,0, (LPTHREAD_START_ROUTINE)_WinMain, 0, 0, 0);

    if (lo_fd > 0)
    {

        do
        {
            FD_ZERO(&rfds);
            FD_SET(lo_fd, &rfds);

            retval = select(lo_fd + 1, &rfds, NULL, NULL, 0); /* no timeout */

            if (retval == -1)
            {

                if (debug) printf("select() error\n");
                exit(1);

            }
            else if (retval > 0)
            {
                if (FD_ISSET(lo_fd, &rfds))
                {

                    lo_server_recv_noblock(st, 0);

                }
            }
        }
        while (!done);
    }
    SavePrefsEx();
    //if (pid && file) fclose(file);
    if (pid)
    {
    SendMessage(ProcessInfo.hProcess, WM_DESTROY, 0, 0);// WM_CLOSE / WM_DESTROY
    SendMessage(ProcessInfo.hProcess, WM_CLOSE, 0, 0);// WM_CLOSE / WM_DESTROY
    //printf("th: %d, pid %d\n",ProcessInfo.hThread,ProcessInfo.hProcess);
    TerminateApp(ProcessInfo.hProcess, 100000);
    TerminateProcess(ProcessInfo.hProcess, 0);
    CloseHandle(ProcessInfo.hThread);
	CloseHandle(ProcessInfo.hProcess);
    }
    closesocket(clientsock);
    closesocket(servsock);
    exit (0);
}
void error(int num, const char *msg, const char *path)
{
    if (debug) printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

int fwd_handler(const char *path, const char *types, lo_arg **argv,
                int argc, void *data, void *user)
{
    int alen,j;
    alen = sizeof(address);
    if ( !clientsock || clientsock < 0 )
    {
        clientsock=accept(servsock, (struct sockaddr *)&address, &alen);
        if ( (clientsock <0) && connected>0)
        {
            connected = 0;
            ModifyTrayIcon( hWndtray, ID_TRAYICON, ID_TRAYICONR, "No tcp connection.." );
        }
        else if ((clientsock >= 0) && connected<=0)
        {
            ModifyTrayIcon( hWndtray, ID_TRAYICON, ID_TRAYICONB, "Tcp Client connected.." );
            connected = 1;
        }
    }
    if ( !strcmp((char *) argv[0],"set") )
    {
        set *msgset;
        int status;
        msgset = calloc(1, sizeof(set));
        strcpy(msgset->path, path);
        strcpy (msgset->name, (char *) argv[0]);
        msgset->sessionID = argv[1]->i;
        msgset->x = argv[2]->f;
        msgset->y = argv[3]->f;
        msgset->X = argv[4]->f;
        msgset->Y = argv[5]->f;
        msgset->m = argv[6]->f;
        if (argc > 7)
        {
            msgset->w = argv[7]->f;
            msgset->h = argv[8]->f;
        }
        if (debug) printf ("path: %s name: %s ID: %d x: %f y: %f X: %f Y: %f m: %f\n",msgset->path, msgset->name, msgset->sessionID, msgset->x, msgset->y, msgset->X, msgset->Y, msgset->m);
        status = send(clientsock,(void *) msgset,sizeof(*msgset),0);
        if (status < 0)
        {
            closesocket(clientsock);
            clientsock = 0;
        }
        free (msgset);
    }
    else if ( !strcmp((char *) argv[0],"fseq") && argv[1]->i != -1 )
    {
        fseq *msgset;
        int status;
        msgset = calloc (1, sizeof(fseq));
        strcpy(msgset->path, path);
        strcpy (msgset->name, (char *) argv[0]);
        msgset->framenum = argv[1]->i;
        if (debug) printf("path: %s name: %s fremseq: %d\n",msgset->path, msgset->name, msgset->framenum);
        alen = sizeof(address);
        status = send(clientsock,(void *) msgset,sizeof(*msgset),0);
        if (status < 0)
        {
            closesocket(clientsock);
            clientsock = 0;
        }
        free(msgset);
    }
    else if ( !strcmp((char *) argv[0],"alive") )
    {
        struct _alive
        {
            char path[12];
            char name[8];
            int blobs[argc];
        };

        struct _alive *msgset;
        int status;
        msgset = calloc (1, sizeof(struct _alive));

        strcpy(msgset->path, path);
        strcpy (msgset->name, (char *) argv[0]);
        msgset->blobs[0] = argc;
        if (debug) printf("path: %s name: %s ",msgset->path, msgset->name);
        for (j=1;j<argc;j++)
        {
            msgset->blobs[j] = argv[j]->i;
            if (debug) printf("Id[%d] - %d ",j-1,msgset->blobs[j]);
        }
        if (debug) printf("\n");
        status = send(clientsock,(void *) msgset,sizeof(*msgset),0);
        if (status < 0)
        {
            closesocket(clientsock);
            clientsock = 0;
        }
        free(msgset);
    }
    return 0;
}

int quit_handler(const char *path, const char *types, lo_arg **argv, int argc,
                 void *data, void *user_data)
{
    if ( !strcmp((char *) path ,"/quit"))
    {
        done = 1;
        return 0;
    }
    else if ( !strcmp((char *) path ,"/toggle"))
    {
        toggle = !toggle;
        if (connected < 0)
        {
            if (!toggle) ModifyTrayIcon( hWndtray, ID_TRAYICON, ID_TRAYICON, "Tuio processing stopped.." );
            else ModifyTrayIcon( hWndtray, ID_TRAYICON, ID_TRAYICON, "Tuio processing enabled.." );
            return 0;
        }
        else if (connected && toggle)
            ModifyTrayIcon( hWndtray, ID_TRAYICON, ID_TRAYICONB, "Tuio processing enabled.." );
        else if (!connected && toggle ) ModifyTrayIcon( hWndtray, ID_TRAYICON, ID_TRAYICONR, "Tuio processing stopped.." );
        else ModifyTrayIcon( hWndtray, ID_TRAYICON, ID_TRAYICON, "Tuio processing stoped.." );
    }
    else if (!strcmp((char *) path ,"/tuio/2Dobj") || !strcmp((char *) path ,"/tuio/2Dcur"))
    {
        return toggle;
    }
    return 0;
}
