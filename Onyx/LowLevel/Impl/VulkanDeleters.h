#pragma once
#include "VulkanGraphicsContext.h"
#include "imgui_impl_vulkan.h"

#include "tracy/Tracy.hpp"

namespace onyx
{

template<>
struct VulkanGraphicsContext::Deleter< vk::Buffer, vma::Allocation > : DeleteQueue::IDeleter
{
	vma::Allocator m_allocator;
	vk::Buffer m_buffer;
	vma::Allocation m_allocation;

	Deleter( vma::Allocator allocator, vk::Buffer buffer, vma::Allocation allocation ) : m_allocator( allocator ), m_buffer( buffer ), m_allocation( allocation ) {}
	void Execute() override { ZoneScoped; m_allocator.destroyBuffer( m_buffer, m_allocation ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::CommandPool > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::CommandPool m_commandPool;

	Deleter( vk::Device device, vk::CommandPool command_pool ) : m_device( device ), m_commandPool( command_pool ) {}
	void Execute() override { ZoneScoped; m_device.destroyCommandPool( m_commandPool ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::DescriptorPool > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::DescriptorPool m_descriptorPool;

	Deleter( vk::Device device, vk::DescriptorPool descriptor_pool ) : m_device( device ), m_descriptorPool( descriptor_pool ) {}
	void Execute() override { ZoneScoped; m_device.destroyDescriptorPool( m_descriptorPool ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::DescriptorSet > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::DescriptorPool m_descriptorPool;
	vk::DescriptorSet m_descriptorSet;

	Deleter( vk::Device device, vk::DescriptorPool descriptor_pool, vk::DescriptorSet descriptor_set )
		: m_device( device ), m_descriptorPool( descriptor_pool ), m_descriptorSet( descriptor_set ) {}

	void Execute() override { ZoneScoped; m_device.freeDescriptorSets( m_descriptorPool, m_descriptorSet ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::DescriptorSetLayout > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::DescriptorSetLayout m_descriptorSetLayout;

	Deleter( vk::Device device, vk::DescriptorSetLayout descriptor_set_layout ) : m_device( device ), m_descriptorSetLayout( descriptor_set_layout ) {}
	void Execute() override { ZoneScoped; m_device.destroyDescriptorSetLayout( m_descriptorSetLayout ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::Device > : DeleteQueue::IDeleter
{
	vk::Device m_device;

	Deleter( vk::Device device ) : m_device( device ) {}
	void Execute() override { ZoneScoped; m_device.destroy(); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::Fence > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::Fence m_fence;
	Deleter( vk::Device device, vk::Fence fence ) : m_device( device ), m_fence( fence ) {}
	void Execute() override { ZoneScoped; m_device.destroyFence( m_fence ); }
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

	void Execute() override { ZoneScoped; m_allocator.destroyImage( m_image, m_allocation ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::ImageView > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::ImageView m_imageView;

	Deleter( vk::Device device, vk::ImageView image_view ) : m_device( device ), m_imageView( image_view ) {}
	void Execute() override { ZoneScoped; m_device.destroyImageView( m_imageView ); }
};

template<>
struct VulkanGraphicsContext::Deleter< ImTextureID > : DeleteQueue::IDeleter
{
	ImTextureID m_imTextureId;

	Deleter( ImTextureID im_texture_id ) : m_imTextureId( im_texture_id ) {}
	void Execute() override { ZoneScoped; ImGui_ImplVulkan_RemoveTexture( reinterpret_cast< VkDescriptorSet >( m_imTextureId ) ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::Instance > : DeleteQueue::IDeleter
{
	vk::Instance m_instance;
	vk::DebugUtilsMessengerEXT m_debugMessenger;

	Deleter( vk::Instance instance, vk::DebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE ) : m_instance( instance ), m_debugMessenger( debug_messenger ) {}
	void Execute() override
	{
		ZoneScoped;

		if ( m_debugMessenger )
			m_instance.destroyDebugUtilsMessengerEXT( m_debugMessenger );

		m_instance.destroy();
	}
};

template<>
struct VulkanGraphicsContext::Deleter< vk::PipelineLayout > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::PipelineLayout m_pipelineLayout;

	Deleter( vk::Device device, vk::PipelineLayout pipeline_layout ) : m_device( device ), m_pipelineLayout( pipeline_layout ) {}
	void Execute() override { ZoneScoped; m_device.destroyPipelineLayout( m_pipelineLayout ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::Pipeline > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::Pipeline m_pipeline;

	Deleter( vk::Device device, vk::Pipeline pipeline ) : m_device( device ), m_pipeline( pipeline ) {}
	void Execute() override { ZoneScoped; m_device.destroyPipeline( m_pipeline ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::Sampler > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::Sampler m_sampler;

	Deleter( vk::Device device, vk::Sampler sampler ) : m_device( device ), m_sampler( sampler ) {}
	void Execute() override { ZoneScoped; m_device.destroySampler( m_sampler ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::Semaphore > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::Semaphore m_semaphore;
	Deleter( vk::Device device, vk::Semaphore semaphore) : m_device( device ), m_semaphore( semaphore ) {}
	void Execute() override { ZoneScoped; m_device.destroySemaphore( m_semaphore ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::ShaderModule > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::ShaderModule m_shaderModule;
	Deleter( vk::Device device, vk::ShaderModule shaderModule ) : m_device( device ), m_shaderModule( shaderModule ) {}
	void Execute() override { ZoneScoped; m_device.destroyShaderModule( m_shaderModule ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::SurfaceKHR > : DeleteQueue::IDeleter
{
	vk::Instance m_instance;
	vk::SurfaceKHR m_surface;

	Deleter( vk::Instance instance, vk::SurfaceKHR surface ) : m_instance( instance ), m_surface( surface ) {}
	void Execute() override { ZoneScoped; m_instance.destroySurfaceKHR( m_surface ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vk::SwapchainKHR > : DeleteQueue::IDeleter
{
	vk::Device m_device;
	vk::SwapchainKHR m_swapchain;

	Deleter( vk::Device device, vk::SwapchainKHR swapchain ) : m_device( device ), m_swapchain( swapchain ) {}
	void Execute() override { ZoneScoped; m_device.destroySwapchainKHR( m_swapchain ); }
};

template<>
struct VulkanGraphicsContext::Deleter< vma::Allocator > : DeleteQueue::IDeleter
{
	vma::Allocator m_allocator;

	Deleter( vma::Allocator allocator ) : m_allocator( allocator ) {}
	void Execute() override { ZoneScoped; m_allocator.destroy(); }
};

}
