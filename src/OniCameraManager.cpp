#include <algorithm>

#include "cinder/Filesystem.h"
#include "cinder/Json.h"
#include "cinder/Log.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"

#include "Config.h"
#include "GlobalData.h"
#include "OniCameraManager.h"

using namespace ci;

OniCameraManager::OniCameraManager()
{
	setupCameras();
	setupParams();
}

OniCameraManager::~OniCameraManager()
{
	for ( auto &oc : mOniCameras )
	{
		if ( oc.mCapture )
		{
			oc.mCapture->stop();
		}

		if ( oc.mOpenThread )
		{
			oc.mOpenThread->join();
		}
	}

	openni::OpenNI::shutdown();
}

void OniCameraManager::setupCameras()
{
	if ( openni::OpenNI::initialize() != openni::STATUS_OK )
	{
		CI_LOG_E( openni::OpenNI::getExtendedError() );
		return;
	}

	mOniCameras.clear();
	openni::Array< openni::DeviceInfo > deviceInfoList;
	openni::OpenNI::enumerateDevices( &deviceInfoList );
	mOniCameras.push_back( OniCamera() ); // blank camera for option menu

	openni::Device device;
	for ( int i = 0; i < deviceInfoList.getSize(); i++ )
	{
		const auto &di = deviceInfoList[ i ];

		OniCamera oniCam;
		oniCam.mName = di.getName();
		oniCam.mUri = di.getUri();

		device.open( oniCam.mUri.c_str() );
		char serial[ 256 ];
		device.getProperty( ONI_DEVICE_PROPERTY_SERIAL_NUMBER, &serial );
		device.close();
		oniCam.mSerial = serial;
		oniCam.mLabel = oniCam.mName + "-" + oniCam.mSerial;

		mOniCameras.push_back( oniCam );

		CI_LOG_I( oniCam.mLabel << " " << oniCam.mUri );
	}

	// sorting cameras by serial number retains order
	std::sort( mOniCameras.begin(), mOniCameras.end(),
			[]( const OniCamera &cam0, const OniCamera &cam1 )
			{
				return cam0.mSerial < cam1.mSerial;
			} );
}

void OniCameraManager::startup()
{
	if ( mLoadCameraConfigAtStart )
	{
		fs::path loadPath( mLastCameraConfig );
		if ( fs::exists( loadPath ) )
		{
			readCameraConfig( loadFile( loadPath ) );
		}
	}
}

size_t OniCameraManager::getNumCameras()
{
	return mOniCameras.size() - 1;
}

ChannelRef OniCameraManager::getCameraChannel( size_t i )
{
	return mOniCameras[ i + 1 ].mDepthChannel;
}

std::string OniCameraManager::getCameraLabel( size_t i )
{
	return mOniCameras[ i + 1 ].mLabel;
}

void OniCameraManager::setupParams()
{
	mParams = params::InterfaceGl::create( "Oni cameras", ivec2( 300, 300 ) );
	mParams->setPosition( ivec2( 232, 16 ) );

	std::vector< std::string > cameraNames;
	cameraNames.push_back( "select" );
	for ( const auto &c : mOniCameras )
	{
		if ( c.mName == "" )
		{
			continue;
		}
		cameraNames.push_back( c.mLabel );
	}
	if ( cameraNames.size() == 1 )
	{
		cameraNames= { "no camera found" };
	}

	mParams->addParam( "Camera", cameraNames, &mOniCameraId );
	mParams->addParam( "Camera resolution", { "320x240", "640x480" },
						reinterpret_cast< int * >( &mCameraResolutionId ) );

	mParams->addButton( "Open camera", [ this ]()
			{
				setupOpenCamera( mOniCameraId );
			} );
	mParams->addSeparator();

	mParams->addButton( "Load camera config", [ this ]()
			{
				fs::path appPath = app::getAppPath();
#ifdef CINDER_MAC
				appPath = appPath.parent_path();
#endif
				fs::path loadPath = app::getOpenFilePath( appPath, { "json" } );
				if ( ! loadPath.empty() )
				{
					mLastCameraConfig = loadPath.string();
					readCameraConfig( loadFile( loadPath ) );
				}
			} );
	mParams->addButton( "Save camera config", [ this ]()
			{
				fs::path appPath = app::getAppPath();
#ifdef CINDER_MAC
				appPath = appPath.parent_path();
#endif
				fs::path savePath = app::getSaveFilePath( appPath, { "json" } );
				if ( ! savePath.empty() )
				{
					writeCameraConfig( writeFile( savePath ) );
				}
			} );
	mParams->addParam( "Load config at startup", &mLoadCameraConfigAtStart );
	mParams->addSeparator();

	mParams->addParam( "Camera debug", &mDebugDraw );

	GlobalData &gd = GlobalData::get();
	gd.mConfig->addVar( "CameraManager.ConfigPath", &mLastCameraConfig, "" );
	gd.mConfig->addVar( "CameraManager.LoadAtStartup", &mLoadCameraConfigAtStart, false );
}

void OniCameraManager::update()
{
	for ( auto &cam : mOniCameras )
	{
		if ( cam.mCapture && cam.mCapture->checkNewDepthFrame() )
		{
			cam.mDepthChannel = Channel::create( cam.mCapture->getDepthImage() );
		}
	}
}

void OniCameraManager::draw()
{
	if ( ! mDebugDraw )
	{
		return;
	}

	const float margin = 16.0f;
	vec2 offset( margin );
	float offsetY = margin;

	for ( auto &cam : mOniCameras )
	{
		if ( cam.mDepthChannel )
		{
			Rectf rect = cam.mDepthChannel->getBounds();
			if ( rect.getX2() + offset.x > app::getWindowWidth() )
			{
				offset = vec2( margin, offsetY + margin + rect.getY2() );
			}

			rect.offset( offset );
			gl::draw( gl::Texture2d::create( *cam.mDepthChannel ), rect );
			gl::drawString( cam.mLabel, offset + vec2( margin ) );

			offset.x += rect.getWidth() + margin;
		}
	}
}

void OniCameraManager::addCameraParams( size_t cameraId )
{
	auto &cam = mOniCameras[ cameraId ];
	auto name = cam.mLabel;
	auto sepName = name + "-sep";
	mParams->addSeparator( sepName );
	mParams->addParam( name + " progress", &cam.mProgressMessage, true ).group( name );
}

void OniCameraManager::setupOpenCamera( size_t cameraId )
{
	if ( ( cameraId > 0 ) && ( cameraId < mOniCameras.size() ) )
	{
		auto &cam = mOniCameras[ cameraId ];
		if ( cam.mOpenThread )
		{
			cam.mOpenThread->join();
			cam.mOpenThread.reset();
		}

		app::AppBase::get()->dispatchAsync(
			std::bind( &OniCameraManager::addCameraParams, this, cameraId ) );

		cam.mOpenThread =
			std::shared_ptr< std::thread >( new std::thread(
				std::bind( &OniCameraManager::openOniCameraThreadFn, this, cameraId ) ) );
	}
}

void OniCameraManager::openOniCameraThreadFn( size_t cameraId )
{
	std::lock_guard< std::mutex > lock( mOniCameraOpenMutex );

	if ( cameraId >= mOniCameras.size() )
	{
		return;
	}

	mndl::oni::OniCapture::Options options;
	options.mEnableColor = false;

	auto &cam = mOniCameras[ cameraId ];

	cam.mProgressMessage = "Connecting...";

	if ( cam.mCapture )
	{
		cam.mCapture->stop();
		cam.mCapture.reset();
	}

	try
	{
		cam.mCapture = mndl::oni::OniCapture::create( cam.mUri.c_str(), options );
	}
	catch ( const mndl::oni::ExcOpenNI &exc )
	{
		CI_LOG_E( exc.what() );
		cam.mProgressMessage = "Failed. " + std::string( exc.what() );
		return;
	}

	openni::VideoMode depthMode;
	ivec2 res = mCameraResolutions[ static_cast< int >( mCameraResolutionId ) ];
	depthMode.setResolution( res.x, res.y );
	depthMode.setFps( 30 );
	depthMode.setPixelFormat( openni::PIXEL_FORMAT_DEPTH_1_MM );
	cam.mCapture->getDepthStreamRef()->setVideoMode( depthMode );
	cam.mCapture->invertDepth();

	cam.mProgressMessage = "Connected";
	cam.mCapture->start();
}

size_t OniCameraManager::findCameraId( const std::string &serial )
{
	auto it = std::find_if( mOniCameras.begin(), mOniCameras.end(),
			[ this, serial ]( const OniCamera &cam )
			{
				return cam.mSerial == serial;
			} );
	if ( it == mOniCameras.end() )
	{
		return 0;
	}
	else
	{
		return it - mOniCameras.begin();
	}
}

void OniCameraManager::readCameraConfig( const ci::DataSourceRef &source )
{
	JsonTree doc( source );

	mCameraResolutionId = static_cast< CameraResolution >( doc.getValueForKey< int >( "CameraResolution" ) );
	const JsonTree &cameras = doc[ "Cameras" ];
	for ( size_t i = 0; i < cameras.getNumChildren(); i++ )
	{
		const JsonTree &cameraData = cameras[ i ];
		std::string name = cameraData.getValueForKey( "name" );
		std::string uri = cameraData.getValueForKey( "uri" );
		std::string serial = cameraData.getValueForKey( "serial" );
		size_t cameraId = findCameraId( serial );
		if ( ! cameraId )
		{
			cameraId = mOniCameras.size();
			OniCamera oniCam;
			oniCam.mName = name;
			oniCam.mUri = uri;
			oniCam.mSerial = serial;
			mOniCameras.push_back( oniCam );
		}

		setupOpenCamera( cameraId );
		// NOTE: without this sleep a missing camera can lock the opening of
		// some of the existing cameras for some unknown reason
		ci::sleep( 100 );
	}
}

void OniCameraManager::writeCameraConfig( const ci::DataTargetRef &target )
{
	JsonTree doc;
	doc.pushBack( JsonTree( "CameraResolution", static_cast< int >( mCameraResolutionId ) ) );

	JsonTree cameras = JsonTree::makeArray( "Cameras" );
	for ( const auto &cam : mOniCameras )
	{
		if ( ! cam.mCapture )
		{
			continue;
		}
		JsonTree cameraData = JsonTree::makeObject( "" );

		cameraData.pushBack( JsonTree( "name", cam.mName ) );
		cameraData.pushBack( JsonTree( "uri", cam.mUri ) );
		cameraData.pushBack( JsonTree( "serial", cam.mSerial ) );
		cameras.pushBack( cameraData );
	}
	doc.pushBack( cameras );
	doc.write( target );
}
