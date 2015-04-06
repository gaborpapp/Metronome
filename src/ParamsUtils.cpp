#include <map>
#include <string>
#include <unordered_map>

#include "cinder/Vector.h"

#include "AntTweakBar.h"

#include "ParamsUtils.h"

namespace mndl { namespace params {

void showAllParams( bool visible, bool alwaysHideHelp /* = true */ )
{
	int windowId = 0;

	while ( TwWindowExists( windowId ) )
	{
		TwSetCurrentWindow( windowId );
		int barCount = TwGetBarCount();

		int32_t visibleInt = visible ? 1 : 0;
		for ( int i = 0; i < barCount; ++i )
		{
			TwBar *bar = TwGetBarByIndex( i );
			TwSetParam( bar, NULL, "visible", TW_PARAM_INT32, 1, &visibleInt );
		}

		windowId++;
	}

	if ( alwaysHideHelp )
		TwDefine( "TW_HELP visible=false" );
}

struct BarInfo
{
	ci::ivec2 mSize;
	ci::ivec2 mPos;
	int mValuesWidth;
	int mIconified;
};

typedef std::shared_ptr< BarInfo > BarInfoRef;
std::unordered_map< std::string, BarInfoRef > sBarInfos;

void addParamsLayoutVars( const mndl::ConfigRef &config )
{
	// TODO: would be more elegant with config var callback fn's instead of add
	int windowId = 0;

	sBarInfos.clear();
	while ( TwWindowExists( windowId ) )
	{
		TwSetCurrentWindow( windowId );
		int barCount = TwGetBarCount();

		for ( int i = 0; i < barCount; ++i )
		{
			TwBar *bar = TwGetBarByIndex( i );
			// FIXME: does not work with names with whitespace
			std::string barName = TwGetBarName( bar );
			BarInfoRef bi = std::make_shared< BarInfo >();

			TwGetParam( bar, NULL, "size", TW_PARAM_INT32, 2, &(bi->mSize.x) );
			TwGetParam( bar, NULL, "position", TW_PARAM_INT32, 2, &(bi->mPos.x) );
			TwGetParam( bar, NULL, "valueswidth", TW_PARAM_INT32, 1, &bi->mValuesWidth );
			TwGetParam( bar, NULL, "iconified", TW_PARAM_INT32, 1, &bi->mIconified );

			sBarInfos[ barName ] = bi;
			config->addVar( barName + ".Size", &bi->mSize, bi->mSize );
			config->addVar( barName + ".Position", &bi->mPos, bi->mPos );
			config->addVar( barName + ".ValuesWidth", &bi->mValuesWidth, bi->mValuesWidth );
			config->addVar( barName + ".Iconified", &bi->mIconified, bi->mIconified );
		}

		windowId++;
	}
}

// call this *after* config->read()
void readParamsLayout()
{
	for ( const auto &bir : sBarInfos )
	{
		TwBar *bar = TwGetBarByName( bir.first.c_str() );
		const BarInfoRef &bi = bir.second;
		TwSetParam( bar, NULL, "size", TW_PARAM_INT32, 2, &(bi->mSize.x) );
		TwSetParam( bar, NULL, "position", TW_PARAM_INT32, 2, &(bi->mPos.x) );
		TwSetParam( bar, NULL, "valueswidth", TW_PARAM_INT32, 1, &bi->mValuesWidth );
		TwSetParam( bar, NULL, "iconified", TW_PARAM_INT32, 1, &bi->mIconified );
	}
}

// call this *before* config->write()
void writeParamsLayout()
{
	for ( const auto &bir : sBarInfos )
	{
		TwBar *bar = TwGetBarByName( bir.first.c_str() );
		const BarInfoRef &bi = bir.second;
		TwGetParam( bar, NULL, "size", TW_PARAM_INT32, 2, &(bi->mSize.x) );
		TwGetParam( bar, NULL, "position", TW_PARAM_INT32, 2, &(bi->mPos.x) );
		TwGetParam( bar, NULL, "valueswidth", TW_PARAM_INT32, 1, &bi->mValuesWidth );
		TwGetParam( bar, NULL, "iconified", TW_PARAM_INT32, 1, &bi->mIconified );
	}
}

} } // namespace mndl::params
