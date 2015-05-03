#include <vector>

#include "cinder/ImageIo.h"
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
    std::shared_ptr< Serial > mSerial;
    string prevSerial;
    string serialMessage;

    ivec2 mousePos;
	ivec2 mControlPos;
	ChannelView mChannelView;

	Sound mSound;
	bool mSoundEnabled;
	bool mDebugEnabled;

    Font				mFont;
    gl::TextureFontRef	mTextureFont;
    
    void displayCells();
    void displaySerial();
    void displayMetronomes( std::vector< int > rawResult, std::vector< int > bpmResult );
    void sendSerial( string s );
    void sendSequencedSerial( vector< int > v );
    void sendMultiStringSerial( vector < string > multiString );
    void sendResetSerial();
    void sendStopSerial();
    void sendOneSerial();
    void sendTwoSerials();
    void sendIndexedSerials( int index, vector< int > bpmEven, vector< int > bpmOdd );
    void sendSync();
    void sendOneSync();
    void sendStartSerial();
    bool canSendIndexed;
    int serialIndex;
    bool canReset;
    
    bool rotateMetronomeMatrix;
    vector< int > rotatedMetronomeIndexes;
    
    int resetTimeOut;

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
		IMAGE,
		MOVIE
	};
	TrackingSourceMode mTrackingSourceMode = TrackingSourceMode::CAMERA;
	qtime::MovieSurfaceRef mMovie;
	ChannelRef mImage;
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
    disableFrameRate();
    gl::enableVerticalSync( false );
    
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

	readConfig();
	mndl::params::showAllParams( true );
    
    auto ctx = audio::master();
    mSound.setup(*ctx);

	if ( mSoundEnabled )
	{
		ctx->enable();
	}

	mOniCameraManager->startup();
    
    serialIndex = 0;
    resetTimeOut = 0;
    rotateMetronomeMatrix = true;
    
    //  init & start sending
    sendResetSerial();
    canSendIndexed = true;
    
    
    // Original indexes from top ( all )
    //  x-axis is flipped because of camera
    //  10 9  8  7  6  5  4  3  2  1
    //  20 19 18 17 16 15 15 13 12 11
    //  30 29 28 27 26 25 24 23 22 21
    //  40 39 38 37 36 35 34 33 32 31
    //  50 49 48 47 46 45 44 43 42 41
    //  60 59 58 57 56 55 54 53 52 51
    //  70 69 68 67 66 65 64 63 62 61
    //  80 79 78 77 76 75 74 73 72 71
    //  90 89 88 87 86 85 84 83 82 81
    // 100 99 98 97 96 95 94 93 92 91
    
    //  Rotated indexes from top ( all )
    //  x-axis is flipped because of camera
    //  91 81 71 61 51 41 31 21 11 1
    //  92 82 72 62 52 42 32 22 12 2
    //  93 83 73 63 53 43 33 23 13 3
    //  94 84 74 64 54 44 34 24 14 4
    //  95 85 75 65 55 45 35 25 15 5
    //  96 86 76 66 56 46 36 26 16 6
    //  97 87 77 67 57 47 37 27 17 7
    //  98 88 78 68 58 48 38 28 18 8
    //  99 89 79 69 59 49 39 29 19 9
    // 100 90 80 70 60 50 40 30 20 10
    
    //  Our metronome matrix is 10 columns & 5 rows (stereo amps, paired metronomes)
    rotatedMetronomeIndexes = { 1,  6, 11, 16, 21, 26, 31, 36, 41, 46,
                                2,  7, 12, 17, 22, 27, 32, 37, 42, 47,
                                3,  8, 13, 18, 23, 28, 33, 38, 43, 48,
                                4,  9, 14, 19, 24, 29, 34, 39, 44, 49,
                                5, 10, 15, 20, 25, 30, 35, 40, 45, 50 };
}

void MetronomeApp::setupParams()
{
	GlobalData &gd = GlobalData::get();

	mParams = params::InterfaceGl::create( "Parameters", ivec2( 200, 300 ) );
	mParams->addParam( "Fps", &mFps, true );
	mParams->addSeparator();

	mParams->addText( "Metronome grid" );
	mParams->addParam( "Grid size", &gd.mGridSize, true );
	mParams->addSeparator();

	mParams->addText( "Simulation" );
	mParams->addParam( "Sound enable", &mSoundEnabled ).updateFn(
			[ this ]()
			{
				audio::master()->setEnabled( mSoundEnabled );
			} );
	mParams->addParam( "Debug enable", &mDebugEnabled );

	gd.mConfig->addVar( "Sound.Enable", &mSoundEnabled, false );
	gd.mConfig->addVar( "Debug.Enable", &mDebugEnabled, false );
	gd.mConfig->addVar( "GridSize", &gd.mGridSize, 9 );
}

void MetronomeApp::setupParamsTracking()
{
	GlobalData &gd = GlobalData::get();
	mParamsTracking = params::InterfaceGl::create( "Tracking", ivec2( 200, 300 ) );
	mParamsTracking->setPosition( ivec2( 548, 16 ) );

	mParamsTracking->addText( "Source" );
	mParamsTracking->addParam( "Input source", { "camera", "image", "movie" },
								reinterpret_cast< int * >( &mTrackingSourceMode ) );
	mParamsTracking->addButton( "Load image", [ & ]()
			{
				fs::path imagePath = app::getOpenFilePath();
				if ( fs::exists( imagePath ) )
				{
					try
					{
						mImage = Channel::create( loadImage( imagePath ) );
					}
					catch ( const ImageIoExceptionFailedLoad & )
					{ }
				}
				else
				{
					mImage.reset();
				}
			} );
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
    try {
        Serial::Device dev = Serial::findDeviceByNameContains("usbserial");
        mSerial = std::make_shared< Serial > ( dev, 115200 );
        serialMessage = "serial device inited";
        
    }
    catch( ... ) {
        serialMessage = "error, couldn't init serial device";
        console() << "There was an error initializing the serial device!" << std::endl;
    }
    
    if( mSerial ) {
        mSerial->flush();
    }
}

void MetronomeApp::update()
{
	mFps = getAverageFps();

	mOniCameraManager->update();

	updateTracking();

	const auto &blobCenters = mCellDetector->getBlobCellCoords();

    mChannelView.update( blobCenters );
	if ( mSoundEnabled )
	{
		mSound.update( mChannelView.getBpmResultAsVector() );
	}
    
    //  Orginial concept of sending data not working with strings on the Fablab guys side,
    //  below is an improvised, dirty work-around
    
    if( canSendIndexed ) { // fill all devices with values
        if( rotateMetronomeMatrix ) { // Fablab Guys connected devices in wrong order, we have to rotate the plane, 90 degrees CCW
            sendIndexedSerials( rotatedMetronomeIndexes[ serialIndex ] - 1, mChannelView.getBpmResultAsVectorEven(), mChannelView.getBpmResultAsVectorOdd() ); //   iterate over devices frame by frame
        }else{
            sendIndexedSerials( serialIndex,  mChannelView.getBpmResultAsVectorEven(), mChannelView.getBpmResultAsVectorOdd() ); //   iterate over devices frame by frame
        }
        serialIndex ++;
    }
    if( serialIndex == 49 ) {   //  number of devices ( half of 100, because of stereo amplifiers)
        canSendIndexed = false;
        sendSync();
        serialIndex++;
    }else if( serialIndex == 50 ){
        if( mBlobTracker->getNumBlobs() == 0 )  {
            if(canReset) {
                if( resetTimeOut > 3 ) {
                    sendResetSerial();
                    canReset = false;
                }
            }
            resetTimeOut++;
        }else if(mBlobTracker->getNumBlobs() > 0) {
            canReset = true;
            resetTimeOut = 0;
        }
        serialIndex++;
    }else if( serialIndex > 50 ) {
        serialIndex = 0;
        canSendIndexed = true;
    }
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
	else
	if ( mTrackingSourceMode == TrackingSourceMode::IMAGE && mImage )
	{
		ip::resize( *mImage, mImage->getBounds(), mTrackerChannel.get(), mTrackerChannel->getBounds() );
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
    
	drawTracking();

	mOniCameraManager->draw();

	mCellDetector->draw();

	if ( mDebugEnabled )
	{
		displayMetronomes( mChannelView.getRawResultAsVector(), mChannelView.getBpmResultAsVector() );
        displayCells();
        displaySerial();
	}

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

void MetronomeApp::displayCells( )
{
	gl::ScopedAlphaBlend blend( false );

    const GlobalData &gd = GlobalData::get();
	for ( int y = 0; y < gd.mGridSize; y++ )
	{
		for ( int x = 0; x < gd.mGridSize; x++ )
		{
            gl::color( Color::ColorT( 0.2, 0.2, 0.2 ) );
            vec2 cellCenter = mCellDetector->getCellCenter( ivec2( x, y ) );
			std::string label = toString( x ) + ", " + toString( y );
			vec2 labelSize = mTextureFont->measureString( label );
			mTextureFont->drawString( label, cellCenter - labelSize * 0.5f );
		}
	}
	gl::color( Color::white() );
}

void MetronomeApp::displaySerial( )
{
    gl::ScopedAlphaBlend blend( false );
    gl::color( Color::ColorT( 0.8, 0.8, 0.2 ) );
            mTextureFont->drawString( serialMessage, vec2(20, getWindowHeight() - 100) );
    gl::color( Color::white() );
}


void MetronomeApp::displayMetronomes( std::vector< int > rawResult, std::vector< int > bpmResult )
{
    gl::ScopedAlphaBlend blend( false );
    
    int c = 0;
    const GlobalData &gd = GlobalData::get();
    for ( int y = 0; y < 10; y++ )
    {
        for ( int x = 0; x < 10; x++ )
        {
            gl::color( Color::ColorT( 0.6, 0.6, 0.6 ) );
            std::string rawValue = toString(rawResult[c]);
            mTextureFont->drawString( "val: " + rawValue, vec2( x / (float)10 * (getWindowWidth()-getWindowWidth()/(gd.mGridSize + 2)) + getWindowWidth()/(gd.mGridSize + 2), y / (float)10 * getWindowHeight() + 10) ) ;
            
            gl::color( Color::ColorT( 1, 0.2, 0.2 ) );
            std::string bpmValue = toString(bpmResult[c]);
            mTextureFont->drawString( "bpm: " + bpmValue, vec2( x / (float)10 * (getWindowWidth()-getWindowWidth()/(gd.mGridSize + 2)) + getWindowWidth()/(gd.mGridSize + 2), y / (float)10 * getWindowHeight() + 20) ) ;
            
            c++;
        }
    }
    gl::color( Color::white() );
}

void MetronomeApp::sendSequencedSerial( vector< int > v ) {
    if( mSerial ) {
        int counter = 1;
        try {
            for( int i = 0; i < v.size(); i+=2) {
                string tmpString = "Set " + to_string( counter ) + " BPM " + to_string( v[ i ] ) + " " + to_string( v[ i + 1 ] ) + "\n";
                mSerial->writeString( tmpString );
                serialMessage = tmpString;
                
                if( i%2 ==0 ) {
                    counter++;
                }
            }
            mSerial->writeString( "Start\n" );
            serialMessage = "Start\n";
            mSerial->writeString( "Set Reset_t_all\n" );
            serialMessage = "Set Reset_t_all\n";
        }
        catch ( SerialExcWriteFailure ) {
            serialMessage = "Serial error: could not send";
            cout << "Serial error: could not send" << endl;
        }
    }
}

void MetronomeApp::sendMultiStringSerial( vector< string > multiString) {
    if( mSerial ) {
        try {
            for( auto s : multiString ) {
                mSerial->writeString( s );
                //cout << s;
                serialMessage = s;
            }
            mSerial->flush();
        }
        catch ( SerialExcWriteFailure ) {
            serialMessage = "Serial error: could not send";
            cout << "Serial error: could not send" << endl;
        }
    }
}

void MetronomeApp::sendStopSerial() {
    if( mSerial ) {
        try {
            //mSerial->writeString( "Set Stop_all\n" );
            mSerial->writeString( "Set Stop_all\n" );
            serialMessage = "Set Stop_all\n";
        }
        catch ( SerialExcWriteFailure ) {
            serialMessage = "Serial error: could not send";
            cout << "Serial error: could not send" << endl;
        }
    }
}

void MetronomeApp::sendResetSerial() {
    if( mSerial ) {
        try {
            //mSerial->writeString( "Set Stop_all\n" );
            mSerial->writeString( "Set Reset_all\n" );
            serialMessage = "Set Reset_all\n";
            mSerial->flush();
        }
        catch ( SerialExcWriteFailure ) {
            serialMessage = "Serial error: could not send";
            cout << "Serial error: could not send" << endl;
        }
    }
}

void MetronomeApp::sendOneSerial() {
    if( mSerial ) {
        try {
            //mSerial->writeString( "Set Stop_all\n" );
            mSerial->writeString( "Set 1 BPM 125 125\n" );
            mSerial->writeString( "Start\n" );
            serialMessage = "Set Reset_t_all\n";
        }
        catch ( SerialExcWriteFailure ) {
            serialMessage = "Serial error: could not send";
            cout << "Serial error: could not send" << endl;
        }
    }
}

void MetronomeApp::sendTwoSerials() {
    if( mSerial ) {
        try {
            mSerial->writeString( "Set 2 BPM 200 25\n" );
            mSerial->writeString( "Start\n" );
            serialMessage = "Set Reset_t_all\n";
        }
        catch ( SerialExcWriteFailure ) {
            serialMessage = "Serial error: could not send";
            cout << "Serial error: could not send" << endl;
        }
    }
}

void MetronomeApp::sendIndexedSerials( int index, std::vector< int > bpmEven, std::vector< int > bpmOdd ) {
    if( mSerial ) {
        try {
            mSerial->writeString( "Set "+ to_string( index + 1 ) + " BPM " + to_string( bpmEven[ index ] ) + " " + to_string( bpmOdd[ index ] ) + "\n" );
    
            //cout << "Set "+ to_string( index + 1 ) + " BPM " + to_string( bpmEven[ index ] ) + " " + to_string( bpmOdd[ index ] ) + "\n";
        }
        catch ( SerialExcWriteFailure ) {
            serialMessage = "Serial error: could not send";
            cout << "Serial error: could not send" << endl;
        }
    }
}

void MetronomeApp::sendSync() {
    if( mSerial ) {
        try {
            mSerial->writeString( "Start\n" );
            serialMessage = "Reset_t_all\n";
            serialMessage = "Reset_t_all\n";
            mSerial->flush();
        }
        catch ( SerialExcWriteFailure ) {
            serialMessage = "Serial error: could not send";
            cout << "Serial error: could not send" << endl;

        }
    }
}

void MetronomeApp::sendOneSync() {
    if( mSerial ) {
        try {
            //mSerial->writeString( "Start\n" );
            mSerial->writeString( "Set 1 Reset_t\n" );
            serialMessage = "Set 1 Reset_t\n";
            mSerial->flush();
        }
        catch ( SerialExcWriteFailure ) {
            serialMessage = "Serial error: could not send";
            cout << "Serial error: could not send" << endl;
            
        }
    }
}

void MetronomeApp::sendStartSerial() {
    if( mSerial ) {
        try {
            //mSerial->writeString( "Start\n" );
            mSerial->writeString( "Start\n" );
            mSerial->flush();
        }
        catch ( SerialExcWriteFailure ) {
            serialMessage = "Serial error: could not send";
            cout << "Serial error: could not send" << endl;
            
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
        case KeyEvent::KEY_1:
            sendStopSerial();
            break;
        
        case KeyEvent::KEY_2:
            sendResetSerial();
            break;
            
        case KeyEvent::KEY_3:
            sendMultiStringSerial( mChannelView.getBpmResultAsMultiString() );
            break;
        
        case KeyEvent::KEY_4:
            sendOneSerial();
            break;
            
        case KeyEvent::KEY_5:
            sendTwoSerials();
            break;
            
        case KeyEvent::KEY_6:
            canSendIndexed = true;
            break;
            
        case KeyEvent::KEY_7:
            sendMultiStringSerial( mChannelView.getBpmResultAsFixedMultiString() );
            break;
        
        case KeyEvent::KEY_8:
            sendSync();
            break;
            
        case KeyEvent::KEY_9:
            sendOneSync();
            break;
            
        case KeyEvent::KEY_b:
            sendStartSerial();
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
