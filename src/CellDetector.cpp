#include "CellDetector.h"

using namespace ci;

CellDetector::CellDetector()
{
	setupParams();
}

void CellDetector::setupParams()
{
	mParams = params::InterfaceGl::create( "Cell detector", ivec2( 300, 300 ) );
	mParams->setPosition( ivec2( 548, 16 ) );
}

void CellDetector::update()
{
}

void CellDetector::draw()
{
}
