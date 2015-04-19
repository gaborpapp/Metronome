#pragma once

#include <memory>

#include "cinder/Rect.h"
#include "cinder/params/Params.h"

#include "mndl/blobtracker/BlobTracker.h"

typedef std::shared_ptr< class CellDetector > CellDetectorRef;

class CellDetector
{
 public:
	static CellDetectorRef create() { return CellDetectorRef( new CellDetector() ); }

	void resize( const ci::Rectf &bounds );

	void update( const std::vector< mndl::blobtracker::BlobRef > &blobs );
	void draw();

	// Returns blobs cell coordinates in grid.
	const std::vector< ci::ivec2 > & getBlobCellCoords() const { return mBlobCellCoords; }

	ci::vec2 getCellCenter( const ci::ivec2 &cellPos );

 protected:
	CellDetector();

	ci::params::InterfaceGlRef mParams;

	void setupParams();

	ci::Rectf mNormalizedGridArea;
	ci::RectMapping mNormalizedToScreenMapping;

	void calcGridCells();
	std::vector< std::vector< ci::Rectf > > mGridCells;
	size_t mLastGridSize = 0;

	std::vector< ci::ivec2 > mBlobCellCoords;
};
