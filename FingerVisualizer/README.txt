Leap SDK Examples -- Finger Visualizer
================================================================================

FingerVisualizer illustrates how to access basic hand tracking data
available from the Leap Motion API.

FingerVisualizer displays the entirety of detected hands as a procedurally drawn skeleton composed 
of white cylinders for bones and colored spheres for joints against a 3D reference grid.

Key Leap source files:

* Main.cpp                        -- The main application source file.
* LeapUtilGL.h and LeapUtilGL.cpp -- OpenGL utilities used from the LeapSDK/util folder

--------------------------------------------------------------------------------
Usage:

Place your hands within the Leap field of view.


Other Controls:

* Left/Right/Up/Down arrow keys rotate the scene.
* Dragging the mouse rotates the scene.
* Rolling the mouse wheel changes camera distance.
* H toggles the help overlay.
* P pauses update pausing.
* Space resets the camera.
* Esc quits the program.


--------------------------------------------------------------------------------
Compiling the source code:

We created this example using the *Juce* cross-platform C++ class library. 
The Juce modules provide easy to use APIs for window, graphics, and sound among
other things. Juce also provides the IntroJucer project utility provides project 
files for Microsoft Visual Studio and Apple Xcode. We have already created 
project files for the example projects, but feel free to adjust the IntroJucer 
settings and regenerate your own project files.

You can learn more about Juce at: http://rawmaterialsoftware.com/juce.php


To open the example in Visual Studio or Xcode:

1. Navigate to the project's Builds folder.
2. Find the project file for your IDE:
   On Mac, we provide Xcode project files.
   On Windows, we provide Visual Studio 2008 and Visual Studio 2010 project files.
 
To compile the example programs, the LeapSDK folder must be in the same 
directory as the main Examples folder. The project is set up to
look for the header files and libraries in this location. If you move the 
project or the SDK, you must update the include file and header references also.
  
To generate a project file with IntroJucer:

1. Run IntroJucer.
   You can use the IntroJucer binary included in this package at:
   ThirdParty/JUCE/bin/<platform_name> or get a copy from: 
   http://sourceforge.net/projects/juce/files/juce/
2. Open the ProjectName.jucer file found inside the example project folder.
3. Select a project file type on the IntroJucer window.
4. Make your changes. You can change settings for the project as a whole as well 
   as settings specifically for debug and release configurations.
5. Save the project file changes.


--------------------------------------------------------------------------------
Graphics cards:

The Leap Motion C++ sample code make use of the JUCE cross-platform
application framework.  The 3D drawing is done using OpenGL in conjunction with
JUCE support for OpenGL drawing.

Integrated graphics card OpenGL drivers have been known to perform slowly or
improperly.

If you are building or running on a laptop with integrated graphics as well
as a discrete graphics card you may wish to change your graphics settings to
always use the discrete graphics card.


--------------------------------------------------------------------------------
Copyright © 2012-2014 Leap Motion, Inc. All rights reserved.

Leap Motion proprietary and confidential. Not for distribution. Use subject to
the terms of the Leap Motion SDK Agreement available at 
https://developer.leapmotion.com/sdk_agreement, or another agreement between 
Leap Motion and you, your company or other organization.
