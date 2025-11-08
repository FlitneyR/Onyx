#include "Assets.h"

// for making new assets of these types
#include "Onyx/Graphics/Texture.h"
#include "Onyx/Graphics/Shader.h"
#include "Onyx/ECS/Scene.h"

#include "imgui_stdlib.h"

#include <set>
#include <map>

namespace onyx
{

void CachedBjSONReader::ReadRecursive( const BjSON::IReadOnlyObject& node, const std::string& partial_path )
{
	ZoneScoped;

	auto contents_list = node.GetArray( "__content"_name );
	if ( !contents_list )
	{
		WEAK_ASSERT( partial_path.size() > 0, "We're at the root node, but it doesn't have a content list, how are we meant to know what's in this asset pack" );
		return;
	}

	for ( u32 index = 0; index < contents_list->Count(); ++index )
	{
		if ( std::shared_ptr< const BjSON::IReadOnlyObject > child_description = WEAK_ASSERT( contents_list->GetChild( index ) ) )
		{
			const std::string child_name = child_description->GetLiteral< std::string >( "name"_name );
			if ( !WEAK_ASSERT( !child_name.empty(), "A child in the contents list has no name, that's not right" ) )
				continue;

			const std::shared_ptr< const BjSON::IReadOnlyObject > child_node = node.GetChild( BjSON::HashName( child_name.c_str() ) );
			if ( !WEAK_ASSERT( child_node, "Child node {} is listed in the node's contents list, but not actually present", child_name ) )
				continue;

			const std::string path = partial_path + "/" + child_name;
			m_readers.insert( { path, child_node } );
			ReadRecursive( *child_node, path );
		}
	}
}

void CachedBjSONReader::Forget( const std::string& path )
{
	m_readers.erase( path );
}

std::shared_ptr< const BjSON::IReadOnlyObject > CachedBjSONReader::GetReader( const std::string& asset_path )
{
	ZoneScoped;

	auto iter = m_readers.find( asset_path );
	if ( iter == m_readers.end() )
	{
		// we don't have it cached, so recursively search for the parent
		const size_t last_delimiter = asset_path.find_last_of( '/' );
		if ( last_delimiter == std::string::npos )
			return nullptr;

		const std::string parent_path = asset_path.substr( 0, last_delimiter );
		const BjSON::IReadOnlyObject* parent = parent_path.empty() ? &m_rootNode : GetReader( parent_path ).get();
		if ( !parent )
			return nullptr;

		// we found the parent, now find the child in the parent
		const std::string child_path = asset_path.substr( last_delimiter + 1 );
		std::shared_ptr< const BjSON::IReadOnlyObject > child = parent->GetChild( BjSON::HashName( child_path.c_str() ) );
		if ( !child )
			return nullptr;

		// insert the child back into the cache
		iter = m_readers.insert( iter, { asset_path, child } );
	}

	// these should never be null, but just to make sure
	WEAK_ASSERT( iter->second );
	return iter->second;
}

AssetManager::AssetManager( const BjSON::IReadOnlyObject& root_node, Flags flags )
	: m_reader( root_node )
	, m_initialFlags( flags )
{
	ZoneScoped;

	if ( flags & ( PreSearch | PreLoad ) )
		m_reader.ReadRecursive( root_node );

	if ( flags & PreLoad )
	{
		for ( auto& [path, reader] : m_reader.m_readers )
		{
			if ( !reader || !reader->GetLiteral( "__assetType"_name ) )
				continue;

			IAsset* asset = nullptr;

			switch ( reader->GetLiteral< BjSON::NameHash >( "__assetType"_name ) )
			{
			case "Texture"_name: asset = New< TextureAsset >( path ).get(); break;
			case "TextureAnimation"_name: asset = New< TextureAnimationAsset >( path ).get(); break;
			case "Shader"_name: asset = New< ShaderAsset >( path ).get(); break;
			case "Scene"_name: asset = New< ecs::Scene >( path ).get(); break;
			}

			if ( WEAK_ASSERT( asset ) )
			{
				asset->SetReader( reader );
				asset->m_assetManager = this;
				asset->m_path = path;
				asset->Load( IAsset::LoadType::Editor );
			}
		}
	}
}

struct CachedBjSONWriter
{
	CachedBjSONWriter( BjSON::IReadWriteObject& root_node )
	{
		m_writers.insert( { "", root_node } );
	}

	BjSON::IReadWriteObject& GetWriter( const std::string& asset_path )
	{
		ZoneScoped;

		auto iter = m_writers.find( asset_path );
		if ( iter == m_writers.end() )
		{
			// we don't have it cached, so recursively search for the parent
			const size_t last_delimiter = asset_path.find_last_of( '/' );

			const std::string parent_path = asset_path.substr( 0, last_delimiter );
			BjSON::IReadWriteObject& parent = GetWriter( parent_path );

			// we found the parent, now add the child to the parent
			const std::string child_name = asset_path.substr( last_delimiter + 1 );
			BjSON::IReadWriteObject& child = parent.AddChild( BjSON::HashName( child_name.c_str() ) );

			// and add the child to the content list ( add a content list if one doesn't already exist )
			BjSON::IReadWriteObjectArray* child_array = parent.GetArray( "__content"_name );
			if ( !child_array )
				child_array = &parent.AddArray( "__content"_name );
			child_array->AddChild().SetLiteral( "name"_name, child_name );

			// insert the child back into the cache
			iter = m_writers.insert( iter, { asset_path, child } );
		}

		return iter->second;
	}

private:
	std::unordered_map< std::string, BjSON::IReadWriteObject& > m_writers;
};

void AssetManager::Save( BjSON::IReadWriteObject& root_node )
{
	ZoneScoped;

	CachedBjSONWriter writer( root_node );

	WEAK_ASSERT( m_initialFlags & PreLoad, "We're about to save an asset pack, but we didn't load everything at the start, so we might not save everything" );

	for ( auto& [ path, asset ] : m_strongAssetReferences )
	{
		if ( asset->GetLoadingState() != LoadingState::Loaded )
		{
			asset->m_assetManager = this;
			asset->m_path = path;
			asset->Load( IAsset::LoadType::Editor );
		}
		
		asset->Save( writer.GetWriter( path ), IAsset::SaveType::Save );
	}
}

static int AssetManagerPathEditCallback( ImGuiInputTextCallbackData* data )
{
	AssetManagerWindow* window = static_cast< AssetManagerWindow* >( data->UserData );
	
	if ( data->Buf[ data->BufTextLen - 1 ] == '/' )
	{
		window->m_currentFolder += data->Buf;
		data->DeleteChars( 0, data->BufTextLen );
	}

	return 0;
}

std::string AssetManagerWindow::GetWindowTitle() const
{
	return std::format( "Asset Pack Manager: {}###{}", m_osFilePath, (u64)this );
}

void AssetManagerWindow::Run( IFrameContext& frame_context )
{
	ZoneScoped;

	if ( ImGui::Begin( GetWindowTitle().c_str(), &m_open, ImGuiWindowFlags_MenuBar ) )
	{
		ImGui::SetWindowSize( { 600, 300 }, ImGuiCond_Once );

		if ( ImGui::BeginMenuBar() )
		{
			if ( ImGui::BeginMenu( "File" ) )
			{
				if ( ImGui::MenuItem( "Open" ) )
				{
					std::string asset_pack_path = LowLevel::GetWindowManager().DoOpenFileDialog();

					if ( !asset_pack_path.empty() )
					{
						m_osFilePath = asset_pack_path;
						Reset();

						m_fileSource = std::ifstream( asset_pack_path, std::ios::binary | std::ios::beg );

						if ( m_fileSource.is_open() )
						{
							m_decoder = std::make_unique< BjSON::Decoder >( m_fileSource );
							m_assetManager = std::make_unique< AssetManager >( m_decoder->GetRootObject(), AssetManager::PreLoad );
						}
					}
				}

				if ( ImGui::MenuItem( "New" ) )
				{
					Reset();

					{
						BjSON::Encoder encoder;
						encoder.GetRootObject().AddArray( "__content"_name );
						encoder.WriteTo( m_bufferSource );
					}

					m_decoder = std::make_unique< BjSON::Decoder >( m_bufferSource );
					m_assetManager = std::make_unique< AssetManager >( m_decoder->GetRootObject(), AssetManager::PreLoad );
				}

				if ( ImGui::MenuItem( "Save", nullptr, nullptr, m_assetManager != nullptr ) )
				{
					std::string asset_pack_path = LowLevel::GetWindowManager().DoSaveFileDialog();

					if ( !asset_pack_path.empty() )
					{
						BjSON::Encoder encoder;
						m_assetManager->Save( encoder.GetRootObject() );
						std::ofstream ofile( asset_pack_path, std::ios::binary | std::ios::trunc );
						encoder.WriteTo( ofile );
					}
				}
				
				ImGui::EndMenu();
			}

			if ( ImGui::BeginMenu( "View" ) )
			{
				ImGui::SliderInt( "Num Icons Per Row", &m_numIconsPerRow, 1, 10 );
				
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		if ( m_assetManager )
		{
			std::string remaining_path = m_currentFolder;
			std::string partial_path = "";
			do {
				const u32 next_delimiter = (u32)remaining_path.find_first_of( '/' );
				const std::string folder = remaining_path.substr( 0, next_delimiter + 1 );
				partial_path += folder;

				if ( ImGui::Button( folder.c_str() ) )
					m_currentFolder = partial_path;

				ImGui::SameLine();
				remaining_path = remaining_path.substr( next_delimiter + 1 );
			}
			while ( !remaining_path.empty() );

			ImGui::SetNextItemWidth( 100.f );
			ImGui::InputText( "##New Folder Name", m_newFolderName, _countof( m_newFolderName ), ImGuiInputTextFlags_CallbackAlways, &AssetManagerPathEditCallback, this );

			ImGui::SameLine();
			if ( ImGui::Button( "New" ) )
				ImGui::OpenPopup( "New Asset" );

			if ( ImGui::BeginPopup( "New Asset" ) )
			{
				ImGui::SetNextItemWidth( 200.f );
				ImGui::InputText( "New Asset Path", m_newAssetPath, sizeof( m_newAssetPath ) );

				ImGui::SetNextItemWidth( 100.f );
				if ( ImGui::BeginCombo( "New Asset", "Type" ) )
				{
					if ( ImGui::Selectable( "Texture" ) )
					{
						TextureAsset* texture = m_assetManager->New< TextureAsset >( m_currentFolder + std::string( m_newAssetPath ) ).get();

						const TextureAsset::Pixel black( 0, 0, 0, 255 );
						texture->Init( 1, 1, &black );

						std::memset( m_newAssetPath, 0, sizeof( m_newAssetPath ) );
						ImGui::CloseCurrentPopup();
					}

					if ( ImGui::Selectable( "Texture Animation" ) )
					{
						m_assetManager->New< TextureAnimationAsset >( m_currentFolder + std::string( m_newAssetPath ) );

						std::memset( m_newAssetPath, 0, sizeof( m_newAssetPath ) );
						ImGui::CloseCurrentPopup();
					}

					if ( ImGui::Selectable( "Shader" ) )
					{
						m_assetManager->New< ShaderAsset >( m_currentFolder + std::string( m_newAssetPath ) );

						std::memset( m_newAssetPath, 0, sizeof( m_newAssetPath ) );
						ImGui::CloseCurrentPopup();
					}

					if ( ImGui::Selectable( "Scene" ) )
					{
						m_assetManager->New< ecs::Scene >( m_currentFolder + std::string( m_newAssetPath ) );

						std::memset( m_newAssetPath, 0, sizeof( m_newAssetPath ) );
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndCombo();
				}

				ImGui::EndPopup();
			}

			ImGui::Separator();

			if ( ImGui::BeginTable( "Icons", m_numIconsPerRow ) )
			{
				std::map< std::string, std::shared_ptr< IAsset > > assets;
				std::set< std::string > folders;

				for ( auto& [path, asset] : m_assetManager->m_strongAssetReferences )
				{
					const size_t last_delimiter = path.find_last_of( '/' ) + 1;

					const std::string folder = path.substr( 0, last_delimiter );
					const std::string name = path.substr( last_delimiter );

					// show assets in the current folder
					if ( folder == m_currentFolder )
					{
						assets.insert( { name, asset } );
					}
					// show folders in the current folder
					else if ( folder.substr( 0, m_currentFolder.size() ) == m_currentFolder )
					{
						const std::string sub_folder = folder.substr( m_currentFolder.size() );
						folders.insert( sub_folder.substr( 0, sub_folder.find_first_of( '/' ) ) + '/' );
					}
				}

				u32 column = 0;
				ImGui::TableNextRow();

				for ( const std::string& folder : folders )
				{
					ImGui::TableNextColumn();
					if ( ImGui::Button( folder.c_str() ) )
						m_currentFolder += folder;

					if ( !( column = ( column + 1 ) % m_numIconsPerRow ) )
						ImGui::TableNextRow();
				}

				for ( auto& [name, asset] : assets )
				{
					const std::string complete_path = m_currentFolder + name;

					ImGui::TableNextColumn();
					{
						ImGuiScopedID scoped_id( name.c_str() );

						const bool should_delete = ImGui::Button( "[-]" );
						std::string new_name;

						ImGui::SameLine();
						if ( ImGui::Button( "Rename" ) )
						{
							new_name = asset->m_path;
							ImGui::OpenPopup( "Rename" );
						}

						ImGui::SameLine();
						if ( ImGui::Button( "Copy Path" ) )
							ImGui::SetClipboardText( complete_path.c_str() );

						if ( ImGui::BeginPopup( "Rename" ) )
						{
							ImGui::InputText( "Name", &new_name );

							if ( ImGui::Button( "Cancel" ) )
								ImGui::CloseCurrentPopup();

							ImGui::SameLine();
							if ( ImGui::Button( "Ok" ) )
							{
								m_assetManager->m_strongAssetReferences.erase( complete_path );
								m_assetManager->m_strongAssetReferences.insert( { new_name, asset } );
							}

							ImGui::EndPopup();
						}

						if ( should_delete )
						{
							m_assetManager->m_strongAssetReferences.erase( complete_path );
							m_assetManager->m_weakAssetReferences.erase( complete_path );
							m_assetManager->m_reader.Forget( complete_path );
							break;
						}

						asset->DoAssetManagerButton( name.c_str(), complete_path.c_str(), ImGui::GetColumnWidth(), asset, frame_context );
					}

					if ( !( column = ( column + 1 ) % m_numIconsPerRow ) )
						ImGui::TableNextRow();
				}

				ImGui::EndTable();
			}
		}
	}

	ImGui::End();
}

}
