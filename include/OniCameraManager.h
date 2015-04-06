#pragma once

#include <memory>

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

 protected:
	OniCameraManager();

	void setupCameras();
	void setupParams();

	ci::params::InterfaceGlRef mParams;

	int mOniCameraId = 0;

	struct OniCamera
	{
		std::string mName;
		std::string mUri;

		std::string mProgressMessage;
		mndl::oni::OniCaptureRef mCapture;
		std::shared_ptr< std::thread > mOpenThread;

		ci::Surface16uRef mDepthSurface;
	};

	std::vector< OniCamera > mOniCameras;

	void openOniCamera( size_t cameraId );
	void addCameraParams( size_t cameraId );
};
