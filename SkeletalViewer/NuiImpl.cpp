//------------------------------------------------------------------------------
// <copyright file="NuiImpl.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

// Implementation of CSkeletalViewerApp methods dealing with NUI processing

#include "stdafx.h"
#include "SkeletalViewer.h"
#include "resource.h"
#include <mmsystem.h>
#include <assert.h>
#include <strsafe.h>

static const COLORREF g_JointColorTable[NUI_SKELETON_POSITION_COUNT] = 
{
    RGB(169, 176, 155), // NUI_SKELETON_POSITION_HIP_CENTER
    RGB(169, 176, 155), // NUI_SKELETON_POSITION_SPINE
    RGB(168, 230, 29),  // NUI_SKELETON_POSITION_SHOULDER_CENTER
    RGB(200, 0,   0),   // NUI_SKELETON_POSITION_HEAD
    RGB(79,  84,  33),  // NUI_SKELETON_POSITION_SHOULDER_LEFT
    RGB(84,  33,  42),  // NUI_SKELETON_POSITION_ELBOW_LEFT
    RGB(255, 126, 0),   // NUI_SKELETON_POSITION_WRIST_LEFT
    RGB(215,  86, 0),   // NUI_SKELETON_POSITION_HAND_LEFT
    RGB(33,  79,  84),  // NUI_SKELETON_POSITION_SHOULDER_RIGHT
    RGB(33,  33,  84),  // NUI_SKELETON_POSITION_ELBOW_RIGHT
    RGB(77,  109, 243), // NUI_SKELETON_POSITION_WRIST_RIGHT
    RGB(37,   69, 243), // NUI_SKELETON_POSITION_HAND_RIGHT
    RGB(77,  109, 243), // NUI_SKELETON_POSITION_HIP_LEFT
    RGB(69,  33,  84),  // NUI_SKELETON_POSITION_KNEE_LEFT
    RGB(229, 170, 122), // NUI_SKELETON_POSITION_ANKLE_LEFT
    RGB(255, 126, 0),   // NUI_SKELETON_POSITION_FOOT_LEFT
    RGB(181, 165, 213), // NUI_SKELETON_POSITION_HIP_RIGHT
    RGB(71, 222,  76),  // NUI_SKELETON_POSITION_KNEE_RIGHT
    RGB(245, 228, 156), // NUI_SKELETON_POSITION_ANKLE_RIGHT
    RGB(77,  109, 243)  // NUI_SKELETON_POSITION_FOOT_RIGHT
};

static const COLORREF g_SkeletonColors[NUI_SKELETON_COUNT] =
{
    RGB( 255, 0, 0),
    RGB( 0, 255, 0 ),
    RGB( 64, 255, 255 ),
    RGB( 255, 255, 64 ),
    RGB( 255, 64, 255 ),
    RGB( 128, 128, 255 )
};

//lookups for color tinting based on player index
static const int g_IntensityShiftByPlayerR[] = { 1, 2, 0, 2, 0, 0, 2, 0 };
static const int g_IntensityShiftByPlayerG[] = { 1, 2, 2, 0, 2, 0, 0, 1 };
static const int g_IntensityShiftByPlayerB[] = { 1, 0, 2, 2, 0, 2, 0, 2 };


//-------------------------------------------------------------------
// Nui_Zero
//
// Zero out member variables
//-------------------------------------------------------------------
void CSkeletalViewerApp::Nui_Zero()
{
    if (m_pNuiSensor)
    {
        m_pNuiSensor->Release();
        m_pNuiSensor = NULL;
    }
    m_hNextDepthFrameEvent = NULL;
    m_hNextColorFrameEvent = NULL;
    m_hNextSkeletonEvent = NULL;
    m_pDepthStreamHandle = NULL;
    m_pVideoStreamHandle = NULL;
    m_hThNuiProcess = NULL;
    m_hEvNuiProcessStop = NULL;
    ZeroMemory(m_Pen,sizeof(m_Pen));
    m_SkeletonDC = NULL;
    m_SkeletonBMP = NULL;
    m_SkeletonOldObj = NULL;
    m_PensTotal = 6;
    ZeroMemory(m_Points,sizeof(m_Points));
    m_LastSkeletonFoundTime = 0;
    m_bScreenBlanked = false;
    m_DepthFramesTotal = 0;
    m_LastDepthFPStime = 0;
    m_LastDepthFramesTotal = 0;
    m_pDrawDepth = NULL;
    m_pDrawColor = NULL;
    ZeroMemory(m_SkeletonIds,sizeof(m_SkeletonIds));
    ZeroMemory(m_TrackedSkeletonIds,sizeof(m_SkeletonIds));
}

void CALLBACK CSkeletalViewerApp::Nui_StatusProcThunk( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName, void * pUserData )
{
    reinterpret_cast<CSkeletalViewerApp *>(pUserData)->Nui_StatusProc( hrStatus, instanceName, uniqueDeviceName );
}

//-------------------------------------------------------------------
// Nui_StatusProc
//
// Callback to handle Kinect status changes
//-------------------------------------------------------------------
void CALLBACK CSkeletalViewerApp::Nui_StatusProc( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName )
{
    // Update UI
    PostMessageW( m_hWnd, WM_USER_UPDATE_COMBO, 0, 0 );

    if( SUCCEEDED(hrStatus) )
    {
        if ( S_OK == hrStatus )
        {
            if ( m_instanceId && 0 == wcscmp(instanceName, m_instanceId) )
            {
                Nui_Init(m_instanceId);
            }
            else if ( !m_pNuiSensor )
            {
                Nui_Init();
            }
        }
    }
    else
    {
        if ( m_instanceId && 0 == wcscmp(instanceName, m_instanceId) )
        {
            Nui_UnInit();
            Nui_Zero();
        }
    }
}

//-------------------------------------------------------------------
// Nui_Init
//
// Initialize Kinect by instance name
//-------------------------------------------------------------------
HRESULT CSkeletalViewerApp::Nui_Init( OLECHAR *instanceName )
{
    // Generic creation failure
    if ( NULL == instanceName )
    {
        MessageBoxResource( IDS_ERROR_NUICREATE, MB_OK | MB_ICONHAND );
        return E_FAIL;
    }

    HRESULT hr = NuiCreateSensorById( instanceName, &m_pNuiSensor );
    
    // Generic creation failure
    if ( FAILED(hr) )
    {
        MessageBoxResource( IDS_ERROR_NUICREATE, MB_OK | MB_ICONHAND );
        return hr;
    }

    SysFreeString(m_instanceId);

    m_instanceId = m_pNuiSensor->NuiDeviceConnectionId();

    return Nui_Init();
}

//-------------------------------------------------------------------
// Nui_Init
//
// Initialize Kinect
//-------------------------------------------------------------------
HRESULT CSkeletalViewerApp::Nui_Init( )
{
    HRESULT  hr;
    RECT     rc;
    bool     result;

    if ( !m_pNuiSensor )
    {
        HRESULT hr = NuiCreateSensorByIndex(0, &m_pNuiSensor);

        if ( FAILED(hr) )
        {
            return hr;
        }

        SysFreeString(m_instanceId);

        m_instanceId = m_pNuiSensor->NuiDeviceConnectionId();
    }

    m_hNextDepthFrameEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    m_hNextColorFrameEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    m_hNextSkeletonEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

    GetWindowRect( GetDlgItem( m_hWnd, IDC_SKELETALVIEW ), &rc );  
    HDC hdc = GetDC( GetDlgItem( m_hWnd, IDC_SKELETALVIEW) );
    
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    m_SkeletonBMP = CreateCompatibleBitmap( hdc, width, height );
    m_SkeletonDC = CreateCompatibleDC( hdc );
    
    ReleaseDC(GetDlgItem(m_hWnd,IDC_SKELETALVIEW), hdc );
    m_SkeletonOldObj = SelectObject( m_SkeletonDC, m_SkeletonBMP );

    m_pDrawDepth = new DrawDevice( );
    result = m_pDrawDepth->Initialize( GetDlgItem( m_hWnd, IDC_DEPTHVIEWER ), m_pD2DFactory, 320, 240, 320 * 4 );
    if ( !result )
    {
        MessageBoxResource( IDS_ERROR_DRAWDEVICE, MB_OK | MB_ICONHAND );
        return E_FAIL;
    }

    m_pDrawColor = new DrawDevice( );
    result = m_pDrawColor->Initialize( GetDlgItem( m_hWnd, IDC_VIDEOVIEW ), m_pD2DFactory, 640, 480, 640 * 4 );
    if ( !result )
    {
        MessageBoxResource( IDS_ERROR_DRAWDEVICE, MB_OK | MB_ICONHAND );
        return E_FAIL;
    }
    
    DWORD nuiFlags = NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON |  NUI_INITIALIZE_FLAG_USES_COLOR;
    hr = m_pNuiSensor->NuiInitialize( nuiFlags );
    if ( E_NUI_SKELETAL_ENGINE_BUSY == hr )
    {
        nuiFlags = NUI_INITIALIZE_FLAG_USES_DEPTH |  NUI_INITIALIZE_FLAG_USES_COLOR;
        hr = m_pNuiSensor->NuiInitialize( nuiFlags) ;
    }
  
    if ( FAILED( hr ) )
    {
        if ( E_NUI_DEVICE_IN_USE == hr )
        {
            MessageBoxResource( IDS_ERROR_IN_USE, MB_OK | MB_ICONHAND );
        }
        else
        {
            MessageBoxResource( IDS_ERROR_NUIINIT, MB_OK | MB_ICONHAND );
        }
        return hr;
    }

    if ( HasSkeletalEngine( m_pNuiSensor ) )
    {
        hr = m_pNuiSensor->NuiSkeletonTrackingEnable( m_hNextSkeletonEvent, 0 );
        if( FAILED( hr ) )
        {
            MessageBoxResource( IDS_ERROR_SKELETONTRACKING, MB_OK | MB_ICONHAND );
            return hr;
        }
    }

    hr = m_pNuiSensor->NuiImageStreamOpen(
        NUI_IMAGE_TYPE_COLOR,
        NUI_IMAGE_RESOLUTION_640x480,
        0,
        2,
        m_hNextColorFrameEvent,
        &m_pVideoStreamHandle );

    if ( FAILED( hr ) )
    {
        MessageBoxResource( IDS_ERROR_VIDEOSTREAM, MB_OK | MB_ICONHAND );
        return hr;
    }

    hr = m_pNuiSensor->NuiImageStreamOpen(
        HasSkeletalEngine(m_pNuiSensor) ? NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX : NUI_IMAGE_TYPE_DEPTH,
        NUI_IMAGE_RESOLUTION_320x240,
        0,
        2,
        m_hNextDepthFrameEvent,
        &m_pDepthStreamHandle );

    if ( FAILED( hr ) )
    {
        MessageBoxResource(IDS_ERROR_DEPTHSTREAM, MB_OK | MB_ICONHAND);
        return hr;
    }

    // Start the Nui processing thread
    m_hEvNuiProcessStop = CreateEvent( NULL, FALSE, FALSE, NULL );
    m_hThNuiProcess = CreateThread( NULL, 0, Nui_ProcessThread, this, 0, NULL );

    return hr;
}

//-------------------------------------------------------------------
// Nui_UnInit
//
// Uninitialize Kinect
//-------------------------------------------------------------------
void CSkeletalViewerApp::Nui_UnInit( )
{
    SelectObject( m_SkeletonDC, m_SkeletonOldObj );
    DeleteDC( m_SkeletonDC );
    DeleteObject( m_SkeletonBMP );

    if ( NULL != m_Pen[0] )
    {
        for ( int i = 0; i < NUI_SKELETON_COUNT; i++ )
        {
            DeleteObject( m_Pen[i] );
        }
        ZeroMemory( m_Pen, sizeof(m_Pen) );
    }

    if ( NULL != m_hFontSkeletonId )
    {
        DeleteObject( m_hFontSkeletonId );
        m_hFontSkeletonId = NULL;
    }

    // Stop the Nui processing thread
    if ( NULL != m_hEvNuiProcessStop )
    {
        // Signal the thread
        SetEvent(m_hEvNuiProcessStop);

        // Wait for thread to stop
        if ( NULL != m_hThNuiProcess )
        {
            WaitForSingleObject( m_hThNuiProcess, INFINITE );
            CloseHandle( m_hThNuiProcess );
        }
        CloseHandle( m_hEvNuiProcessStop );
    }

    if ( m_pNuiSensor )
    {
        m_pNuiSensor->NuiShutdown( );
    }
    if ( m_hNextSkeletonEvent && ( m_hNextSkeletonEvent != INVALID_HANDLE_VALUE ) )
    {
        CloseHandle( m_hNextSkeletonEvent );
        m_hNextSkeletonEvent = NULL;
    }
    if ( m_hNextDepthFrameEvent && ( m_hNextDepthFrameEvent != INVALID_HANDLE_VALUE ) )
    {
        CloseHandle( m_hNextDepthFrameEvent );
        m_hNextDepthFrameEvent = NULL;
    }
    if ( m_hNextColorFrameEvent && ( m_hNextColorFrameEvent != INVALID_HANDLE_VALUE ) )
    {
        CloseHandle( m_hNextColorFrameEvent );
        m_hNextColorFrameEvent = NULL;
    }

    if ( m_pNuiSensor )
    {
        m_pNuiSensor->Release();
        m_pNuiSensor = NULL;
    }

    // clean up graphics
    delete m_pDrawDepth;
    m_pDrawDepth = NULL;

    delete m_pDrawColor;
    m_pDrawColor = NULL;    
}

DWORD WINAPI CSkeletalViewerApp::Nui_ProcessThread(LPVOID pParam)
{
    CSkeletalViewerApp *pthis = (CSkeletalViewerApp *) pParam;
    return pthis->Nui_ProcessThread();
}

//-------------------------------------------------------------------
// Nui_ProcessThread
//
// Thread to handle Kinect processing
//-------------------------------------------------------------------
DWORD WINAPI CSkeletalViewerApp::Nui_ProcessThread()
{
    const int numEvents = 4;
    HANDLE hEvents[numEvents] = { m_hEvNuiProcessStop, m_hNextDepthFrameEvent, m_hNextColorFrameEvent, m_hNextSkeletonEvent };
    int    nEventIdx;
    DWORD  t;

    m_LastDepthFPStime = timeGetTime( );

    //blank the skeleton display on startup
    m_LastSkeletonFoundTime = 0;

    // Main thread loop
    bool continueProcessing = true;
    while ( continueProcessing )
    {
        // Wait for any of the events to be signalled
        nEventIdx = WaitForMultipleObjects( numEvents, hEvents, FALSE, 100 );

        // Process signal events
        switch ( nEventIdx )
        {
            case WAIT_TIMEOUT:
                continue;

            // If the stop event, stop looping and exit
            case WAIT_OBJECT_0:
                continueProcessing = false;
                continue;

            case WAIT_OBJECT_0 + 1:
                Nui_GotDepthAlert();
                 ++m_DepthFramesTotal;
                break;

            case WAIT_OBJECT_0 + 2:
                Nui_GotColorAlert();
                break;

            case WAIT_OBJECT_0 + 3:
                Nui_GotSkeletonAlert( );
                break;
        }

        // Once per second, display the depth FPS
        t = timeGetTime( );
        if ( (t - m_LastDepthFPStime) > 1000 )
        {
            int fps = ((m_DepthFramesTotal - m_LastDepthFramesTotal) * 1000 + 500) / (t - m_LastDepthFPStime);
            PostMessageW( m_hWnd, WM_USER_UPDATE_FPS, IDC_FPS, fps );
            m_LastDepthFramesTotal = m_DepthFramesTotal;
            m_LastDepthFPStime = t;
        }

        // Blank the skeleton panel if we haven't found a skeleton recently
        if ( (t - m_LastSkeletonFoundTime) > 250 )
        {
            if ( !m_bScreenBlanked )
            {
                Nui_BlankSkeletonScreen( GetDlgItem( m_hWnd, IDC_SKELETALVIEW ), true );
                m_bScreenBlanked = true;
            }
        }
    }

    return 0;
}

//-------------------------------------------------------------------
// Nui_GotDepthAlert
//
// Handle new color data
//-------------------------------------------------------------------
void CSkeletalViewerApp::Nui_GotColorAlert( )
{
    NUI_IMAGE_FRAME imageFrame;

    HRESULT hr = m_pNuiSensor->NuiImageStreamGetNextFrame( m_pVideoStreamHandle, 0, &imageFrame );

    if ( FAILED( hr ) )
    {
        return;
    }

    INuiFrameTexture * pTexture = imageFrame.pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    pTexture->LockRect( 0, &LockedRect, NULL, 0 );
    if ( LockedRect.Pitch != 0 )
    {
        m_pDrawColor->Draw( static_cast<BYTE *>(LockedRect.pBits), LockedRect.size );
    }
    else
    {
        OutputDebugString( L"Buffer length of received texture is bogus\r\n" );
    }

    pTexture->UnlockRect( 0 );

    m_pNuiSensor->NuiImageStreamReleaseFrame( m_pVideoStreamHandle, &imageFrame );
}

//-------------------------------------------------------------------
// Nui_GotDepthAlert
//
// Handle new depth data
//-------------------------------------------------------------------
void CSkeletalViewerApp::Nui_GotDepthAlert( )
{
    NUI_IMAGE_FRAME imageFrame;

    HRESULT hr = m_pNuiSensor->NuiImageStreamGetNextFrame(
        m_pDepthStreamHandle,
        0,
        &imageFrame );

    if ( FAILED( hr ) )
    {
        return;
    }

    INuiFrameTexture * pTexture = imageFrame.pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    pTexture->LockRect( 0, &LockedRect, NULL, 0 );
    if ( 0 != LockedRect.Pitch )
    {
        DWORD frameWidth, frameHeight;
        
        NuiImageResolutionToSize( imageFrame.eResolution, frameWidth, frameHeight );
        
        // draw the bits to the bitmap
        RGBQUAD * rgbrun = m_rgbWk;
        USHORT * pBufferRun = (USHORT *)LockedRect.pBits;

        // end pixel is start + width*height - 1
        USHORT * pBufferEnd = pBufferRun + (frameWidth * frameHeight);

        assert( frameWidth * frameHeight <= ARRAYSIZE(m_rgbWk) );

        while ( pBufferRun < pBufferEnd )
        {
            *rgbrun = Nui_ShortToQuad_Depth( *pBufferRun );
            ++pBufferRun;
            ++rgbrun;
        }

        m_pDrawDepth->Draw( (BYTE*) m_rgbWk, frameWidth * frameHeight * 4 );
    }
    else
    {
        OutputDebugString( L"Buffer length of received texture is bogus\r\n" );
    }

    pTexture->UnlockRect( 0 );

    m_pNuiSensor->NuiImageStreamReleaseFrame( m_pDepthStreamHandle, &imageFrame );
}

void CSkeletalViewerApp::Nui_BlankSkeletonScreen (HWND hWnd, bool getDC )
{
    HDC hdc = getDC ? GetDC( hWnd ) : m_SkeletonDC;

    RECT rct;
    GetClientRect( hWnd, &rct );
    PatBlt( hdc, 0, 0, rct.right, rct.bottom, BLACKNESS );

    if ( getDC )
    {
        ReleaseDC( hWnd, hdc );
    }
}

void CSkeletalViewerApp::Nui_DrawSkeletonSegment( NUI_SKELETON_DATA * pSkel, int numJoints, ... )
{
    va_list vl;
    va_start(vl,numJoints);

    POINT segmentPositions[NUI_SKELETON_POSITION_COUNT];
    int segmentPositionsCount = 0;

    DWORD polylinePointCounts[NUI_SKELETON_POSITION_COUNT];
    int numPolylines = 0;
    int currentPointCount = 0;

    // Note the loop condition: We intentionally run one iteration beyond the
    // last element in the joint list, so we can properly end the final polyline.
    for ( int iJoint = 0; iJoint <= numJoints; iJoint++ )
    {
        if ( iJoint < numJoints )
        {
            NUI_SKELETON_POSITION_INDEX jointIndex = va_arg( vl, NUI_SKELETON_POSITION_INDEX );

            if ( pSkel->eSkeletonPositionTrackingState[jointIndex] != NUI_SKELETON_POSITION_NOT_TRACKED )
            {
                // This joint is tracked: add it to the array of segment positions.            
                segmentPositions[segmentPositionsCount] = m_Points[jointIndex];
                segmentPositionsCount++;
                currentPointCount++;

                // Fully processed the current joint; move on to the next one
                continue;
            }
        }

        // If we fall through to here, we're either beyond the last joint, or
        // the current joint is not tracked: end the current polyline here.
        if ( currentPointCount > 1 )
        {
            // Current polyline already has at least two points: save the count.
            polylinePointCounts[numPolylines++] = currentPointCount;
        }
        else if ( currentPointCount == 1 )
        {
            // Current polyline has only one point: ignore it.
            segmentPositionsCount--;
        }
        currentPointCount = 0;
    }

#ifdef _DEBUG
    // We should end up with no more points in segmentPositions than the
    // original number of joints.
    assert(segmentPositionsCount <= numJoints);

    int totalPointCount = 0;
    for (int i = 0; i < numPolylines; i++)
    {
        // Each polyline should contain at least two points.
        assert(polylinePointCounts[i] > 1);

        totalPointCount += polylinePointCounts[i];
    }

    // Total number of points in all polylines should be the same as number
    // of points in segmentPositions.
    assert(totalPointCount == segmentPositionsCount);
#endif

    if (numPolylines > 0)
    {
        PolyPolyline( m_SkeletonDC, segmentPositions, polylinePointCounts, numPolylines );
    }

    va_end(vl);
}

void CSkeletalViewerApp::Nui_DrawSkeleton( NUI_SKELETON_DATA * pSkel, HWND hWnd, int WhichSkeletonColor )
{
    HGDIOBJ hOldObj = SelectObject( m_SkeletonDC, m_Pen[WhichSkeletonColor % m_PensTotal] );
    
    RECT rct;
    GetClientRect(hWnd, &rct);
    int width = rct.right;
    int height = rct.bottom;
    
    if ( m_Pen[0] == NULL )
    {
        for (int i = 0; i < m_PensTotal; i++)
        {
            m_Pen[i] = CreatePen( PS_SOLID, width / 80, g_SkeletonColors[i] );
        }
    }

    int i;
    USHORT depth;
    for (i = 0; i < NUI_SKELETON_POSITION_COUNT; i++)
    {
        NuiTransformSkeletonToDepthImage( pSkel->SkeletonPositions[i], &m_Points[i].x, &m_Points[i].y, &depth );

        m_Points[i].x = (m_Points[i].x * width) / 320;
        m_Points[i].y = (m_Points[i].y * height) / 240;
    }

    SelectObject(m_SkeletonDC,m_Pen[WhichSkeletonColor%m_PensTotal]);
    
    Nui_DrawSkeletonSegment(pSkel,4,NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_SPINE, NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_HEAD);
    Nui_DrawSkeletonSegment(pSkel,5,NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT);
    Nui_DrawSkeletonSegment(pSkel,5,NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT);
    Nui_DrawSkeletonSegment(pSkel,5,NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_LEFT, NUI_SKELETON_POSITION_KNEE_LEFT, NUI_SKELETON_POSITION_ANKLE_LEFT, NUI_SKELETON_POSITION_FOOT_LEFT);
    Nui_DrawSkeletonSegment(pSkel,5,NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_RIGHT, NUI_SKELETON_POSITION_KNEE_RIGHT, NUI_SKELETON_POSITION_ANKLE_RIGHT, NUI_SKELETON_POSITION_FOOT_RIGHT);
    
    // Draw the joints in a different color
    for ( i = 0; i < NUI_SKELETON_POSITION_COUNT ; i++ )
    {
        if ( pSkel->eSkeletonPositionTrackingState[i] != NUI_SKELETON_POSITION_NOT_TRACKED )
        {
            HPEN hJointPen;
        
            hJointPen = CreatePen( PS_SOLID, 9, g_JointColorTable[i] );
            hOldObj = SelectObject( m_SkeletonDC, hJointPen );

            MoveToEx( m_SkeletonDC, m_Points[i].x, m_Points[i].y, NULL );
            LineTo( m_SkeletonDC, m_Points[i].x, m_Points[i].y );

            SelectObject( m_SkeletonDC, hOldObj );
            DeleteObject( hJointPen );
        }
    }

    if (m_bAppTracking)
    {
        Nui_DrawSkeletonId(pSkel, hWnd, WhichSkeletonColor);
    }
}

void CSkeletalViewerApp::Nui_DrawSkeletonId( NUI_SKELETON_DATA * pSkel, HWND hWnd, int WhichSkeletonColor )
{
    RECT rct;
    GetClientRect( hWnd, &rct );

    float fx = 0, fy = 0;

    NuiTransformSkeletonToDepthImage( pSkel->Position, &fx, &fy );

    int skelPosX = (int)( fx * rct.right + 0.5f );
    int skelPosY = (int)( fy * rct.bottom + 0.5f );

    WCHAR number[20];
    size_t length;

    if ( FAILED(StringCchPrintf(number, ARRAYSIZE(number), L"%d", pSkel->dwTrackingID)) )
    {
        return;
    }

    if ( FAILED(StringCchLength(number, ARRAYSIZE(number), &length)) )
    {
        return;
    }

    if ( m_hFontSkeletonId == NULL )
    {
        LOGFONT lf;
        GetObject( (HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf );
        lf.lfHeight *= 2;
        m_hFontSkeletonId = CreateFontIndirect( &lf );
    }

    HGDIOBJ hLastFont = SelectObject( m_SkeletonDC, m_hFontSkeletonId );
    SetTextAlign( m_SkeletonDC, TA_CENTER );
    SetTextColor( m_SkeletonDC, g_SkeletonColors[WhichSkeletonColor] );
    SetBkColor( m_SkeletonDC, RGB(0, 0, 0) );

    TextOut( m_SkeletonDC, skelPosX, skelPosY, number, (int)length);

    SelectObject( m_SkeletonDC, hLastFont );
}

void CSkeletalViewerApp::Nui_DoDoubleBuffer( HWND hWnd, HDC hDC )
{
    RECT rct;
    GetClientRect(hWnd, &rct);

    HDC hdc = GetDC( hWnd );

    BitBlt( hdc, 0, 0, rct.right, rct.bottom, hDC, 0, 0, SRCCOPY );

    ReleaseDC( hWnd, hdc );
}

//-------------------------------------------------------------------
// Nui_GotSkeletonAlert
//
// Handle new skeleton data
//-------------------------------------------------------------------
void CSkeletalViewerApp::Nui_GotSkeletonAlert( )
{
    NUI_SKELETON_FRAME SkeletonFrame = {0};

    bool bFoundSkeleton = false;

    if ( SUCCEEDED(m_pNuiSensor->NuiSkeletonGetNextFrame( 0, &SkeletonFrame )) )
    {
        for ( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
        {
            if( SkeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED ||
                (SkeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_POSITION_ONLY && m_bAppTracking))
            {
                bFoundSkeleton = true;
            }
        }
    }

    // no skeletons!
    if( !bFoundSkeleton )
    {
        return;
    }

    // smooth out the skeleton data
    HRESULT hr = m_pNuiSensor->NuiTransformSmooth(&SkeletonFrame,NULL);
    if ( FAILED(hr) )
    {
        return;
    }

    // we found a skeleton, re-start the skeletal timer
    m_bScreenBlanked = false;
    m_LastSkeletonFoundTime = timeGetTime( );

    // draw each skeleton color according to the slot within they are found.
    Nui_BlankSkeletonScreen( GetDlgItem( m_hWnd, IDC_SKELETALVIEW ), false );

    bool bSkeletonIdsChanged = false;
    for ( int i = 0 ; i < NUI_SKELETON_COUNT; i++ )
    {
        if ( m_SkeletonIds[i] != SkeletonFrame.SkeletonData[i].dwTrackingID )
        {
            m_SkeletonIds[i] = SkeletonFrame.SkeletonData[i].dwTrackingID;
            bSkeletonIdsChanged = true;
        }

        // Show skeleton only if it is tracked, and the center-shoulder joint is at least inferred.
        if ( SkeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED &&
            SkeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_SHOULDER_CENTER] != NUI_SKELETON_POSITION_NOT_TRACKED)
        {
            Nui_DrawSkeleton( &SkeletonFrame.SkeletonData[i], GetDlgItem( m_hWnd, IDC_SKELETALVIEW ), i );
        }
        else if ( m_bAppTracking && SkeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_POSITION_ONLY )
        {
            Nui_DrawSkeletonId( &SkeletonFrame.SkeletonData[i], GetDlgItem( m_hWnd, IDC_SKELETALVIEW ), i );
        }
    }

    if ( bSkeletonIdsChanged )
    {
        UpdateTrackingComboBoxes();
    }

    Nui_DoDoubleBuffer(GetDlgItem(m_hWnd,IDC_SKELETALVIEW), m_SkeletonDC);
}

//-------------------------------------------------------------------
// Nui_ShortToQuad_Depth
//
// Get the player colored depth value
//-------------------------------------------------------------------
RGBQUAD CSkeletalViewerApp::Nui_ShortToQuad_Depth( USHORT s )
{
    USHORT RealDepth = NuiDepthPixelToDepth(s);
    USHORT Player    = NuiDepthPixelToPlayerIndex(s);

    // transform 13-bit depth information into an 8-bit intensity appropriate
    // for display (we disregard information in most significant bit)
    BYTE intensity = (BYTE)~(RealDepth >> 4);

    // tint the intensity by dividing by per-player values
    RGBQUAD color;
    color.rgbRed   = intensity >> g_IntensityShiftByPlayerR[Player];
    color.rgbGreen = intensity >> g_IntensityShiftByPlayerG[Player];
    color.rgbBlue  = intensity >> g_IntensityShiftByPlayerB[Player];

    return color;
}

void CSkeletalViewerApp::Nui_SetApplicationTracking(bool applicationTracks)
{
    if ( HasSkeletalEngine(m_pNuiSensor) )
    {
        HRESULT hr = m_pNuiSensor->NuiSkeletonTrackingEnable( m_hNextSkeletonEvent, applicationTracks ? NUI_SKELETON_TRACKING_FLAG_TITLE_SETS_TRACKED_SKELETONS : 0);
        if ( FAILED( hr ) )
        {
            MessageBoxResource(IDS_ERROR_SKELETONTRACKING, MB_OK | MB_ICONHAND);
        }
    }
}

void CSkeletalViewerApp::Nui_SetTrackedSkeletons(int skel1, int skel2)
{
    m_TrackedSkeletonIds[0] = skel1;
    m_TrackedSkeletonIds[1] = skel2;
    DWORD tracked[NUI_SKELETON_MAX_TRACKED_COUNT] = { skel1, skel2 };
    if ( FAILED(m_pNuiSensor->NuiSkeletonSetTrackedSkeletons(tracked)) )
    {
        MessageBoxResource(IDS_ERROR_SETTRACKED, MB_OK | MB_ICONHAND);
    }
}