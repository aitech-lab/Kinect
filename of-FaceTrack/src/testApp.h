#pragma once

#include "ofMain.h"
#include "FaceTracker.h"

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
		
		FaceTracker faceTracker;
		ofImage     color;
		ofImage     depth;

		//static void FTHelperCallingBack(PVOID pVoid);
		
		//FTHelper             m_FTHelper;
		//IFTImage*            m_pImageBuffer;
		//IFTImage*            m_pVideoBuffer;
		//
		//NUI_IMAGE_TYPE       m_depthType;
		//NUI_IMAGE_TYPE       m_colorType;
		//NUI_IMAGE_RESOLUTION m_depthRes;
		//NUI_IMAGE_RESOLUTION m_colorRes;
		//BOOL                 m_bNearMode;
		//BOOL                 m_bSeatedSkeletonMode;
		
};
