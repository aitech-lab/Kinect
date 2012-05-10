#ifndef __KINECT__
#define __KINECT__
// Windows Header Files

#include "resource.h"
#include <windows.h>
#include <WinBase.h>
#include <ole2.h>
#include <stdio.h>

// C RunTime Header Files
#include <stdlib.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <strsafe.h> 

#include "NuiApi.h"


class Kinect {

public:
    Kinect();
   ~Kinect();

    HRESULT                 Nui_Init  ( );
    void                    Nui_UnInit( );

    void                    Nui_GotDepthAlert   ( );
    void                    Nui_GotColorAlert   ( );
    void                    Nui_GotSkeletonAlert( );

    void                    Nui_Zero();
    void                    Nui_DoDoubleBuffer(HWND hWnd,HDC hDC);

    void                    Nui_EnableSeatedTracking(bool seated);
    void                    Nui_SetApplicationTracking(bool applicationTracks);
    void                    Nui_SetTrackedSkeletons(int skel1, int skel2);

    RGBQUAD                 Nui_ShortToQuad_Depth( USHORT s );

    static LRESULT CALLBACK MessageRouter      (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK        WndProc            (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static void CALLBACK    Nui_StatusProcThunk(HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName, void* pUserData);
    void CALLBACK           Nui_StatusProc     ( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName );

    // HWND                    m_hWnd;
    // HINSTANCE               m_hInstance;

	unsigned char colorBuffer[640*480*4];
	unsigned char depthBuffer[320*240*4];

private:
    void UpdateComboBox();
    void ClearComboBox();

    void UpdateTrackingComboBoxes();
    void UpdateTrackingFromComboBoxes();

    bool                    m_fUpdatingUi;
    TCHAR                   m_szAppTitle[256];    // Application title
    static DWORD WINAPI     Nui_ProcessThread(LPVOID pParam);
    DWORD WINAPI            Nui_ProcessThread();

    // Current kinect
    INuiSensor *            m_pNuiSensor;
    BSTR                    m_instanceId;

    // Draw devices
    // DrawDevice *            m_pDrawDepth;
    // DrawDevice *            m_pDrawColor;
    // ID2D1Factory *          m_pD2DFactory;

    // thread handling
    HANDLE        m_hThNuiProcess;
    HANDLE        m_hEvNuiProcessStop;
    HANDLE        m_hNextDepthFrameEvent;
    HANDLE        m_hNextColorFrameEvent;
    HANDLE        m_hNextSkeletonEvent;
    HANDLE        m_pDepthStreamHandle;
    HANDLE        m_pVideoStreamHandle;

	HFONT         m_hFontFPS;
    HFONT         m_hFontSkeletonId;

	HPEN          m_Pen[NUI_SKELETON_COUNT];
    HDC           m_SkeletonDC;
    HBITMAP       m_SkeletonBMP;
    HGDIOBJ       m_SkeletonOldObj;
    int           m_PensTotal;
    POINT         m_Points[NUI_SKELETON_POSITION_COUNT];
    RGBQUAD       m_rgbWk[640*480];
    DWORD         m_LastSkeletonFoundTime;
    bool          m_bScreenBlanked;
    bool          m_bAppTracking;
    int           m_DepthFramesTotal;
    DWORD         m_LastDepthFPStime;
    int           m_LastDepthFramesTotal;
    DWORD         m_SkeletonIds[NUI_SKELETON_COUNT];
    DWORD         m_TrackedSkeletonIds[NUI_SKELETON_MAX_TRACKED_COUNT];
    ULONG_PTR     m_GdiplusToken;

};

#endif __KINECT__

