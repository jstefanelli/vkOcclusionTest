#include "FrameData.h"
#include <array>
#include "Buffer.h"

FrameData::FrameData(std::shared_ptr<Instance> inst, int index, const Swapchain& swapchain, glm::ivec2 hzbSize,
					 const vk::RenderPass& zPass, const vk::DescriptorSetLayout& downsampleLayout, const vk::Sampler& downsampleSampler) :
	instance(std::move(inst)), _index(index), _globals_buffer(instance, sizeof(Push_Constants), vk::BufferUsageFlagBits::eUniformBuffer,
															  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent),
															  _hzBuffer(instance, hzbSize, downsampleLayout, downsampleSampler) {
	_command_buffer = instance->device().allocateCommandBuffers({ instance->graphics_command_pool(), vk::CommandBufferLevel::ePrimary, 1})[0];
	_in_flight_fence = instance->device().createFence({ vk::FenceCreateFlagBits::eSignaled });
	_render_in_progress_semaphore = instance->device().createSemaphore({});

	vk::FramebufferCreateInfo zFBInfo( {}, zPass, 1, &_hzBuffer.depth_view(), _hzBuffer.depth_texture().size().x, _hzBuffer.depth_texture().size().y, 1);
	_z_framebuffer = instance->device().createFramebuffer(zFBInfo);

	update_framebuffer(swapchain);
}

FrameData::~FrameData() {
	if (_in_flight_fence) {
		instance->device().destroyFence(_in_flight_fence);
	}
	if (_render_in_progress_semaphore) {
		instance->device().destroySemaphore(_render_in_progress_semaphore);
	}
	if (_z_framebuffer) {
		instance->device().destroyFramebuffer(_z_framebuffer);
	}
}

void FrameData::update_globals(const Push_Constants &data) const {
	auto map = _globals_buffer.map();
	std::memcpy(static_cast<void*>(map), &data, sizeof(Push_Constants));
}

void FrameData::update_framebuffer(const Swapchain &swapchain) {
	_final_framebuffer = swapchain.framebuffers()[_index];
}
