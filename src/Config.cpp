#include "cinder/Utilities.h"

#include "Config.h"

namespace mndl
{

void Config::read( const ci::DataSourceRef &source )
{
	ci::JsonTree doc = ci::JsonTree( source );
	for ( auto f : mConfigReadCallbacks )
	{
		f( doc );
	}
}

void Config::write( const ci::DataTargetRef &target )
{
	ci::JsonTree doc;
	for ( auto f : mConfigWriteCallbacks )
	{
		f( doc );
	}
	doc.write( target );
}

ci::JsonTree & Config::addParent( const std::string &relativePath, ci::JsonTree &json, std::string &childKey )
{
	std::vector< std::string > tokens = ci::split( relativePath, "." );
	std::string parentId = "";
	for ( int i = 0; i < tokens.size() - 1; i++ )
	{
		const std::string &token = tokens[ i ];
		if ( ( i == 0 ) && ( ! json.hasChild( token ) ) )
		{
			json.pushBack( ci::JsonTree::makeArray( token ) );
		}
		else if ( ( i > 0 ) && ( ! json.hasChild( parentId + "." + token ) ) )
		{
			json.getChild( parentId ).addChild( ci::JsonTree::makeArray( token ) );
		}
		parentId += ( ( i == 0 ) ? "" : "." ) + token;
	}
	childKey = tokens.back();
	return ( parentId == "" ) ? json : json.getChild( parentId );
}

std::string Config::colorToHex( const ci::ColorA &color )
{
	uint32_t a = ( static_cast< uint32_t >( color.a * 255 ) & 0xff ) << 24;
	uint32_t r = ( static_cast< uint32_t >( color.r * 255 ) & 0xff ) << 16;
	uint32_t g = ( static_cast< uint32_t >( color.g * 255 ) & 0xff ) << 8;
	uint32_t b = ( static_cast< uint32_t >( color.b * 255 ) & 0xff );

	uint32_t value = a + r + g + b;

	std::stringstream clr;
	clr << std::hex << value;
	std::string clrStr = clr.str();

	// add missing alpha for completely transparent colors for the sake of
	// readability
	if ( a == 0 )
	{
		clrStr = "00" + clrStr;
	}

	return clrStr;
}

ci::ColorA Config::hexToColor( const std::string &hexStr )
{
	std::stringstream converter( hexStr );
	uint32_t value;
	converter >> std::hex >> value;

	float a = ( ( value >> 24 ) & 0xff ) / 255.0f;
	float r = ( ( value >> 16 ) & 0xff ) / 255.0f;
	float g = ( ( value >> 8 ) & 0xff ) / 255.0f;
	float b = ( value & 0xff ) / 255.0f;

	return ci::ColorA( r, g, b, a );
}

}
