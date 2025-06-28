#pragma once
#include <unordered_map>
#include "BjSON/BjSON.h"

#include "Common/Editor/Window.h"
#include "Common/Graphics/FrameContext.h"

namespace onyx
{

struct AssetLoader;
struct AssetManager;

struct IAsset
{
	enum struct LoadingState
	{
		Unloaded = 0,
		Loading,
		Loaded,
		Errored,
		Count
	};

	enum struct LoadType
	{
		// load everything we need for a game
		Complete = 0,
		// load only what we need to later stream the rest when needed
		Stream,
		// load everything we need in the editor
		Editor
	};

	enum struct SaveType
	{
		// include everything we need to open this again in an editor
		Save = 0,
		// include only what we need to open this in a game
		Export,
	};

	LoadingState GetLoadingState() const { return m_loadingState; }

	const BjSON::IReadOnlyObject* GetReader() const { return m_reader.get(); }
	virtual void SetReader( std::shared_ptr< const BjSON::IReadOnlyObject > reader )
	{
		m_reader = reader;
		m_loadingState = LoadingState::Unloaded;
	}

	virtual void Load( LoadType type ) = 0;
	virtual void Save( BjSON::IReadWriteObject& writer, SaveType type ) = 0;
	virtual void DoAssetManagerButton( const char* name, const char* path, f32 width, std::shared_ptr< IAsset > asset, IFrameContext& frame_context ) = 0;

	AssetLoader* m_assetLoader = nullptr;
	AssetManager* m_assetManager = nullptr;
	std::string m_path = "";

protected:
	LoadingState m_loadingState = LoadingState::Loaded;

private:
	std::shared_ptr< const BjSON::IReadOnlyObject > m_reader;
};

struct CachedBjSONReader
{
	CachedBjSONReader( const BjSON::IReadOnlyObject& root_node )
		: m_rootNode( root_node )
	{}

	std::shared_ptr< const BjSON::IReadOnlyObject > GetReader( const std::string& asset_path );

	void ReadRecursive( const BjSON::IReadOnlyObject& node, const std::string& partial_path = "" );

private:
	friend struct AssetManager;
	friend struct AssetManagerWindow;

	const BjSON::IReadOnlyObject& m_rootNode;
	std::unordered_map< std::string, std::shared_ptr< const BjSON::IReadOnlyObject > > m_readers;
};

// for lazily streaming assets from an asset pack in-game
// prevents reloading an asset that has already been loaded and is still in use
struct AssetLoader
{
	AssetLoader( const BjSON::IReadOnlyObject& root_node ) : m_reader( root_node ) {}

private:
	CachedBjSONReader m_reader;
	std::unordered_map< std::string, std::weak_ptr< IAsset > > m_assets;

public:
	template< typename Asset >
	std::shared_ptr< Asset > Load( std::string asset_path )
	{
		auto iter = m_assets.find( asset_path );
		if ( iter == m_assets.end() )
			iter = m_assets.insert( { asset_path, {} } ).first;

		std::shared_ptr< IAsset > result = iter->second.lock();
		
		if ( !result )
		{
			if ( std::shared_ptr< const BjSON::IReadOnlyObject > reader = m_reader.GetReader( asset_path ) )
			{
				iter->second = ( result = std::make_shared< Asset >() );
				result->SetReader( reader );
				result->m_assetLoader = this;
				result->Load( IAsset::LoadType::Stream );
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
};

// for managing asset packs in-editor
// immediately loads all assets and keeps them loaded until closed, but only deserialises them when requested
struct AssetManager
{
	AssetManager( const BjSON::IReadOnlyObject& root_node );
	
private:
	friend struct AssetManagerWindow;

	CachedBjSONReader m_reader;
	std::unordered_map< std::string, std::shared_ptr< IAsset > > m_assets;

public:
	void Save( BjSON::IReadWriteObject& root_node );

	template< typename Asset >
	std::shared_ptr< Asset > New( std::string path_to_asset )
	{
		if ( !LOG_ASSERT( m_assets.find( path_to_asset ) == m_assets.end(), "An asset already exists at {}", path_to_asset ) )
			return nullptr;

		std::shared_ptr< Asset > asset = std::make_shared< Asset >();
		asset->m_assetManager = this;
		asset->m_path = path_to_asset;

		m_assets.insert( { path_to_asset, asset } );
		return asset;
	}

	template< typename Asset >
	std::shared_ptr< Asset > Load( std::string path_to_asset )
	{
		auto iter = m_assets.find( path_to_asset );
		if ( !LOG_ASSERT( iter != m_assets.end(), "Tried to load an asset at {} but none exists", path_to_asset ) )
			return nullptr;

		if ( !iter->second )
		{
			iter->second = std::make_shared< Asset >();
			iter->second->SetReader( m_reader.GetReader( path_to_asset ) );
			iter->second->m_assetManager = this;
			iter->second->m_path = path_to_asset;
			iter->second->Load( IAsset::LoadType::Editor );
		}

		std::shared_ptr< Asset > asset = std::dynamic_pointer_cast< Asset >( iter->second );
		LOG_ASSERT( asset, "An asset exists at {}, but it has already been loaded as something other than {}",
			path_to_asset, typeid( Asset ).name() );

		return asset;
	}
};

struct AssetManagerWindow : onyx::editor::IWindow
{
	std::vector< byte > m_bufferSource;
	std::ifstream m_fileSource;

	std::unique_ptr< BjSON::Decoder > m_decoder;
	std::unique_ptr< AssetManager > m_assetManager;
	std::string m_currentFolder = "/";

	std::string m_osFilePath = "";

	char m_newFolderName[ 32 ];
	bool m_makingNewFolder = false;

	char m_newAssetPath[ 32 ];
	bool m_newAsset = false;

	int m_numIconsPerRow = 3;

	void Reset()
	{
		m_makingNewFolder = false;
		std::memset( m_newFolderName, 0, sizeof( m_newFolderName ) );
		m_currentFolder = "/";
		m_assetManager = nullptr;
		m_decoder = nullptr;
		// m_fileSource = {};
		// m_bufferSource.clear();
	}

	inline static const char* const s_name = "Asset Pack Manager";
	const char* GetName() const override { return s_name; }
	void Run( IFrameContext& frame_context ) override;
	std::string GetWindowTitle() const override;
};

}
