#include "Common/LowLevel/LowLevelInterface.h"
#include "VulkanGraphicsContext.h"
#include "VulkanDeleters.h"
#include "SDLWindowManager.h"

#include "SDL2/SDL_vulkan.h"

#include "vulkan/vulkan.h"
#include "vulkan/vulkan_to_string.hpp"

#include "imgui_impl_vulkan.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace onyx
{

static constexpr vk::Format s_imguiRenderTargetFormat = vk::Format::eR8G8B8A8Srgb;

static VkBool32 VulkanDebugMessageCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT          message_severity,
	VkDebugUtilsMessageTypeFlagsEXT                 message_types,
	const VkDebugUtilsMessengerCallbackDataEXT*		callback_data,
	void*											user_data
) {
	switch ( message_severity )
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:		INFO( "Vulkan: {}", callback_data->pMessage ); break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:	WARN( "Vulkan: {}", callback_data->pMessage ); break;
	default:
		WEAK_ASSERT( message_severity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, "Recieved a vulkan error: {}", callback_data->pMessage );
		break;
	}

	return false;
}

VulkanGraphicsContext::VulkanGraphicsContext()
{
	const vkb::Result< vkb::Instance > vkb_instance = vkb::InstanceBuilder()
		.set_debug_callback( VulkanDebugMessageCallback )
		.set_debug_callback_user_data_pointer( this )
		.enable_validation_layers()
		.require_api_version( 1, 3, 0 )
		.build();

	STRONG_ASSERT( vkb_instance, "Failed to instantiate vulkan: {}", vkb_instance.error().message() );
	m_vkInstance = vkb_instance.value().instance;
	m_shutdownDeleteQueue.Add< Deleter< vk::Instance > >( m_vkInstance, vkb_instance.value().debug_messenger );

	const vkb::Result< vkb::PhysicalDevice> vkb_physical_device = vkb::PhysicalDeviceSelector( vkb_instance.value() )
		.defer_surface_initialization()
		.set_required_features_13( vk::PhysicalDeviceVulkan13Features()
			.setDynamicRendering( true ) )
		.select();

	STRONG_ASSERT( vkb_physical_device, "Failed to find a vulkan device: {}", vkb_physical_device.error().message() );
	m_vkPhysicalDevice = vkb_physical_device.value().physical_device;

	const vkb::Result< vkb::Device > vkb_device = vkb::DeviceBuilder( vkb_physical_device.value() )
		.build();

	STRONG_ASSERT( vkb_device, "Failed to create a vulkan logical device: {}", vkb_device.error().message() );
	m_vkDevice = vkb_device.value().device;
	m_shutdownDeleteQueue.Add< Deleter< vk::Device > >( m_vkDevice );

	VULKAN_HPP_DEFAULT_DISPATCHER.init( m_vkInstance, m_vkDevice );

	const vk::ResultValue< vma::Allocator > allocator = vma::createAllocator( vma::AllocatorCreateInfo()
		.setInstance( m_vkInstance )
		.setPhysicalDevice( m_vkPhysicalDevice )
		.setDevice( m_vkDevice )
	);

	STRONG_ASSERT( allocator.result == vk::Result::eSuccess, "Failed to initialise VMA allocator: {}", to_string( allocator.result ) );
	m_vmaAllocator = allocator.value;
	m_shutdownDeleteQueue.Add< Deleter< vma::Allocator > >( m_vmaAllocator );

	const vkb::Result< VkQueue > graphics_queue = vkb_device.value().get_queue( vkb::QueueType::graphics );
	const vkb::Result< VkQueue > transfer_queue = vkb_device.value().get_queue( vkb::QueueType::transfer );
	const vkb::Result< VkQueue > compute_queue = vkb_device.value().get_queue( vkb::QueueType::compute );

	const vkb::Result< u32 > graphics_queue_index = vkb_device.value().get_queue_index( vkb::QueueType::graphics );
	const vkb::Result< u32 > transfer_queue_index = vkb_device.value().get_queue_index( vkb::QueueType::transfer );
	const vkb::Result< u32 > compute_queue_index = vkb_device.value().get_queue_index( vkb::QueueType::compute );

	STRONG_ASSERT( graphics_queue, "Failed to find a graphics queue: {}", graphics_queue.error().message() );
	STRONG_ASSERT( transfer_queue, "Failed to find a transfer queue: {}", transfer_queue.error().message() );
	STRONG_ASSERT( compute_queue, "Failed to find a compute queue: {}", compute_queue.error().message() );

	STRONG_ASSERT( graphics_queue_index, "Failed to find a graphics queue: {}", graphics_queue_index.error().message() );
	STRONG_ASSERT( transfer_queue_index, "Failed to find a transfer queue: {}", transfer_queue_index.error().message() );
	STRONG_ASSERT( compute_queue_index, "Failed to find a compute queue: {}", compute_queue_index.error().message() );

	m_vkGraphicsQueue = graphics_queue.value();
	m_vkTransferQueue = transfer_queue.value();
	m_vkComputeQueue = compute_queue.value();

	m_vkGraphicsQueueIndex = graphics_queue_index.value();
	m_vkTransferQueueIndex = transfer_queue_index.value();
	m_vkComputeQueueIndex = compute_queue_index.value();

	const vk::ResultValue< vk::CommandPool > command_pool = m_vkDevice.createCommandPool( vk::CommandPoolCreateInfo()
		.setFlags( vk::CommandPoolCreateFlagBits::eResetCommandBuffer )
		.setQueueFamilyIndex( m_vkGraphicsQueueIndex ) );

	STRONG_ASSERT( command_pool.result == vk::Result::eSuccess );
	m_vkRenderingCommandPool = command_pool.value;
	m_shutdownDeleteQueue.Add< Deleter< vk::CommandPool > >( m_vkDevice, m_vkRenderingCommandPool );
}

VulkanGraphicsContext::~VulkanGraphicsContext()
{
	m_shutdownDeleteQueue.Execute();
}

std::unique_ptr< IWindowContext > VulkanGraphicsContext::CreateWindowContext( IWindow& window )
{
	return std::make_unique< WindowContext >( window );
}

IFrameContext* VulkanGraphicsContext::BeginFrame( IWindow& window )
{
	WindowContext& window_context = static_cast< WindowContext& >( window.GetWindowContext() );
	FrameContext& frame_context = static_cast< FrameContext& >( window_context.GetFrameContext() );

	if ( window.HasClosed() )
		return nullptr;

	if ( window_context.m_failedToResize )
		window_context.OnResize();

	if ( window_context.m_failedToResize )
		return nullptr;

	if ( LowLevel::GetConfig().enableImGui )
	{
		ImGui_ImplVulkan_NewFrame();
		LowLevel::GetWindowManager().ImGuiNewFrame();
		ImGui::NewFrame();
	}

	// request the next swapchain image for this window
	const vk::ResultValue< u32 > next_image_index = m_vkDevice.acquireNextImageKHR( window_context.m_swapchain, ~0, nullptr, frame_context.m_swapchainImageAcquiredFence );
	switch ( next_image_index.result )
	{
	case vk::Result::eErrorOutOfDateKHR:
	case vk::Result::eSuboptimalKHR:
		window_context.OnResize();
		return nullptr;
	default:
		STRONG_ASSERT( next_image_index.result == vk::Result::eSuccess );
	}

	frame_context.m_swapchainImageIndex = next_image_index.value;

	// wait to make sure this command buffer is not still rendering a previous frame
	STRONG_ASSERT( m_vkDevice.waitForFences( frame_context.m_finishedRenderingFence, true, ~0 ) == vk::Result::eSuccess );
	STRONG_ASSERT( m_vkDevice.resetFences( frame_context.m_finishedRenderingFence ) == vk::Result::eSuccess );

	STRONG_ASSERT( frame_context.m_commandBuffer.begin( vk::CommandBufferBeginInfo()
		.setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit ) ) == vk::Result::eSuccess );

	const vk::ImageSubresourceRange subresource_range = vk::ImageSubresourceRange()
		.setAspectMask( vk::ImageAspectFlagBits::eColor )
		.setLayerCount( 1 )
		.setLevelCount( 1 );

	frame_context.m_commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eTransfer,
		vk::DependencyFlagBits::eByRegion,
		{}, {}, vk::ImageMemoryBarrier()
			.setImage( window_context.m_swapchainImages[ frame_context.m_swapchainImageIndex ] )
			.setNewLayout( vk::ImageLayout::eTransferDstOptimal )
			.setDstAccessMask( vk::AccessFlagBits::eTransferWrite )
			.setSubresourceRange( subresource_range )
	);

	frame_context.m_commandBuffer.clearColorImage(
		window_context.m_swapchainImages[ frame_context.m_swapchainImageIndex ],
		vk::ImageLayout::eTransferDstOptimal,
		vk::ClearColorValue( 0.f, 0.f, 0.f, 1.f ),
		subresource_range
	);

	return &frame_context;
}

void VulkanGraphicsContext::EndFrame( IFrameContext& _frame_context )
{
	FrameContext& frame_context = static_cast<FrameContext&>( _frame_context );
	WindowContext& window_context = static_cast<WindowContext&>( *frame_context.m_windowContext );
	vk::CommandBuffer cmd = frame_context.m_commandBuffer;

	const vk::ImageSubresourceRange subresource_range = vk::ImageSubresourceRange()
		.setAspectMask( vk::ImageAspectFlagBits::eColor )
		.setLayerCount( 1 )
		.setLevelCount( 1 );

	// render imgui
	if ( LowLevel::GetConfig().enableImGui )
	{
		const glm::ivec2 window_size = window_context.m_renderTargetSize;

		// render imgui to this window's imgui target
		cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::DependencyFlagBits::eByRegion,
			{}, {}, vk::ImageMemoryBarrier()
				.setImage( window_context.m_imGuiRenderTarget )
				.setDstAccessMask( vk::AccessFlagBits::eColorAttachmentWrite )
				.setNewLayout( vk::ImageLayout::eColorAttachmentOptimal )
				.setSubresourceRange( subresource_range )
		);

		const vk::RenderingAttachmentInfo color_attachment = vk::RenderingAttachmentInfo()
			.setImageView( window_context.m_imGuiRenderTargetView )
			.setClearValue( vk::ClearValue( { 0.f, 0.f, 0.f, 0.f } ) )
			.setImageLayout( vk::ImageLayout::eColorAttachmentOptimal )
			.setLoadOp( vk::AttachmentLoadOp::eClear )
			.setStoreOp( vk::AttachmentStoreOp::eStore );

		cmd.beginRendering( vk::RenderingInfo()
			.setColorAttachments( color_attachment )
			.setRenderArea( vk::Rect2D()
				.setExtent( vk::Extent2D( window_size.x, window_size.y ) )
			)
			.setLayerCount( 1 )
		);

		ImGui::Render();
		if ( ImDrawData* const draw_data = STRONG_ASSERT( ImGui::GetDrawData() ) )
			ImGui_ImplVulkan_RenderDrawData( draw_data, cmd );

		cmd.endRendering();

		// blit this window's imgui render target to the present image
		cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlagBits::eByRegion,
			{}, {}, vk::ImageMemoryBarrier()
				.setImage( window_context.m_imGuiRenderTarget )
				.setSrcAccessMask( vk::AccessFlagBits::eColorAttachmentWrite )
				.setOldLayout( vk::ImageLayout::eColorAttachmentOptimal )
				.setDstAccessMask( vk::AccessFlagBits::eTransferRead )
				.setNewLayout( vk::ImageLayout::eTransferSrcOptimal )
				.setSubresourceRange( subresource_range )
		);

		cmd.blitImage(
			window_context.m_imGuiRenderTarget, vk::ImageLayout::eTransferSrcOptimal,
			window_context.m_swapchainImages[ frame_context.m_swapchainImageIndex ], vk::ImageLayout::eTransferDstOptimal,
			vk::ImageBlit()
				.setSrcOffsets( { vk::Offset3D( 0, 0, 0 ), vk::Offset3D( window_size.x, window_size.y, 1 ) } )
				.setDstOffsets( { vk::Offset3D( 0, 0, 0 ), vk::Offset3D( window_size.x, window_size.y, 1 ) } )
				.setSrcSubresource( vk::ImageSubresourceLayers().setAspectMask( vk::ImageAspectFlagBits::eColor ).setLayerCount( 1 ) )
				.setDstSubresource( vk::ImageSubresourceLayers().setAspectMask( vk::ImageAspectFlagBits::eColor ).setLayerCount( 1 ) ),
			vk::Filter::eNearest
		);
	}

	cmd.pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::DependencyFlagBits::eByRegion,
		{}, {}, vk::ImageMemoryBarrier()
			.setImage( window_context.m_swapchainImages[ frame_context.m_swapchainImageIndex ] )
			.setOldLayout( vk::ImageLayout::eTransferDstOptimal )
			.setSrcAccessMask( vk::AccessFlagBits::eTransferWrite )
			.setNewLayout( vk::ImageLayout::ePresentSrcKHR )
			.setSubresourceRange( subresource_range )
	);

	STRONG_ASSERT( cmd.end() == vk::Result::eSuccess );

	// make sure we got the swapchain image we requested in BeginFrame
	STRONG_ASSERT( m_vkDevice.waitForFences( frame_context.m_swapchainImageAcquiredFence, true, ~0 ) == vk::Result::eSuccess );
	STRONG_ASSERT( m_vkDevice.resetFences( frame_context.m_swapchainImageAcquiredFence ) == vk::Result::eSuccess );

	const vk::Result submit_result = m_vkGraphicsQueue.submit( vk::SubmitInfo()
		.setCommandBuffers( cmd )
		.setSignalSemaphores( frame_context.m_finishedRenderingSemaphore ),
		frame_context.m_finishedRenderingFence );

	STRONG_ASSERT( submit_result == vk::Result::eSuccess );

	const vk::Result present_result = m_vkGraphicsQueue.presentKHR( vk::PresentInfoKHR()
		.setWaitSemaphores( frame_context.m_finishedRenderingSemaphore )
		.setSwapchains( window_context.m_swapchain )
		.setImageIndices( frame_context.m_swapchainImageIndex ) );

	switch ( present_result )
	{
	case vk::Result::eErrorOutOfDateKHR:
	case vk::Result::eSuboptimalKHR:
		window_context.OnResize();
		break;
	default:
		STRONG_ASSERT( present_result == vk::Result::eSuccess );
		break;
	}

	window_context.m_frameCount++;
}

VulkanGraphicsContext::WindowContext::WindowContext( IWindow& window ) : IWindowContext( window )
{
	OnResize();

	VulkanGraphicsContext& ctx = static_cast<VulkanGraphicsContext&>( LowLevel::GetGraphicsContext() );

	vk::ResultValue< std::vector< vk::CommandBuffer > > command_buffers = ctx.m_vkDevice.allocateCommandBuffers( vk::CommandBufferAllocateInfo()
		.setCommandBufferCount( m_framesInFlight )
		.setCommandPool( ctx.m_vkRenderingCommandPool )
		.setLevel( vk::CommandBufferLevel::ePrimary ) );

	STRONG_ASSERT( command_buffers.result == vk::Result::eSuccess );

	for ( u32 frame_context_index = 0; frame_context_index < m_framesInFlight; ++frame_context_index )
	{
		FrameContext& frame_context = m_frameContexts[ frame_context_index ];

		vk::ResultValue< vk::Fence > image_acquired_fence = ctx.m_vkDevice.createFence( vk::FenceCreateInfo() );
		vk::ResultValue< vk::Semaphore > finished_rendering_semaphore = ctx.m_vkDevice.createSemaphore( vk::SemaphoreCreateInfo() );
		vk::ResultValue< vk::Fence > finished_rendering_fence = ctx.m_vkDevice.createFence( vk::FenceCreateInfo().setFlags( vk::FenceCreateFlagBits::eSignaled ) );

		STRONG_ASSERT( image_acquired_fence.result == vk::Result::eSuccess );
		STRONG_ASSERT( finished_rendering_semaphore.result == vk::Result::eSuccess );
		STRONG_ASSERT( finished_rendering_fence.result == vk::Result::eSuccess );

		frame_context.m_windowContext = this;
		frame_context.m_finishedRenderingFence = finished_rendering_fence.value;
		frame_context.m_swapchainImageAcquiredFence = image_acquired_fence.value;
		frame_context.m_finishedRenderingSemaphore = finished_rendering_semaphore.value;
		frame_context.m_commandBuffer = command_buffers.value[ frame_context_index ];

		m_deleteQueue.Add< Deleter< vk::Fence > >( ctx.m_vkDevice, image_acquired_fence.value );
		m_deleteQueue.Add< Deleter< vk::Fence > >( ctx.m_vkDevice, finished_rendering_fence.value );
		m_deleteQueue.Add< Deleter< vk::Semaphore > >( ctx.m_vkDevice, finished_rendering_semaphore.value );
	}

	if ( LowLevel::GetConfig().enableImGui )
	{
		vk::DescriptorPoolSize imgui_descriptor_pool_sizes[] {
			{ vk::DescriptorType::eCombinedImageSampler,	1000 },
			{ vk::DescriptorType::eSampledImage,			1000 },
			{ vk::DescriptorType::eStorageImage,			1000 },
			{ vk::DescriptorType::eUniformTexelBuffer,		1000 },
			{ vk::DescriptorType::eStorageTexelBuffer,		1000 },
			{ vk::DescriptorType::eUniformBuffer,			1000 },
			{ vk::DescriptorType::eStorageBuffer,			1000 },
			{ vk::DescriptorType::eUniformBufferDynamic,	1000 },
			{ vk::DescriptorType::eStorageBufferDynamic,	1000 },
			{ vk::DescriptorType::eInputAttachment,			1000 },
		};

		vk::ResultValue< vk::DescriptorPool > imgui_descriptor_pool = ctx.m_vkDevice.createDescriptorPool( vk::DescriptorPoolCreateInfo()
			.setMaxSets( 1'000 )
			.setPoolSizes( imgui_descriptor_pool_sizes )
			.setFlags( vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet ) );

		STRONG_ASSERT( imgui_descriptor_pool.result == vk::Result::eSuccess );
		m_deleteQueue.Add< Deleter< vk::DescriptorPool > >( ctx.m_vkDevice, imgui_descriptor_pool.value );

		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = ctx.m_vkInstance;
		init_info.PhysicalDevice = ctx.m_vkPhysicalDevice;
		init_info.Device = ctx.m_vkDevice;
		init_info.Queue = ctx.m_vkGraphicsQueue;
		init_info.DescriptorPool = imgui_descriptor_pool.value;
		init_info.MinImageCount = init_info.ImageCount = m_framesInFlight;
		init_info.UseDynamicRendering = true;
		init_info.ColorAttachmentFormat = (VkFormat)s_imguiRenderTargetFormat;

		ImGui_ImplVulkan_Init( &init_info, VK_NULL_HANDLE );
	}
}

VulkanGraphicsContext::WindowContext::~WindowContext()
{
	static_cast< VulkanGraphicsContext& >( LowLevel::GetGraphicsContext() ).m_vkDevice.waitIdle();
	ImGui_ImplVulkan_Shutdown();
	m_resizeDeleteQueue.Execute();
}

void VulkanGraphicsContext::WindowContext::OnResize()
{
	m_failedToResize = true;

	if ( m_window->HasClosed() )
		return;

	const glm::ivec2 window_size = m_window->GetSize();
	if ( window_size.x == 0 || window_size.y == 0 )
		return;

	m_renderTargetSize = window_size;

	static_cast<VulkanGraphicsContext&>( LowLevel::GetGraphicsContext() ).m_vkDevice.waitIdle();
	m_resizeDeleteQueue.Execute();

	switch ( LowLevel::GetConfig().windowManager )
	{
	case LowLevel::Config::WindowManager::SDL: SDLSetup(); break;
	default:
		STRONG_ASSERT( false, "Unrecognised or unsupported window manager: {}", (u32)LowLevel::GetConfig().windowManager );
	}

	if ( LowLevel::GetConfig().enableImGui )
	{
		VulkanGraphicsContext& ctx = static_cast<VulkanGraphicsContext&>( LowLevel::GetGraphicsContext() );

		const vk::ResultValue< std::pair< vk::Image, vma::Allocation > > imgui_render_target = ctx.m_vmaAllocator.createImage(
			vk::ImageCreateInfo()
				.setImageType( vk::ImageType::e2D )
				.setExtent( vk::Extent3D( window_size.x, window_size.y, 1 ) )
				.setArrayLayers( 1 )
				.setMipLevels( 1 )
				.setFormat( s_imguiRenderTargetFormat )
				.setSamples( vk::SampleCountFlagBits::e1 )
				.setUsage( vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eColorAttachment ),
			vma::AllocationCreateInfo()
				.setUsage( vma::MemoryUsage::eGpuOnly )
		);

		STRONG_ASSERT( imgui_render_target.result == vk::Result::eSuccess, "Failed to create imgui render target" );
		const auto [imgui_render_target_image, imgui_render_target_allocation] = imgui_render_target.value;

		m_resizeDeleteQueue.Add< Deleter< vk::Image, vma::Allocation > >( ctx.m_vmaAllocator, imgui_render_target_image, imgui_render_target_allocation );

		m_imGuiRenderTarget = imgui_render_target_image;

		const vk::ResultValue< vk::ImageView > imgui_render_target_image_view = ctx.m_vkDevice.createImageView( vk::ImageViewCreateInfo()
			.setImage( m_imGuiRenderTarget )
			.setFormat( s_imguiRenderTargetFormat )
			.setViewType( vk::ImageViewType::e2D )
			.setSubresourceRange( vk::ImageSubresourceRange()
				.setAspectMask( vk::ImageAspectFlagBits::eColor )
				.setLayerCount( 1 )
				.setLevelCount( 1 ) )
		);

		STRONG_ASSERT( imgui_render_target_image_view.result == vk::Result::eSuccess, "{}", to_string( imgui_render_target_image_view.result ) );
		m_imGuiRenderTargetView = imgui_render_target_image_view.value;

		m_resizeDeleteQueue.Add< Deleter< vk::ImageView > >( ctx.m_vkDevice, m_imGuiRenderTargetView );
	}
	
	m_failedToResize = false;
}

void VulkanGraphicsContext::WindowContext::SDLSetup()
{
	SDLWindow& window = static_cast< SDLWindow& >( *m_window );
	VulkanGraphicsContext& ctx = static_cast<VulkanGraphicsContext&>( LowLevel::GetGraphicsContext() );

	VkSurfaceKHR surface;
	STRONG_ASSERT( SDL_Vulkan_CreateSurface( window.m_window, ctx.m_vkInstance, &surface ) );
	m_surface = surface;
	m_resizeDeleteQueue.Add< Deleter< vk::SurfaceKHR > >( ctx.m_vkInstance, m_surface );

	vkb::Result< vkb::Swapchain > swapchain = vkb::SwapchainBuilder( ctx.m_vkPhysicalDevice, ctx.m_vkDevice, surface )
		.set_desired_present_mode( VK_PRESENT_MODE_FIFO_KHR )
		.add_image_usage_flags( VkImageUsageFlags( vk::ImageUsageFlagBits::eTransferDst ) )
		.build();

	STRONG_ASSERT( swapchain, "Failed to create a swapchain for this window: {}, {}",
		swapchain.error().message(), vk::to_string( vk::Result( swapchain.vk_result() ) ) );

	m_swapchain = swapchain.value().swapchain;
	m_resizeDeleteQueue.Add< Deleter< vk::SwapchainKHR > >( ctx.m_vkDevice, m_swapchain );

	vkb::Result< std::vector< VkImage > > images = swapchain.value().get_images();
	vkb::Result< std::vector< VkImageView > > image_views = swapchain.value().get_image_views();
	STRONG_ASSERT( images, "Failed to create swapchain images: {}", images.error().message() );
	STRONG_ASSERT( image_views, "Failed to create swapchain image views: {}", image_views.error().message() );

	m_swapchainImages.clear();
	m_swapchainImageViews.clear();

	m_swapchainImages.reserve( images.value().size() );
	m_swapchainImageViews.reserve( image_views.value().size() );

	for ( const VkImage& image : images.value() )
		m_swapchainImages.push_back( image );

	for ( const VkImageView& image_view : image_views.value() )
	{
		m_swapchainImageViews.push_back( image_view );
		m_resizeDeleteQueue.Add< Deleter< vk::ImageView > >( ctx.m_vkDevice, image_view );
	}
}

IFrameContext& VulkanGraphicsContext::WindowContext::GetFrameContext()
{
	return m_frameContexts[ m_frameCount % m_framesInFlight ];
}

}
