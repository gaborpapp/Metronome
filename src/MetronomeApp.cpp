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

#include "cinder/audio/Context.h"
#include "Sound.h"

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
	void mouseMove( MouseEvent event ) override;

	void cleanup() override;

 private:
	params::InterfaceGlRef mParams;
	params::InterfaceGlRef mParamsTracking;

	float mFps;

	void setupParams();
	void setupParamsTracking();
	void setupSerial();
	Serial mSerial;

    ivec2 mousePos;
	ivec2 mControlPos;
	ChannelView mChannelView;

	Sound mSound;
	bool mSoundEnabled;

	OniCameraManagerRef mOniCameraManager;

	void readConfig();
	void writeConfig();

	static const int kNumCameras = 4;
	struct CameraData
	{
		Area mSrcArea;
		ivec2 mOffset;
	};
	ivec2 mTrackingResolution;
	CameraData mCameraData[ kNumCameras ];
};

void MetronomeApp::setup()
{
	GlobalData &gd = GlobalData::get();
	gd.mConfig = mndl::Config::create();
	gd.gridSize = 18;

	setupParams();
	setupParamsTracking();

	mOniCameraManager = OniCameraManager::create();

	setupSerial();

	mChannelView.setup();

	auto ctx = audio::master();
	mSound.setup(*ctx);

	readConfig();
	mndl::params::showAllParams( true );

	if ( mSoundEnabled )
	{
		ctx->enable();
	}
}

void MetronomeApp::setupParams()
{
	mParams = params::InterfaceGl::create( "Parameters", ivec2( 200, 300 ) );
	mParams->addParam( "Fps", &mFps, true );
	mParams->addSeparator();

	mParams->addParam( "Sound enable", &mSoundEnabled ).updateFn(
			[ this ]()
			{
				audio::master()->setEnabled( mSoundEnabled );
			} );

	GlobalData &gd = GlobalData::get();
	gd.mConfig->addVar( "Sound.Enable", &mSoundEnabled, false );
}

void MetronomeApp::setupParamsTracking()
{
	GlobalData &gd = GlobalData::get();
	mParamsTracking = params::InterfaceGl::create( "Tracking", ivec2( 200, 300 ) );
	mParamsTracking->setPosition( ivec2( 548, 16 ) );

	mParamsTracking->addText( "Arrangement" );
	const std::string resolutionGroup = "Resolution";
	mParamsTracking->addParam( "Resolution X", &mTrackingResolution.x ).min( 320 ).group( resolutionGroup );
	mParamsTracking->addParam( "Resolution Y", &mTrackingResolution.y ).min( 240 ).group( resolutionGroup );
	mParamsTracking->setOptions( resolutionGroup, " opened=false " );

	for ( size_t i = 0; i < kNumCameras; i++ )
	{
		std::string groupName = "Camera #" + toString( i );
		std::string areaGroup = groupName + " area #" + toString( i );
		mParamsTracking->addParam( groupName + " X1", &mCameraData[ i ].mSrcArea.x1 ).min( 0 ).group( areaGroup );
		mParamsTracking->addParam( groupName + " Y1", &mCameraData[ i ].mSrcArea.y1 ).min( 0 ).group( areaGroup );
		mParamsTracking->addParam( groupName + " X2", &mCameraData[ i ].mSrcArea.x2 ).min( 0 ).group( areaGroup );
		mParamsTracking->addParam( groupName + " Y2", &mCameraData[ i ].mSrcArea.y2 ).min( 0 ).group( areaGroup );
		std::string offsetGroup = groupName + " offset #" + toString( i );
		mParamsTracking->addParam( groupName + " offset X", &mCameraData[ i ].mOffset.x ).min( 0 ).group( offsetGroup );
		mParamsTracking->addParam( groupName + " offset Y", &mCameraData[ i ].mOffset.y ).min( 0 ).group( offsetGroup );
		mParamsTracking->setOptions( areaGroup, "group='" + groupName + "'" + " opened=false " );
		mParamsTracking->setOptions( offsetGroup, "group='" + groupName + "'" + " opened=false " );
		mParamsTracking->setOptions( groupName, " opened=false " );

		std::string srcAreaCfg = "Tracking.Camera" + toString( i ) + ".SrcArea.";
		gd.mConfig->addVar( srcAreaCfg + "x1", &mCameraData[ i ].mSrcArea.x1, 0 );
		gd.mConfig->addVar( srcAreaCfg + "y1", &mCameraData[ i ].mSrcArea.y1, 0 );
		gd.mConfig->addVar( srcAreaCfg + "x2", &mCameraData[ i ].mSrcArea.x2, 320 );
		gd.mConfig->addVar( srcAreaCfg + "y2", &mCameraData[ i ].mSrcArea.y2, 240 );
		std::string srcOffsetCfg = "Tracking.Camera" + toString( i );
		gd.mConfig->addVar( srcOffsetCfg, &mCameraData[ i ].mOffset,
				ivec2( ( i & 1 ) * 320, ( i / 2 ) * 240 ) );
	}

	gd.mConfig->addVar( "Tracking.Resolution", &mTrackingResolution, ivec2( 640, 480 ) );
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
	vector<ivec2> blobCenters;
	blobCenters.push_back(ivec2( 0, 0 ));
	//blobCenters.push_back(ivec2( 9, 0 ));

	mChannelView.update( blobCenters );
    mSound.update( mousePos.x * 4 );
}

void MetronomeApp::draw()
{
	gl::viewport( getWindowSize() );
	gl::setMatricesWindow( getWindowSize() );

	gl::clear( Color( 0.4, 0.4, 0.4 ), true );

	mChannelView.draw();

	mOniCameraManager->draw();

	mParams->draw();
}

void MetronomeApp::mouseMove( MouseEvent event )
{
    mousePos = event.getPos();
	//  const vec2 scale( 10 );
	//  mControlPos = vec2( event.getPos() ) / vec2( getWindowSize() ) * scale;
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
			cout << mChannelView.getResult() << endl;
			//  cout << channelView.getRoundedChannelPixels() << endl;
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
