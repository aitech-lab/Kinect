#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup() {

	faceTracker.Nui_Init();

	//	m_FTHelper.Init(
	//		FTHelperCallingBack,
	//		this,
	//		m_depthType,
	//		m_depthRes,
	//		m_bNearMode,
	//		TRUE, // if near mode doesn't work, fall back to default mode
	//		m_colorType,
	//		m_colorRes,
	//		m_bSeatedSkeletonMode);
}

//--------------------------------------------------------------
void testApp::update() {
	color.setFromPixels((unsigned char *) faceTracker.colorBuffer, 640,480, OF_IMAGE_COLOR_ALPHA, false);
	color.update();

	depth.setFromPixels((unsigned char *) faceTracker.depthBuffer, 320,240, OF_IMAGE_COLOR_ALPHA);
	depth.update();
}

//--------------------------------------------------------------
void testApp::exit() {
	faceTracker.Nui_UnInit();
}


//--------------------------------------------------------------
void testApp::draw() {

	color.draw(  0, 0, 320, 240);
	depth.draw(320, 0, 320, 240);

	FT_VECTOR2D* points;
    UINT         count;
    HRESULT hr = faceTracker.pFTResult->Get2DShapePoints(&points, &count);
	if (hr >=0) {
		// cout << pts2DCount << " - ";
		for(int i=0; i<count; i++) {
			ofCircle(points[i].x, points[i].y, 3);
		}
	}	

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