#include <vector>

#include "cinder/Log.h"
#include "cinder/Serial.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#include "OniCameraManager.h"

class MetronomeApp : public App
{
 public:
	void setup() override;
	void draw() override;
	void update() override;

	void keyDown( KeyEvent event ) override;

 private:
	params::InterfaceGlRef mParams;

	void setupSerial();
	Serial mSerial;

	OniCameraManagerRef mOniCameraManager;
};

void MetronomeApp::setup()
{
	mParams = params::InterfaceGl::create( "Parameters", ivec2( 200, 300 ) );

	mOniCameraManager = OniCameraManager::create();

	setupSerial();
}

void MetronomeApp::setupSerial()
{
	const vector< Serial::Device > &devices = Serial::getDevices();
	for ( const auto device : devices )
	{
		console() << "Device: " << device.getName() << endl;
	}
}

void MetronomeApp::update()
{
	mOniCameraManager->update();
}

void MetronomeApp::draw()
{
	gl::viewport( getWindowSize() );
	gl::setMatricesWindow( getWindowSize() );

	gl::clear();

	mOniCameraManager->draw();

	mParams->draw();
}

void MetronomeApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			if ( ! isFullScreen() )
			{
				setFullScreen( true );
				if ( mParams->isVisible() )
				{
					showCursor();
				}
				else
				{
					hideCursor();
				}
			}
			else
			{
				setFullScreen( false );
				showCursor();
			}
			break;

		case KeyEvent::KEY_s:
			mParams->show( ! mParams->isVisible() );
			if ( isFullScreen() )
			{
				if ( mParams->isVisible() )
				{
					showCursor();
				}
				else
				{
					hideCursor();
				}
			}
			break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

CINDER_APP( MetronomeApp, RendererGl )
