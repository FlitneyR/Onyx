#pragma once
#include "Common/Graphics/FrameContext.h"
#include "Common/Graphics/RenderTarget.h"
#include "Common/Graphics/GraphicsContext.h"
#include "Common/Graphics/Texture.h"
#include "Common/Graphics/Shader.h"

#pragma warning( push )
#pragma warning( disable : 4996 ) // disable warnings about strncpy in vulkan.hpp
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VULKAN_HPP_NO_EXCEPTIONS
#define VK_NO_PROTOTYPES
#define VULKAN_HPP_ASSERT_ON_RESULT ignore
#include <vulkan/vulkan.hpp>
#pragma warning( pop )

#include "VkBootstrap.h"
#include "vulkan-memory-allocator-hpp/vk_mem_alloc.hpp"

#include <map>

namespace onyx
{

struct VulkanGraphicsContext final : IGraphicsContext
{
	VulkanGraphicsContext();
	~VulkanGraphicsContext();

	std::unique_ptr< IWindowContext > CreateWindowContext( IWindow& window ) override;

	IFrameContext* BeginFrame( IWindow& window ) override;
	void EndFrame( IFrameContext& frame_context ) override;

	std::shared_ptr< IRenderTarget > CreateRenderTarget( const glm::uvec2& dimensions ) override;
	std::shared_ptr< ITextureResource > CreateTextureResource( const TextureAsset& asset ) override;
	std::shared_ptr< ISpriteRenderer > CreateSpriteRenderer() override;

private:

	vk::Instance m_vkInstance;
	vk::PhysicalDevice m_vkPhysicalDevice;
	vk::Device m_vkDevice;

	vma::Allocator m_vmaAllocator;

	vk::Queue m_vkGraphicsQueue;
	u32 m_vkGraphicsQueueIndex;

	vk::Queue m_vkComputeQueue;
	u32 m_vkComputeQueueIndex;

	vk::Queue m_vkTransferQueue;
	u32 m_vkTransferQueueIndex;

	vk::CommandPool m_vkRenderingCommandPool;
	vk::CommandPool m_vkTransientCommandPool;

	vk::Fence m_stagingBufferInUseFence;
	vk::Buffer m_stagingBuffer;
	vma::Allocation m_stagingBufferAllocation;

	struct TransientCommand
	{
		vk::CommandBuffer cmd;
		vk::Buffer stagingBuffer;
		vma::Allocation stagingAllocation;
	};

	TransientCommand BeginTransientCommand( u32 required_staging_buffer_size );
	void SubmitTransientCommand( TransientCommand tc );

	struct WindowContext;
	struct FrameContext;

	struct FrameContext final : IFrameContext
	{
		WindowContext* m_windowContext;
		vk::CommandBuffer m_cmd;
		vk::Fence m_swapchainImageAcquiredFence;
		vk::Semaphore m_finishedRenderingSemaphore;
		vk::Fence m_finishedRenderingFence;
		u32 m_swapchainImageIndex;

		void BlitRenderTarget( std::shared_ptr< IRenderTarget >& render_target, glm::uvec2 position, glm::uvec2 size ) override;

		glm::uvec2 GetSize() const override { return m_windowContext->m_renderTargetSize; }
	};

	struct WindowContext final : IWindowContext
	{
		vk::SwapchainKHR m_swapchain;
		vk::SurfaceKHR m_surface;
		std::vector< vk::Image > m_swapchainImages;
		std::vector< vk::ImageView > m_swapchainImageViews;

		glm::uvec2 m_renderTargetSize {};

		vk::Image m_imGuiRenderTarget;
		vk::ImageView m_imGuiRenderTargetView;

		DeleteQueue m_resizeDeleteQueue;

		u32 m_frameCount = 0;
		u32 m_framesInFlight = 3;

		constexpr static u32 s_maxFramesInFlight = 3;
		FrameContext m_frameContexts[ s_maxFramesInFlight ];

		bool m_failedToResize = false;

		WindowContext( IWindow& window );
		~WindowContext();
		void OnResize();

		// setup the surface, swapchain, images, and image views from an SDLWindow
		void SDLSetup();
		
		IFrameContext& GetFrameContext() override;
	};

	struct RenderTarget final : IRenderTarget
	{
		vk::Image m_image;
		vk::Sampler m_sampler;
		vk::ImageView m_imageView;
		ImTextureID m_imTextureId = nullptr;
		vk::ImageLayout m_layout = vk::ImageLayout::eUndefined;

		RenderTarget( const glm::uvec2& size ) : IRenderTarget( size ) {}

		void Clear( IFrameContext& frame_ctx, const glm::vec4& colour ) override;

		void DoLayoutTransition( FrameContext& ctx, vk::ImageLayout layout, vk::PipelineStageFlags stage, vk::AccessFlags access );

		void PrepareForRendering( IFrameContext& frame_ctx ) override;
		void PrepareForCompositing( IFrameContext& frame_ctx ) override;
		void PrepareForSampling( IFrameContext& frame_ctx ) override;

		ImTextureID GetImTextureID() override;
	};

	struct TextureResource final : ITextureResource
	{
		vk::Image m_image;
		vk::Sampler m_sampler;
		vk::ImageView m_imageView;
		ImTextureID m_imTextureId = nullptr;

		~TextureResource()
		{
			INFO( "Deleting texture resource" );
		}

		ImTextureID GetImTextureID() override;
	};

	struct SpriteRenderingResources
	{
		vk::DescriptorSetLayout descriptorSetLayout;
		vk::DescriptorPool descriptorPool;
		vk::PipelineLayout pipelineLayout;
		vk::Pipeline pipeline;
	};

	std::unique_ptr< SpriteRenderingResources > m_spriteRenderingResources;
	void InitSpriteRendererResources();

	struct SpriteRenderer final : ISpriteRenderer
	{
		static constexpr u32 c_maxTextures = 1'000;
		
		struct PerFrameData final : IGraphicsResource
		{
			vk::Buffer transformBuffer;
			vma::Allocation transformBufferAllocation;
			vk::DescriptorSet descriptorSet;

			PerFrameData()
			{
				INFO( "Created a new PerFrameData set" );
			}

			~PerFrameData()
			{
				INFO( "Destroyed a PerFrameData set" );
			}
		};

		static std::shared_ptr< PerFrameData > CreatePerFrameData(
			VulkanGraphicsContext& ctx,
			u32 required_transform_buffer_size
		);

		std::map< const IFrameContext*, std::shared_ptr< PerFrameData > > m_perFrameData;

		void Render( IFrameContext& frame, std::shared_ptr< IRenderTarget >& render_target, const SpriteRenderData& data ) override;
	};

	template< typename ... > struct Deleter;
};

}
