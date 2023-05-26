#include "Swapchain.h"
#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>

Swapchain::Swapchain(SDL_Window* window, std::shared_ptr<Instance> instance) : instance(instance) {
	if(!SDL_Vulkan_CreateSurface(window, static_cast<VkInstance>(instance->instance()), reinterpret_cast<VkSurfaceKHR*>(&_surface))) {
		throw std::runtime_error("Failed to create Vulkan surface");
	}
}

bool Swapchain::create_swapchain(glm::ivec2 sz) {
	if (sz == _size) {
		return true;
	}

	destroy_swapchain();

	auto surfaceCapabilities = instance->physical_device().getSurfaceCapabilitiesKHR(_surface);
	auto surfaceFormats = instance->physical_device().getSurfaceFormatsKHR(_surface);

	int surfaceFormatIdx = -1;
	for(int i = 0; i < surfaceFormats.size(); i++) {
		if (surfaceFormats[i].format == vk::Format::eB8G8R8A8Unorm) {
			surfaceFormatIdx = i;
		}
	}

	if (surfaceFormatIdx < 0) {
		return false;
	}

	_surface_format = surfaceFormats[surfaceFormatIdx];

	sz.x = glm::clamp(sz.x, (int) surfaceCapabilities.minImageExtent.width, (int) surfaceCapabilities.maxImageExtent.width);
	sz.y = glm::clamp(sz.y, (int) surfaceCapabilities.minImageExtent.height, (int) surfaceCapabilities.maxImageExtent.height);

	vk::SwapchainCreateInfoKHR swapchainCreateInfo({}, _surface, surfaceCapabilities.minImageCount, _surface_format.format, _surface_format.colorSpace,
												   vk::Extent2D(sz.x, sz.y), 1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eTransferDst,
												   {}, nullptr, surfaceCapabilities.currentTransform,
												   vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::PresentModeKHR::eFifo, true);

	std::vector<uint32_t> queueFamilyIndices { (uint32_t) instance->graphics_queue_index() };
	if (instance->graphics_queue_index() != instance->present_queue_index()) {
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
	} else {
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
		queueFamilyIndices.push_back((uint32_t) instance->present_queue_index());
	}
	swapchainCreateInfo.setQueueFamilyIndices(queueFamilyIndices);

	_swapchain = instance->device().createSwapchainKHR(swapchainCreateInfo);
	auto swapchainImages = instance->device().getSwapchainImagesKHR(_swapchain);

	std::vector<vk::ImageView> imageViews;

	for(const auto& img : swapchainImages) {
		vk::ImageViewCreateInfo imgViewCreateInfo({}, img, vk::ImageViewType::e2D, _surface_format.format, {}, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

		imageViews.push_back(instance->device().createImageView(imgViewCreateInfo));
	}

	_image_views = imageViews;
	_size = sz;
	return true;
}

bool Swapchain::destroy_swapchain() {
	if (!_swapchain) {
		return true;
	}

	instance->wait_idle();

	for(auto& img : _image_views) {
		instance->device().destroyImageView(img);
	}
	_image_views.clear();

	instance->device().destroySwapchainKHR(_swapchain);

	return true;
}

Swapchain::~Swapchain() {
	destroy_swapchain();
	instance->instance().destroySurfaceKHR(_surface);
}
