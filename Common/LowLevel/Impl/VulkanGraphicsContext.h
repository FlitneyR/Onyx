#pragma once
#include "Common/Graphics/FrameContext.h"
#include "Common/Graphics/GraphicsContext.h"

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

namespace onyx
{

struct VulkanGraphicsContext : IGraphicsContext
{
	VulkanGraphicsContext();
	~VulkanGraphicsContext();

	std::unique_ptr< IWindowContext > CreateWindowContext( IWindow& window ) override;

	IFrameContext* BeginFrame( IWindow& window ) override;
	void EndFrame( IFrameContext& frame_context ) override;

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

	struct WindowContext;
	struct FrameContext;

	struct FrameContext : IFrameContext
	{
		WindowContext* m_windowContext;
		vk::CommandBuffer m_commandBuffer;
		vk::Fence m_swapchainImageAcquiredFence;
		vk::Semaphore m_finishedRenderingSemaphore;
		vk::Fence m_finishedRenderingFence;
		u32 m_swapchainImageIndex;
	};

	struct WindowContext : IWindowContext
	{
		vk::SwapchainKHR m_swapchain;
		vk::SurfaceKHR m_surface;
		std::vector< vk::Image > m_swapchainImages;
		std::vector< vk::ImageView > m_swapchainImageViews;

		glm::ivec2 m_renderTargetSize {};

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

	template< typename ... > struct Deleter;
};

}
