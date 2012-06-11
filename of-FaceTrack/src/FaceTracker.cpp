
#include "FaceTracker.h"


//-------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------
FaceTracker::FaceTracker() : 
	m_pNuiSensor(NULL), 
	pFTResult   (NULL),  
	pFT         (NULL),        
	pColorFrame (NULL),
	pDepthFrame (NULL) {

	Nui_Zero();
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
FaceTracker::~FaceTracker() {
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
void FaceTracker::Nui_Zero() {
    
	if(m_pNuiSensor) { m_pNuiSensor->Release(); m_pNuiSensor = NULL; }
	if(pFTResult   ) { pFTResult->Release()   ; pFTResult    = NULL; }
	if(pFT         ) { pFT->Release()         ; pFT          = NULL; }
	if(pColorFrame ) { pColorFrame->Release() ; pColorFrame  = NULL; }
	if(pDepthFrame ) { pDepthFrame->Release() ; pDepthFrame  = NULL; }

    m_hNextDepthFrameEvent = NULL;
    m_hNextColorFrameEvent = NULL;
    m_hNextSkeletonEvent   = NULL;
    m_pDepthStreamHandle   = NULL;
    m_pVideoStreamHandle   = NULL;
    m_hThNuiProcess        = NULL;
    m_hEvNuiProcessStop    = NULL;
   
    m_bScreenBlanked = false;
    
}

void CALLBACK FaceTracker::Nui_StatusProcThunk( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName, void * pUserData ) {
    reinterpret_cast<FaceTracker *>(pUserData)->Nui_StatusProc( hrStatus, instanceName, uniqueDeviceName );
}

//-------------------------------------------------------------------
// Nui_StatusProc
//
// Callback to handle FaceTracker status changes
//-------------------------------------------------------------------
void CALLBACK FaceTracker::Nui_StatusProc( HRESULT hrStatus, const OLECHAR* instanceName, const OLECHAR* uniqueDeviceName ) {

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
// Initialize FaceTracker
//-------------------------------------------------------------------
HRESULT FaceTracker::Nui_Init( ) {

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
    m_hNextSkeletonEvent   = CreateEventW( NULL, TRUE, FALSE, NULL );

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
    if ( E_NUI_SKELETAL_ENGINE_BUSY == hr ) {
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
        0, 2,
        m_hNextColorFrameEvent,
        &m_pVideoStreamHandle );

    if ( FAILED( hr ) ) {
        printf("RGB stream open problem\n");
        return hr;
    }

    hr = m_pNuiSensor->NuiImageStreamOpen(
        HasSkeletalEngine(m_pNuiSensor) ? NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX : NUI_IMAGE_TYPE_DEPTH,
        NUI_IMAGE_RESOLUTION_320x240,
        0, 2,
        m_hNextDepthFrameEvent,
        &m_pDepthStreamHandle );

    if ( FAILED( hr ) ) {
        printf("Depth stream open problem\n");
        return hr;
    }

	// FACE TRACKER INIT
	// Create an instance of a face tracker
	pFT = FTCreateFaceTracker();
	if(!pFT) {
		// Handle errors
		printf("Face Tracker creation problem\n");
		return -1;
	}

	// Video camera config with width, height, focal length in pixels
	// NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS focal length is computed for 640x480 resolution
	// If you use different resolutions, multiply this focal length by the scaling factor
	FT_CAMERA_CONFIG videoCameraConfig = {640, 480, NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS};

	// Depth camera config with width, height, focal length in pixels
	// NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS focal length is computed for 320x240 resolution
	// If you use different resolutions, multiply this focal length by the scaling factor
	FT_CAMERA_CONFIG depthCameraConfig = {320, 240, NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS};

	// Initialize the face tracker
	hr = pFT->Initialize(&videoCameraConfig, &depthCameraConfig, NULL, NULL);
	if( FAILED(hr) ) {
		printf("Face Tracker initialization problem\n");
		return hr;
	}

	// Create a face tracking result interface
	pFTResult = NULL;
	hr = pFT->CreateFTResult(&pFTResult);
	if(FAILED(hr)) {
		// Handle errors
		printf("Face Tracker result creation problem\n");
		return hr;
	}

	// Prepare image interfaces that hold RGB and depth data
	pColorFrame = FTCreateImage();
	pDepthFrame = FTCreateImage();
	if(!pColorFrame || !pDepthFrame) {
		// Handle errors
	}

	// Attach created interfaces to the RGB and depth buffers that are filled with
	// corresponding RGB and depth frame data from Kinect cameras
	pColorFrame->Attach(640, 480, colorBuffer, FTIMAGEFORMAT_UINT8_R8G8B8, 640*3);
	pDepthFrame->Attach(320, 240, depthBuffer, FTIMAGEFORMAT_UINT16_D13P3, 320*2);
	// You can also use Allocate() method in which case IFTImage interfaces own their memory.
	// In this case use CopyTo() method to copy buffers


	sensorData.pVideoFrame = pColorFrame;
	sensorData.pDepthFrame = pDepthFrame;
	sensorData.ZoomFactor = 1.0f;       // Not used must be 1.0
	//sensorData.ViewOffset = POINT(0, 0); // Not used must be (0,0)

    // Start the Nui processing thread
    m_hEvNuiProcessStop = CreateEventW( NULL, FALSE, FALSE, NULL );
    m_hThNuiProcess =     CreateThread( NULL, 0, Nui_ProcessThread, this, 0, NULL );

    return hr;
}

//-------------------------------------------------------------------
// Nui_UnInit
//
// Uninitialize FaceTracker
//-------------------------------------------------------------------
void FaceTracker::Nui_UnInit( ) {

    // Stop the Nui processing thread
    if ( NULL != m_hEvNuiProcessStop ) {
        // Signal the thread
        SetEvent(m_hEvNuiProcessStop);

        // Wait for thread to stop
        if ( NULL != m_hThNuiProcess ) {
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

DWORD WINAPI FaceTracker::Nui_ProcessThread(LPVOID pParam) {
    FaceTracker *pthis = (FaceTracker *) pParam;
    return pthis->Nui_ProcessThread();
}

//-------------------------------------------------------------------
// Nui_ProcessThread
//
// Thread to handle FaceTracker processing
//-------------------------------------------------------------------
DWORD WINAPI FaceTracker::Nui_ProcessThread() {
    
	const int numEvents = 4;
    HANDLE hEvents[numEvents] = { m_hEvNuiProcessStop, m_hNextDepthFrameEvent, m_hNextColorFrameEvent, m_hNextSkeletonEvent };
    int    nEventIdx;
    DWORD  t;

    //m_LastDepthFPStime = timeGetTime( );

    //blank the skeleton display on startup
    m_LastSkeletonFoundTime = 0;

    // Main thread loop
    bool continueProcessing = true;
    while ( continueProcessing ) {

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
				TrackFace();
                break;

            case WAIT_OBJECT_0 + 3:
                Nui_GotSkeletonAlert();
                break;
        }

    }

    return 0;
}

void FaceTracker::TrackFace() {

	static bool isFaceTracked = false;
	HRESULT hr;
	// Check if we are already tracking a face
    if(!isFaceTracked) {
        // Initiate face tracking.
        // This call is more expensive and searches over the input RGB frame for a face.
        hr = pFT->StartTracking(&sensorData, NULL, NULL, pFTResult);
		if(SUCCEEDED(hr) && SUCCEEDED(pFTResult->GetStatus())) {
            isFaceTracked = true;
        } else {
            // No faces found
            isFaceTracked = false;
        }
    } else {
        // Continue tracking. It uses a previously known face position.
        // This call is less expensive than StartTracking()
        hr = pFT->ContinueTracking(&sensorData, NULL, pFTResult);
        if(FAILED(hr) || FAILED (pFTResult->GetStatus())) {
            // Lost the face
            isFaceTracked = false;
        }
    }
}

//-------------------------------------------------------------------
// Nui_GotDepthAlert
//
// Handle new color data
//-------------------------------------------------------------------

void FaceTracker::Nui_GotColorAlert( ) {

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
void FaceTracker::Nui_GotDepthAlert( ) {

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

void FaceTracker::Nui_DoDoubleBuffer( HWND hWnd, HDC hDC ) {
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
void FaceTracker::Nui_GotSkeletonAlert( ) {

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
RGBQUAD FaceTracker::Nui_ShortToQuad_Depth( USHORT s ) {

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

void FaceTracker::Nui_SetApplicationTracking(bool applicationTracks) {
    if ( HasSkeletalEngine(m_pNuiSensor) ) {
        HRESULT hr = m_pNuiSensor->NuiSkeletonTrackingEnable( m_hNextSkeletonEvent, applicationTracks ? NUI_SKELETON_TRACKING_FLAG_TITLE_SETS_TRACKED_SKELETONS : 0);
        if ( FAILED( hr ) ) {
            printf("Set tracking error");
        }
    }
}

void FaceTracker::Nui_SetTrackedSkeletons(int skel1, int skel2) {
    m_TrackedSkeletonIds[0] = skel1;
    m_TrackedSkeletonIds[1] = skel2;
    DWORD tracked[NUI_SKELETON_MAX_TRACKED_COUNT] = { skel1, skel2 };
    if ( FAILED(m_pNuiSensor->NuiSkeletonSetTrackedSkeletons(tracked)) ) {
        printf("Tracked Skeleton Error");
    }
}