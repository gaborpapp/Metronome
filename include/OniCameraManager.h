#pragma once

#include <memory>

#include "cinder/params/Params.h"

#include "CinderOni.h"

typedef std::shared_ptr< class OniCameraManager > OniCameraManagerRef;

class OniCameraManager
{
 public:
	static OniCameraManagerRef create() { return OniCameraManagerRef( new OniCameraManager() ); }

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
	};

	std::vector< OniCamera > mOniCameras;
};
