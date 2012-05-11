
#include "Kinect.h"


//-------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------
Kinect::Kinect() :m_pNuiSensor(NULL) {

	Nui_Zero();
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
Kinect::~Kinect() {
    Nui_Zero();
}

//lookups for color tinting based on player index
static const int g_IntensityShiftByPlayerR[] = { 1, 2, 0, 2, 0, 0, 2, 0 };
static const int g_IntensityShiftByPlayerG[] = { 1, 2, 2, 0, 2, 0, 0, 1 };
static const int g_IntensityShiftByPlayerB[] = { 1, 0, 2, 2, 0, 2, 0, 2 };


//-------------------------------------------------------------------
// Nui_Zero
//
// Zero out member variables
//-------------------------------------------------------------------
void Kinect::Nui_Zero() {
    
	if (m_pNuiSensor) {
        m_pNuiSensor->Release();
        m_pNuiSensor = NULL;
    }

    m_hNextDepthFrameEvent = NULL;
    m_hNextColorFrameEvent = NULL;
    m_hNextSkeletonEvent   = NULL;
    m_pDepthStreamHandle   = NULL;
    m_pVideoStreamHandle   = NULL;
    m_hThNuiProcess        = NULL;
    m_hEvNuiProcessStop    = NULL;
   
    m_bScreenBlanked = false;
    
}

void CALLBACK Kinect::Nui_StatusProcThunk( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName, void * pUserData ) {
    reinterpret_cast<Kinect *>(pUserData)->Nui_StatusProc( hrStatus, instanceName, uniqueDeviceName );
}

//-------------------------------------------------------------------
// Nui_StatusProc
//
// Callback to handle Kinect status changes
//-------------------------------------------------------------------
void CALLBACK Kinect::Nui_StatusProc( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName ) {

    // Update UI
    // PostMessageW( m_hWnd, WM_USER_UPDATE_COMBO, 0, 0 );

    if( SUCCEEDED(hrStatus) ) {
        if ( S_OK == hrStatus ) {
			Nui_Init();
        }
    } else {
        if ( m_instanceId && 0 == wcscmp(instanceName, m_instanceId) ) {
            Nui_UnInit();
            Nui_Zero();
        }
    }
}

//-------------------------------------------------------------------
// Nui_Init
//
// Initialize Kinect
//-------------------------------------------------------------------
HRESULT Kinect::Nui_Init( ) {

    HRESULT  hr;

    if ( !m_pNuiSensor ) {

        HRESULT hr = NuiCreateSensorByIndex(0, &m_pNuiSensor);

        if ( FAILED(hr) ) {
            return hr;
        }

        //SysFreeString(m_instanceId);
		//
        //m_instanceId = m_pNuiSensor->NuiDeviceConnectionId();
    }

    m_hNextDepthFrameEvent = CreateEventW( NULL, TRUE, FALSE, NULL );
    m_hNextColorFrameEvent = CreateEventW( NULL, TRUE, FALSE, NULL );
    m_hNextSkeletonEvent =   CreateEventW( NULL, TRUE, FALSE, NULL );

    // GetWindowRect( GetDlgItem( m_hWnd, IDC_SKELETALVIEW ), &rc );  
    // HDC hdc = GetDC( GetDlgItem( m_hWnd, IDC_SKELETALVIEW) );
    
    // int width = rc.right - rc.left;
    // int height = rc.bottom - rc.top;

    // m_SkeletonBMP = CreateCompatibleBitmap( hdc, width, height );
    // m_SkeletonDC = CreateCompatibleDC( hdc );
    
    // ReleaseDC(GetDlgItem(m_hWnd,IDC_SKELETALVIEW), hdc );
    // m_SkeletonOldObj = SelectObject( m_SkeletonDC, m_SkeletonBMP );

    
    DWORD nuiFlags = NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON |  NUI_INITIALIZE_FLAG_USES_COLOR;
    hr = m_pNuiSensor->NuiInitialize( nuiFlags );
    if ( E_NUI_SKELETAL_ENGINE_BUSY == hr )
    {
        nuiFlags = NUI_INITIALIZE_FLAG_USES_DEPTH |  NUI_INITIALIZE_FLAG_USES_COLOR;
        hr = m_pNuiSensor->NuiInitialize( nuiFlags) ;
    }
  
    if ( FAILED( hr ) ) {
        if ( E_NUI_DEVICE_IN_USE == hr ) {
            printf("Kinect in use\n");
        } else {
            printf("Kinect init problem\n");
        }
        return hr;
    }

    if ( HasSkeletalEngine( m_pNuiSensor ) ) {

        hr = m_pNuiSensor->NuiSkeletonTrackingEnable( m_hNextSkeletonEvent, 0 );
        if( FAILED( hr ) ) {
            printf("Skeletal engine problem\n");
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

    if ( FAILED( hr ) ) {
        printf("RGB stream open problem\n");
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
        printf("Depth stream open problem\n");
        return hr;
    }

    // Start the Nui processing thread
    m_hEvNuiProcessStop = CreateEventW( NULL, FALSE, FALSE, NULL );
    m_hThNuiProcess =     CreateThread( NULL, 0, Nui_ProcessThread, this, 0, NULL );

    return hr;
}

//-------------------------------------------------------------------
// Nui_UnInit
//
// Uninitialize Kinect
//-------------------------------------------------------------------
void Kinect::Nui_UnInit( ) {

    // Stop the Nui processing thread
    if ( NULL != m_hEvNuiProcessStop ) {
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

    if ( m_pNuiSensor ) {
        m_pNuiSensor->NuiShutdown( );
    }

    if ( m_hNextSkeletonEvent && ( m_hNextSkeletonEvent != INVALID_HANDLE_VALUE ) ) {
        CloseHandle( m_hNextSkeletonEvent );
        m_hNextSkeletonEvent = NULL;
    }
    if ( m_hNextDepthFrameEvent && ( m_hNextDepthFrameEvent != INVALID_HANDLE_VALUE ) ) {
        CloseHandle( m_hNextDepthFrameEvent );
        m_hNextDepthFrameEvent = NULL;
    }

    if ( m_hNextColorFrameEvent && ( m_hNextColorFrameEvent != INVALID_HANDLE_VALUE ) ) {
        CloseHandle( m_hNextColorFrameEvent );
        m_hNextColorFrameEvent = NULL;
    }

    if ( m_pNuiSensor ) {
        m_pNuiSensor->Release();
        m_pNuiSensor = NULL;
    }
}

DWORD WINAPI Kinect::Nui_ProcessThread(LPVOID pParam) {
    Kinect *pthis = (Kinect *) pParam;
    return pthis->Nui_ProcessThread();
}

//-------------------------------------------------------------------
// Nui_ProcessThread
//
// Thread to handle Kinect processing
//-------------------------------------------------------------------
DWORD WINAPI Kinect::Nui_ProcessThread() {
    const int numEvents = 4;
    HANDLE hEvents[numEvents] = { m_hEvNuiProcessStop, m_hNextDepthFrameEvent, m_hNextColorFrameEvent, m_hNextSkeletonEvent };
    int    nEventIdx;
    DWORD  t;

    //m_LastDepthFPStime = timeGetTime( );

    //blank the skeleton display on startup
    m_LastSkeletonFoundTime = 0;

    // Main thread loop
    bool continueProcessing = true;
    while ( continueProcessing )
    {
        // Wait for any of the events to be signalled
        nEventIdx = WaitForMultipleObjects( numEvents, hEvents, FALSE, 100 );

        // Process signal events
        switch ( nEventIdx ) {

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

    }

    return 0;
}

//-------------------------------------------------------------------
// Nui_GotDepthAlert
//
// Handle new color data
//-------------------------------------------------------------------

void Kinect::Nui_GotColorAlert( ) {

	NUI_IMAGE_FRAME imageFrame;

    HRESULT hr = m_pNuiSensor->NuiImageStreamGetNextFrame( m_pVideoStreamHandle, 0, &imageFrame );

    if ( FAILED( hr ) ) {
        return;
    }

    INuiFrameTexture * pTexture = imageFrame.pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    pTexture->LockRect( 0, &LockedRect, NULL, 0 );

	if ( LockedRect.Pitch != 0 ) {
		memcpy(colorBuffer, LockedRect.pBits, 640*480*4);
		//color.setFromPixels((const unsigned char*)(LockedRect.pBits), 640,480, OF_IMAGE_COLOR);
		
    } 

    // pTexture->UnlockRect( 0 );

    m_pNuiSensor->NuiImageStreamReleaseFrame( m_pVideoStreamHandle, &imageFrame );
}

//-------------------------------------------------------------------
// Nui_GotDepthAlert
//
// Handle new depth data
//-------------------------------------------------------------------
void Kinect::Nui_GotDepthAlert( ) {

    NUI_IMAGE_FRAME imageFrame;

    HRESULT hr = m_pNuiSensor->NuiImageStreamGetNextFrame(
        m_pDepthStreamHandle,
        0,
        &imageFrame );

    if ( FAILED( hr ) ) {
        return;
    }

    INuiFrameTexture * pTexture = imageFrame.pFrameTexture;
    NUI_LOCKED_RECT LockedRect;
    pTexture->LockRect( 0, &LockedRect, NULL, 0 );
    if ( 0 != LockedRect.Pitch ) {

		DWORD frameWidth, frameHeight;
		
		NuiImageResolutionToSize( imageFrame.eResolution, frameWidth, frameHeight );
		
		// draw the bits to the bitmap
		RGBQUAD * rgbrun = (RGBQUAD*) depthBuffer;
		USHORT * pBufferRun = (USHORT *)LockedRect.pBits;

		// end pixel is start + width*height - 1
		USHORT * pBufferEnd = pBufferRun + (frameWidth * frameHeight);

		assert( frameWidth * frameHeight <= ARRAYSIZE(m_rgbWk) );

		while ( pBufferRun < pBufferEnd ) {
		    *rgbrun = Nui_ShortToQuad_Depth( *pBufferRun );
		    ++pBufferRun;
		    ++rgbrun;
		}
    }
    pTexture->UnlockRect( 0 );

    m_pNuiSensor->NuiImageStreamReleaseFrame( m_pDepthStreamHandle, &imageFrame );
}

void Kinect::Nui_DoDoubleBuffer( HWND hWnd, HDC hDC ) {
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
void Kinect::Nui_GotSkeletonAlert( ) {

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
    if( !bFoundSkeleton ) {
        return;
    }

    // smooth out the skeleton data
    HRESULT hr = m_pNuiSensor->NuiTransformSmooth(&SkeletonFrame,NULL);
    if ( FAILED(hr) ) {
        return;
    }

    // we found a skeleton, re-start the skeletal timer
    // m_bScreenBlanked = false;
    // m_LastSkeletonFoundTime = timeGetTime( );

    // draw each skeleton color according to the slot within they are found.
    // Nui_BlankSkeletonScreen( GetDlgItem( m_hWnd, IDC_SKELETALVIEW ), false );
	// 
    // bool bSkeletonIdsChanged = false;
    // for ( int i = 0 ; i < NUI_SKELETON_COUNT; i++ ) {
    //     if ( m_SkeletonIds[i] != SkeletonFrame.SkeletonData[i].dwTrackingID ) {
    //         m_SkeletonIds[i] = SkeletonFrame.SkeletonData[i].dwTrackingID;
    //         bSkeletonIdsChanged = true;
    //     }
	// 
    //     // Show skeleton only if it is tracked, and the center-shoulder joint is at least inferred.
    //     if( SkeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED &&
    //         SkeletonFrame.SkeletonData[i].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_SHOULDER_CENTER] != NUI_SKELETON_POSITION_NOT_TRACKED)
    //     {
    //         //Nui_DrawSkeleton( &SkeletonFrame.SkeletonData[i], GetDlgItem( m_hWnd, IDC_SKELETALVIEW ), i );
    //     } else if ( m_bAppTracking && SkeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_POSITION_ONLY ) {
    //         //Nui_DrawSkeletonId( &SkeletonFrame.SkeletonData[i], GetDlgItem( m_hWnd, IDC_SKELETALVIEW ), i );
    //     }
    // }
	// 
    //Nui_DoDoubleBuffer(GetDlgItem(m_hWnd,IDC_SKELETALVIEW), m_SkeletonDC);
}

//-------------------------------------------------------------------
// Nui_ShortToQuad_Depth
//
// Get the player colored depth value
//-------------------------------------------------------------------
RGBQUAD Kinect::Nui_ShortToQuad_Depth( USHORT s ) {

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

void Kinect::Nui_SetApplicationTracking(bool applicationTracks) {
    if ( HasSkeletalEngine(m_pNuiSensor) ) {
        HRESULT hr = m_pNuiSensor->NuiSkeletonTrackingEnable( m_hNextSkeletonEvent, applicationTracks ? NUI_SKELETON_TRACKING_FLAG_TITLE_SETS_TRACKED_SKELETONS : 0);
        if ( FAILED( hr ) ) {
            printf("Set tracking error");
        }
    }
}

void Kinect::Nui_SetTrackedSkeletons(int skel1, int skel2) {
    m_TrackedSkeletonIds[0] = skel1;
    m_TrackedSkeletonIds[1] = skel2;
    DWORD tracked[NUI_SKELETON_MAX_TRACKED_COUNT] = { skel1, skel2 };
    if ( FAILED(m_pNuiSensor->NuiSkeletonSetTrackedSkeletons(tracked)) ) {
        printf("Tracked Skeleton Error");
    }
}