#pragma once
#include <unordered_map>
#include "BjSON/BjSON.h"

#include "Common/Editor/Window.h"
#include "Common/Graphics/FrameContext.h"

namespace onyx
{

struct AssetManager;

enum struct LoadingState
{
	Unloaded = 0,
	Loading,
	Loaded,
	Errored,
	Count
};

struct IAsset
{
	#define RETURN_LOAD_ERRORED() { m_loadingState = LoadingState::Errored; return; }

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
	virtual void DoAssetManagerButton(
		const char* name, const char* path, f32 width,
		std::shared_ptr< IAsset > asset, IFrameContext& frame_context
	) = 0;

	AssetManager* m_assetManager = nullptr;
	std::string m_path = "";

private:
	std::shared_ptr< const BjSON::IReadOnlyObject > m_reader;

protected:
	LoadingState m_loadingState = LoadingState::Loaded;
};

struct CachedBjSONReader
{
	CachedBjSONReader( const BjSON::IReadOnlyObject& root_node )
		: m_rootNode( root_node )
	{}

	std::shared_ptr< const BjSON::IReadOnlyObject > GetReader( const std::string& asset_path );

	void ReadRecursive( const BjSON::IReadOnlyObject& node, const std::string& partial_path = "" );

	void Forget( const std::string& asset_path );

private:
	friend struct AssetManager;
	friend struct AssetManagerWindow;

	const BjSON::IReadOnlyObject& m_rootNode;
	std::unordered_map< std::string, std::shared_ptr< const BjSON::IReadOnlyObject > > m_readers;
};

// for managing asset packs in-editor
// immediately loads all assets and keeps them loaded until closed, but only deserialises them when requested
struct AssetManager
{
	enum Flags : u8
	{
		None = 0,
		// load the asset structure immediately
		PreSearch,
		// load all assets immediately, implies PreSearch
		PreLoad,
	};

	AssetManager( const BjSON::IReadOnlyObject& root_node, Flags flags = None );
	~AssetManager()
	{
		INFO( "Destroying asset manager" );

		for ( auto& [path, weak_ref] : m_weakAssetReferences )
			WEAK_ASSERT( weak_ref.expired(), "{} still has {} strong references when its asset manager is being destroyed", path, weak_ref.use_count() );
	}
	
private:
	friend struct AssetManagerWindow;

	CachedBjSONReader m_reader;

	// every asset gets a weak reference stored
	std::unordered_map< std::string, std::weak_ptr< IAsset > > m_weakAssetReferences;

	// some assets also get a strong reference
	std::unordered_map< std::string, std::shared_ptr< IAsset > > m_strongAssetReferences;

	Flags m_initialFlags = None;

public:
	void Save( BjSON::IReadWriteObject& root_node );

	template< typename Asset >
	std::shared_ptr< Asset > New( std::string path_to_asset, bool hold_reference = true )
	{
		const bool asset_exists = m_weakAssetReferences.find( path_to_asset ) != m_weakAssetReferences.end();
		!LOG_ASSERT( !asset_exists, "An asset already exists at {}", path_to_asset );

		std::shared_ptr< Asset > asset = std::make_shared< Asset >();
		asset->m_assetManager = this;
		asset->m_path = path_to_asset;

		if ( hold_reference )
			m_strongAssetReferences.insert( { path_to_asset, asset } );
		
		m_weakAssetReferences.insert( { path_to_asset, asset } );

		return asset;
	}

	template< typename Asset >
	std::shared_ptr< Asset > Load( std::string path_to_asset, bool hold_reference = false, IAsset::LoadType load_type = IAsset::LoadType::Stream )
	{
		//auto iter = m_assets.find( path_to_asset );
		//if ( !LOG_ASSERT( iter != m_assets.end(), "Tried to load an asset at {} but none exists", path_to_asset ) )
		//	return nullptr;

		auto weak_ref_iter = m_weakAssetReferences.find( path_to_asset );
		if ( weak_ref_iter != m_weakAssetReferences.end() )
		{
			if ( std::shared_ptr< IAsset > iasset = weak_ref_iter->second.lock() )
			{
				if ( std::shared_ptr< Asset > asset = WEAK_ASSERT( std::dynamic_pointer_cast<Asset>( iasset ),
					"An asset exists at {} but it isn't a {}", path_to_asset, typeid( Asset ).name() ) )
					return asset;
				else
					return nullptr;
			}
		}

		#ifndef NDEBUG
		auto strong_ref_iter = m_strongAssetReferences.find( path_to_asset );
		WEAK_ASSERT( strong_ref_iter == m_strongAssetReferences.end(), "We don't have a valid weak reference, but do have a strong reference? How?" );
		#endif

		if ( std::shared_ptr< const BjSON::IReadOnlyObject > reader = LOG_ASSERT( m_reader.GetReader( path_to_asset ), "No asset exists at {}", path_to_asset ) )
		{
			std::shared_ptr< Asset > asset = std::make_shared< Asset >();
			asset->SetReader( reader );
			asset->m_assetManager = this;
			asset->m_path = path_to_asset;
			asset->Load( load_type );

			if ( hold_reference )
				m_strongAssetReferences.insert( { path_to_asset, asset } );

			m_weakAssetReferences.insert( { path_to_asset, asset } );

			return asset;
		}

		return nullptr;
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

	char m_newFolderName[ 32 ] = "";
	bool m_makingNewFolder = false;

	char m_newAssetPath[ 32 ] = "";
	bool m_newAsset = false;

	int m_numIconsPerRow = 3;

	void Reset()
	{
		m_makingNewFolder = false;
		std::memset( m_newFolderName, 0, sizeof( m_newFolderName ) );
		m_currentFolder = "/";
		m_assetManager = nullptr;
		m_decoder = nullptr;
	}

	inline static const char* const s_name = "Asset Pack Manager";
	const char* GetName() const override { return s_name; }
	void Run( IFrameContext& frame_context ) override;
	std::string GetWindowTitle() const override;
};

}
