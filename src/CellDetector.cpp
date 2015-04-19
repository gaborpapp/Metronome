#include <string>

#include "cinder/gl/gl.h"

#include "CellDetector.h"
#include "GlobalData.h"

using namespace ci;

CellDetector::CellDetector()
{
	setupParams();
}

void CellDetector::setupParams()
{
	GlobalData &gd = GlobalData::get();

	mParams = params::InterfaceGl::create( "Cell detector", ivec2( 300, 300 ) );
	mParams->setPosition( ivec2( 548, 16 ) );

	mParams->addText( "Grid" );
	const std::string gridAreaGroup = "grid area";
	mParams->addParam( "Grid area X1", &mNormalizedGridArea.x1 ).min( 0.0f ).
		max( 1.0f ).step( 0.005f ).group( gridAreaGroup );
	mParams->addParam( "Grid area Y1", &mNormalizedGridArea.y1 ).min( 0.0f).
		max( 1.0f ).step( 0.005f ).group( gridAreaGroup );
	mParams->addParam( "Grid area X2", &mNormalizedGridArea.x2 ).min( 0.0f).
		max( 1.0f ).step( 0.005f ).group( gridAreaGroup );
	mParams->addParam( "Grid area Y2", &mNormalizedGridArea.y2 ).min( 0.0f).
		max( 1.0f ).step( 0.005f ).group( gridAreaGroup );

	gd.mConfig->addVar( "CellDetector.Grid.Area.x1", &mNormalizedGridArea.x1, 0.0f );
	gd.mConfig->addVar( "CellDetector.Grid.Area.y1", &mNormalizedGridArea.y1, 0.0f );
	gd.mConfig->addVar( "CellDetector.Grid.Area.x2", &mNormalizedGridArea.x2, 1.0f );
	gd.mConfig->addVar( "CellDetector.Grid.Area.y2", &mNormalizedGridArea.y2, 1.0f );
}

void CellDetector::update( const std::vector< mndl::blobtracker::BlobRef > &blobs )
{
	const GlobalData &gd = GlobalData::get();

	if ( mLastGridSize != gd.mGridSize )
	{
		calcGridCells();
	}
}

void CellDetector::draw( const Rectf &bounds )
{
	RectMapping mapping( Rectf( vec2( 0.0f ), vec2( 1.0f ) ), bounds );

	for ( const auto &gridRow : mGridCells )
	{
		for ( const auto &cellRect : gridRow )
		{
			Rectf mappedRect = mapping.map( cellRect );
			gl::drawStrokedRect( mappedRect );
		}
	}
}

void CellDetector::calcGridCells()
{
	mGridCells.clear();

	const GlobalData &gd = GlobalData::get();
	const size_t gridSize = gd.mGridSize;

	vec2 step = vec2( mNormalizedGridArea.getWidth() / gridSize,
					  mNormalizedGridArea.getHeight() / gridSize );
	vec2 pos = mNormalizedGridArea.getUpperLeft();

	for ( int y = 0; y < gd.mGridSize; y++, pos.y += step.y )
	{
		std::vector< Rectf > gridRow;
		pos.x = mNormalizedGridArea.getX1();
		for ( int x = 0; x < gd.mGridSize; x++, pos.x += step.x )
		{
			gridRow.emplace_back( Rectf( pos, pos + step ) );
		}
		mGridCells.emplace_back( gridRow );
	}
}
