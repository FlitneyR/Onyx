#pragma once
#include <unordered_map>
#include "BjSON/BjSON.h"

namespace onyx
{

struct IAsset
{
	virtual bool HasDeserialised() const = 0;
	virtual void Deserialise( const BjSON::IReadOnlyObject& reader ) = 0;
	virtual void Serialise( BjSON::IReadWriteObject& writer ) = 0;
};

// for lazily streaming assets from an asset pack in-game
// prevents reloading an asset that has already been loaded and is still in use
struct AssetLoader
{
	AssetLoader( std::shared_ptr< const BjSON::IReadOnlyObject > root_node ) : m_rootNode( root_node ) {}

private:
	std::shared_ptr< const BjSON::IReadOnlyObject > m_rootNode;
	std::unordered_map< std::string, std::shared_ptr< const BjSON::IReadOnlyObject > > m_readers; 
	std::unordered_map< std::string, std::weak_ptr< IAsset > > m_assets;

public:
	template< typename Asset >
	std::shared_ptr< Asset > Load( std::string asset_path )
	{
		auto iter = m_assets.find( asset_path );
		if ( iter == m_assets.end() )
			iter = m_assets.insert( { asset_path, {} } );

		std::shared_ptr< IAsset > result = iter->second.lock();
		
		if ( !result )
		{
			if ( std::shared_ptr< const BjSON::IReadOnlyObject > reader = GetReader( asset_path ) )
			{
				iter->second = ( result = std::make_shared< Asset >() );
				result->Deserialise( *reader );
			}
			else
			{
				// the asset doesn't actually exist, no need to keep this
				ERROR( "Tried to load an asset from path {} but it doesn't exist", asset_path );
				m_assets.erase( iter );
				return nullptr;
			}
		}

		std::shared_ptr< Asset > asset = std::dynamic_pointer_cast< Asset >( result );
		LOG_ASSERT( asset, "An asset exists at {} but it isn't a {}", asset_path, typeid( Asset ).name() );

		return asset;
	}

	std::shared_ptr< const BjSON::IReadOnlyObject > GetReader( std::string asset_path );
};

// for managing asset packs in-editor
// immediately loads all assets and keeps them loaded until closed, but only deserialises them when requested
struct AssetManager
{
	AssetManager( std::shared_ptr< const BjSON::IReadOnlyObject > root_node );
	
private:
	const std::shared_ptr< const BjSON::IReadOnlyObject > m_rootNode;
	std::unordered_map< std::string, std::shared_ptr< const BjSON::IReadOnlyObject > > m_readers;
	std::unordered_map< std::string, std::shared_ptr< IAsset > > m_assets;

public:
	void Save( BjSON::IReadWriteObject& root_node );

	template< typename Asset >
	std::shared_ptr< Asset > New( std::string path_to_asset )
	{
		if ( !LOG_ASSERT( m_assets.find( path_to_asset ) == m_assets.end(), "An asset already exists at {}", path_to_asset ) )
			return nullptr;

		std::shared_ptr< Asset > asset = std::make_shared< Asset >();
		m_assets.insert( { path_to_asset, asset } );
		return asset;
	}

	template< typename Asset >
	std::shared_ptr< Asset > Load( std::string path_to_asset )
	{
		auto iter = m_assets.find( path_to_asset );
		if ( !LOG_ASSERT( iter != m_assets.end(), "Tried to load an asset at {} but none exists", path_to_asset ) )
			return nullptr;

		std::shared_ptr< Asset > asset = std::dynamic_pointer_cast< Asset >( iter->second );
		LOG_ASSERT( asset, "An asset exists at {}, but it isn't a {}", path_to_asset, typeid( Asset ).name() );
		return asset;
	}
};

}
