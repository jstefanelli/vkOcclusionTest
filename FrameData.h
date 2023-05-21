#ifndef VKOCCLUSIONTEST_FRAMEDATA_H
#define VKOCCLUSIONTEST_FRAMEDATA_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include "Instance.h"
#include "GlobalTypes.h"
#include "Swapchain.h"
#include "Buffer.h"
#include "HZBuffer.h"

class FrameData {
private:
	std::shared_ptr<Instance> instance;
	uint32_t _index;
	vk::CommandBuffer _command_buffer;
	vk::Framebuffer _z_framebuffer;
	vk::Framebuffer _final_framebuffer;
	vk::Fence _in_flight_fence;
	vk::Semaphore _render_in_progress_semaphore;
	Buffer _globals_buffer;
	HZBuffer _hzBuffer;
public:
	FrameData(std::shared_ptr<Instance> instance, int index, const Swapchain& swapchain, glm::ivec2 hzBufferSize,
			  const vk::RenderPass& zPass, const vk::DescriptorSetLayout& downsampleLayout, const vk::Sampler& downsampleSampler);

	FrameData(const FrameData&) = delete;
	FrameData(const FrameData&&) = delete;
	~FrameData();

	void update_globals(const Push_Constants& data) const;
	void update_framebuffer(const Swapchain& swapchain);

	inline const vk::Framebuffer& final_framebuffer() const {
		return _final_framebuffer;
	}

	inline const vk::Framebuffer& z_framebuffer() const {
		return _z_framebuffer;
	}

	inline const Buffer& globals_buffer() const {
		return _globals_buffer;
	}

	inline const vk::CommandBuffer& command_buffer() const {
		return _command_buffer;
	}

	inline const vk::Fence& in_flight_fence() const {
		return _in_flight_fence;
	}

	inline const vk::Semaphore& render_in_progress_semaphore() const {
		return _render_in_progress_semaphore;
	}

	inline const HZBuffer& hz_buffer() const {
		return _hzBuffer;
	}

	inline uint32_t index() const {
		return _index;
	}
};


#endif //VKOCCLUSIONTEST_FRAMEDATA_H
