#include "Texture.h"

#include "Common/LowLevel/LowLevelInterface.h"

#include "Common/ECS/ComponentReflector.h"

#include "stb_image.h"
#include "stb_image_write.h"

#include "imgui_stdlib.h"

namespace onyx
{

DEFINE_DEFAULT_DESERIALISE_PROPERTY( std::shared_ptr< TextureAsset > )
{ std::string path; reader.GetLiteral( name, path ); value = asset_manager.Load< TextureAsset >( path ); }

DEFINE_DEFAULT_PROPERTY_DIFF_HINT( std::shared_ptr< TextureAsset > )
{ ImGui::SetTooltip( !value ? "No Texture" : value->m_path.c_str() ); }

DEFINE_DEFAULT_PROPERTY_EDITOR_UI( std::shared_ptr< TextureAsset > )
{
	ImGuiScopedID scoped_id( name );

	ImGui::Text( name );
	if ( value ? ImGui::ImageButton( value->GetGraphicsResource()->GetImTextureID(), { 100, 100 } ) : ImGui::Button( "Select Texture" ) )
		ImGui::OpenPopup( "Select Texture" );

	bool was_edited = false;
	if ( ImGui::BeginPopup( "Select Texture" ) )
	{
		static std::string new_texture_path;
		ImGui::InputText( "Path", &new_texture_path );

		if ( ImGui::Button( "Cancel" ) )
			ImGui::CloseCurrentPopup();

		ImGui::SameLine();
		if ( ImGui::Button( "Ok" ) )
		{
			if ( new_texture_path.empty() )
			{
				was_edited = true;
				value = nullptr;
			}
			else if ( auto texture = asset_manager.Load< TextureAsset >( new_texture_path ) )
			{
				was_edited = true;
				value = texture;
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}

	return was_edited;
}

DEFINE_DEFAULT_SERIALISE_PROPERTY( std::shared_ptr< TextureAsset > )
{ writer.SetLiteral( name, !value ? "" : value->m_path ); }

DEFINE_DEFAULT_DESERIALISE_PROPERTY( std::shared_ptr< TextureAnimationAsset > )
{ std::string path; reader.GetLiteral( name, path ); if ( !path.empty() ) value = asset_manager.Load< TextureAnimationAsset >( path ); }

DEFINE_DEFAULT_PROPERTY_DIFF_HINT( std::shared_ptr< TextureAnimationAsset > )
{ ImGui::SetTooltip( !value ? "No Texture" : value->m_path.c_str() ); }

DEFINE_DEFAULT_PROPERTY_EDITOR_UI( std::shared_ptr< TextureAnimationAsset > )
{
	ImGuiScopedID scoped_id( name );

	if ( ImGui::Button( std::format( "{} : {}", name, !value ? "No Animation" : value->m_path ).c_str() ) )
		ImGui::OpenPopup( "Select Texture" );

	bool was_edited = false;

	if ( ImGui::BeginPopup( "Select Texture" ) )
	{
		static std::string new_path;
		ImGui::InputText( "Path", &new_path );

		if ( ImGui::Button( "Cancel" ) )
			ImGui::CloseCurrentPopup();

		ImGui::SameLine();
		if ( ImGui::Button( "Ok" ) )
		{
			if ( new_path.empty() )
			{
				was_edited = true;
				value = nullptr;
			}
			else if ( auto animation = asset_manager.Load< TextureAnimationAsset >( new_path ) )
			{
				was_edited = true;
				value = animation;
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}

	return was_edited;
}

DEFINE_DEFAULT_SERIALISE_PROPERTY( std::shared_ptr< TextureAnimationAsset > )
{ writer.SetLiteral( name, !value ? "" : value->m_path ); }

void TextureAsset::Init( u32 width, u32 height, const Pixel* pixels )
{
	m_dimensions = { width, height };
	m_pixels.resize( width * height );
	std::memcpy( m_pixels.data(), pixels, width * height * sizeof( Pixel ) );

	m_gpuResource.reset();
}

bool TextureAsset::Import( const char* filename )
{
	int width, height, channels_in_file;
	// todo: load only as many channels are in the file
	byte* const pixels = stbi_load( filename, &width, &height, &channels_in_file, 4 );

	if ( !WEAK_ASSERT( pixels, "Failed to import image from: {}", filename ) )
		return false;

	Init( width, height, reinterpret_cast< const Pixel* >( pixels ) );
	stbi_image_free( pixels );
	return true;
}

void TextureAsset::Load( LoadType type )
{
	m_loadingState = LoadingState::Loading;

	const BjSON::IReadOnlyObject* const reader = GetReader();
	if ( !WEAK_ASSERT( reader ) )
		RETURN_LOAD_ERRORED();

	if ( u32 asset_type; !WEAK_ASSERT( reader->GetLiteral( "__assetType"_name, asset_type ) && asset_type == "Texture"_name ) )
		RETURN_LOAD_ERRORED();

	const std::string filter_mode = reader->GetLiteral< std::string >( "FilterMode"_name );
	const std::string compression_mode = reader->GetLiteral< std::string >( "CompressionMode"_name );

	m_filterMode = DecodeEnum( s_ImageFilterModeNames, filter_mode.c_str(), ImageFilterMode::Smooth );
	m_compressionMode = DecodeEnum( s_ImageCompressionModeNames, compression_mode.c_str(), ImageCompressionMode::Lossless );

	//WEAK_ASSERT( reader->GetLiteral( "FilterMode"_name, m_filterMode ) == sizeof( m_filterMode ) );
	//WEAK_ASSERT( reader->GetLiteral( "CompressionMode"_name, m_compressionMode ) == sizeof( m_compressionMode ) );

	const u32 data_size = reader->GetLiteral( "Data"_name );
	std::vector< byte > data;
	data.resize( data_size );

	reader->GetLiteral( "Data"_name, &data[ 0 ], (u32)data.size() );

	int width, height, channels_in_file;
	byte* const pixels = stbi_load_from_memory( data.data(), (i32)data.size(), &width, &height, &channels_in_file, 4 );
	if ( !WEAK_ASSERT( pixels, "Failed to load image data" ) )
		RETURN_LOAD_ERRORED();

	Init( width, height, reinterpret_cast< const Pixel* >( pixels ) );
	stbi_image_free( pixels );

	m_loadingState = LoadingState::Loaded;
}

static void BufferWriteFunc( void* context, void* data, i32 size )
{
	std::vector< byte >& image_data = *reinterpret_cast< std::vector< byte >* >( context );
	image_data.insert( image_data.end(), reinterpret_cast< byte* >( data ), reinterpret_cast<byte*>( data ) + size );
}

void TextureAsset::Save( BjSON::IReadWriteObject& writer, SaveType type )
{
	writer.SetLiteral( "__assetType"_name, "Texture"_name );
	writer.SetLiteral( "FilterMode"_name, s_ImageFilterModeNames[ u32(m_filterMode) ], (u32)strlen( s_ImageFilterModeNames[ u32( m_filterMode ) ] ) );
	writer.SetLiteral( "CompressionMode"_name, s_ImageCompressionModeNames[ u32( m_compressionMode ) ], (u32)strlen( s_ImageCompressionModeNames[ u32( m_compressionMode ) ] ) );

	std::vector< byte > image_data;

	i32 save_result = -1;
	switch ( m_compressionMode )
	{
	case ImageCompressionMode::Lossy:		save_result = stbi_write_jpg_to_func( BufferWriteFunc, &image_data, m_dimensions.x, m_dimensions.y, 4, m_pixels.data(), 100 );	break;
	case ImageCompressionMode::Lossless:	save_result = stbi_write_png_to_func( BufferWriteFunc, &image_data, m_dimensions.x, m_dimensions.y, 4, m_pixels.data(), m_dimensions.x * sizeof( Pixel ) );	break;
	}

	WEAK_ASSERT( save_result != 0, "Failed to save image" );

	writer.SetLiteral( "Data"_name, image_data.data(), (u32)image_data.size() );
}

void TextureAsset::DoAssetManagerButton( const char* name, const char* path, f32 width, std::shared_ptr< IAsset > asset, IFrameContext& frame_context )
{
	switch ( m_loadingState )
	{
	case LoadingState::Loaded:
		ImGui::PushStyleColor( ImGuiCol_Button,			ImVec4( 0.f, 0.25f, 0.0f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered,	ImVec4( 0.f, 0.5f, 0.0f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive,	ImVec4( 0.f, 0.125f, 0.0f, 1.f ) );
		break;
	case LoadingState::Errored:
		ImGui::PushStyleColor( ImGuiCol_Button,			ImVec4( 0.25f, 0.f, 0.f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered,	ImVec4( 0.5f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive,	ImVec4( 0.125f, 0.f, 0.f, 1.f ) );
		break;
	default:
		ImGui::PushStyleColor( ImGuiCol_Button,			ImVec4( 0.25f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered,	ImVec4( 0.25f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive,	ImVec4( 0.125f, 0.125f, 0.125f, 1.f ) );
		break;
	}

	if ( ImGui::Button( name ) )
	{
		if ( m_loadingState == LoadingState::Unloaded )
			Load( IAsset::LoadType::Editor );

		TexturePreviewWindow* const window = editor::AddWindow< TexturePreviewWindow >();
		window->texture = std::static_pointer_cast< TextureAsset >( asset );
	}

	if ( auto resource = GetGraphicsResource() )
	{
		frame_context.RegisterUsedResource( resource );
		ImGui::Image( resource->GetImTextureID(), ImVec2( width, GetDimensions().y * width / GetDimensions().x ) );
	}

	ImGui::PopStyleColor( 3 );
}

std::shared_ptr< ITextureResource > TextureAsset::GetGraphicsResource()
{
	if ( GetLoadingState() == LoadingState::Loaded && !m_gpuResource )
		m_gpuResource = LowLevel::GetGraphicsContext().CreateTextureResource( *this );

	return m_gpuResource;
}

void TexturePreviewWindow::Run( IFrameContext& frame_context )
{
	if ( ImGui::Begin( GetWindowTitle().c_str(), &m_open, ImGuiWindowFlags_MenuBar ) )
	{
		if ( ImGui::BeginMenuBar() )
		{
			if ( ImGui::BeginMenu( "File" ) )
			{
				if ( ImGui::MenuItem( "Import", nullptr, nullptr, texture != nullptr ) )
					if ( const std::string file_path = LowLevel::GetWindowManager().DoOpenFileDialog(); !file_path.empty() )
						texture->Import( file_path.c_str() );

				ImGui::EndMenu();
			}

			bool is_resource_out_of_date = false;

			if ( ImGui::BeginMenu( "Edit" ) )
			{
				i32 filter_mode = (i32)texture->m_filterMode;
				i32 compression_mode = (i32)texture->m_compressionMode;

				is_resource_out_of_date |= ImGui::Combo( "Filter", &filter_mode, s_ImageFilterModeNames, _countof( s_ImageFilterModeNames ) );
				is_resource_out_of_date |= ImGui::Combo( "Compression", &compression_mode, s_ImageCompressionModeNames, _countof( s_ImageCompressionModeNames ) );

				texture->m_filterMode = ImageFilterMode( filter_mode );
				texture->m_compressionMode = ImageCompressionMode( compression_mode );

				ImGui::EndMenu();
			}

			if ( is_resource_out_of_date )
				texture->ResetResource();

			ImGui::EndMenuBar();
		}

		if ( texture )
		{
			const ImVec2 window_size = ImGui::GetWindowContentRegionMax();
			const glm::uvec2 texture_size = texture->GetDimensions();

			const f32 width_scale = f32( window_size.x ) / f32( texture_size.x );
			const f32 height_scale = f32( window_size.y ) / f32( texture_size.y );

			const f32 scale = std::min( width_scale, height_scale ) * 0.9f;

			frame_context.RegisterUsedResource( texture->GetGraphicsResource() );

			ImGui::Image(
				texture->GetGraphicsResource()->GetImTextureID(),
				{ scale * f32( texture->GetDimensions().x ), scale * f32( texture->GetDimensions().y ) }
			);
		}
	}

	ImGui::End();
}

std::string TexturePreviewWindow::GetWindowTitle() const
{
	return std::format( "Texture Preview: {}###{}", texture ? texture->m_path : "", (u64)this );
}

void TextureAnimationAsset::Load( LoadType type )
{
	m_loadingState = LoadingState::Loading;

	const BjSON::IReadOnlyObject* const reader = GetReader();
	if ( !reader || reader->GetLiteral< u32 >( "__assetType"_name ) != "TextureAnimation"_name )
		RETURN_LOAD_ERRORED();

	reader->GetLiteral( "Rate"_name, m_rate );

	if ( auto frames = WEAK_ASSERT( reader->GetArray( "Frames"_name ) ) )
	{
		m_frames.resize( frames->Count() );

		for ( u32 idx = 0; idx < frames->Count(); ++idx )
		{
			Frame& frame = m_frames[ idx ];
			auto frame_reader = frames->GetChild( idx );

			if ( m_assetManager ) frame.texture = m_assetManager->Load< TextureAsset >( frame_reader->GetLiteral< std::string >( "TexturePath"_name ) );

			frame_reader->GetLiteral( "Offset"_name, &frame.offset, sizeof( frame.offset ) );
			frame_reader->GetLiteral( "Extent"_name, &frame.extent, sizeof( frame.extent ) );
			frame_reader->GetLiteral( "Denom"_name, &frame.denom, sizeof( frame.denom ) );
		}
	}

	m_loadingState = LoadingState::Loaded;
}

void TextureAnimationAsset::Save( BjSON::IReadWriteObject& writer, SaveType type )
{
	writer.SetLiteral( "__assetType"_name, "TextureAnimation"_name );
	writer.SetLiteral( "Rate"_name, m_rate );
	auto& frames = writer.AddArray( "Frames"_name );

	for ( auto& frame : m_frames )
	{
		frames.AddChild()
			.SetLiteral( "TexturePath"_name, frame.texture ? frame.texture->m_path : "" )
			.SetLiteral( "Offset"_name, frame.offset )
			.SetLiteral( "Extent"_name, frame.extent )
			.SetLiteral( "Denom"_name, frame.denom );
	}
}

void TextureAnimationAsset::DoAssetManagerButton( const char* name, const char* path, f32 width, std::shared_ptr< IAsset > asset, IFrameContext& frame_context )
{
	switch ( m_loadingState )
	{
	case LoadingState::Loaded:
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.1f, 0.25f, 0.0f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.2f, 0.5f, 0.0f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.05f, 0.125f, 0.0f, 1.f ) );
		break;
	case LoadingState::Errored:
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.25f, 0.f, 0.f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.5f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.125f, 0.f, 0.f, 1.f ) );
		break;
	default:
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.25f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.25f, 0.25f, 0.25f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0.125f, 0.125f, 0.125f, 1.f ) );
		break;
	}

	if ( ImGui::Button( name ) )
	{
		if ( m_loadingState == LoadingState::Unloaded )
			Load( IAsset::LoadType::Editor );

		TextureAnimationEditor* const window = editor::AddWindow< TextureAnimationEditor >();
		window->animation = std::static_pointer_cast< TextureAnimationAsset >( asset );
	}

	ImGui::PopStyleColor( 3 );
}

void TextureAnimationEditor::Run( IFrameContext& frame_context )
{
	if ( ImGui::Begin( GetWindowTitle().c_str(), &m_open, ImGuiWindowFlags_MenuBar ) )
	{
		if ( ImGui::BeginMenuBar() )
		{
			if ( ImGui::BeginMenu( "View" ) )
			{
				ImGui::SliderFloat( "Scale", &m_scale, 0.1f, 10.f );

				ImGui::EndMenu();
			}

			if ( ImGui::BeginMenu( "Edit" ) )
			{
				if ( animation )
					ImGui::DragFloat( "Default Rate", &animation->m_rate, 1.f );

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		if ( animation )
		{
			u32 frame_to_remove = ~0u;
			u32 frame_to_copy = ~0u;
			std::pair< u32, u32 > swap_frames = {};

			for ( u32 idx = 0; idx < animation->m_frames.size(); ++idx )
			{
				auto& frame = animation->m_frames[ idx ];

				ImGui::PushID( idx );

				if ( ImGui::Button( "[-]" ) )
					frame_to_remove = idx;

				ImGui::SameLine();
				if ( ImGui::ArrowButton( "MoveUp", ImGuiDir_Up ) )
					swap_frames = { idx, idx - 1 };

				ImGui::SameLine();
				if ( ImGui::ArrowButton( "MoveDown", ImGuiDir_Down ) )
					swap_frames = { idx, idx + 1 };

				ImGui::SameLine();
				if ( ImGui::Button( "Select Texture" ) )
					ImGui::OpenPopup( "Select Texture" );

				if ( ImGui::BeginPopup( "Select Texture" ) )
				{
					ImGui::InputText( "Path", &m_selectTexturePath );

					if ( ImGui::Button( "Select" ) )
					{
						std::shared_ptr< TextureAsset > new_texture;

						if ( animation->m_assetManager )
							new_texture = animation->m_assetManager->Load< TextureAsset >( m_selectTexturePath );

						if ( animation->m_assetManager )
							new_texture = animation->m_assetManager->Load< TextureAsset >( m_selectTexturePath );

						if ( new_texture )
						{
							frame.texture = new_texture;

							ImGui::CloseCurrentPopup();
						}
					}

					ImGui::SameLine();
					if ( ImGui::Button( "Cancel" ) )
						ImGui::CloseCurrentPopup();

					ImGui::EndPopup();
				}

				ImGui::SameLine();
				if ( ImGui::Button( "Copy" ) )
					frame_to_copy = idx;

				ImGui::DragFloat2( "Offset", reinterpret_cast< f32* >( &frame.offset.x ), 0.1f, 0.f, 1.f, "%.6f" );
				ImGui::DragFloat2( "Extent", reinterpret_cast< f32* >( &frame.extent.x ), 0.1f, 0.f, 1.f, "%.6f" );
				ImGui::DragFloat2( "Denom", reinterpret_cast< f32* >( &frame.denom.x ), 1.f, 10.f, 1.f, "%.6f" );

				if ( frame.texture )
				{
					ImGui::Image( frame.texture->GetGraphicsResource()->GetImTextureID(),
						ImVec2(
							m_scale * frame.extent.x * (f32)frame.texture->GetDimensions().x / frame.denom.x,
							m_scale * frame.extent.y * (f32)frame.texture->GetDimensions().y / frame.denom.y
						),
						ImVec2( frame.offset.x / frame.denom.x, frame.offset.y / frame.denom.y ),
						ImVec2( frame.offset.x / frame.denom.x + frame.extent.x / frame.denom.x, frame.offset.y / frame.denom.y + frame.extent.y / frame.denom.y )
					);
				}
				else
				{
					ImGui::Text( "Texture Missing" );
				}

				ImGui::PopID();
			}

			if ( frame_to_remove < animation->m_frames.size() )
				animation->m_frames.erase( animation->m_frames.begin() + frame_to_remove );

			if ( frame_to_copy < animation->m_frames.size() )
				animation->m_frames.insert( animation->m_frames.begin() + frame_to_copy, animation->m_frames[ frame_to_copy ] );

			if ( swap_frames.first < animation->m_frames.size() && swap_frames.second < animation->m_frames.size() )
				std::swap( animation->m_frames[ swap_frames.first ], animation->m_frames[ swap_frames.second ] );

			if ( ImGui::Button( "[+] " ) )
				ImGui::OpenPopup( "Select Texture" );

			if ( ImGui::BeginPopup( "Select Texture" ) )
			{
				ImGui::InputText( "Path", &m_selectTexturePath );

				if ( ImGui::Button( "Select" ) )
				{
					std::shared_ptr< TextureAsset > new_texture;

					if ( animation->m_assetManager )
						new_texture = animation->m_assetManager->Load< TextureAsset >( m_selectTexturePath );

					if ( animation->m_assetManager )
						new_texture = animation->m_assetManager->Load< TextureAsset >( m_selectTexturePath );

					if ( new_texture )
					{
						animation->m_frames.push_back( TextureAnimationAsset::Frame { new_texture } );

						ImGui::CloseCurrentPopup();
					}
				}

				ImGui::SameLine();
				if ( ImGui::Button( "Cancel" ) )
					ImGui::CloseCurrentPopup();

				ImGui::EndPopup();
			}

		}
	}

	ImGui::End();
}

std::string TextureAnimationEditor::GetWindowTitle() const
{
	return std::format( "Texture Animation Preview: {}###{}", animation ? animation->m_path : "", (u64)this );
}

}
