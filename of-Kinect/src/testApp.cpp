#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){

	kinect.Nui_Init();

}

//--------------------------------------------------------------
void testApp::exit() {


	kinect.Nui_UnInit();

}

//--------------------------------------------------------------
void testApp::update(){
	color.setFromPixels((unsigned char *) kinect.colorBuffer, 640,480, OF_IMAGE_COLOR_ALPHA, false);
	color.update();

	depth.setFromPixels((unsigned char *) kinect.depthBuffer, 320,240, OF_IMAGE_COLOR_ALPHA);
	depth.update();
}

//--------------------------------------------------------------
void testApp::draw(){
	color.draw(  0, 0, 320, 240);
	depth.draw(320, 0, 320, 240);
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}
