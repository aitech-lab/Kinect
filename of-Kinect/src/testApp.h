#pragma once

#include "ofMain.h"
#include "Kinect.h"


#define SZ_APPDLG_WINDOW_CLASS          _T("SkeletalViewerAppDlgWndClass")
#define WM_USER_UPDATE_FPS              WM_USER
#define WM_USER_UPDATE_COMBO            WM_USER+1
#define WM_USER_UPDATE_TRACKING_COMBO   WM_USER+2

class testApp : public ofBaseApp{

	public:

		void setup ();
		void exit  ();
		void update();
		void draw  ();

		void keyPressed   (int key);
		void keyReleased  (int key);
		void mouseMoved   (int x, int y );
		void mouseDragged (int x, int y, int button);
		void mousePressed (int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		Kinect kinect;
		ofImage color;
		ofImage depth;

};
