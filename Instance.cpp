#include "Instance.h"
#include <SDL2/SDL_vulkan.h>
#include <stdexcept>
#include "Swapchain.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

Instance::Instance(SDL_Window* window) {

	uint32_t extension_count = 0;
	if(!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr)) {
		throw std::runtime_error("Failed to get required Vulkan _instance extensions count for SDL window");
	}

	std::vector<const char*> extensions(extension_count);
	if(!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data())) {
		throw std::runtime_error("Failed to get required Vulkan _instance extensions for SDL window");
	}

	vk::DynamicLoader dl;
	auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	std::vector<const char*> validationLayers { "VK_LAYER_KHRONOS_validation" };

	vk::ApplicationInfo appInfo("vkOcclusionTest", VK_MAKE_VERSION(0, 1, 0), "vkExperiment1", VK_MAKE_VERSION(0, 1, 0), VK_API_VERSION_1_3);
	vk::InstanceCreateInfo instanceInfo({}, &appInfo, validationLayers, extensions);

	_instance = vk::createInstance(instanceInfo, nullptr);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(_instance);

	auto physical_devices = _instance.enumeratePhysicalDevices();

	if (physical_devices.empty()) {
		throw std::runtime_error("No Vulkan physical devices found");
	}

	vk::PhysicalDevice discrete_device;
	vk::PhysicalDevice integrated_device;

	for(const auto& pd : physical_devices) {
		auto props = pd.getProperties();



		if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
			discrete_device = pd;
			break;
		}

		if (props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
			integrated_device = pd;
			break;
		}
	}

	if (discrete_device) {
		_physical_device = discrete_device;
	} else {
		_physical_device = integrated_device;
	}

	if (!_physical_device) {
		throw std::runtime_error("No Discrete GPU Found");
	}
}


void Instance::create_device(const Swapchain &swapchain) {
	auto queue_properties = _physical_device.getQueueFamilyProperties();

	_graphics_index = -1;
	_compute_index = 1;
	_present_index = -1;

	int queueFamilyIndex = 0;
	for(const auto& queue_family : queue_properties) {
		if (queue_family.queueCount > 0 && queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
			_graphics_index = queueFamilyIndex;
		}

		if (queue_family.queueCount > 0 && queue_family.queueFlags & vk::QueueFlagBits::eCompute) {
			_compute_index = queueFamilyIndex;
		}

		if (_physical_device.getSurfaceSupportKHR(queueFamilyIndex, swapchain.surface())) {
			_present_index = queueFamilyIndex;
		}

		if (_graphics_index != -1 && _compute_index != -1 && _present_index != -1) {
			break;
		}

		queueFamilyIndex++;
	}

	std::vector<float> queuePriorities { 1.0f };

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos {
			{ {}, (uint32_t) _graphics_index, queuePriorities },
	};

	if (_graphics_index != _present_index) {
		queueCreateInfos.push_back({{}, (uint32_t) _present_index, queuePriorities });
	}

	if (_compute_index != _graphics_index && _compute_index != _present_index) {
		queueCreateInfos.push_back({{}, (uint32_t) _compute_index, queuePriorities });
	}

	std::vector<const char*> validationLayers { "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> deviceExtensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	vk::PhysicalDeviceFeatures2 deviceFeatures;
	deviceFeatures.features.samplerAnisotropy = true;

	vk::DeviceCreateInfo deviceCreateInfo({}, queueCreateInfos, validationLayers, deviceExtensions, nullptr);
	deviceCreateInfo.setPNext(&deviceFeatures);
	_device = _physical_device.createDevice(deviceCreateInfo);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(_device);

	_graphics_queue = _device.getQueue(_graphics_index, 0);
	_present_queue = _device.getQueue(_present_index, 0);
	_compute_queue = _device.getQueue(_compute_index, 0);

	_graphics_command_pool = _device.createCommandPool({ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, (uint32_t) _graphics_index});
	_compute_command_pool = _device.createCommandPool({ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, (uint32_t) _compute_index});

	_memory_properties = _physical_device.getMemoryProperties();

	std::array<vk::DescriptorPoolSize, 4> sizes = {
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 100),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 100),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 100),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 100)
	};

	_descriptor_pool = _device.createDescriptorPool({{}, 1000, sizes});
}

void Instance::wait_idle() const {
	_device.waitIdle();
}

Instance::~Instance() {
	_device.waitIdle();

	_device.destroyDescriptorPool(_descriptor_pool);
	_device.destroyCommandPool(_graphics_command_pool);
	_device.destroyCommandPool(_compute_command_pool);
	_device.destroy();
	_instance.destroy();
}

std::vector<vk::DescriptorSet> Instance::create_descriptor_sets(const vk::ArrayProxy<vk::DescriptorSetLayout>& info) {
	return _device.allocateDescriptorSets({_descriptor_pool, info});
}
