#include "Onyx/LowLevel/LowLevelInterface.h"
#include "VulkanGraphicsContext.h"
#include "VulkanDeleters.h"
#include "SDLWindowManager.h"

#include "SDL2/SDL_vulkan.h"

#include "vulkan/vulkan.h"
#include "vulkan/vulkan_to_string.hpp"

#include "imgui_impl_vulkan.h"

#include "tracy/Tracy.hpp"

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

#pragma region VulkanGraphicsContext

VulkanGraphicsContext::VulkanGraphicsContext()
{
	ZoneScoped;

	const vkb::Result< vkb::Instance > vkb_instance = vkb::InstanceBuilder()
#ifdef _DEBUG
		.add_validation_feature_enable( (VkValidationFeatureEnableEXT)vk::ValidationFeatureEnableEXT::eGpuAssisted ) // << improved debugging for bindless rendering
		.set_debug_callback( VulkanDebugMessageCallback )
		.set_debug_callback_user_data_pointer( this )
		.enable_validation_layers()
#endif
		.require_api_version( 1, 3, 0 )
		.build();

	STRONG_ASSERT( vkb_instance, "Failed to instantiate vulkan: {}", vkb_instance.error().message() );
	m_vkInstance = vkb_instance.value().instance;
	m_shutdownDeleteQueue.Add< Deleter< vk::Instance > >( m_vkInstance, vkb_instance.value().debug_messenger );

	const vkb::Result< vkb::PhysicalDevice > vkb_physical_device = vkb::PhysicalDeviceSelector( vkb_instance.value() )
		.defer_surface_initialization()
		.add_required_extension_features( vk::PhysicalDeviceDescriptorIndexingFeatures()
			.setDescriptorBindingPartiallyBound( true ) // << allows us to bind large descriptor sets that are sparsely bound for bindless rendering
			.setRuntimeDescriptorArray( true ) // << allows us to have unbounded descriptor arrays in shaders
			.setShaderSampledImageArrayNonUniformIndexing( true ) ) // << allows us to access unbounded arrays of sampled images in shaders
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

	const vk::ResultValue< vk::CommandPool > transient_command_pool = m_vkDevice.createCommandPool( vk::CommandPoolCreateInfo()
		.setFlags( vk::CommandPoolCreateFlagBits::eTransient )
		.setQueueFamilyIndex( m_vkGraphicsQueueIndex ) );

	STRONG_ASSERT( transient_command_pool.result == vk::Result::eSuccess );
	m_vkTransientCommandPool = transient_command_pool.value;
	m_shutdownDeleteQueue.Add< Deleter< vk::CommandPool > >( m_vkDevice, m_vkTransientCommandPool );
}

VulkanGraphicsContext::~VulkanGraphicsContext()
{
	ZoneScoped;

	for ( StagingBuffer& staging_buffer : m_stagingBuffers )
	{
		WEAK_ASSERT( staging_buffer.buffer && staging_buffer.allocation );
		m_vmaAllocator.destroyBuffer( staging_buffer.buffer, staging_buffer.allocation );
		m_vkDevice.destroyFence( staging_buffer.inUseFence );
	}

	m_shutdownDeleteQueue.Execute();
}

std::unique_ptr< IWindowContext > VulkanGraphicsContext::CreateWindowContext( IWindow& window )
{
	ZoneScoped;

	return std::make_unique< WindowContext >( window );
}

IFrameContext* VulkanGraphicsContext::BeginFrame( IWindow& window )
{
	ZoneScoped;

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

	{
		ZoneScopedN( "Wait for fences" );

		// wait to make sure this command buffer is not still rendering a previous frame
		STRONG_ASSERT(m_vkDevice.waitForFences(frame_context.m_finishedRenderingFence, true, ~0) == vk::Result::eSuccess);
		STRONG_ASSERT(m_vkDevice.resetFences(frame_context.m_finishedRenderingFence) == vk::Result::eSuccess);
	}

	frame_context.OnFinishFrame();

	STRONG_ASSERT( frame_context.m_cmd.begin( vk::CommandBufferBeginInfo()
		.setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit ) ) == vk::Result::eSuccess );

	const vk::ImageSubresourceRange subresource_range = vk::ImageSubresourceRange()
		.setAspectMask( vk::ImageAspectFlagBits::eColor )
		.setLayerCount( 1 )
		.setLevelCount( 1 );

	frame_context.m_cmd.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eTransfer,
		vk::DependencyFlagBits::eByRegion,
		{}, {}, vk::ImageMemoryBarrier()
			.setImage( window_context.m_swapchainImages[ frame_context.m_swapchainImageIndex ] )
			.setNewLayout( vk::ImageLayout::eTransferDstOptimal )
			.setDstAccessMask( vk::AccessFlagBits::eTransferWrite )
			.setSubresourceRange( subresource_range )
	);

	frame_context.m_cmd.clearColorImage(
		window_context.m_swapchainImages[ frame_context.m_swapchainImageIndex ],
		vk::ImageLayout::eTransferDstOptimal,
		vk::ClearColorValue( 0.f, 0.f, 0.f, 1.f ),
		subresource_range
	);

	return &frame_context;
}

void VulkanGraphicsContext::EndFrame( IFrameContext& _frame_context )
{
	ZoneScoped;

	FrameContext& frame_context = static_cast<FrameContext&>( _frame_context );
	WindowContext& window_context = static_cast<WindowContext&>( *frame_context.m_windowContext );
	vk::CommandBuffer cmd = frame_context.m_cmd;

	const vk::ImageSubresourceRange subresource_range = vk::ImageSubresourceRange()
		.setAspectMask( vk::ImageAspectFlagBits::eColor )
		.setLayerCount( 1 )
		.setLevelCount( 1 );

	// render imgui
	if ( LowLevel::GetConfig().enableImGui )
	{
		ZoneScopedN( "Render ImGui ");

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

	{
		ZoneScopedN( "Wait for fences" );

		// make sure we got the swapchain image we requested in BeginFrame
		STRONG_ASSERT(m_vkDevice.waitForFences(frame_context.m_swapchainImageAcquiredFence, true, ~0) == vk::Result::eSuccess);
		STRONG_ASSERT(m_vkDevice.resetFences(frame_context.m_swapchainImageAcquiredFence) == vk::Result::eSuccess);
	}

	{
		ZoneScopedN( "Submit command buffer" );

		const vk::Result submit_result = m_vkGraphicsQueue.submit( vk::SubmitInfo()
			.setCommandBuffers( cmd )
			.setSignalSemaphores( frame_context.m_finishedRenderingSemaphore ),
			frame_context.m_finishedRenderingFence );

		STRONG_ASSERT( submit_result == vk::Result::eSuccess );
	}

	{
		ZoneScopedN( "Present image" );

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
	}

	window_context.m_frameCount++;
}

VulkanGraphicsContext::StagingBuffer::StagingBuffer()
{
	ZoneScoped;

	VulkanGraphicsContext& ctx = static_cast<VulkanGraphicsContext&>( LowLevel::GetGraphicsContext() );

	const vk::ResultValue< vk::Fence > fence = ctx.m_vkDevice.createFence( vk::FenceCreateInfo( vk::FenceCreateFlagBits::eSignaled ) );
	STRONG_ASSERT( fence.result == vk::Result::eSuccess, "Failed to create fence: {}", vk::to_string( fence.result ) );
	inUseFence = fence.value;
}

bool VulkanGraphicsContext::StagingBuffer::IsInUse() const
{
	VulkanGraphicsContext& ctx = static_cast< VulkanGraphicsContext& >( LowLevel::GetGraphicsContext() );

	const vk::Result status = ctx.m_vkDevice.getFenceStatus( inUseFence );
	STRONG_ASSERT( status == vk::Result::eNotReady || status == vk::Result::eSuccess, "Unexpected status of staging buffer fence: {}", vk::to_string( status ) );

	return status != vk::Result::eSuccess;
}

u32 VulkanGraphicsContext::StagingBuffer::GetSize() const
{
	VulkanGraphicsContext& ctx = static_cast<VulkanGraphicsContext&>( LowLevel::GetGraphicsContext() );

	const vma::AllocationInfo alloc_info = ctx.m_vmaAllocator.getAllocationInfo( allocation );
	return (u32)alloc_info.size;
}

void VulkanGraphicsContext::StagingBuffer::Resize( u32 new_size )
{
	ZoneScoped;

	VulkanGraphicsContext& ctx = static_cast<VulkanGraphicsContext&>( LowLevel::GetGraphicsContext() );

	if ( buffer && STRONG_ASSERT( allocation ) )
	{
		ctx.m_vmaAllocator.destroyBuffer( buffer, allocation );
		buffer = nullptr;
		allocation = nullptr;
	}

	const auto create_buffer_result = ctx.m_vmaAllocator.createBuffer(
		vk::BufferCreateInfo()
			.setSize( new_size )
			.setUsage( vk::BufferUsageFlagBits::eTransferSrc ),
		vma::AllocationCreateInfo()
			.setUsage( vma::MemoryUsage::eCpuToGpu )
	);

	STRONG_ASSERT( create_buffer_result.result == vk::Result::eSuccess, "Failed to resize staging buffer: {}", vk::to_string( create_buffer_result.result ) );
	buffer = create_buffer_result.value.first;
	allocation = create_buffer_result.value.second;

	ctx.m_vmaAllocator.setAllocationName( allocation, "Transient staging buffer" );
}

VulkanGraphicsContext::TransientCommand VulkanGraphicsContext::BeginTransientCommand( u32 required_staging_buffer_size )
{
	ZoneScoped;

	// try to find a staging buffer
	StagingBuffer* staging_buffer = nullptr;

	if ( required_staging_buffer_size > 0 )
	{
		// DEBUG( "Looking for a staging buffer of {} bytes", required_staging_buffer_size );

		// make sure no other thread tampers with the staging buffer pool
		std::scoped_lock mutex_lock( m_stagingBufferMutex );

		u32 staging_buffer_size = 0;
		for ( StagingBuffer& existing_staging_buffer : m_stagingBuffers )
		{
			const std::string& is_in_use = existing_staging_buffer.IsInUse() ? "is in use" : "is not in use";
			// DEBUG( "Staging buffer {} is {} bytes and {}",
			// 	&existing_staging_buffer - &m_stagingBuffers[0], existing_staging_buffer.GetSize(), is_in_use );

			// we can't use this one, it's currently in use
			if ( existing_staging_buffer.IsInUse() )
				continue;

			const u32 existing_staging_buffer_size = existing_staging_buffer.GetSize();

			// any staging buffer is better than no staging buffer
			// a smaller buffer that is still big enough is better than a buffer that's bigger than we need
			// a buffer that is too small is not great, but if we're going to have to use a small staging buffer, we should grow the smallest so we have more big buffers to choose from in future
			const bool existing_staging_buffer_is_better = !staging_buffer
				|| ( existing_staging_buffer_size >= required_staging_buffer_size && existing_staging_buffer_size < staging_buffer_size )
				|| ( staging_buffer_size < required_staging_buffer_size && existing_staging_buffer_size < staging_buffer_size );

			if ( existing_staging_buffer_is_better )
			{
				staging_buffer = &existing_staging_buffer;
				staging_buffer_size = existing_staging_buffer_size;
			}
		}

		// we didn't find any appropriate staging buffer, so create a new one
		if ( !staging_buffer )
		{
			// DEBUG( "Couldn't find a staging buffer, so adding a new one" );
			m_stagingBuffers.push_back( StagingBuffer() );
			staging_buffer = &m_stagingBuffers.back();
		}

		// is it not big enough - or not initialised? Then *make* it big enough!
		if ( staging_buffer_size < required_staging_buffer_size )
		{
			// DEBUG( "Staging buffer {} is {} bytes, needs to be {} bytes, so resizing", staging_buffer - &m_stagingBuffers[ 0 ], staging_buffer_size, required_staging_buffer_size );
			staging_buffer->Resize( required_staging_buffer_size );
		}

		STRONG_ASSERT( !staging_buffer->IsInUse() );
	}

	STRONG_ASSERT( staging_buffer );
	m_vkDevice.resetFences( staging_buffer->inUseFence );

	const vk::ResultValue< std::vector< vk::CommandBuffer > > command_buffers = m_vkDevice.allocateCommandBuffers( vk::CommandBufferAllocateInfo()
		.setCommandPool( m_vkTransientCommandPool )
		.setCommandBufferCount( 1 ) );

	STRONG_ASSERT( command_buffers.result == vk::Result::eSuccess, "Failed to create transient command buffer: {}", vk::to_string( command_buffers.result ) );
	
	const vk::Result begin_result = command_buffers.value.front().begin( vk::CommandBufferBeginInfo().setFlags( vk::CommandBufferUsageFlagBits::eOneTimeSubmit ) );
	STRONG_ASSERT( begin_result == vk::Result::eSuccess, "Failed to begin a transient command: {}", vk::to_string( begin_result ) );

	return {
		command_buffers.value.front(),
		staging_buffer ? std::optional< StagingBuffer >{ *staging_buffer } : std::nullopt
	};
}

void VulkanGraphicsContext::SubmitTransientCommand( TransientCommand tc )
{
	ZoneScoped;

	STRONG_ASSERT( tc.cmd.end() == vk::Result::eSuccess );

	const vk::Result transient_submit_result = m_vkGraphicsQueue.submit(
		vk::SubmitInfo()
			.setCommandBuffers( tc.cmd ),
		tc.stagingBuffer ? tc.stagingBuffer->inUseFence : nullptr
	);

	STRONG_ASSERT( transient_submit_result == vk::Result::eSuccess, "Failed to submit transient command: {}", vk::to_string( transient_submit_result ) );
}

std::shared_ptr< IRenderTarget > VulkanGraphicsContext::CreateRenderTarget( const glm::uvec2& dimensions )
{
	ZoneScoped;

	std::shared_ptr< RenderTarget > render_target = std::make_shared< RenderTarget >( dimensions );

	const vk::ResultValue< std::pair< vk::Image, vma::Allocation > > create_image_result = m_vmaAllocator.createImage(
		vk::ImageCreateInfo()
			.setArrayLayers( 1 )
			.setExtent( { dimensions.x, dimensions.y, 1 } )	
			.setFormat( vk::Format::eR8G8B8A8Srgb )
			.setImageType( vk::ImageType::e2D )	
			.setMipLevels( 1 )
			.setUsage(
				vk::ImageUsageFlagBits::eColorAttachment |
				vk::ImageUsageFlagBits::eTransferSrc |
				vk::ImageUsageFlagBits::eTransferDst |
				vk::ImageUsageFlagBits::eSampled ),
		vma::AllocationCreateInfo()
			.setUsage( vma::MemoryUsage::eGpuOnly )
	);

	if ( !WEAK_ASSERT( create_image_result.result == vk::Result::eSuccess, "Failed to create render target: {}", vk::to_string( create_image_result.result ) ) )
		return nullptr;

	m_vmaAllocator.setAllocationName( create_image_result.value.second, "Render Target" );

	render_target->m_deleteQueue.Add< Deleter< vk::Image, vma::Allocation > >( m_vmaAllocator, create_image_result.value.first, create_image_result.value.second );
	render_target->m_image = create_image_result.value.first;

	const vk::ResultValue< vk::ImageView > create_image_view = m_vkDevice.createImageView( vk::ImageViewCreateInfo()
		.setFormat( vk::Format::eR8G8B8A8Srgb )
		.setImage( render_target->m_image )
		.setSubresourceRange( vk::ImageSubresourceRange()
			.setAspectMask( vk::ImageAspectFlagBits::eColor )
			.setLayerCount( 1 )
			.setLevelCount( 1 ) )
		.setViewType( vk::ImageViewType::e2D ) );

	if ( !WEAK_ASSERT( create_image_view.result == vk::Result::eSuccess, "Failed to create render target view: {}", vk::to_string( create_image_view.result ) ) )
		return nullptr;

	render_target->m_imageView = create_image_view.value;
	render_target->m_deleteQueue.Add< Deleter< vk::ImageView > >( m_vkDevice, render_target->m_imageView );

	const vk::ResultValue< vk::Sampler > create_sampler_result = m_vkDevice.createSampler(
		vk::SamplerCreateInfo()
			.setAddressModeU( vk::SamplerAddressMode::eRepeat )
			.setAddressModeV( vk::SamplerAddressMode::eRepeat )
			.setAddressModeW( vk::SamplerAddressMode::eClampToEdge )
			.setMagFilter( vk::Filter::eLinear )
			.setMinFilter( vk::Filter::eLinear )
			.setMipmapMode( vk::SamplerMipmapMode::eNearest )
			.setMaxLod( vk::LodClampNone )
			.setMinLod( 0.f )
	);

	if ( !WEAK_ASSERT( create_sampler_result.result == vk::Result::eSuccess, "Failed to create sampler: {}", vk::to_string( create_sampler_result.result ) ) )
		return nullptr;

	render_target->m_deleteQueue.Add< Deleter< vk::Sampler > >( m_vkDevice, create_sampler_result.value );
	render_target->m_sampler = create_sampler_result.value;

	return render_target;
}

std::shared_ptr< ITextureResource > VulkanGraphicsContext::CreateTextureResource( const TextureAsset& asset )
{
	ZoneScoped;

	std::shared_ptr< TextureResource > texture = std::make_shared< TextureResource >();

	const glm::uvec2 dimensions = asset.GetDimensions();
	const u32 required_mip_levels = std::max< u32 >( (u32)std::ceil( std::log2( std::max( dimensions.x, dimensions.y ) ) ), 1 );
	const vk::Filter filter = asset.m_filterMode == ImageFilterMode::Pixel ? vk::Filter::eNearest : vk::Filter::eLinear;

	// create resources
	{
		constexpr vk::Format format = vk::Format::eR8G8B8A8Srgb;
		const vk::ResultValue< std::pair< vk::Image, vma::Allocation > > create_image_result = m_vmaAllocator.createImage(
			vk::ImageCreateInfo()
				.setArrayLayers( 1 )
				.setExtent( vk::Extent3D( dimensions.x, dimensions.y, 1 ) )
				.setFormat( format )
				.setImageType( vk::ImageType::e2D )
				.setMipLevels( required_mip_levels )
				.setSamples( vk::SampleCountFlagBits::e1 )
				.setUsage(
					vk::ImageUsageFlagBits::eTransferSrc |
					vk::ImageUsageFlagBits::eTransferDst |
					vk::ImageUsageFlagBits::eSampled
				),
			vma::AllocationCreateInfo()
				.setUsage( vma::MemoryUsage::eGpuOnly )
		);

		if ( !WEAK_ASSERT( create_image_result.result == vk::Result::eSuccess, "Failed to create image: {}", vk::to_string( create_image_result.result ) ) )
			return nullptr;

		m_vmaAllocator.setAllocationName( create_image_result.value.second, std::format( "Texture Resource: {}", asset.m_path ).c_str() );
		texture->m_deleteQueue.Add< Deleter< vk::Image, vma::Allocation > >( m_vmaAllocator, create_image_result.value.first, create_image_result.value.second );
		texture->m_image = create_image_result.value.first;

		const vk::ResultValue< vk::ImageView > create_image_view_result = m_vkDevice.createImageView(
			vk::ImageViewCreateInfo()
				.setFormat( format )
				.setImage( texture->m_image )
				.setViewType( vk::ImageViewType::e2D )
				.setSubresourceRange(
					vk::ImageSubresourceRange()
						.setAspectMask( vk::ImageAspectFlagBits::eColor )
						.setLayerCount( 1 )
						.setLevelCount( required_mip_levels )
				)
		);

		if ( !WEAK_ASSERT( create_image_view_result.result == vk::Result::eSuccess, "Failed to create image view: {}", vk::to_string( create_image_view_result.result ) ) )
			return nullptr;

		texture->m_deleteQueue.Add< Deleter< vk::ImageView > >( m_vkDevice, create_image_view_result.value );
		texture->m_imageView = create_image_view_result.value;

		const vk::ResultValue< vk::Sampler > create_sampler_result = m_vkDevice.createSampler(
			vk::SamplerCreateInfo()
			.setAddressModeU( vk::SamplerAddressMode::eRepeat )
			.setAddressModeV( vk::SamplerAddressMode::eRepeat )
			.setAddressModeW( vk::SamplerAddressMode::eClampToEdge )
			.setMagFilter( filter )
			.setMinFilter( filter )
			.setMipmapMode( asset.m_filterMode == ImageFilterMode::Pixel ? vk::SamplerMipmapMode::eNearest : vk::SamplerMipmapMode::eLinear )
			.setMaxLod( vk::LodClampNone )
			.setMinLod( 0.f )
		);

		if ( !WEAK_ASSERT( create_sampler_result.result == vk::Result::eSuccess, "Failed to create sampler: {}", vk::to_string( create_sampler_result.result ) ) )
			return nullptr;

		texture->m_deleteQueue.Add< Deleter< vk::Sampler > >( m_vkDevice, create_sampler_result.value );
		texture->m_sampler = create_sampler_result.value;
	}

	// populate the image
	{
		const u32 image_data_size = dimensions.x * dimensions.y * sizeof( TextureAsset::Pixel );
		TransientCommand tc = BeginTransientCommand( image_data_size );

		if ( vk::ResultValue< void* > mapped_memory = m_vmaAllocator.mapMemory( tc.stagingBuffer->allocation );
			WEAK_ASSERT( mapped_memory.result == vk::Result::eSuccess && mapped_memory.value != nullptr,
				"Failed to map staging buffer memory: {}, {}", vk::to_string( mapped_memory.result ), mapped_memory.value )
		) {
			std::memcpy( mapped_memory.value, asset.GetPixels(), dimensions.x * dimensions.y * sizeof( TextureAsset::Pixel ) );
			m_vmaAllocator.unmapMemory( tc.stagingBuffer->allocation );

			tc.cmd.pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlagBits::eByRegion,
				{}, {}, vk::ImageMemoryBarrier()
					.setImage( texture->m_image )
					.setDstAccessMask( vk::AccessFlagBits::eTransferWrite )
					.setNewLayout( vk::ImageLayout::eTransferDstOptimal )
					.setSubresourceRange( vk::ImageSubresourceRange()
						.setAspectMask( vk::ImageAspectFlagBits::eColor )
						.setLayerCount( 1 )
						.setLevelCount( 1 )
					)
			);

			tc.cmd.copyBufferToImage( tc.stagingBuffer->buffer, texture->m_image, vk::ImageLayout::eTransferDstOptimal,
				vk::BufferImageCopy()
					.setBufferRowLength( dimensions.x )
					.setBufferImageHeight( dimensions.y )
					.setImageExtent( vk::Extent3D( dimensions.x, dimensions.y, 1 ) )
					.setImageSubresource( vk::ImageSubresourceLayers()
						.setAspectMask( vk::ImageAspectFlagBits::eColor )
						.setLayerCount( 1 ) )
			);

			if ( required_mip_levels == 1 )
			{
				tc.cmd.pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eBottomOfPipe,
					vk::DependencyFlagBits::eByRegion,
					{}, {}, {
						vk::ImageMemoryBarrier()
							.setImage( texture->m_image )
							.setSrcAccessMask( vk::AccessFlagBits::eTransferWrite )
							.setOldLayout( vk::ImageLayout::eTransferDstOptimal )
							.setNewLayout( vk::ImageLayout::eShaderReadOnlyOptimal )
							.setSubresourceRange( vk::ImageSubresourceRange()
								.setAspectMask( vk::ImageAspectFlagBits::eColor )
								.setLayerCount( 1 )
								.setLevelCount( required_mip_levels )
							),
					}
				);
			}
			else
			{
				glm::uvec2 src_dimensions = dimensions;

				for ( u32 source_mip_level = 0; source_mip_level < required_mip_levels - 1; ++source_mip_level )
				{
					glm::uvec2 dst_dimensions( src_dimensions.x / 2, src_dimensions.y / 2 );

					tc.cmd.pipelineBarrier(
						vk::PipelineStageFlagBits::eTransfer,
						vk::PipelineStageFlagBits::eTransfer,
						vk::DependencyFlagBits::eByRegion,
						{}, {}, {
							vk::ImageMemoryBarrier()
								.setImage( texture->m_image )
								.setSrcAccessMask( vk::AccessFlagBits::eTransferWrite )
								.setOldLayout( vk::ImageLayout::eTransferDstOptimal )
								.setDstAccessMask( vk::AccessFlagBits::eTransferRead )
								.setNewLayout( vk::ImageLayout::eTransferSrcOptimal )
								.setSubresourceRange( vk::ImageSubresourceRange()
									.setAspectMask( vk::ImageAspectFlagBits::eColor )
									.setLayerCount( 1 )
									.setBaseMipLevel( source_mip_level )
									.setLevelCount( 1 )
								),
							vk::ImageMemoryBarrier()
								.setImage( texture->m_image )
								.setDstAccessMask( vk::AccessFlagBits::eTransferWrite )
								.setNewLayout( vk::ImageLayout::eTransferDstOptimal )
								.setSubresourceRange( vk::ImageSubresourceRange()
									.setAspectMask( vk::ImageAspectFlagBits::eColor )
									.setLayerCount( 1 )
									.setBaseMipLevel( source_mip_level + 1 )
									.setLevelCount( 1 )
								),
						}
					);

					tc.cmd.blitImage(
						texture->m_image, vk::ImageLayout::eTransferSrcOptimal,
						texture->m_image, vk::ImageLayout::eTransferDstOptimal,
						vk::ImageBlit()
							.setSrcSubresource( vk::ImageSubresourceLayers()
								.setAspectMask( vk::ImageAspectFlagBits::eColor )
								.setLayerCount( 1 )
								.setMipLevel( source_mip_level )
							)
							.setSrcOffsets( { vk::Offset3D(), vk::Offset3D( src_dimensions.x, src_dimensions.y, 1 ) } )
							.setDstSubresource( vk::ImageSubresourceLayers()
								.setAspectMask( vk::ImageAspectFlagBits::eColor )
								.setLayerCount( 1 )
								.setMipLevel( source_mip_level + 1 )
							)
							.setDstOffsets( { vk::Offset3D(), vk::Offset3D( dst_dimensions.x, dst_dimensions.y, 1 ) } ),
						vk::Filter::eLinear
					);

					src_dimensions = dst_dimensions;
				}

				tc.cmd.pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer,
					vk::PipelineStageFlagBits::eBottomOfPipe,
					vk::DependencyFlagBits::eByRegion,
					{}, {}, {
						vk::ImageMemoryBarrier()
							.setImage( texture->m_image )
							.setSrcAccessMask( vk::AccessFlagBits::eTransferRead )
							.setOldLayout( vk::ImageLayout::eTransferSrcOptimal )
							.setNewLayout( vk::ImageLayout::eShaderReadOnlyOptimal )
							.setSubresourceRange( vk::ImageSubresourceRange()
								.setAspectMask( vk::ImageAspectFlagBits::eColor )
								.setLayerCount( 1 )
								.setLevelCount( required_mip_levels - 1 )
							),
						vk::ImageMemoryBarrier()
							.setImage( texture->m_image )
							.setSrcAccessMask( vk::AccessFlagBits::eTransferWrite )
							.setOldLayout( vk::ImageLayout::eTransferDstOptimal )
							.setNewLayout( vk::ImageLayout::eShaderReadOnlyOptimal )
							.setSubresourceRange( vk::ImageSubresourceRange()
								.setAspectMask( vk::ImageAspectFlagBits::eColor )
								.setLayerCount( 1 )
								.setBaseMipLevel( required_mip_levels - 1 )
								.setLevelCount( 1 )
							),
					}
				);
			}
		}

		SubmitTransientCommand( tc );
	}

	return texture;
}

std::shared_ptr< ISpriteRenderer > VulkanGraphicsContext::CreateSpriteRenderer()
{
	return std::make_shared< SpriteRenderer >();
}

void VulkanGraphicsContext::InitSpriteRendererResources()
{
	ZoneScoped;

	if ( m_spriteRenderingResources )
		return;

	SpriteRenderingResources& res = *( m_spriteRenderingResources = std::make_unique< SpriteRenderingResources >() );

	{// create descriptor set layout
		const vk::DescriptorSetLayoutBinding bindings[] {
			vk::DescriptorSetLayoutBinding()
				.setBinding( 0 )
				.setDescriptorCount( 1 )
				.setDescriptorType( vk::DescriptorType::eStorageBuffer )
				.setStageFlags( vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment ),
			vk::DescriptorSetLayoutBinding()
				.setBinding( 1 )
				.setDescriptorCount( SpriteRenderer::c_maxTextures )
				.setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
				.setStageFlags( vk::ShaderStageFlagBits::eFragment ),
		};

		// indicate that the texture array may not be fully bound at render time
		const vk::DescriptorBindingFlags flags[] {
			{},
			vk::DescriptorBindingFlagBits::ePartiallyBound
		};

		const auto binding_flags = vk::DescriptorSetLayoutBindingFlagsCreateInfo()
			.setBindingFlags( flags );

		const vk::ResultValue< vk::DescriptorSetLayout > create_desc_set_layout = m_vkDevice.createDescriptorSetLayout( vk::DescriptorSetLayoutCreateInfo()
			.setBindings( bindings )
			.setPNext( &binding_flags ) );

		STRONG_ASSERT( create_desc_set_layout.result == vk::Result::eSuccess, "Failed to create sprite renderer descriptor set layout: {}", vk::to_string( create_desc_set_layout.result ) );

		res.descriptorSetLayout = create_desc_set_layout.value;
		m_shutdownDeleteQueue.Add< Deleter< vk::DescriptorSetLayout > >( m_vkDevice, res.descriptorSetLayout );
	}

	{// create descriptor pool
		const auto pool_sizes = vk::DescriptorPoolSize()
			.setType( vk::DescriptorType::eCombinedImageSampler )
			.setDescriptorCount( SpriteRenderer::c_maxTextures );

		const vk::ResultValue< vk::DescriptorPool > create_desc_pool = m_vkDevice.createDescriptorPool( vk::DescriptorPoolCreateInfo()
			.setFlags( vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet )
			.setMaxSets( 100 )
			.setPoolSizes( pool_sizes )
		);

		STRONG_ASSERT( create_desc_pool.result == vk::Result::eSuccess, "Failed to create sprite renderer descriptor pool: {}", vk::to_string( create_desc_pool.result ) );

		res.descriptorPool = create_desc_pool.value;
		m_shutdownDeleteQueue.Add< Deleter< vk::DescriptorPool > >( m_vkDevice, res.descriptorPool );
	}

	{// create pipline layout
		const auto camera_matrix_pc_range = vk::PushConstantRange()
			.setOffset( 0 )
			.setStageFlags( vk::ShaderStageFlagBits::eVertex )
			.setSize( sizeof( SpriteRenderData::cameraMatrix ) );

		const vk::ResultValue< vk::PipelineLayout > create_pipeline_layout = m_vkDevice.createPipelineLayout( vk::PipelineLayoutCreateInfo()
			.setSetLayouts( res.descriptorSetLayout )
			.setPushConstantRanges( camera_matrix_pc_range ) );

		STRONG_ASSERT( create_pipeline_layout.result == vk::Result::eSuccess, "Failed to create sprite renderer pipeline layout: {}", vk::to_string( create_pipeline_layout.result ) );

		res.pipelineLayout = create_pipeline_layout.value;
		m_shutdownDeleteQueue.Add< Deleter< vk::PipelineLayout > >( m_vkDevice, res.pipelineLayout );
	}

	{// create pipeline
		AssetManager* const core_asset_manager = STRONG_ASSERT( LowLevel::GetCoreAssetManager() );
		std::shared_ptr< ShaderAsset > vertex_shader_asset = STRONG_ASSERT( core_asset_manager->Load< ShaderAsset >( "/shaders/sprite_renderer/vert" ), "SpriteRenderer vertex shader missing" );
		std::shared_ptr< ShaderAsset > fragment_shader_asset = STRONG_ASSERT( core_asset_manager->Load< ShaderAsset >( "/shaders/sprite_renderer/frag" ), "SpriteRenderer fragment shader missing" );

		const vk::ResultValue< vk::ShaderModule > create_vertex_shader_module = m_vkDevice.createShaderModule( vk::ShaderModuleCreateInfo().setCode( vertex_shader_asset->m_byteCode ) );
		STRONG_ASSERT( create_vertex_shader_module.result == vk::Result::eSuccess, "Failed to create sprite renderer vertex shader: {}", vk::to_string( create_vertex_shader_module.result ) );
		m_shutdownDeleteQueue.Add< Deleter< vk::ShaderModule > >( m_vkDevice, create_vertex_shader_module.value );

		const vk::ResultValue< vk::ShaderModule > create_fragment_shader_module = m_vkDevice.createShaderModule( vk::ShaderModuleCreateInfo().setCode( fragment_shader_asset->m_byteCode ) );
		STRONG_ASSERT( create_fragment_shader_module.result == vk::Result::eSuccess, "Failed to create sprite renderer fragment shader: {}", vk::to_string( create_fragment_shader_module.result ) );
		m_shutdownDeleteQueue.Add< Deleter< vk::ShaderModule > >( m_vkDevice, create_fragment_shader_module.value );

		const auto attachment = vk::PipelineColorBlendAttachmentState()
			.setBlendEnable( true )
			.setAlphaBlendOp( vk::BlendOp::eMax )
			.setColorBlendOp( vk::BlendOp::eAdd )
			.setSrcAlphaBlendFactor( vk::BlendFactor::eOne )
			.setDstAlphaBlendFactor( vk::BlendFactor::eOne )
			.setSrcColorBlendFactor( vk::BlendFactor::eSrcAlpha )
			.setDstColorBlendFactor( vk::BlendFactor::eOneMinusSrcAlpha )
			.setColorWriteMask( vk::FlagTraits< vk::ColorComponentFlagBits >::allFlags );
		
		const auto color_blend_state = vk::PipelineColorBlendStateCreateInfo()
			.setAttachments( attachment );

		const auto depth_stencil_state = vk::PipelineDepthStencilStateCreateInfo();

		const vk::DynamicState dynamic_states[] { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		const auto dynamic_state = vk::PipelineDynamicStateCreateInfo().setDynamicStates( dynamic_states );

		const auto input_assembly_state = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology( vk::PrimitiveTopology::eTriangleFan );

		const auto multisample_state = vk::PipelineMultisampleStateCreateInfo();
		const auto rasterization_state = vk::PipelineRasterizationStateCreateInfo()
			.setLineWidth( 1.f )
			.setPolygonMode( vk::PolygonMode::eFill );

		const auto tessellation_state = vk::PipelineTessellationStateCreateInfo();
		const auto vertex_input_state = vk::PipelineVertexInputStateCreateInfo();
		const auto viewport_state = vk::PipelineViewportStateCreateInfo()
			.setScissorCount( 1 )
			.setViewportCount( 1 );

		const auto color_format = vk::Format::eR8G8B8A8Srgb;
		const auto dynamic_rendering_info = vk::PipelineRenderingCreateInfo()
			.setColorAttachmentFormats( color_format );

		const vk::PipelineShaderStageCreateInfo stages[] {
			vk::PipelineShaderStageCreateInfo()
				.setModule( create_vertex_shader_module.value )
				.setPName( "main" )
				.setStage( vk::ShaderStageFlagBits::eVertex ),
			vk::PipelineShaderStageCreateInfo()
				.setModule( create_fragment_shader_module.value )
				.setPName( "main" )
				.setStage( vk::ShaderStageFlagBits::eFragment )	
		};

		const vk::ResultValue< vk::Pipeline > create_pipeline = m_vkDevice.createGraphicsPipeline( nullptr, vk::GraphicsPipelineCreateInfo()
			.setLayout( res.pipelineLayout )
			.setPColorBlendState( &color_blend_state )
			.setPDepthStencilState( &depth_stencil_state )
			.setPDynamicState( &dynamic_state )
			.setPInputAssemblyState( &input_assembly_state )
			.setPMultisampleState( &multisample_state)
			.setPRasterizationState( &rasterization_state )
			.setPTessellationState( &tessellation_state )
			.setPVertexInputState( &vertex_input_state )
			.setPViewportState( &viewport_state )
			.setStages( stages )
			.setRenderPass( nullptr ).setPNext( &dynamic_rendering_info )
		);

		STRONG_ASSERT( create_pipeline.result == vk::Result::eSuccess, "Failed to create sprite renderer pipeline: {}", vk::to_string( create_pipeline.result ) );

		res.pipeline = create_pipeline.value;

		m_shutdownDeleteQueue.Add< Deleter< vk::Pipeline > >( m_vkDevice, res.pipeline );
	}
}

#pragma endregion

#pragma region WindowContext

VulkanGraphicsContext::WindowContext::WindowContext( IWindow& window ) : IWindowContext( window )
{
	ZoneScoped;

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
		frame_context.m_cmd = command_buffers.value[ frame_context_index ];

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
	ZoneScoped;

	WEAK_ASSERT( static_cast< VulkanGraphicsContext& >( LowLevel::GetGraphicsContext() ).m_vkDevice.waitIdle() == vk::Result::eSuccess );

	if ( LowLevel::GetConfig().enableImGui )
		ImGui_ImplVulkan_Shutdown();

	m_resizeDeleteQueue.Execute();

	for ( FrameContext& frame_context : m_frameContexts )
		frame_context.OnFinishFrame();
}

void VulkanGraphicsContext::WindowContext::OnResize()
{
	ZoneScoped;

	m_failedToResize = true;

	if ( m_window->HasClosed() )
		return;

	VulkanGraphicsContext& ctx = static_cast<VulkanGraphicsContext&>( LowLevel::GetGraphicsContext() );

	// vkbootstrap doesn't check for zero width/height surfaces, so do it here manually
	if ( m_surface )
	{
		vk::ResultValue< vk::SurfaceCapabilitiesKHR > capabilities = ctx.m_vkPhysicalDevice.getSurfaceCapabilitiesKHR( m_surface );
		if ( !WEAK_ASSERT( capabilities.result == vk::Result::eSuccess, "Failed to get surface capabilities: {}", vk::to_string( capabilities.result ) ) )
			return;

		if ( std::min( capabilities.value.minImageExtent.width, capabilities.value.minImageExtent.height ) == 0 )
			return;
	}

	const glm::ivec2 window_size = m_window->GetSize();
	if ( window_size.x == 0 || window_size.y == 0 )
		return;

	m_renderTargetSize = window_size;

	WEAK_ASSERT( ctx.m_vkDevice.waitIdle() == vk::Result::eSuccess );
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
				.setUsage(
					vk::ImageUsageFlagBits::eTransferSrc |
					vk::ImageUsageFlagBits::eColorAttachment
				),
			vma::AllocationCreateInfo()
				.setUsage( vma::MemoryUsage::eGpuOnly )
		);

		STRONG_ASSERT( imgui_render_target.result == vk::Result::eSuccess, "Failed to create imgui render target" );
		const auto [imgui_render_target_image, imgui_render_target_allocation] = imgui_render_target.value;
		ctx.m_vmaAllocator.setAllocationName( imgui_render_target_allocation, "Imgui Render Target" );

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
	ZoneScoped;

	SDLWindow& window = static_cast< SDLWindow& >( *m_window );
	VulkanGraphicsContext& ctx = static_cast<VulkanGraphicsContext&>( LowLevel::GetGraphicsContext() );

	VkSurfaceKHR surface;
	STRONG_ASSERT( SDL_Vulkan_CreateSurface( window.m_window, ctx.m_vkInstance, &surface ) );
	m_surface = surface;
	m_resizeDeleteQueue.Add< Deleter< vk::SurfaceKHR > >( ctx.m_vkInstance, m_surface );

	vkb::Result< vkb::Swapchain > swapchain = vkb::SwapchainBuilder( ctx.m_vkPhysicalDevice, ctx.m_vkDevice, surface )
		.set_desired_present_mode( VkPresentModeKHR( vk::PresentModeKHR::eImmediate ) )
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

#pragma endregion

#pragma region FrameContext

void VulkanGraphicsContext::FrameContext::BlitRenderTarget( std::shared_ptr< IRenderTarget >& render_target, glm::uvec2 position, glm::uvec2 size )
{
	ZoneScoped;

	RegisterUsedResource( render_target );
	RenderTarget& rt = static_cast< RenderTarget& >( *render_target );

	rt.PrepareForCompositing( *this );

	m_cmd.blitImage(
		rt.m_image, vk::ImageLayout::eTransferSrcOptimal,
		m_windowContext->m_swapchainImages[ m_swapchainImageIndex ], vk::ImageLayout::eTransferDstOptimal,
		vk::ImageBlit()
			.setSrcOffsets( { vk::Offset3D( 0, 0, 0 ), vk::Offset3D( rt.GetSize().x, rt.GetSize().y, 1 ) } )
			.setDstOffsets( { vk::Offset3D( position.x, position.y, 0 ), vk::Offset3D( size.x, size.y, 1 ) } )
			.setSrcSubresource( vk::ImageSubresourceLayers()
				.setAspectMask( vk::ImageAspectFlagBits::eColor )
				.setLayerCount( 1 ) )
			.setDstSubresource( vk::ImageSubresourceLayers()
				.setAspectMask( vk::ImageAspectFlagBits::eColor )
				.setLayerCount( 1 ) ),
		vk::Filter::eLinear
	);
}

#pragma endregion

#pragma region RenderTarget

void VulkanGraphicsContext::RenderTarget::Clear( IFrameContext& frame_ctx, const glm::vec4& colour )
{
	ZoneScoped;

	FrameContext& ctx = static_cast< FrameContext& >( frame_ctx );
	
	if ( m_layout != vk::ImageLayout::eTransferDstOptimal )
	{
		ctx.m_cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eTransfer,
			vk::DependencyFlagBits::eByRegion,
			{}, {}, vk::ImageMemoryBarrier()
				.setImage( m_image )
				.setOldLayout( m_layout )
				.setNewLayout( vk::ImageLayout::eTransferDstOptimal )
				.setDstAccessMask( vk::AccessFlagBits::eTransferWrite )
				.setSubresourceRange( vk::ImageSubresourceRange()
					.setAspectMask( vk::ImageAspectFlagBits::eColor )
					.setLayerCount( 1 )
					.setLevelCount( 1 ) )
		);

		m_layout = vk::ImageLayout::eTransferDstOptimal;
	}

	ctx.m_cmd.clearColorImage(
		m_image, m_layout,
		vk::ClearColorValue( colour.r, colour.g, colour.b, colour.a ),
		vk::ImageSubresourceRange()
			.setAspectMask( vk::ImageAspectFlagBits::eColor )
			.setLayerCount( 1 )
			.setLevelCount( 1 )
	);
}

void VulkanGraphicsContext::RenderTarget::DoLayoutTransition( FrameContext& ctx, vk::ImageLayout layout, vk::PipelineStageFlags stage, vk::AccessFlags access )
{
	if ( m_layout != layout )
	{
		ctx.m_cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			stage,
			vk::DependencyFlagBits::eByRegion,
			{}, {}, vk::ImageMemoryBarrier()
				.setImage( m_image )
				.setOldLayout( m_layout )
				.setNewLayout( layout )
				.setDstAccessMask( access )
				.setSubresourceRange( vk::ImageSubresourceRange()
					.setAspectMask( vk::ImageAspectFlagBits::eColor )
					.setLayerCount( 1 )
					.setLevelCount( 1 ) )
		);

		m_layout = layout;
	}
}

void VulkanGraphicsContext::RenderTarget::PrepareForRendering( IFrameContext& frame_ctx )
{
	FrameContext& ctx = static_cast< FrameContext& >( frame_ctx );

	DoLayoutTransition( ctx,
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::AccessFlagBits::eColorAttachmentWrite
	);
}

void VulkanGraphicsContext::RenderTarget::PrepareForCompositing( IFrameContext& frame_ctx )
{
	FrameContext& ctx = static_cast< FrameContext& >( frame_ctx );

	DoLayoutTransition( ctx,
		vk::ImageLayout::eTransferSrcOptimal,
		vk::PipelineStageFlagBits::eTransfer,
		vk::AccessFlagBits::eTransferRead
	);
}

void VulkanGraphicsContext::RenderTarget::PrepareForSampling( IFrameContext& frame_ctx )
{
	FrameContext& ctx = static_cast<FrameContext&>( frame_ctx );

	DoLayoutTransition( ctx,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::PipelineStageFlagBits::eAllGraphics,
		vk::AccessFlagBits::eShaderRead
	);
}

ImTextureID VulkanGraphicsContext::RenderTarget::GetImTextureID()
{
	if ( !m_imTextureId )
	{
		m_imTextureId = ImGui_ImplVulkan_AddTexture( m_sampler, m_imageView, (VkImageLayout)vk::ImageLayout::eShaderReadOnlyOptimal );

		if ( WEAK_ASSERT( m_imTextureId ) )
			m_deleteQueue.Add< Deleter< ImTextureID > >( m_imTextureId );
	}

	return m_imTextureId;
}

#pragma endregion

#pragma region TextureResource

ImTextureID VulkanGraphicsContext::TextureResource::GetImTextureID()
{
	if ( !m_imTextureId )
	{
		m_imTextureId = ImGui_ImplVulkan_AddTexture( m_sampler, m_imageView, (VkImageLayout)vk::ImageLayout::eShaderReadOnlyOptimal );

		if ( WEAK_ASSERT( m_imTextureId ) )
			m_deleteQueue.Add< Deleter< ImTextureID > >( m_imTextureId );
	}

	return m_imTextureId;
}

#pragma endregion

#pragma region SpriteRenderer

std::shared_ptr< VulkanGraphicsContext::SpriteRenderer::PerFrameData > VulkanGraphicsContext::SpriteRenderer::CreatePerFrameData( VulkanGraphicsContext& ctx, u32 required_transform_buffer_size )
{
	ZoneScoped;

	const vk::ResultValue< std::pair< vk::Buffer, vma::Allocation > > create_buffer_result = ctx.m_vmaAllocator.createBuffer(
		vk::BufferCreateInfo()
			.setSize( required_transform_buffer_size )
			.setUsage( vk::BufferUsageFlagBits::eStorageBuffer ),
		vma::AllocationCreateInfo()
			.setUsage( vma::MemoryUsage::eCpuToGpu )
	);

	if ( !WEAK_ASSERT( create_buffer_result.result == vk::Result::eSuccess, "Failed to create transform buffer: {}", vk::to_string( create_buffer_result.result ) ) )
		return nullptr;

	std::shared_ptr< PerFrameData > frame_data = std::make_shared< PerFrameData >();

	ctx.m_vmaAllocator.setAllocationName( create_buffer_result.value.second, "Transform Buffer" );
	frame_data->transformBuffer = create_buffer_result.value.first;
	frame_data->transformBufferAllocation = create_buffer_result.value.second;
	frame_data->m_deleteQueue.Add< Deleter< vk::Buffer, vma::Allocation > >( ctx.m_vmaAllocator, frame_data->transformBuffer, frame_data->transformBufferAllocation );

	if ( !ctx.m_spriteRenderingResources )
		ctx.InitSpriteRendererResources();

	const vk::ResultValue< std::vector< vk::DescriptorSet > > alloc_desc_set = ctx.m_vkDevice.allocateDescriptorSets( vk::DescriptorSetAllocateInfo()
		.setDescriptorPool( ctx.m_spriteRenderingResources->descriptorPool )
		.setDescriptorSetCount( 1 )
		.setSetLayouts( ctx.m_spriteRenderingResources->descriptorSetLayout ) );

	STRONG_ASSERT( alloc_desc_set.result == vk::Result::eSuccess, "Failed to create sprite renderer descriptor set: {}", vk::to_string( alloc_desc_set.result ) );
	STRONG_ASSERT( alloc_desc_set.value.size() == 1 && alloc_desc_set.value[ 0 ], "Failed to create sprite renderer descriptor set" );

	frame_data->descriptorSet = alloc_desc_set.value[ 0 ];
	frame_data->m_deleteQueue.Add< Deleter< vk::DescriptorSet > >( ctx.m_vkDevice, ctx.m_spriteRenderingResources->descriptorPool, frame_data->descriptorSet );

	return frame_data;
}

void VulkanGraphicsContext::SpriteRenderer::Render( IFrameContext& frame_context, std::shared_ptr< IRenderTarget >& render_target, const SpriteRenderData& data )
{
	ZoneScoped;

	RenderTarget& rt = static_cast< RenderTarget& >( *render_target );
	FrameContext& frame = static_cast< FrameContext& >( frame_context );
	VulkanGraphicsContext& ctx = static_cast< VulkanGraphicsContext& >( LowLevel::GetGraphicsContext() );

	// get or create a frame data set
	auto& [ _, frame_data ] = *m_perFrameData.insert( { &frame_context, {} } ).first;

	std::vector< SpriteRenderData::SpriteInstance > all_sprites;
	{
		ZoneScopedN( "Flattening Sprite Layers" );

		for ( auto& layer : data.spriteInstances )
			all_sprites.insert( all_sprites.end(), layer.begin(), layer.end() );
	}

	// stop here if there's nothing to render
	const u32 required_transform_buffer_size = u32( all_sprites.size() * sizeof( all_sprites[ 0 ] ) );
	if ( required_transform_buffer_size == 0 )
		return;

	// check if we need to rebuild our frame data
	if ( frame_data )
	{
		const vma::AllocationInfo alloc_info = ctx.m_vmaAllocator.getAllocationInfo( frame_data->transformBufferAllocation );
		if ( alloc_info.size < required_transform_buffer_size )
			frame_data.reset();
	}
	if ( !frame_data )
		frame_data = CreatePerFrameData( ctx, required_transform_buffer_size );

	{
		ZoneScopedN( "Update Transform Buffer" );

		const vk::ResultValue< void* > mapped_memory = ctx.m_vmaAllocator.mapMemory( frame_data->transformBufferAllocation );
		if ( !WEAK_ASSERT( mapped_memory.result == vk::Result::eSuccess && mapped_memory.value, "Failed to map transform buffer: {}", vk::to_string( mapped_memory.result ) ) )
			return;

		std::memcpy( mapped_memory.value, all_sprites.data(), required_transform_buffer_size );
		ctx.m_vmaAllocator.unmapMemory( frame_data->transformBufferAllocation );
	}

	{
		ZoneScopedN( "Update Descriptors" );

		std::vector< vk::WriteDescriptorSet > descriptor_writes;
		std::vector< vk::DescriptorImageInfo > image_infos;
		descriptor_writes.reserve( c_maxTextures );
		image_infos.reserve( c_maxTextures );

		for ( u32 idx = 0; idx < data.textures.size(); ++idx )
		{
			const std::shared_ptr< ITextureResource >& texture_resource = data.textures[ idx ];
			frame.RegisterUsedResource( texture_resource );

			TextureResource& texture = static_cast<TextureResource&>( *texture_resource );

			image_infos.push_back( vk::DescriptorImageInfo()
				.setSampler( texture.m_sampler )
				.setImageView( texture.m_imageView )
				.setImageLayout( vk::ImageLayout::eShaderReadOnlyOptimal ) );

			descriptor_writes.push_back( vk::WriteDescriptorSet()
				.setDstSet( frame_data->descriptorSet )
				.setDescriptorType( vk::DescriptorType::eCombinedImageSampler )
				.setDstBinding( 1 )
				.setDescriptorCount( 1 )
				.setDstArrayElement( idx )
				.setImageInfo( image_infos.back() )
			);
		}

		const auto buffer_info = vk::DescriptorBufferInfo()
			.setBuffer( frame_data->transformBuffer )
			.setRange( required_transform_buffer_size );

		descriptor_writes.push_back( vk::WriteDescriptorSet()
			.setDstSet( frame_data->descriptorSet )
			.setDescriptorType( vk::DescriptorType::eStorageBuffer )
			.setDstBinding( 0 )
			.setDescriptorCount( 1 )
			.setBufferInfo( buffer_info ) );

		ctx.m_vkDevice.updateDescriptorSets( descriptor_writes, {} );
	}

	{
		ZoneScopedN( "Record command buffer" );

		frame_context.RegisterUsedResource( frame_data );
		frame_context.RegisterUsedResource( render_target );
		rt.PrepareForRendering( frame_context );

		const auto color_attachment = vk::RenderingAttachmentInfo()
			.setImageLayout( vk::ImageLayout::eColorAttachmentOptimal )
			.setImageView( rt.m_imageView )
			.setLoadOp( vk::AttachmentLoadOp::eLoad )
			.setStoreOp( vk::AttachmentStoreOp::eStore );

		const vk::Rect2D rt_rect( {}, { render_target->GetSize().x, render_target->GetSize().y } );

		frame.m_cmd.beginRendering( vk::RenderingInfo()
			.setColorAttachments( color_attachment )
			.setLayerCount( 1 )
			.setRenderArea( rt_rect ) );

		frame.m_cmd.bindPipeline( vk::PipelineBindPoint::eGraphics, ctx.m_spriteRenderingResources->pipeline );
		frame.m_cmd.bindDescriptorSets( vk::PipelineBindPoint::eGraphics, ctx.m_spriteRenderingResources->pipelineLayout, 0, frame_data->descriptorSet, {} );
		frame.m_cmd.pushConstants< glm::mat3x4 >( ctx.m_spriteRenderingResources->pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, data.cameraMatrix );

		frame.m_cmd.setScissor( 0, rt_rect );
		frame.m_cmd.setViewport( 0, vk::Viewport( 0, 0, (f32)render_target->GetSize().x, (f32)render_target->GetSize().y, 0.f, 1.f ) );
		frame.m_cmd.draw( 4, (u32)all_sprites.size(), 0, 0 );

		frame.m_cmd.endRendering();
	}
}

#pragma endregion

}
