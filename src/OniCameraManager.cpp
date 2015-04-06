#include "cinder/Log.h"

#include "OniCameraManager.h"

using namespace ci;

OniCameraManager::OniCameraManager()
{
	setupCameras();
	setupParams();
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
	for ( const auto &c : mOniCameras )
	{
		cameraNames.push_back( c.mName + "-" + c.mUri );
	}
	if ( cameraNames.empty() )
	{
		cameraNames.push_back( "no camera found" );
	}
	mParams->addParam( "Camera", cameraNames, &mOniCameraId );
}
