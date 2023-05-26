#include "FrameData.h"
#include <array>
#include "Buffer.h"
#include <cmath>

#define DS_ID_MESH_AND_CAMERA 0
#define DS_ID_INSTANCES_ONLY 1
#define DS_ID_COPY 2
#define DS_ID_CAMERA_ONLY 3
#define DS_ID_INSTANCES_AND_INDIRECTIONS_COMPUTE 4
#define DS_ID_QUERY 5
#define DS_ID_INSTANCES_AND_INDIRECTIONS_GRAPHICS 6
#define DS_ID_MATERIALS_AND_TEXTURES_0 7
#define DS_ID_MATERIALS_AND_TEXTURES_1 8
#define DS_ID_MATERIALS_AND_TEXTURES_2 9

FrameData::FrameData(std::shared_ptr<Instance> inst, int index, const Swapchain& swapchain, glm::ivec2 hzbSize,
					 PipelineCollection& pipelines, const vk::Sampler& downsampleSampler) :
					 instance(std::move(inst)), _index(index),
					 _hzBuffer(instance, hzbSize, pipelines.downsample_pass()->descriptor_set_layouts()[0], downsampleSampler),
					 _descriptors_up_to_date(false) {

	_command_buffer = instance->device().allocateCommandBuffers({ instance->graphics_command_pool(), vk::CommandBufferLevel::ePrimary, 1})[0];
	_in_flight_fence = instance->device().createFence({ vk::FenceCreateFlagBits::eSignaled });
	_render_in_progress_semaphore = instance->device().createSemaphore({});
	_cameraBuffer = std::make_unique<Buffer>(instance, sizeof(UniformData), vk::BufferUsageFlagBits::eUniformBuffer,
											 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	vk::FramebufferCreateInfo zFBInfo( {}, pipelines.z_pass()->render_pass(), 1, &_hzBuffer.depth_view(), _hzBuffer.depth_texture().size().x, _hzBuffer.depth_texture().size().y, 1);
	_z_framebuffer = instance->device().createFramebuffer(zFBInfo);

	std::vector<vk::DescriptorSetLayout> layouts {
		pipelines.z_pass()->descriptor_set_layouts()[0],
		pipelines.z_pass()->descriptor_set_layouts()[1],
		pipelines.copy_pass()->descriptor_set_layouts()[0],
		pipelines.query_pass()->descriptor_set_layouts()[0],
		pipelines.query_pass()->descriptor_set_layouts()[1],
		pipelines.query_pass()->descriptor_set_layouts()[2],
		pipelines.draw_pass()->descriptor_set_layouts()[1],
		pipelines.draw_pass()->descriptor_set_layouts()[2],
		pipelines.draw_pass()->descriptor_set_layouts()[2],
		pipelines.draw_pass()->descriptor_set_layouts()[2],
	};

	descriptorSets = instance->create_descriptor_sets(layouts);

	nearestSampler = std::make_unique<Sampler>(instance, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest);
	linearSampler = std::make_unique<Sampler>(instance, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear);
	whiteTexture = std::make_unique<Texture>(instance, PIPELINE_COLOR_FORMAT, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, glm::ivec2(2, 2), 1);

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

	if (_draw_frame_buffer) {
		instance->device().destroyFramebuffer(_draw_frame_buffer);
	}

	if (_draw_color_view) {
		instance->device().destroyImageView(_draw_color_view);
	}

	if (_draw_depth_view) {
		instance->device().destroyImageView(_draw_depth_view);
	}
}

void FrameData::draw(const vk::CommandBuffer& cmd, Scene &s, const UniformData& camera, PipelineCollection& pipelines,
					 const vk::Image& swapchainImg, glm::ivec2 finalSize) {

	if (_draw_color == nullptr || _draw_depth == nullptr || _draw_color->size() != finalSize) {
		update_draw_fb(pipelines, finalSize);
	}

	{
		auto map = _cameraBuffer->map_t<UniformData>();
		static_cast<UniformData*>(map)[0] = camera;
	}

	if (s.batches_amount() == 0 || s.objects().size() == 0) {
		vk::ImageMemoryBarrier presentBarrier(vk::AccessFlagBits::eNone,
											  vk::AccessFlagBits::eNone,
											  vk::ImageLayout::eUndefined,
											  vk::ImageLayout::ePresentSrcKHR,
											  VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
											  swapchainImg,
											  {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, presentBarrier);
		return;
	}

	if (_instanceBuffer == nullptr || _instanceBuffer->size() / sizeof(ObjectInstance) < s.objects().size()) {
		_instanceBuffer = std::make_unique<Buffer>(instance, s.objects().size() * sizeof(ObjectInstance), vk::BufferUsageFlagBits::eStorageBuffer,
												   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		_descriptors_up_to_date = false;
	}

	if (_batchesBuffer == nullptr || _batchesBuffer->size() / sizeof(DrawBatch) < s.batches_amount()) {
		_batchesBuffer = std::make_unique<Buffer>(instance, s.batches_amount() * sizeof(DrawBatch), vk::BufferUsageFlagBits::eStorageBuffer,
												  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		_descriptors_up_to_date = false;
	}

	if (_drawBuffer == nullptr || _drawBuffer->size() / sizeof(VkDrawIndirectCommand) < s.batches_amount()) {
		_drawBuffer = std::make_unique<Buffer>(instance, s.batches_amount() * sizeof(VkDrawIndexedIndirectCommand), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer,
												  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		_descriptors_up_to_date = false;
	}

	if (_clearBuffer == nullptr || _clearBuffer->size() / sizeof(VkDrawIndirectCommand) < s.batches_amount()) {
		_clearBuffer = std::make_unique<Buffer>(instance, s.batches_amount() * sizeof(VkDrawIndexedIndirectCommand), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer,
											   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		_descriptors_up_to_date = false;
	}

	if (_indirectBuffer == nullptr || _indirectBuffer->size() / sizeof(uint32_t) < s.objects().size()) {
		_indirectBuffer = std::make_unique<Buffer>(instance, s.objects().size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eStorageBuffer,
												   vk::MemoryPropertyFlagBits::eDeviceLocal);
		_descriptors_up_to_date = false;
	}

	s.fill_buffers(_instanceBuffer, _batchesBuffer, _drawBuffer, _clearBuffer);

	if (!_descriptors_up_to_date) {
		update_descriptor_sets(*s.meshes());
	}

	run_z_pass(cmd, pipelines, s.batches_amount());

	vk::ImageMemoryBarrier copyRedBarrier(vk::AccessFlagBits::eDepthStencilAttachmentWrite,
										  vk::AccessFlagBits::eShaderRead,
										  vk::ImageLayout::eTransferSrcOptimal,
										  vk::ImageLayout::eShaderReadOnlyOptimal,
										  VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
										  _hzBuffer.depth_texture().image(),
										  {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
	vk::ImageMemoryBarrier copyWriteBarrier(vk::AccessFlagBits::eNone,
											vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead,
											vk::ImageLayout::eUndefined,
											vk::ImageLayout::eGeneral,
											VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
											_hzBuffer.texture().image(),
											{vk::ImageAspectFlagBits::eColor, 0, _hzBuffer.texture().levels(), 0, 1});

	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, nullptr, {{ copyRedBarrier, copyWriteBarrier }});

	run_copy_pass(cmd, pipelines);

	run_downsample(cmd, pipelines);

	vk::ImageMemoryBarrier afterDownsampleBarrier(vk::AccessFlagBits::eShaderWrite,
												  vk::AccessFlagBits::eShaderRead,
												  vk::ImageLayout::eGeneral,
												  vk::ImageLayout::eShaderReadOnlyOptimal,
												  VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
												  _hzBuffer.texture().image(),
												  {vk::ImageAspectFlagBits::eColor, 0, _hzBuffer.texture().levels(), 0, 1});

	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, nullptr, afterDownsampleBarrier);

	run_query(cmd, pipelines, s.objects().size());

	std::array<vk::ImageMemoryBarrier, 2> drawBarriers {
			vk::ImageMemoryBarrier(vk::AccessFlagBits::eNone,
								   vk::AccessFlagBits::eColorAttachmentWrite,
								   vk::ImageLayout::eUndefined,
								   vk::ImageLayout::eColorAttachmentOptimal,
								   VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
								   _draw_color->image(),
								   {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}),
			vk::ImageMemoryBarrier(vk::AccessFlagBits::eNone,
								   vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead,
								   vk::ImageLayout::eUndefined,
								   vk::ImageLayout::eDepthStencilAttachmentOptimal,
								   VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
								   _draw_depth->image(),
								   {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1})
	};

	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eDrawIndirect | vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests, {},
						vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eIndirectCommandRead), nullptr, drawBarriers);

	draw_final(cmd, pipelines, s.batches_amount(), finalSize);

	std::array<vk::ImageMemoryBarrier, 2> blitBarriers {
		vk::ImageMemoryBarrier(vk::AccessFlagBits::eColorAttachmentWrite,
							   vk::AccessFlagBits::eTransferRead,
							   vk::ImageLayout::eColorAttachmentOptimal,
							   vk::ImageLayout::eTransferSrcOptimal,
							   VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
							   _draw_color->image(),
							   {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}),
		vk::ImageMemoryBarrier(vk::AccessFlagBits::eNone,
							   vk::AccessFlagBits::eTransferWrite,
							   vk::ImageLayout::eUndefined,
							   vk::ImageLayout::eTransferDstOptimal,
							   VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
							   swapchainImg,
							   {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
	};

	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTransfer, {},
						vk::MemoryBarrier(vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eTransferRead), nullptr, blitBarriers);

	std::array<vk::ImageBlit, 1> blit {
		vk::ImageBlit(
				vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
				{{ { 0, 0, 0}, {finalSize.x, finalSize.y, 1} }},
				vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
				{{ {0, 0, 0}, {finalSize.x, finalSize.y, 1} }}
				)
	};
	cmd.blitImage(_draw_color->image(), vk::ImageLayout::eTransferSrcOptimal, swapchainImg, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eNearest);

	vk::ImageMemoryBarrier presentBarrier(vk::AccessFlagBits::eTransferWrite,
										  vk::AccessFlagBits::eNone,
										  vk::ImageLayout::eTransferDstOptimal,
										  vk::ImageLayout::ePresentSrcKHR,
										  VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
										  swapchainImg,
										  {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

	cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands, {}, nullptr, nullptr, presentBarrier);
}

void FrameData::update_descriptor_sets(const MeshBuffer& meshes) {
	auto meshBufferInfo = vk::DescriptorBufferInfo(meshes.buffer()->buffer(),0, meshes.buffer()->size());
	auto uniformBufferInfo = vk::DescriptorBufferInfo(_cameraBuffer->buffer(), 0, _cameraBuffer->size());
	auto instanceBufferInfo = vk::DescriptorBufferInfo(_instanceBuffer->buffer(), 0, _instanceBuffer->size());
	auto indirectBufferInfo = vk::DescriptorBufferInfo(_indirectBuffer->buffer(), 0, _indirectBuffer->size());
	auto clearBufferInfo = vk::DescriptorBufferInfo(_clearBuffer->buffer(), 0, _clearBuffer->size());

	auto copySourceInfo = vk::DescriptorImageInfo(nearestSampler->sampler(), _hzBuffer.depth_view(), vk::ImageLayout::eShaderReadOnlyOptimal);
	auto copyTargetInfo = vk::DescriptorImageInfo(nullptr, _hzBuffer.level_views()[0], vk::ImageLayout::eGeneral);
	auto queryTextureInfo = vk::DescriptorImageInfo(nearestSampler->sampler(), _hzBuffer.full_view(), vk::ImageLayout::eShaderReadOnlyOptimal);

	std::vector<vk::WriteDescriptorSet> writes {
		vk::WriteDescriptorSet(descriptorSets[DS_ID_MESH_AND_CAMERA], 0, 0, vk::DescriptorType::eStorageBuffer, nullptr, meshBufferInfo, nullptr),
		vk::WriteDescriptorSet(descriptorSets[DS_ID_MESH_AND_CAMERA], 1, 0, vk::DescriptorType::eUniformBuffer, nullptr, uniformBufferInfo, nullptr),

		vk::WriteDescriptorSet(descriptorSets[DS_ID_INSTANCES_ONLY], 0, 0, vk::DescriptorType::eStorageBuffer, nullptr, instanceBufferInfo, nullptr),

		vk::WriteDescriptorSet(descriptorSets[DS_ID_COPY], 0, 0, vk::DescriptorType::eCombinedImageSampler, copySourceInfo, nullptr, nullptr),
		vk::WriteDescriptorSet(descriptorSets[DS_ID_COPY], 1, 0, vk::DescriptorType::eStorageImage, copyTargetInfo, nullptr, nullptr),

		vk::WriteDescriptorSet(descriptorSets[DS_ID_CAMERA_ONLY], 0, 0, vk::DescriptorType::eUniformBuffer, nullptr, uniformBufferInfo, nullptr),

		vk::WriteDescriptorSet(descriptorSets[DS_ID_INSTANCES_AND_INDIRECTIONS_COMPUTE], 0, 0, vk::DescriptorType::eStorageBuffer, nullptr, instanceBufferInfo, nullptr),
		vk::WriteDescriptorSet(descriptorSets[DS_ID_INSTANCES_AND_INDIRECTIONS_COMPUTE], 1, 0, vk::DescriptorType::eStorageBuffer, nullptr, indirectBufferInfo, nullptr),

		vk::WriteDescriptorSet(descriptorSets[DS_ID_INSTANCES_AND_INDIRECTIONS_GRAPHICS], 0, 0, vk::DescriptorType::eStorageBuffer, nullptr, instanceBufferInfo, nullptr),
		vk::WriteDescriptorSet(descriptorSets[DS_ID_INSTANCES_AND_INDIRECTIONS_GRAPHICS], 1, 0, vk::DescriptorType::eStorageBuffer, nullptr, indirectBufferInfo, nullptr),

		vk::WriteDescriptorSet(descriptorSets[DS_ID_QUERY], 0, 0, vk::DescriptorType::eCombinedImageSampler, queryTextureInfo, nullptr, nullptr),
		vk::WriteDescriptorSet(descriptorSets[DS_ID_QUERY], 1, 0, vk::DescriptorType::eStorageBuffer, nullptr, clearBufferInfo, nullptr),

		//vk::WriteDescriptorSet(descriptorSets[DS_ID_MATERIALS_AND_TEXTURES_0], 0, 0, vk::DescriptorType::eStorageBuffer, nullptr, )
	};

	instance->device().updateDescriptorSets(writes, nullptr);
	_descriptors_up_to_date = true;
}

void FrameData::run_z_pass(const vk::CommandBuffer& cmd, PipelineCollection &pipelines, int batchesAmount) {
	const auto& zPassPipeline = pipelines.z_pass();
	auto clearDepth = vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0));


	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, zPassPipeline->pipeline_layout(), 0,
						   {{ descriptorSets[DS_ID_MESH_AND_CAMERA], descriptorSets[DS_ID_INSTANCES_ONLY] }}, nullptr);

	auto hzbSize = _hzBuffer.sizes()[0];

	cmd.beginRenderPass({zPassPipeline->render_pass(), _z_framebuffer,
								   {{ 0, 0 }, {(uint32_t)hzbSize.x, (uint32_t)hzbSize.y}},
								   clearDepth}, vk::SubpassContents::eInline);
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, zPassPipeline->pipeline());
	cmd.setViewport(0, {{0, 0, (float) hzbSize.x, (float)hzbSize.y, 0.0f, 1.0f}});
	cmd.setScissor(0, {{{0, 0}, {(uint32_t) hzbSize.x, (uint32_t)hzbSize.y}}});
	cmd.setCullMode(vk::CullModeFlagBits::eNone);

	cmd.drawIndirect(_drawBuffer->buffer(), 0, batchesAmount, sizeof(VkDrawIndirectCommand));

	cmd.endRenderPass();
}

void FrameData::run_copy_pass(const vk::CommandBuffer &cmd, PipelineCollection &pipelines) {
	const auto& copyPipeline = pipelines.copy_pass();

	auto hz_size = _hzBuffer.depth_texture().size();

	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, copyPipeline->pipeline());
	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, copyPipeline->pipeline_layout(), 0, descriptorSets[DS_ID_COPY], nullptr);

	cmd.dispatch(glm::max(1, hz_size.x / 16), glm::max(1, hz_size.y / 16), 1);
}

void FrameData::run_downsample(const vk::CommandBuffer &cmd, PipelineCollection &pipelines) {
	const auto& downsamplePipeline = pipelines.downsample_pass();
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, downsamplePipeline->pipeline());
	for (uint32_t level = 1; level < _hzBuffer.texture().levels(); level++) {
		vk::MemoryBarrier barrier { vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead };

		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader,
							{}, {{ barrier }}, nullptr, nullptr);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, downsamplePipeline->pipeline_layout(), 0, _hzBuffer.downsample_descriptor_sets()[level - 1], nullptr);

		auto size = _hzBuffer.sizes()[level];

		cmd.dispatch(glm::max(1, size.x / 16), glm::max(1, size.y / 16), 1);
	}
}

void FrameData::run_query(const vk::CommandBuffer &cmd, PipelineCollection &pipelines, int objectsAmount) {
	const auto& queryPipeline = pipelines.query_pass();
	cmd.bindPipeline(vk::PipelineBindPoint::eCompute, queryPipeline->pipeline());

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, queryPipeline->pipeline_layout(), 0,
						   {{descriptorSets[DS_ID_CAMERA_ONLY], descriptorSets[DS_ID_INSTANCES_AND_INDIRECTIONS_COMPUTE], descriptorSets[DS_ID_QUERY] }},
						   nullptr);
	cmd.pushConstants(queryPipeline->pipeline_layout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t), &objectsAmount);
	auto amount = objectsAmount % 16 == 0 ? objectsAmount / 16 : static_cast<int>(std::ceil(objectsAmount / 16.0f));

	cmd.dispatch(amount, 1, 1);
}

void FrameData::update_draw_fb(PipelineCollection &pipelines, glm::ivec2 size) {
	if (_draw_frame_buffer) {
		instance->device().destroyFramebuffer(_draw_frame_buffer);
	}

	if (_draw_color_view) {
		instance->device().destroyImageView(_draw_color_view);
	}

	if (_draw_depth_view) {
		instance->device().destroyImageView(_draw_depth_view);
	}

	_draw_color = std::make_unique<Texture>(instance, PIPELINE_COLOR_FORMAT, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment, size, 1);
	_draw_depth = std::make_unique<Texture>(instance, PIPELINE_DEPTH_FORMAT, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eInputAttachment, size, 1);

	vk::ComponentMapping mapping = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
	_draw_color_view = instance->device().createImageView(vk::ImageViewCreateInfo({}, _draw_color->image(), vk::ImageViewType::e2D, PIPELINE_COLOR_FORMAT, mapping,
																				  vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)));

	_draw_depth_view = instance->device().createImageView(vk::ImageViewCreateInfo({}, _draw_depth->image(), vk::ImageViewType::e2D, PIPELINE_DEPTH_FORMAT, mapping,
																				  vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1)));

	std::array<vk::ImageView, 2> views {
		_draw_color_view,
		_draw_depth_view
	};

	vk::FramebufferCreateInfo info({}, pipelines.draw_pass()->render_pass(), views, size.x, size.y, 1);
	_draw_frame_buffer = instance->device().createFramebuffer(info);
}

void FrameData::draw_final(const vk::CommandBuffer &cmd, PipelineCollection &pipelines, int batches_amount,
						   glm::ivec2 size) {
	const auto& drawPipeline = pipelines.draw_pass();
	auto clearColor = vk::ClearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));
	auto clearDepth = vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0));

	std::array<vk::ClearValue, 2> clears = {
			clearColor, clearDepth
	};

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, drawPipeline->pipeline_layout(), 0,
						   {{ descriptorSets[DS_ID_MESH_AND_CAMERA], descriptorSets[DS_ID_INSTANCES_AND_INDIRECTIONS_GRAPHICS] }}, nullptr);


	cmd.beginRenderPass({drawPipeline->render_pass(), _draw_frame_buffer,
						 {{ 0, 0 }, {(uint32_t)size.x, (uint32_t)size.y}},
						 clears}, vk::SubpassContents::eInline);
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, drawPipeline->pipeline());
	cmd.setViewport(0, {{0, 0, (float) size.x, (float)size.y, 0.0f, 1.0f}});
	cmd.setScissor(0, {{{0, 0}, {(uint32_t) size.x, (uint32_t)size.y}}});
	cmd.setCullMode(vk::CullModeFlagBits::eNone);

	cmd.drawIndirect(_clearBuffer->buffer(), 0, batches_amount, sizeof(VkDrawIndirectCommand));

	cmd.endRenderPass();
}

