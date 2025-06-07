#pragma once
#include "VulkanGraphicsContext.h"

namespace onyx
{

template<>
struct VulkanGraphicsContext::Deleter< vk::CommandPool > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::CommandPool m_commandPool;

	Deleter( vk::Device device, vk::CommandPool command_pool ) : m_device( device ), m_commandPool( command_pool ) {}
	void Execute() override { m_device.destroyCommandPool( m_commandPool ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::DescriptorPool > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::DescriptorPool m_descriptorPool;

	Deleter( vk::Device device, vk::DescriptorPool descriptor_pool ) : m_device( device ), m_descriptorPool( descriptor_pool ) {}
	void Execute() override { m_device.destroyDescriptorPool( m_descriptorPool ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::Device > : DeleteQueue::IDeleter
{
	vk::Device m_device;

	Deleter( vk::Device device ) : m_device( device ) {}
	void Execute() override { m_device.destroy(); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::Fence > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::Fence m_fence;
	Deleter( vk::Device device, vk::Fence fence ) : m_device( device ), m_fence( fence ) {}
	void Execute() override { m_device.destroyFence( m_fence ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::Image, vma::Allocation > : DeleteQueue::IDeleter
{
	vma::Allocator m_allocator;
	vk::Image m_image;
	vma::Allocation m_allocation;

	Deleter( vma::Allocator allocator, vk::Image image, vma::Allocation allocation ) :
		m_allocator( allocator ), m_image( image ), m_allocation( allocation )
	{}

	void Execute() override { m_allocator.destroyImage( m_image, m_allocation ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::ImageView > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::ImageView m_imageView;

	Deleter( vk::Device device, vk::ImageView image_view ) : m_device( device ), m_imageView( image_view ) {}
	void Execute() override { m_device.destroyImageView( m_imageView ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::Instance > : DeleteQueue::IDeleter
{
	vk::Instance m_instance;
	vk::DebugUtilsMessengerEXT m_debugMessenger;

	Deleter( vk::Instance instance, vk::DebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE ) : m_instance( instance ), m_debugMessenger( debug_messenger ) {}
	void Execute() override
	{
		if ( m_debugMessenger )
			m_instance.destroyDebugUtilsMessengerEXT( m_debugMessenger );

		m_instance.destroy();
	}
};

template<>
struct VulkanGraphicsContext::Deleter< vk::Semaphore > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::Semaphore m_semaphore;
	Deleter( vk::Device device, vk::Semaphore semaphore) : m_device( device ), m_semaphore( semaphore ) {}
	void Execute() override { m_device.destroySemaphore( m_semaphore ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::SurfaceKHR > : DeleteQueue::IDeleter
{
	vk::Instance m_instance;
	vk::SurfaceKHR m_surface;

	Deleter( vk::Instance instance, vk::SurfaceKHR surface ) : m_instance( instance ), m_surface( surface ) {}
	void Execute() override { m_instance.destroySurfaceKHR( m_surface ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::SwapchainKHR > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::SwapchainKHR m_swapchain;

	Deleter( vk::Device device, vk::SwapchainKHR swapchain ) : m_device( device ), m_swapchain( swapchain ) {}
	void Execute() override { m_device.destroySwapchainKHR( m_swapchain ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vma::Allocator > : DeleteQueue::IDeleter
{
	vma::Allocator m_allocator;

	Deleter( vma::Allocator allocator ) : m_allocator( allocator ) {}
	void Execute() override { m_allocator.destroy(); }
};

}
