SkeletalViewer - READ ME 

Copyright (c) Microsoft Corporation. All rights reserved.

=============================
OVERVIEW  
.............................
This module provides sample code used to demonstrate Kinect NUI processing such as
capturing depth stream, color video stream and skeletal tracking frames and
displaying them on the screen.
When sample is executed you should be able to see the following:
- the depth stream, showing background in grayscale and different people in different
  colors, darker colors meaning farther distance from camera. Note that people will
  only be detected if their entire body fits within captured frame.
- Tracked NUI skeletons of people detected within frame. Note that skeletons will
  only appear if the entire body of at least one person fits within captured frame.
- Color video stream
- Frame rate at which capture is being delivered to sample application.

=============================
FILES   
.............................
- DrawDevice.cpp: implementation of DrawDevice helper class used to manage Direct3D device
- DrawDevice.h: declaration of DrawDevice helper class
- NuiImpl.cpp: Implementation of CSkeletalViewerApp methods dealing with NUI processing
- Resource.h: declaration of resource identifiers
- SkeletalViewer.cpp: Application's main function and WndProc
- SkeletalViewer.h: Declaration of CSkeletalViewerApp class
- SkeletalViewer.ico: Application icon used in title bar and task bar
- SkeletalViewer.rc: Declaration of sample application resources
- stdafx.cpp: used to create a pre-compiled header from common includes
- stdafx.h: includes common files needed by sample
- targetver.h: used for windows platform versioning

=============================
OPENING IN VISUAL STUDIO   
.............................
1. Launch Start/All Programs/Microsoft Kinect SDK v1.0/Kinect SDK Sample Browser
   (Start -> typing "Kinect SDK" finds it quickly)
2. In the list of samples, select this sample.
3. Click on "Install" button.
4. Provide a location to install the sample to.
5. Open the Solution file (.sln) that was installed.
