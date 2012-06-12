#ifndef _TEST_APP_
#define _TEST_APP_

#include "ofMain.h"
#include "FTHelper.h"

class testApp : public ofBaseApp {

	public:
		void setup ();
		void update();
		void draw  ();
		void exit  ();

		void keyPressed   (int key);
		void keyReleased  (int key);
		void mouseMoved   (int x, int y);
		void mouseDragged (int x, int y, int button);
		void mousePressed (int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent    (ofDragInfo dragInfo);
		void gotMessage   (ofMessage msg);

		unsigned char colorBuffer[640*480*4];
		ofImage color;

		// TRACKER STUFS
		static void FTHelperCallingBack(PVOID pVoid);
		
		FTHelper             faceTracker;
		
		NUI_IMAGE_TYPE       depthType;
		NUI_IMAGE_TYPE       colorType;
		NUI_IMAGE_RESOLUTION depthRes;
		NUI_IMAGE_RESOLUTION colorRes;
		BOOL                 nearMode;
		BOOL                 seatedSkeletonMode;
		
};
#endif