#include <vector>

#include "cinder/Log.h"
#include "cinder/Serial.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/audio/Context.h"
#include "cinder/gl/gl.h"
#include "cinder/ip/Fill.h"
#include "cinder/ip/Resize.h"
#include "cinder/params/Params.h"
#include "cinder/qtime/QuickTime.h"

#include "mndl/blobtracker/BlobTracker.h"
#include "mndl/blobtracker/DebugDrawer.h"

#include "CellDetector.h"
#include "ChannelView.h"
#include "Config.h"
#include "GlobalData.h"
#include "OniCameraManager.h"
#include "ParamsUtils.h"
#include "Sound.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class MetronomeApp : public App
{
 public:
	static void prepareSettings( Settings *settings );

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
    
    Font				mFont;
    gl::TextureFontRef	mTextureFont;
    
    void displayCells( string result );

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

	mndl::blobtracker::BlobTracker::Options mBlobTrackerOptions;
	mndl::blobtracker::BlobTrackerRef mBlobTracker;
	mndl::blobtracker::DebugDrawer::Options mDebugOptions;
	ChannelRef mTrackerChannel;

	enum class TrackingSourceMode : int
	{
		CAMERA = 0,
		MOVIE
	};
	TrackingSourceMode mTrackingSourceMode = TrackingSourceMode::CAMERA;
	qtime::MovieSurfaceRef mMovie;
	void loadMovie( const fs::path &moviePath );

	void updateTracking();
	void drawTracking();

	CellDetectorRef mCellDetector;
};

// static
void MetronomeApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1280, 800 );
}

void MetronomeApp::setup()
{
	GlobalData &gd = GlobalData::get();
	gd.mConfig = mndl::Config::create();

	mBlobTracker = mndl::blobtracker::BlobTracker::create( mBlobTrackerOptions );

	mCellDetector = CellDetector::create();

	setupParams();
	setupParamsTracking();

	mOniCameraManager = OniCameraManager::create();

	setupSerial();

	mChannelView.setup();
    
    mFont = Font( "Arial", 12 );
    mTextureFont = gl::TextureFont::create( mFont );

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
	GlobalData &gd = GlobalData::get();

	mParams = params::InterfaceGl::create( "Parameters", ivec2( 200, 300 ) );
	mParams->addParam( "Fps", &mFps, true );
	mParams->addSeparator();

	mParams->addText( "Metronome grid" );
	mParams->addParam( "Grid size", &gd.mGridSize );
	mParams->addSeparator();

	mParams->addText( "Simulation" );
	mParams->addParam( "Sound enable", &mSoundEnabled ).updateFn(
			[ this ]()
			{
				audio::master()->setEnabled( mSoundEnabled );
			} );

	gd.mConfig->addVar( "Sound.Enable", &mSoundEnabled, false );
	gd.mConfig->addVar( "GridSize", &gd.mGridSize, 10 );
}

void MetronomeApp::setupParamsTracking()
{
	GlobalData &gd = GlobalData::get();
	mParamsTracking = params::InterfaceGl::create( "Tracking", ivec2( 200, 300 ) );
	mParamsTracking->setPosition( ivec2( 548, 16 ) );

	mParamsTracking->addText( "Source" );
	mParamsTracking->addParam( "Input source", { "camera", "movie" },
								reinterpret_cast< int * >( &mTrackingSourceMode ) );
	mParamsTracking->addButton( "Load movie", [ & ]()
			{
				fs::path moviePath = app::getOpenFilePath();
				if ( fs::exists( moviePath ) )
				{
					loadMovie( moviePath );
				}
				else
				{
					mMovie.reset();
				}
			} );
	mParamsTracking->addSeparator();

	mParamsTracking->addText( "Arrangement" );
	const std::string resolutionGroup = "Resolution";
	mParamsTracking->addParam( "Resolution X", &mTrackingResolution.x ).min( 320 ).group( resolutionGroup ).
		updateFn( [ this ]() { mTrackerChannel.reset(); } );
	mParamsTracking->addParam( "Resolution Y", &mTrackingResolution.y ).min( 240 ).group( resolutionGroup ).
		updateFn( [ this ]() { mTrackerChannel.reset(); } );
	mParamsTracking->setOptions( resolutionGroup, " opened=false " );
	gd.mConfig->addVar( "Tracking.Resolution", &mTrackingResolution, ivec2( 640, 480 ) );

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
	mParamsTracking->addSeparator();

	mParamsTracking->addText( "Blob tracker" );
	mParamsTracking->addParam( "Flip", &mBlobTrackerOptions.mFlip );
	mParamsTracking->addParam( "Threshold", &mBlobTrackerOptions.mThreshold ).min( 0 ).max( 255 );
	mParamsTracking->addParam( "Threshold inverts", &mBlobTrackerOptions.mThresholdInvertEnabled );
	mParamsTracking->addParam( "Blur size", &mBlobTrackerOptions.mBlurSize ).min( 1 ).max( 15 );
	mParamsTracking->addParam( "Min area", &mBlobTrackerOptions.mMinArea ).min( 0.0f ).max( 1.0f ).step( 0.0001f );
	mParamsTracking->addParam( "Max area", &mBlobTrackerOptions.mMaxArea ).min( 0.0f ).max( 1.0f ).step( 0.001f );
	mParamsTracking->addParam( "Bounds", &mBlobTrackerOptions.mBoundsEnabled );
	mParamsTracking->addParam( "Top left x", &mBlobTrackerOptions.mNormalizedRegionOfInterest.x1 )
		.min( 0.f ).max( 1.f ).step( 0.001f ).group( "Region of Interest" );
	mParamsTracking->addParam( "Top left y", &mBlobTrackerOptions.mNormalizedRegionOfInterest.y1 )
		.min( 0.f ).max( 1.f ).step( 0.001f ).group( "Region of Interest" );
	mParamsTracking->addParam( "Bottom right x", &mBlobTrackerOptions.mNormalizedRegionOfInterest.x2 )
		.min( 0.f ).max( 1.f ).step( 0.001f ).group( "Region of Interest" );
	mParamsTracking->addParam( "Bottom right y", &mBlobTrackerOptions.mNormalizedRegionOfInterest.y2 )
		.min( 0.f ).max( 1.f ).step( 0.001f ).group( "Region of Interest" );
	mParamsTracking->addParam( "Blank outside Roi", &mBlobTrackerOptions.mBlankOutsideRoi )
		.group( "Region of Interest" );
	mParamsTracking->setOptions( "Region of Interest", "opened=false" );
	mParamsTracking->addSeparator();

	gd.mConfig->addVar( "Tracking.Flip", &mBlobTrackerOptions.mFlip, false );
	gd.mConfig->addVar( "Tracking.Threshold", &mBlobTrackerOptions.mThreshold, 150 );
	gd.mConfig->addVar( "Tracking.ThresholdInverts", &mBlobTrackerOptions.mThresholdInvertEnabled, false );
	gd.mConfig->addVar( "Tracking.BlurSize", &mBlobTrackerOptions.mBlurSize, 10 );
	gd.mConfig->addVar( "Tracking.MinArea", &mBlobTrackerOptions.mMinArea, 0.01f );
	gd.mConfig->addVar( "Tracking.MaxArea", &mBlobTrackerOptions.mMaxArea, 0.45f );
	gd.mConfig->addVar( "Tracking.ROI.x1", &mBlobTrackerOptions.mNormalizedRegionOfInterest.x1, 0.0f );
	gd.mConfig->addVar( "Tracking.ROI.y1", &mBlobTrackerOptions.mNormalizedRegionOfInterest.y1, 0.0f );
	gd.mConfig->addVar( "Tracking.ROI.x2", &mBlobTrackerOptions.mNormalizedRegionOfInterest.x2, 1.0f );
	gd.mConfig->addVar( "Tracking.ROI.y2", &mBlobTrackerOptions.mNormalizedRegionOfInterest.y2, 1.0f );
	gd.mConfig->addVar( "Tracking.ROI.BlankOutside", &mBlobTrackerOptions.mBlankOutsideRoi, false );

	mParamsTracking->addText( "Debug" );

	std::vector< std::string > debugModeNames = { "none", "blended", "overwrite" };
	mDebugOptions.mDebugMode = mndl::blobtracker::DebugDrawer::Options::DebugMode::BLENDED;
	mParamsTracking->addParam( "Debug mode", debugModeNames, reinterpret_cast< int * >( &mDebugOptions.mDebugMode ) );

	std::vector< std::string > drawModeNames = { "original", "blurred", "thresholded" };
	mDebugOptions.mDrawMode = mndl::blobtracker::DebugDrawer::Options::DrawMode::ORIGINAL;
	mParamsTracking->addParam( "Draw mode", drawModeNames, reinterpret_cast< int * >( &mDebugOptions.mDrawMode ) );
	mDebugOptions.mDrawProportionalFit = true;
	mParamsTracking->addSeparator();
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

	updateTracking();

	const auto &blobCenters = mCellDetector->getBlobCellCoords();
	mChannelView.update( blobCenters );

	mSound.update( mousePos.x * 4 );
}

void MetronomeApp::updateTracking()
{
	if ( ! mTrackerChannel )
	{
		mTrackerChannel = Channel::create( mTrackingResolution.x, mTrackingResolution.y );
	}

	if ( mTrackingSourceMode == TrackingSourceMode::CAMERA )
	{
		ip::fill( mTrackerChannel.get(), (uint8_t)0 );

		size_t numCameras = math< size_t >::min( kNumCameras, mOniCameraManager->getNumCameras() );
		for ( size_t i = 0; i < numCameras; i++ )
		{
			ChannelRef camChannel = mOniCameraManager->getCameraChannel( i );
			if ( camChannel )
			{
				Area srcArea = mCameraData[ i ].mSrcArea.getClipBy( camChannel->getBounds() );
				mTrackerChannel->copyFrom( *camChannel, srcArea, mCameraData[ i ].mOffset );
			}
		}
	}
	else // movie
	if ( mMovie && mMovie->checkNewFrame() )
	{
		Channel8u movieChannel( *mMovie->getSurface() );
		ip::resize( movieChannel, movieChannel.getBounds(), mTrackerChannel.get(), mTrackerChannel->getBounds() );
	}

	mBlobTracker->update( *mTrackerChannel );

	Rectf outputRect = Rectf( mTrackerChannel->getBounds() ).getCenteredFit( getWindowBounds(), true );
	mCellDetector->resize( outputRect );

	mCellDetector->update( mBlobTracker->getBlobs() );
}

void MetronomeApp::draw()
{
	gl::viewport( getWindowSize() );
	gl::setMatricesWindow( getWindowSize() );

	gl::clear( Color( 0.4, 0.4, 0.4 ), true );

	mChannelView.draw();

	drawTracking();

	mOniCameraManager->draw();

	mCellDetector->draw();

    displayCells( mChannelView.getResult() );

	mParams->draw();
}

void MetronomeApp::drawTracking()
{
	Rectf outputRect = Rectf( mTrackerChannel->getBounds() ).getCenteredFit( getWindowBounds(), true );
	gl::draw( gl::Texture2d::create( *mTrackerChannel ), outputRect );

	RectMapping mapping( mTrackerChannel->getBounds(), outputRect );
	size_t numCameras = math< size_t >::min( kNumCameras, mOniCameraManager->getNumCameras() );
	for ( size_t i = 0; i < numCameras; i++ )
	{
		const ivec2 margin( 16 );
		ChannelRef camChannel = mOniCameraManager->getCameraChannel( i );
		if ( camChannel )
		{
			std::string label = mOniCameraManager->getCameraLabel( i );
			ivec2 offset = mCameraData[ i ].mSrcArea.getUL() +
							mCameraData[ i ].mOffset + margin;
			gl::drawString( label, mapping.map( offset ) );
		}
	}

	mndl::blobtracker::DebugDrawer::draw( mBlobTracker, getWindowBounds(), mDebugOptions );
}

void MetronomeApp::loadMovie( const fs::path &moviePath )
{
	mMovie = qtime::MovieSurface::create( moviePath );
	mMovie->setLoop();
	mMovie->play();
}

void MetronomeApp::mouseMove( MouseEvent event )
{
    mousePos = event.getPos();
    GlobalData &gd = GlobalData::get();
	const vec2 scale( gd.mGridSize );
	mControlPos = vec2( event.getPos() ) / vec2( getWindowSize() ) * scale;
}

void MetronomeApp::displayCells( string result )
{
    gl::color( Color::white() );
    mTextureFont->drawString( result, ivec2( getWindowWidth() / 2, getWindowHeight() / 2 ) );

	// TODO: use something like this instead to display the result
	const GlobalData &gd = GlobalData::get();
	for ( int y = 0; y < gd.mGridSize; y++ )
	{
		for ( int x = 0; x < gd.mGridSize; x++ )
		{
			vec2 cellCenter = mCellDetector->getCellCenter( ivec2( x, y ) );
			gl::drawStringCentered( toString( x ) + ", " + toString( y ), cellCenter );
		}
	}
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

CINDER_APP( MetronomeApp, RendererGl, MetronomeApp::prepareSettings )
