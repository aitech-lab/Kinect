#include "testApp.h"

void testApp::FTHelperCallingBack(PVOID pVoid) {
	testApp* pApp = reinterpret_cast<testApp*>(pVoid);
    if (pApp) {
        
		IFTResult* pResult = pApp->faceTracker.GetResult();
        if (pResult && SUCCEEDED(pResult->GetStatus())) {
            // FLOAT* pAU = NULL;
            // UINT   numAU;
            // pResult->GetAUCoefficients(&pAU, &numAU);
            // pApp->m_eggavatar.SetCandideAU(pAU, numAU);
			
			//FT_VECTOR2D* points;
			//UINT         count;
			//pResult->Get2DShapePoints(&points, &count);

			// IFTImage*  img = pApp->m_FTHelper.GetColorImage();
			// memcpy(pApp->colorBuffer, img->GetBuffer(), 640*480*4);
			
			// FLOAT scale;
            // FLOAT rotationXYZ[3];
            // FLOAT translationXYZ[3];
            // pResult->Get3DPose(&scale, rotationXYZ, translationXYZ);
            // pApp->m_eggavatar.SetTranslations(translationXYZ[0], translationXYZ[1], translationXYZ[2]);
            // pApp->m_eggavatar.SetRotations(rotationXYZ[0], rotationXYZ[1], rotationXYZ[2]);
        }
    }	
}


//--------------------------------------------------------------
void testApp::setup() {

	ofSetWindowTitle("Ailove-FaceTracking");
	ofSetFrameRate(25);

	depthType          = NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX;
    colorType          = NUI_IMAGE_TYPE_COLOR;
    depthRes           = NUI_IMAGE_RESOLUTION_320x240;
    colorRes           = NUI_IMAGE_RESOLUTION_640x480;
    nearMode           = TRUE;
	seatedSkeletonMode = FALSE;

	faceTracker.Init(
		FTHelperCallingBack,
		this,
		depthType,
		depthRes,
		nearMode,
		TRUE, // if near mode doesn't work, fall back to default mode
		colorType,
		colorRes,
		seatedSkeletonMode);
}

//--------------------------------------------------------------
void testApp::update() {

	IFTImage* img = faceTracker.GetColorImage();
	if(img) memcpy(colorBuffer, img->GetBuffer(), 640*480*4);
	
	color.setFromPixels(colorBuffer, 640,480, OF_IMAGE_COLOR_ALPHA, false);
	color.update();

	//depth.setFromPixels((unsigned char *) faceTracker.depthBuffer, 320,240, OF_IMAGE_COLOR_ALPHA);
	//depth.update();
}

//--------------------------------------------------------------
void testApp::exit() {
	// faceTracker.Nui_UnInit();
}


//--------------------------------------------------------------
void testApp::draw() {

	color.draw( 0, 0);
	//depth.draw(320, 0, 320, 240);
	//
	//FT_VECTOR2D* points;
    //UINT         count;
    //HRESULT hr = faceTracker.pFTResult->Get2DShapePoints(&points, &count);
	//if (hr >=0) {
	//	// cout << pts2DCount << " - ";
	//	for(int i=0; i<count; i++) {
	//		ofCircle(points[i].x, points[i].y, 3);
	//	}
	//}	

}

//--------------------------------------------------------------
void testApp::keyPressed(int key) {

}

//--------------------------------------------------------------
void testApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ) {

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}