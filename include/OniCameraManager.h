#pragma once

#include <memory>

#include "cinder/DataSource.h"
#include "cinder/Surface.h"
#include "cinder/Thread.h"
#include "cinder/params/Params.h"

#include "CinderOni.h"

typedef std::shared_ptr< class OniCameraManager > OniCameraManagerRef;

class OniCameraManager
{
 public:
	static OniCameraManagerRef create() { return OniCameraManagerRef( new OniCameraManager() ); }
	~OniCameraManager();

	void update();
	void draw();

	size_t getNumCameras();
	ci::Surface16uRef getCameraSurface( size_t i );
	std::string getCameraLabel( size_t i );

 protected:
	OniCameraManager();

	void setupCameras();
	void setupParams();

	ci::params::InterfaceGlRef mParams;

	int mOniCameraId = 0;

	enum class CameraResolution : int
	{
		RES_320x240 = 0,
		RES_640x480
	};
	CameraResolution mCameraResolutionId = CameraResolution::RES_320x240;

	std::vector< ci::ivec2 > mCameraResolutions =
		{ ci::ivec2( 320, 240 ), ci::ivec2( 640, 480 ) };

	struct OniCamera
	{
		std::string mName;
		std::string mUri;
		std::string mSerial;
		std::string mLabel; // unique label created from name and serial

		std::string mProgressMessage;
		mndl::oni::OniCaptureRef mCapture;
		std::shared_ptr< std::thread > mOpenThread;

		ci::Surface16uRef mDepthSurface;
	};

	std::vector< OniCamera > mOniCameras;

	void setupOpenCamera( size_t cameraId );
	void openOniCameraThreadFn( size_t cameraId );
	void addCameraParams( size_t cameraId );

	//! Returns id according to the \a serial number in the mOniCameras vector or 0 if not found.
	size_t findCameraId( const std::string &serial );

	void readCameraConfig( const ci::DataSourceRef &source );
	void writeCameraConfig( const ci::DataTargetRef &target );

	std::mutex mOniCameraOpenMutex;
};
