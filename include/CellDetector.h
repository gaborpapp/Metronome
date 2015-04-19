#pragma once

#include <memory>

#include "cinder/params/Params.h"

typedef std::shared_ptr< class CellDetector > CellDetectorRef;

class CellDetector
{
 public:
	static CellDetectorRef create() { return CellDetectorRef( new CellDetector() ); }

	void update();
	void draw();

 protected:
	CellDetector();

	ci::params::InterfaceGlRef mParams;

	void setupParams();
};
