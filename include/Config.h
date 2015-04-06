#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "cinder/Log.h"
#include "cinder/Color.h"
#include "cinder/Vector.h"
#include "cinder/Json.h"

namespace mndl
{

typedef std::shared_ptr< class Config > ConfigRef;

class Config
{
 public:
	static ConfigRef create() { return ConfigRef( new Config() ); }

	template< typename T, typename TVAL = T >
	void addVar( const std::string &name, T *var, const TVAL &defVal = T() )
	{
		*var = (T)defVal;
		mConfigReadCallbacks.push_back( [ = ] ( ci::JsonTree &json )
				{ Config::readVar( var, defVal, name, json ); } );
		mConfigWriteCallbacks.push_back( [ = ] ( ci::JsonTree &json )
				{ Config::writeVar( var, name, json ); } );
	}

	template< typename T  >
	void addPreset( int32_t presetId, const std::string &name, const T &presetVal )
	{
		mPresetCallbacks[ presetId ][ name ] =
				[ = ]( void *var )
				{
					*((T *)(var)) = presetVal;
				};
	}

	template< typename T  >
	void setPreset( int32_t presetId, const std::string &name, T *var )
	{
		mPresetCallbacks[ presetId ][ name ]( var );
	}

	void read( const ci::DataSourceRef &source );
	void write( const ci::DataTargetRef &target );

 protected:
	Config() {}

	template< typename T, typename TVAL >
	void readVar( T *var, TVAL defVal, const std::string &name, ci::JsonTree &json )
	{
		try
		{
			*var = json.getChild( name ).getValue< T >();
		}
		catch ( const ci::JsonTree::Exception & )
		{
			// no child yet, or trying to convert from a JSON object instead of a value
			*var = (T)defVal;
		}
	}

	template< typename T >
	void writeVar( T *var, const std::string &name, ci::JsonTree &json )
	{
		std::string key;
		ci::JsonTree &parent = addParent( name, json, key );
		parent.pushBack( ci::JsonTree( key, *var ) );
	}

	template< typename T, glm::precision P >
	void readVar( glm::tvec2< T, P > *var, const glm::tvec2< T, P > &defVal,
					const std::string &name, ci::JsonTree &json )
	{
		try
		{
			var->x = json.getChild( name + ".x" ).getValue< T >();
			var->y = json.getChild( name + ".y" ).getValue< T >();
		}
		catch ( const ci::JsonTree::Exception & )
		{
			// no child yet, or trying to convert from a JSON object instead of a value
			*var = defVal;
		}
	}

	template< typename T, glm::precision P >
	void writeVar( glm::tvec2< T, P > *var, const std::string &name, ci::JsonTree &json )
	{
		std::string key;
		ci::JsonTree &parent = addParent( name, json, key );
		ci::JsonTree vec = ci::JsonTree::makeObject( key );
		vec.pushBack( ci::JsonTree( "x", var->x ) );
		vec.pushBack( ci::JsonTree( "y", var->y ) );
		parent.pushBack( vec );
	}

	template< typename T, glm::precision P >
	void readVar( glm::tvec3< T, P > *var, const glm::tvec3< T, P > &defVal,
					const std::string &name, ci::JsonTree &json )
	{
		try
		{
			var->x = json.getChild( name + ".x" ).getValue< T >();
			var->y = json.getChild( name + ".y" ).getValue< T >();
			var->z = json.getChild( name + ".z" ).getValue< T >();
		}
		catch ( const ci::JsonTree::Exception & )
		{
			// no child yet, or trying to convert from a JSON object instead of a value
			*var = defVal;
		}
	}

	template< typename T, glm::precision P >
	void writeVar( glm::tvec3< T, P > *var, const std::string &name, ci::JsonTree &json )
	{
		std::string key;
		ci::JsonTree &parent = addParent( name, json, key );
		ci::JsonTree vec = ci::JsonTree::makeObject( key );
		vec.pushBack( ci::JsonTree( "x", var->x ) );
		vec.pushBack( ci::JsonTree( "y", var->y ) );
		vec.pushBack( ci::JsonTree( "z", var->z ) );
		parent.pushBack( vec );
	}

	std::string colorToHex( const ci::ColorA &color );
	ci::ColorA hexToColor( const std::string &hexStr );

	template< typename T >
	void readVar( ci::ColorAT< T > *var, const ci::ColorAT< T > &defVal,
						  const std::string &name, ci::JsonTree &json )
	{
		try
		{
			std::string colorStr = json.getChild( name ).getValue< std::string >();
			*var = hexToColor( colorStr );
		}
		catch ( const ci::JsonTree::Exception & )
		{
			// no child yet, or trying to convert from a JSON object instead of a value
			*var = defVal;
		}
	}

	template< typename T >
	void writeVar( ci::ColorAT< T > *var, const std::string &name, ci::JsonTree &json )
	{
		std::string key;
		ci::JsonTree &parent = addParent( name, json, key );
		parent.pushBack( ci::JsonTree( key, colorToHex( *var ) ) );
	}

	template< typename T >
	void readVar( ci::ColorT< T > *var, const ci::ColorT< T > &defVal,
						  const std::string &name, ci::JsonTree &json )
	{
		try
		{
			std::string colorStr = json.getChild( name ).getValue< std::string >();
			*var = hexToColor( colorStr );
		}
		catch ( const ci::JsonTree::Exception & )
		{
			// no child yet, or trying to convert from a JSON object instead of a value
			*var = defVal;
		}
	}

	template< typename T >
	void writeVar( ci::ColorT< T > *var, const std::string &name, ci::JsonTree &json )
	{
		std::string key;
		ci::JsonTree &parent = addParent( name, json, key );
		parent.pushBack( ci::JsonTree( key, colorToHex( *var ) ) );
	}

	ci::JsonTree & addParent( const std::string &relativePath, ci::JsonTree &json, std::string &childKey );

	std::vector< std::function< void ( ci::JsonTree & ) > > mConfigReadCallbacks;
	std::vector< std::function< void ( ci::JsonTree & ) > > mConfigWriteCallbacks;

	std::unordered_map< int32_t, std::unordered_map< std::string, std::function< void ( void * ) > > > mPresetCallbacks;
};

};

