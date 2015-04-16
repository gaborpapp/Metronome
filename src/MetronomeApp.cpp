#include <vector>

#include "cinder/Log.h"
#include "cinder/Serial.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#include "Config.h"
#include "GlobalData.h"
#include "OniCameraManager.h"
#include "ChannelView.h"
#include "ParamsUtils.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class MetronomeApp : public App
{
 public:
    void setup() override;
	void draw() override;
	void update() override;

	void keyDown( KeyEvent event ) override;
    void mouseDrag( MouseEvent event ) override;

	void cleanup() override;

 private:
	params::InterfaceGlRef mParams;

	float mFps;

	void setupParams();
	void setupSerial();
	Serial mSerial;

    ivec2 controlpos;
    ChannelView channelView;
    
	OniCameraManagerRef mOniCameraManager;

	void readConfig();
	void writeConfig();
};

void MetronomeApp::setup()
{
    
	GlobalData &gd = GlobalData::get();
	gd.mConfig = mndl::Config::create();
    gd.gridSize = 18;
    
	setupParams();

	mOniCameraManager = OniCameraManager::create();

	setupSerial();
    
    channelView.setup();

	readConfig();
	mndl::params::showAllParams( true );
}

void MetronomeApp::setupParams()
{
	mParams = params::InterfaceGl::create( "Parameters", ivec2( 200, 300 ) );
	mParams->addParam( "Fps", &mFps, true );
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
	mFps = getAverageFps();

	mOniCameraManager->update();
    
    // blob positions with grid coordinates
    vector<ivec2> testPoints;
    testPoints.push_back(ivec2( controlpos.x, controlpos.y ));
    testPoints.push_back(ivec2( 18, 18 ));

    channelView.update(testPoints);
}

void MetronomeApp::draw()
{
	gl::viewport( getWindowSize() );
	gl::setMatricesWindow( getWindowSize() );

	gl::clear( Color( 0.4, 0.4, 0.4 ), true );

    channelView.draw();
    
	mOniCameraManager->draw();

	mParams->draw();
    
    
}

void MetronomeApp::mouseDrag(MouseEvent event) {
	const vec2 scale( 10 );
	controlpos = vec2( event.getPos() ) / vec2( getWindowSize() ) * scale;
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
			mndl::params::showAllParams( ! mParams->isVisible() );
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
        
        case KeyEvent::KEY_SPACE:
            //  cout << "Gray pixel values of randomized baseChannel: " << endl;
            //  cout << channelView.getResult() << endl;
            cout << channelView.getRoundedChannelPixels() << endl;
            break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

void MetronomeApp::cleanup()
{
	writeConfig();
}

void MetronomeApp::readConfig()
{
	const GlobalData &gd = GlobalData::get();
	mndl::params::addParamsLayoutVars( gd.mConfig );
	fs::path configPath = app::getAppPath();
#ifdef CINDER_MAC
	configPath = configPath.parent_path();
#endif
	configPath /= "config.json";
	if ( fs::exists( configPath ) )
	{
		gd.mConfig->read( loadFile( configPath ) );
		mndl::params::readParamsLayout();
	}
}

void MetronomeApp::writeConfig()
{
	GlobalData &gd = GlobalData::get();
	fs::path configPath = app::getAppPath();
#ifdef CINDER_MAC
	configPath = configPath.parent_path();
#endif
	configPath /= "config.json";
	mndl::params::writeParamsLayout();
	gd.mConfig->write( writeFile( configPath ) );
}

CINDER_APP( MetronomeApp, RendererGl )
