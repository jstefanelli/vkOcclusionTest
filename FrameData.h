#ifndef VKOCCLUSIONTEST_FRAMEDATA_H
#define VKOCCLUSIONTEST_FRAMEDATA_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include "Instance.h"
#include "GlobalTypes.h"
#include "Swapchain.h"
#include "Buffer.h"
#include "HZBuffer.h"
#include "Scene.h"
#include "GraphicsPipeline.h"
#include "ComputePipeline.h"
#include "PipelineCollection.h"
#include "Sampler.h"
#include "Texture.h"

class FrameData {
private:
	std::shared_ptr<Instance> instance;
	uint32_t _index;
	vk::CommandBuffer _command_buffer;
	vk::Framebuffer _z_framebuffer;
	vk::Framebuffer _draw_frame_buffer;
	std::unique_ptr<Texture> _draw_color;
	vk::ImageView _draw_color_view;
	std::unique_ptr<Texture> _draw_depth;
	vk::ImageView _draw_depth_view;
	vk::Fence _in_flight_fence;
	vk::Semaphore _render_in_progress_semaphore;

	std::unique_ptr<Buffer> _cameraBuffer;
	std::unique_ptr<Buffer> _instanceBuffer;
	std::unique_ptr<Buffer> _batchesBuffer;
	std::unique_ptr<Buffer> _drawBuffer;
	std::unique_ptr<Buffer> _clearBuffer;
	std::unique_ptr<Buffer> _indirectBuffer;

	std::vector<vk::DescriptorSet> descriptorSets;
	std::unique_ptr<Sampler> linearSampler;
	std::unique_ptr<Sampler> nearestSampler;
	std::unique_ptr<Texture> whiteTexture;
	bool _descriptors_up_to_date;
	HZBuffer _hzBuffer;

	void update_descriptor_sets(const MeshBuffer& meshes);

	void run_z_pass(const vk::CommandBuffer& cmd, PipelineCollection& pipelines, int batches_amount);
	void run_copy_pass(const vk::CommandBuffer& cmd, PipelineCollection& pipelines);
	void run_downsample(const vk::CommandBuffer& cmd, PipelineCollection& pipelines);
	void run_query(const vk::CommandBuffer& cmd, PipelineCollection& pipelines, int objects_amount);
	void draw_final(const vk::CommandBuffer& cmd, PipelineCollection& pipelines, int batches_amount, glm::ivec2 size);
	void update_draw_fb(PipelineCollection& pipelines, glm::ivec2 size);
public:
	FrameData(std::shared_ptr<Instance> instance, int index, const Swapchain& swapchain, glm::ivec2 hzBufferSize,
			  PipelineCollection& pipelines, const vk::Sampler& downsampleSampler);

	FrameData(const FrameData&) = delete;
	FrameData(const FrameData&&) = delete;
	~FrameData();

	void draw(const vk::CommandBuffer& cmd, Scene& s, const UniformData& camera, PipelineCollection& pipelines, const vk::Image& swapchainImg, glm::ivec2 finalSize);

	inline const vk::Framebuffer& z_framebuffer() const {
		return _z_framebuffer;
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
