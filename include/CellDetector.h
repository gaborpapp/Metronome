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

	void update( const std::vector< mndl::blobtracker::BlobRef > &blobs );
	void draw( const ci::Rectf &bounds );

	// Returns blobs cell coordinates in grid.
	const std::vector< ci::ivec2 > & getBlobCellCoords() const { return mBlobCellCoords; }

 protected:
	CellDetector();

	ci::params::InterfaceGlRef mParams;

	void setupParams();

	ci::Rectf mNormalizedGridArea;

	void calcGridCells();
	std::vector< std::vector< ci::Rectf > > mGridCells;
	size_t mLastGridSize = 0;

	std::vector< ci::ivec2 > mBlobCellCoords;
};
