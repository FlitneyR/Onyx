#pragma once

#include "Common/Assets.h"
#include "Common/LowLevel/LowLevelInterface.h"

namespace onyx
{

struct ShaderAsset final : IAsset
{
	std::string m_source;
	std::vector< u32 > m_byteCode;

	bool Import( const char* filename );

	enum struct ShaderStage
	{
		None = 0,
		Compute,
		Vertex,
		Fragment,
		Count,
	};

	inline static const char* const s_ShaderStageNames[] {
		"None",
		"Compute",
		"Vertex",
		"Fragment"
	};

	bool CompileSource( LowLevel::Config::GraphicsAPI api, ShaderStage stage, std::string& log );

	// IAsset
	void Load( LoadType type ) override;
	void Save( BjSON::IReadWriteObject& writer, SaveType type ) override;
	void DoAssetManagerButton( const char* name, const char* path, f32 width, std::shared_ptr< IAsset > asset, IFrameContext& frame_context ) override;
};

struct ShaderEditorWindow : editor::IWindow
{
	inline static const char* const s_name = "Shader Editor";
	const char* GetName() const { return s_name; }

	void Run( IFrameContext& frame_context ) override;
	std::string GetWindowTitle() const override;

	std::string m_log;
	std::shared_ptr< ShaderAsset > m_shader;

	LowLevel::Config::GraphicsAPI m_compileGraphicsAPI;
	ShaderAsset::ShaderStage m_compileShaderStage;
};

}
