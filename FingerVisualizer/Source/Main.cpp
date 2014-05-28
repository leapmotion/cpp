/******************************************************************************\
* Copyright (C) Leap Motion, Inc. 2011-2013.                                   *
* Leap Motion proprietary and  confidential.  Not for distribution.            *
* Use subject to the terms of the Leap Motion SDK Agreement available at       *
* https://developer.leapmotion.com/sdk_agreement, or another agreement between *
* Leap Motion and you, your company or other organization.                     *
\******************************************************************************/

#include "../JuceLibraryCode/JuceHeader.h"
#include "Leap.h"
#include "LeapUtilGL.h"
#include <cctype>

class FingerVisualizerWindow;
class OpenGLCanvas;

// intermediate class for convenient conversion from JUCE color
// to float vector argument passed to GL functions
struct GLColor  : public LeapUtilGL::GLVector4fv
{
  GLColor() : GLVector4fv( 1.0f, 1.0f, 1.0f, 1.0f ) {}

  GLColor( float r, float g, float b, float a=1.0f ) : GLVector4fv( r, g, b, a ) {}

  GLColor( const Leap::Vector& vRHS, float a = 1.0f ) : GLVector4fv( vRHS, a ) {}

  explicit GLColor( const Colour& juceColor ) 
    : GLVector4fv(juceColor.getFloatRed(),
                  juceColor.getFloatGreen(),
                  juceColor.getFloatBlue(),
                  juceColor.getFloatAlpha())
  {}
};

//==============================================================================
class FingerVisualizerApplication  : public JUCEApplication
{
public:
    //==============================================================================
    FingerVisualizerApplication()
    {
    }

    ~FingerVisualizerApplication()
    {
    }

    //==============================================================================
    void initialise (const String& commandLine);

    void shutdown()
    {
        // Do your application's shutdown code here..
        
    }

    //==============================================================================
    void systemRequestedQuit()
    {
        quit();
    }

    //==============================================================================
    const String getApplicationName()
    {
        return "Leap Finger Visualizer";
    }

    const String getApplicationVersion()
    {
        return ProjectInfo::versionString;
    }

    bool moreThanOneInstanceAllowed()
    {
        return true;
    }

    void anotherInstanceStarted (const String& commandLine)
    {
      (void)commandLine;        
    }

    static Leap::Controller& getController() 
    {
        static Leap::Controller s_controller;

        return  s_controller;
    }

private:
    ScopedPointer<FingerVisualizerWindow>  m_pMainWindow; 
};

//==============================================================================
class OpenGLCanvas  : public Component,
                      public OpenGLRenderer,
                      Leap::Listener
{
public:
    OpenGLCanvas()
      : Component( "OpenGLCanvas" )
    {
        m_openGLContext.setRenderer (this);
        m_openGLContext.setComponentPaintingEnabled (true);
        m_openGLContext.attachTo (*this);
        setBounds( 0, 0, 1024, 768 );

        m_fLastUpdateTimeSeconds = Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks());
        m_fLastRenderTimeSeconds = m_fLastUpdateTimeSeconds;

        FingerVisualizerApplication::getController().addListener( *this );

        initColors();

        resetCamera();

        setWantsKeyboardFocus( true );

        m_bPaused = false;

        m_fFrameScale = 0.0075f;
        m_vFrameTranslation = Leap::Vector(0.0f, -2.0f, 0.5f);

        m_bShowHelp = false;

        m_strHelp = "ESC - quit\n"
                    "h - Toggle help and frame rate display\n"
                    "p - Toggle pause\n"
                    "Mouse Drag  - Rotate camera\n"
                    "Mouse Wheel - Zoom camera\n"
                    "Arrow Keys  - Rotate camera\n"
                    "Space       - Reset camera";

        m_strPrompt = "Press 'h' for help";
    }

    ~OpenGLCanvas()
    {
        FingerVisualizerApplication::getController().removeListener( *this );
        m_openGLContext.detach();
    }

    void newOpenGLContextCreated()
    {
        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(true);
        glDepthFunc(GL_LESS);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
        glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
        glShadeModel(GL_SMOOTH);

        glEnable(GL_LIGHTING);

        m_fixedFont = Font("Courier New", 24, Font::plain );
    }

    void openGLContextClosing()
    {
    }

    bool keyPressed( const KeyPress& keyPress )
    {
      int iKeyCode = toupper(keyPress.getKeyCode());

      if ( iKeyCode == KeyPress::escapeKey )
      {
        JUCEApplication::quit();
        return true;
      }

      if ( iKeyCode == KeyPress::upKey )
      {
        m_camera.RotateOrbit( 0, 0, LeapUtil::kfHalfPi * -0.05f );
        return true;
      }

      if ( iKeyCode == KeyPress::downKey )
      {
        m_camera.RotateOrbit( 0, 0, LeapUtil::kfHalfPi * 0.05f );
        return true;
      }

      if ( iKeyCode == KeyPress::leftKey )
      {
        m_camera.RotateOrbit( 0, LeapUtil::kfHalfPi * -0.05f, 0 );
        return true;
      }

      if ( iKeyCode == KeyPress::rightKey )
      {
        m_camera.RotateOrbit( 0, LeapUtil::kfHalfPi * 0.05f, 0 );
        return true;
      }

      switch( iKeyCode )
      {
      case ' ':
        resetCamera();
        break;
      case 'H':
        m_bShowHelp = !m_bShowHelp;
        break;
      case 'P':
        m_bPaused = !m_bPaused;
        break;
      default:
        return false;
      }

      return true;
    }

    void mouseDown (const MouseEvent& e)
    {
        m_camera.OnMouseDown( LeapUtil::FromVector2( e.getPosition() ) );
    }

    void mouseDrag (const MouseEvent& e)
    {
        m_camera.OnMouseMoveOrbit( LeapUtil::FromVector2( e.getPosition() ) );
        m_openGLContext.triggerRepaint();
    }

    void mouseWheelMove ( const MouseEvent& e,
                          const MouseWheelDetails& wheel )
    {
      (void)e;
      m_camera.OnMouseWheel( wheel.deltaY );
      m_openGLContext.triggerRepaint();
    }

    void resized()
    {
    }

    void paint(Graphics&)
    {
    }

    void renderOpenGL2D() 
    {
        LeapUtilGL::GLAttribScope attribScope( GL_ENABLE_BIT );

        // when enabled text draws poorly.
        glDisable(GL_CULL_FACE);

        ScopedPointer<LowLevelGraphicsContext> glRenderer (createOpenGLGraphicsContext (m_openGLContext, getWidth(), getHeight()));

        if (glRenderer != nullptr)
        {
            Graphics g(*glRenderer.get());

            int iMargin   = 10;
            int iFontSize = static_cast<int>(m_fixedFont.getHeight());
            int iLineStep = iFontSize + (iFontSize >> 2);
            int iBaseLine = 20;
            Font origFont = g.getCurrentFont();

            const Rectangle<int>& rectBounds = getBounds();

            if ( m_bShowHelp )
            {
                g.setColour( Colours::seagreen );
                g.setFont( static_cast<float>(iFontSize) );

                if ( !m_bPaused )
                {
                  g.drawSingleLineText( m_strUpdateFPS, iMargin, iBaseLine );
                }

                g.drawSingleLineText( m_strRenderFPS, iMargin, iBaseLine + iLineStep );

                g.setFont( m_fixedFont );
                g.setColour( Colours::slateblue );

                g.drawMultiLineText(  m_strHelp,
                                      iMargin,
                                      iBaseLine + iLineStep * 3,
                                      rectBounds.getWidth() - iMargin*2 );
            }

            g.setFont( origFont );
            g.setFont( static_cast<float>(iFontSize) );

            g.setColour( Colours::salmon );
            g.drawMultiLineText(  m_strPrompt,
                                  iMargin,
                                  rectBounds.getBottom() - (iFontSize + iFontSize + iLineStep),
                                  rectBounds.getWidth()/4 );
        }
    }

    //
    // calculations that should only be done once per leap data frame but may be drawn many times should go here.
    //   
    void update( Leap::Frame frame )
    {
        ScopedLock sceneLock(m_renderMutex);

        double curSysTimeSeconds = Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks());

        float deltaTimeSeconds = static_cast<float>(curSysTimeSeconds - m_fLastUpdateTimeSeconds);
      
        m_fLastUpdateTimeSeconds = curSysTimeSeconds;
        float fUpdateDT = m_avgUpdateDeltaTime.AddSample( deltaTimeSeconds );
        float fUpdateFPS = (fUpdateDT > 0) ? 1.0f/fUpdateDT : 0.0f;
        m_strUpdateFPS = String::formatted( "UpdateFPS: %4.2f", fUpdateFPS );
    }

    /// affects model view matrix.  needs to be inside a glPush/glPop matrix block!
    void setupScene()
    {
        OpenGLHelpers::clear (Colours::black.withAlpha (1.0f));

        m_camera.SetAspectRatio( getWidth() / static_cast<float>(getHeight()) );

        m_camera.SetupGLProjection();

        m_camera.ResetGLView();

        // left, high, near - corner light
        LeapUtilGL::GLVector4fv vLight0Position( -3.0f, 3.0f, -3.0f, 1.0f );
        // right, near - side light
        LeapUtilGL::GLVector4fv vLight1Position(  3.0f, 0.0f, -1.5f, 1.0f );
        // near - head light
        LeapUtilGL::GLVector4fv vLight2Position( 0.0f, 0.0f,  -3.0f, 1.0f );

        /// JUCE turns off the depth test every frame when calling paint.
        glEnable(GL_DEPTH_TEST);
        glDepthMask(true);
        glDepthFunc(GL_LESS);

        glEnable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);

        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, GLColor(Colours::darkgrey));

        glLightfv(GL_LIGHT0, GL_POSITION, vLight0Position);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, GLColor(0.25f, 0.20f, 0.20f, 1.0f));
        glLightfv(GL_LIGHT0, GL_AMBIENT, GLColor(Colours::black));

        glLightfv(GL_LIGHT1, GL_POSITION, vLight1Position);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, GLColor(0.0f, 0.0f, 0.125f, 1.0f));
        glLightfv(GL_LIGHT1, GL_AMBIENT, GLColor(Colours::black));

        glLightfv(GL_LIGHT2, GL_POSITION, vLight2Position);
        glLightfv(GL_LIGHT2, GL_DIFFUSE, GLColor(0.15f, 0.15f, 0.15f, 1.0f));
        glLightfv(GL_LIGHT2, GL_AMBIENT, GLColor(Colours::black));

        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_LIGHT2);

        m_camera.SetupGLView();
    }

    // data should be drawn here but no heavy calculations done.
    // any major calculations that only need to be updated per leap data frame
    // should be handled in update and cached in members.
    void renderOpenGL()
    {
		    {
			      MessageManagerLock mm (Thread::getCurrentThread());
			      if (! mm.lockWasGained())
				      return;
		    }

        Leap::Frame frame = m_lastFrame;

        double  curSysTimeSeconds = Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks());
        float   fRenderDT = static_cast<float>(curSysTimeSeconds - m_fLastRenderTimeSeconds);
        fRenderDT = m_avgRenderDeltaTime.AddSample( fRenderDT );
        m_fLastRenderTimeSeconds = curSysTimeSeconds;

        float fRenderFPS = (fRenderDT > 0) ? 1.0f/fRenderDT : 0.0f;

        m_strRenderFPS = String::formatted( "RenderFPS: %4.2f", fRenderFPS );

        LeapUtilGL::GLMatrixScope sceneMatrixScope;

        setupScene();

        // draw the grid background
        {
            LeapUtilGL::GLAttribScope colorScope(GL_CURRENT_BIT);

            glColor3f( 0, 0, 1 );

            {
                LeapUtilGL::GLMatrixScope gridMatrixScope;

                glTranslatef( 0, 0.0f, -1.5f );

                glScalef( 3, 3, 3 );

                LeapUtilGL::drawGrid( LeapUtilGL::kPlane_XY, 20, 20 );
            }

            {
                LeapUtilGL::GLMatrixScope gridMatrixScope;

                glTranslatef( 0, -1.5f, 0.0f );

                glScalef( 3, 3, 3 );

                LeapUtilGL::drawGrid( LeapUtilGL::kPlane_ZX, 20, 20 );
            }
        }

        // draw fingers/tools as lines with sphere at the tip.
        drawHands( frame );

        {
          ScopedLock renderLock(m_renderMutex);

          // draw the text overlay
          renderOpenGL2D();
        }
    }

    void drawHands( Leap::Frame frame )
    {
        LeapUtilGL::GLMatrixScope matrixScope;

        glTranslatef(m_vFrameTranslation.x, m_vFrameTranslation.y, m_vFrameTranslation.z);
        glScalef(m_fFrameScale, m_fFrameScale, m_fFrameScale);

        const Leap::HandList& hands = frame.hands();

        for ( size_t i = 0, e = hands.count(); i < e; i++ )
        {
            const Leap::Hand& hand        = hands[i];
            const uint32_t    colorIndex  = static_cast<uint32_t>(hand.id()) % kNumColors;

            LeapUtilGL::drawSkeletonHand( hand, m_vBoneColor, m_avJointColors[colorIndex] );
        }
    }

    virtual void onInit(const Leap::Controller&) 
    {
    }

    virtual void onConnect(const Leap::Controller&) 
    {
    }

    virtual void onDisconnect(const Leap::Controller&) 
    {
    }

    virtual void onFrame(const Leap::Controller& controller)
    {
        if ( !m_bPaused )
        {
          Leap::Frame frame = controller.frame();
          update( frame );
          m_lastFrame = frame;
          m_openGLContext.triggerRepaint();
        }
    }

    void resetCamera()
    {
        m_camera.SetOrbitTarget( Leap::Vector::zero() );
        m_camera.SetPOVLookAt( Leap::Vector( 0, 0, 4 ), m_camera.GetOrbitTarget() );
    }

    void initColors()
    {
      unsigned int i = 0;

      m_vBoneColor = GLColor(Colours::darkgrey);

      m_avJointColors[i++] = GLColor(Colours::aqua);
      m_avJointColors[i++] = GLColor(Colours::darkgreen);
      m_avJointColors[i++] = GLColor(Colours::blueviolet);
      m_avJointColors[i++] = GLColor(Colours::crimson);
      m_avJointColors[i++] = GLColor(Colours::salmon);
      m_avJointColors[i++] = GLColor(Colours::blue);
      m_avJointColors[i++] = GLColor(Colours::seagreen);
      m_avJointColors[i++] = GLColor(Colours::orange);

      jassert(i == kNumColors);
    }

private:
    OpenGLContext               m_openGLContext;
    LeapUtilGL::CameraGL        m_camera;
    Leap::Frame                 m_lastFrame;
    double                      m_fLastUpdateTimeSeconds;
    double                      m_fLastRenderTimeSeconds;
    Leap::Vector                m_vFrameTranslation;
    float                       m_fFrameScale;
    LeapUtil::RollingAverage<>  m_avgUpdateDeltaTime;
    LeapUtil::RollingAverage<>  m_avgRenderDeltaTime;
    String                      m_strUpdateFPS;
    String                      m_strRenderFPS;
    String                      m_strPrompt;
    String                      m_strHelp;
    Font                        m_fixedFont;
    CriticalSection             m_renderMutex;
    bool                        m_bShowHelp;
    bool                        m_bPaused;

    GLColor                     m_vBoneColor;
    enum  { kNumColors = 8 };
    GLColor                     m_avJointColors[kNumColors];
};

//==============================================================================
/**
    This is the top-level window that we'll pop up. Inside it, we'll create and
    show a component from the MainComponent.cpp file (you can open this file using
    the Jucer to edit it).
*/
class FingerVisualizerWindow  : public DocumentWindow
{
public:
    //==============================================================================
    FingerVisualizerWindow()
        : DocumentWindow ("Leap Finger Visualizer",
                          Colours::lightgrey,
                          DocumentWindow::allButtons,
                          true)
    {
        setContentOwned (new OpenGLCanvas(), true);

        // Centre the window on the screen
        centreWithSize (getWidth(), getHeight());

        // And show it!
        setVisible (true);

        getChildComponent(0)->grabKeyboardFocus();
    }

    ~FingerVisualizerWindow()
    {
        // (the content component will be deleted automatically, so no need to do it here)
    }

    //==============================================================================
    void closeButtonPressed()
    {
        // When the user presses the close button, we'll tell the app to quit. This
        JUCEApplication::quit();
    }
};

void FingerVisualizerApplication::initialise (const String& commandLine)
{
    (void) commandLine;
    // Do your application's initialisation code here..
    m_pMainWindow = new FingerVisualizerWindow();
}

//==============================================================================
// This macro generates the main() routine that starts the app.
START_JUCE_APPLICATION(FingerVisualizerApplication)
