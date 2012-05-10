//------------------------------------------------------------------------------
// <copyright file="SkeletalViewer.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

// This module provides sample code used to demonstrate Kinect NUI processing

// Note: 
//     Platform SDK lib path should be added before the VC lib
//     path, because uuid.lib in VC lib path may be older

#include "stdafx.h"
#include <strsafe.h>
#include "SkeletalViewer.h"
#include "resource.h"

// Global Variables:
CSkeletalViewerApp  g_skeletalViewerApp;  // Application class

#define INSTANCE_MUTEX_NAME L"SkeletalViewerInstanceCheck"

//-------------------------------------------------------------------
// _tWinMain
//
// Entry point for the application
//-------------------------------------------------------------------
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    MSG       msg;
    WNDCLASS  wc;
        
    // unique mutex, if it already exists there is already an instance of this app running
    // in that case we want to show the user an error dialog
    HANDLE hMutex = CreateMutex( NULL, FALSE, INSTANCE_MUTEX_NAME );
    if ( (hMutex != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS) ) 
    {
        TCHAR szAppTitle[256] = { 0 };
        TCHAR szRes[512] = { 0 };

        //load the app title
        LoadString( hInstance, IDS_APPTITLE, szAppTitle, _countof(szAppTitle) );

        //load the error string
        LoadString( hInstance, IDS_ERROR_APP_INSTANCE, szRes, _countof(szRes) );

        MessageBox( NULL, szRes, szAppTitle, MB_OK | MB_ICONHAND );

        CloseHandle(hMutex);
        return -1;
    }

    // Store the instance handle
    g_skeletalViewerApp.m_hInstance = hInstance;

    // Dialog custom window class
    ZeroMemory( &wc,sizeof(wc) );
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbWndExtra = DLGWINDOWEXTRA;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL,IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_SKELETALVIEWER));
    wc.lpfnWndProc = DefDlgProc;
    wc.lpszClassName = SZ_APPDLG_WINDOW_CLASS;
    if( !RegisterClass(&wc) )
    {
        return 0;
    }

    // Create main application window
    HWND hWndApp = CreateDialogParam(
        hInstance,
        MAKEINTRESOURCE(IDD_APP),
        NULL,
        (DLGPROC) CSkeletalViewerApp::MessageRouter, 
        reinterpret_cast<LPARAM>(&g_skeletalViewerApp));

    // Show window
    ShowWindow(hWndApp,nCmdShow); 

    // Main message loop:
    while( GetMessage( &msg, NULL, 0, 0 ) ) 
    {
        // If a dialog message will be taken care of by the dialog proc
        if ( (hWndApp != NULL) && IsDialogMessage(hWndApp, &msg) )
        {
            continue;
        }

        // otherwise do our window processing
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CloseHandle( hMutex );

    return static_cast<int>(msg.wParam);
}

//-------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------
CSkeletalViewerApp::CSkeletalViewerApp() : m_hInstance(NULL)
{
    ZeroMemory(m_szAppTitle, sizeof(m_szAppTitle));
    LoadString(m_hInstance, IDS_APPTITLE, m_szAppTitle, _countof(m_szAppTitle));

    m_fUpdatingUi = false;
    Nui_Zero();

    // Init Direct2D
    D2D1CreateFactory( D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory );
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
CSkeletalViewerApp::~CSkeletalViewerApp()
{
    // Clean up Direct2D
    SafeRelease( m_pD2DFactory );

    Nui_Zero();
    SysFreeString(m_instanceId);
}

void CSkeletalViewerApp::ClearComboBox()
{
    for ( long i = 0; i < SendDlgItemMessage(m_hWnd, IDC_CAMERAS, CB_GETCOUNT, 0, 0); i++ )
    {
        SysFreeString( reinterpret_cast<BSTR>( SendDlgItemMessage(m_hWnd, IDC_CAMERAS, CB_GETITEMDATA, i, 0) ) );
    }
    SendDlgItemMessage(m_hWnd, IDC_CAMERAS, CB_RESETCONTENT, 0, 0);
}

void CSkeletalViewerApp::UpdateComboBox()
{
    m_fUpdatingUi = true;
    ClearComboBox();

    int numDevices = 0;
    HRESULT hr = NuiGetSensorCount(&numDevices);

    if ( FAILED(hr) )
    {
        return;
    }

    EnableWindow(GetDlgItem(m_hWnd, IDC_APPTRACKING), numDevices > 0);

    long selectedIndex = 0;
    for ( int i = 0; i < numDevices; i++ )
    {
        INuiSensor *pNui = NULL;
        HRESULT hr = NuiCreateSensorByIndex(i,  &pNui);
        if (SUCCEEDED(hr))
        {
            HRESULT status = pNui ? pNui->NuiStatus() : E_NUI_NOTCONNECTED;
            if (status == E_NUI_NOTCONNECTED)
            {
                pNui->Release();
                continue;
            }
            
            WCHAR kinectName[MAX_PATH];
            StringCchPrintfW( kinectName, _countof(kinectName), L"Kinect %d", i);
            long index = static_cast<long>( SendDlgItemMessage(m_hWnd, IDC_CAMERAS, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(kinectName)) );
            SendDlgItemMessage( m_hWnd, IDC_CAMERAS, CB_SETITEMDATA, index, reinterpret_cast<LPARAM>(pNui->NuiUniqueId()) );
            if (m_pNuiSensor && pNui == m_pNuiSensor)
            {
                selectedIndex = index;
            }
            pNui->Release();
        }
    }

    SendDlgItemMessage(m_hWnd, IDC_CAMERAS, CB_SETCURSEL, selectedIndex, 0);
    m_fUpdatingUi = false;
}

void CSkeletalViewerApp::UpdateTrackingComboBoxes()
{
    SendDlgItemMessage(m_hWnd, IDC_TRACK0, CB_RESETCONTENT, 0, 0);
    SendDlgItemMessage(m_hWnd, IDC_TRACK1, CB_RESETCONTENT, 0, 0);

    SendDlgItemMessage(m_hWnd, IDC_TRACK0, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"0"));
    SendDlgItemMessage(m_hWnd, IDC_TRACK1, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"0"));
    
    SendDlgItemMessage(m_hWnd, IDC_TRACK0, CB_SETITEMDATA, 0, 0);
    SendDlgItemMessage(m_hWnd, IDC_TRACK1, CB_SETITEMDATA, 0, 0);

    bool setCombo0 = false;
    bool setCombo1 = false;
    
    for ( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
    {
        if ( m_SkeletonIds[i] != 0 )
        {
            WCHAR trackingId[MAX_PATH];
            StringCchPrintfW(trackingId, _countof(trackingId), L"%d", m_SkeletonIds[i]);
            LRESULT pos0 = SendDlgItemMessage(m_hWnd, IDC_TRACK0, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(trackingId));
            LRESULT pos1 = SendDlgItemMessage(m_hWnd, IDC_TRACK1, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(trackingId));

            SendDlgItemMessage(m_hWnd, IDC_TRACK0, CB_SETITEMDATA, pos0, m_SkeletonIds[i]);
            SendDlgItemMessage(m_hWnd, IDC_TRACK1, CB_SETITEMDATA, pos1, m_SkeletonIds[i]);

            if ( m_TrackedSkeletonIds[0] == m_SkeletonIds[i] )
            {
                SendDlgItemMessage(m_hWnd, IDC_TRACK0, CB_SETCURSEL, pos0, 0);
                setCombo0 = true;
            }

            if ( m_TrackedSkeletonIds[1] == m_SkeletonIds[i] )
            {
                SendDlgItemMessage(m_hWnd, IDC_TRACK1, CB_SETCURSEL, pos1, 0);
                setCombo1 = true;
            }
        }
    }
    
    if ( !setCombo0 )
    {
        SendDlgItemMessage( m_hWnd, IDC_TRACK0, CB_SETCURSEL, 0, 0 );
    }

    if ( !setCombo1 )
    {
        SendDlgItemMessage( m_hWnd, IDC_TRACK1, CB_SETCURSEL, 0, 0 );
    }
}

void CSkeletalViewerApp::UpdateTrackingFromComboBoxes()
{
    LRESULT trackingIndex0 = SendDlgItemMessage(m_hWnd, IDC_TRACK0, CB_GETCURSEL, 0, 0);
    LRESULT trackingIndex1 = SendDlgItemMessage(m_hWnd, IDC_TRACK1, CB_GETCURSEL, 0, 0);

    LRESULT trackingId0 = SendDlgItemMessage(m_hWnd, IDC_TRACK0, CB_GETITEMDATA, trackingIndex0, 0);
    LRESULT trackingId1 = SendDlgItemMessage(m_hWnd, IDC_TRACK0, CB_GETITEMDATA, trackingIndex1, 0);

    Nui_SetTrackedSkeletons(static_cast<int>(trackingId0), static_cast<int>(trackingId1));
}

LRESULT CALLBACK CSkeletalViewerApp::MessageRouter( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CSkeletalViewerApp *pThis = NULL;
    
    if ( WM_INITDIALOG == uMsg )
    {
        pThis = reinterpret_cast<CSkeletalViewerApp*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        NuiSetDeviceStatusCallback( &CSkeletalViewerApp::Nui_StatusProcThunk, pThis );
    }
    else
    {
        pThis = reinterpret_cast<CSkeletalViewerApp*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if ( NULL != pThis )
    {
        return pThis->WndProc( hwnd, uMsg, wParam, lParam );
    }

    return 0;
}

//-------------------------------------------------------------------
// WndProc
//
// Handle windows messages
//-------------------------------------------------------------------
LRESULT CALLBACK CSkeletalViewerApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_INITDIALOG:
        {
            LOGFONT lf;

            // Clean state the class
            Nui_Zero();

            // Bind application window handle
            m_hWnd = hWnd;

            // Set the font for Frames Per Second display
            GetObject( (HFONT) GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf );
            lf.lfHeight *= 4;
            m_hFontFPS = CreateFontIndirect(&lf);
            SendDlgItemMessage( hWnd, IDC_FPS, WM_SETFONT, (WPARAM) m_hFontFPS, 0 );

            UpdateComboBox();
            SendDlgItemMessage(m_hWnd, IDC_CAMERAS, CB_SETCURSEL, 0, 0);

            // Initialize and start NUI processing
            Nui_Init();
        }
        break;

        case WM_USER_UPDATE_FPS:
        {
            ::SetDlgItemInt( m_hWnd, static_cast<int>(wParam), static_cast<int>(lParam), FALSE );
        }
        break;

        case WM_USER_UPDATE_COMBO:
        {
            UpdateComboBox();
        }
        break;

        case WM_COMMAND:
        {
            if( HIWORD( wParam ) == CBN_SELCHANGE )
            {
                switch (LOWORD(wParam))
                {
                    case IDC_CAMERAS:
                    {
                        LRESULT index = ::SendDlgItemMessage(m_hWnd, IDC_CAMERAS, CB_GETCURSEL, 0, 0);

                        // Don't reconnect as a result of updating the combo box
                        if ( !m_fUpdatingUi )
                        {
                            Nui_UnInit();
                            Nui_Zero();
                            Nui_Init(reinterpret_cast<BSTR>(::SendDlgItemMessage(m_hWnd, IDC_CAMERAS, CB_GETITEMDATA, index, 0)));
                        }
                    }
                    break;

                    case IDC_TRACK0:
                    case IDC_TRACK1:
                    {
                        UpdateTrackingFromComboBoxes();
                    }
                    break;
                }
            }
            else if ( HIWORD( wParam ) == BN_CLICKED )
            {
                switch (LOWORD(wParam))
                {
                    case IDC_APPTRACKING:
                    {
                        bool checked = IsDlgButtonChecked(m_hWnd, IDC_APPTRACKING) == BST_CHECKED;
                        m_bAppTracking = checked;

                        EnableWindow( GetDlgItem(m_hWnd, IDC_TRACK0), checked );
                        EnableWindow( GetDlgItem(m_hWnd, IDC_TRACK1), checked );

                        if ( checked )
                        {
                            UpdateTrackingComboBoxes();
                        }

                        Nui_SetApplicationTracking(checked);

                        if ( checked )
                        {
                            UpdateTrackingFromComboBoxes();
                        }
                    }
                    break;
                }
            }
        }
        break;

        // If the titlebar X is clicked destroy app
        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            // Uninitialize NUI
            Nui_UnInit();

            // Other cleanup
            ClearComboBox();
            DeleteObject(m_hFontFPS);

            // Quit the main message pump
            PostQuitMessage(0);
            break;
    }

    return FALSE;
}

//-------------------------------------------------------------------
// MessageBoxResource
//
// Display a MessageBox with a string table table loaded string
//-------------------------------------------------------------------
int CSkeletalViewerApp::MessageBoxResource( UINT nID, UINT nType )
{
    static TCHAR szRes[512];

    LoadString( m_hInstance, nID, szRes, _countof(szRes) );
    return MessageBox( m_hWnd, szRes, m_szAppTitle, nType );
}