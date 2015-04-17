#include "cinder/Log.h"
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"

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
	for ( int i = 0; i < deviceInfoList.getSize(); i++ )
	{
		const auto &di = deviceInfoList[ i ];
		OniCamera oniCam;
		oniCam.mName = di.getName();
		oniCam.mUri = di.getUri();
		mOniCameras.push_back( oniCam );

		CI_LOG_I( oniCam.mName << " " << oniCam.mUri );
	}
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
		cameraNames.push_back( c.mName + "-" + c.mUri );
	}
	if ( cameraNames.size() == 1 )
	{
		cameraNames= { "no camera found" };
	}

	mParams->addParam( "Camera", cameraNames, &mOniCameraId );
	mParams->addButton( "Open camera", [ this ]()
			{
				if ( ( mOniCameraId > 0 ) && ( mOniCameraId < mOniCameras.size() ) )
				{
					auto &cam = mOniCameras[ mOniCameraId ];
					if ( cam.mOpenThread )
					{
						cam.mOpenThread->join();
						cam.mOpenThread.reset();
					}

					app::AppBase::get()->dispatchAsync(
						std::bind( &OniCameraManager::addCameraParams, this, mOniCameraId ) );

					cam.mOpenThread =
						std::shared_ptr< std::thread >( new std::thread(
							std::bind( &OniCameraManager::openOniCamera, this, mOniCameraId ) ) );
				}
			} );
}

void OniCameraManager::update()
{
	for ( auto &cam : mOniCameras )
	{
		if ( cam.mCapture && cam.mCapture->checkNewDepthFrame() )
		{
			cam.mDepthSurface = Surface16u::create( cam.mCapture->getDepthImage() );
		}
	}
}

void OniCameraManager::draw()
{
	const float margin = 16.0f;
	vec2 offset( margin );
	float offsetY = margin;

	for ( auto &cam : mOniCameras )
	{
		if ( cam.mDepthSurface )
		{
			Rectf rect = cam.mDepthSurface->getBounds();
			if ( rect.getX2() + offset.x > app::getWindowWidth() )
			{
				offset = vec2( margin, offsetY + margin + rect.getY2() );
			}

			rect.offset( offset );
			gl::draw( gl::Texture2d::create( *cam.mDepthSurface ), rect );
			gl::drawString( cam.mName + "-" + cam.mUri, offset + vec2( margin ) );

			offset.x += rect.getWidth() + margin;
		}
	}
}

void OniCameraManager::addCameraParams( size_t cameraId )
{
	auto &cam = mOniCameras[ cameraId ];
	auto name = cam.mName + "-" + cam.mUri;
	auto sepName = name + "-sep";
	mParams->addSeparator( sepName );
	mParams->addParam( name + " progress", &cam.mProgressMessage, true ).group( name );
}

void OniCameraManager::openOniCamera( size_t cameraId )
{
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
	depthMode.setResolution( 640, 480 );
	depthMode.setFps( 30 );
	depthMode.setPixelFormat( openni::PIXEL_FORMAT_DEPTH_1_MM );
	cam.mCapture->getDepthStreamRef()->setVideoMode( depthMode );

	cam.mProgressMessage = "Connected";
	cam.mCapture->start();
}
