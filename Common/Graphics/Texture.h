#pragma once
#include "Common/Assets.h"
#include "GraphicsResource.h"

namespace onyx
{

enum struct ImageFilterMode : u8
{
	Pixel = 0,
	Smooth,
};

enum struct ImageCompressionMode : u8
{
	Lossy = 0,
	Lossless,
};

const char* const s_ImageFilterModeNames[] = { "Pixel", "Smooth" };
const char* const s_ImageCompressionModeNames[] = { "Lossy", "Lossless" };

template< typename T, const size_t count >
T DecodeEnum( const char* const ( &names )[ count ], const char* name, T default_val )
{
	for ( u32 val = 0; val < count; ++val )
		if ( !strcmp( names[ val ], name ) )
			return T( val );

	return default_val;
}

// An RGBA 8bit image
// CPU side: can be loaded from disk, or initialised from CPU memory
// GPU side: can be sampled in a shader
#pragma region 

// GPU-side representation of a Texture
struct ITextureResource : IGraphicsResource
{
	virtual ImTextureID GetImTextureID() = 0;
};

// CPU-side representation of a Texture
struct TextureAsset final : IAsset
{
	using Pixel = glm::vec< 4, byte >;

	glm::uvec2 GetDimensions() const { return m_dimensions; }
	const Pixel* GetPixels() const { return m_pixels.data(); }

	void Init( u32 width, u32 height, const Pixel* pixels );

	bool Import( const char* filename );

	// IAsset
	void Load( LoadType type ) override;
	void Save( BjSON::IReadWriteObject& writer, SaveType type ) override;
	void DoAssetManagerButton( const char* name, const char* path, f32 width, std::shared_ptr< IAsset > asset, IFrameContext& frame_context, const IAssetManagerCallbacks& callbacks ) override;

	std::shared_ptr< ITextureResource > GetGraphicsResource();

	ImageFilterMode m_filterMode = ImageFilterMode::Smooth;
	ImageCompressionMode m_compressionMode = ImageCompressionMode::Lossy;

	void ResetResource() { m_gpuResource.reset(); }

private:
	std::shared_ptr< ITextureResource > m_gpuResource;
	std::vector< Pixel > m_pixels;
	glm::uvec2 m_dimensions;
};

struct TexturePreviewWindow : editor::IWindow
{
	inline static const char* const s_name = "Texture Preview";
	const char* GetName() const override { return s_name; }
	void Run( IFrameContext& frame_context ) override;
	std::string GetWindowTitle() const override;

	std::shared_ptr< TextureAsset > m_texture;
};

struct TextureAnimationAsset final : IAsset
{
	struct Frame
	{
		std::shared_ptr< TextureAsset > texture;
		glm::vec2 offset {};
		glm::vec2 extent { 1.f, 1.f };
		glm::vec2 denom { 1.f, 1.f };
	};

	std::vector< Frame > m_frames;
	f32 m_rate = 1.f;

	// IAsset
	void Load( LoadType type ) override;
	void Save( BjSON::IReadWriteObject& writer, SaveType type ) override;
	void DoAssetManagerButton( const char* name, const char* path, f32 width, std::shared_ptr< IAsset > asset, IFrameContext& frame_context, const IAssetManagerCallbacks& callbacks ) override;
};

struct TextureAnimationEditor : editor::IWindow
{
	inline static const char* const s_name = "Texture Animation Editor";
	const char* GetName() const override { return s_name; }
	void Run( IFrameContext& frame_context ) override;
	std::string GetWindowTitle() const override;

	f32 m_scale = 1.f;
	
	std::string m_selectTexturePath;
	std::shared_ptr< TextureAnimationAsset > m_animation;
};

#pragma endregion

}
