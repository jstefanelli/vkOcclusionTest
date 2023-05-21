#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>
#include <filesystem>
#include <fstream>
#include "Buffer.h"
#include "Instance.h"
#include "Swapchain.h"
#include "GlobalTypes.h"
#include "GraphicsPipeline.h"
#include "FrameData.h"
#include "Texture.h"
#include "ComputePipeline.h"
#include "Sampler.h"

void printSdlError(const char* file, int line) {
	const char* err = SDL_GetError();
	if (err != nullptr) {
		std::cerr << "[SDL2][" << file << ":" << line << "]: " << err << std::endl;
	}
}

#define SDL_CHECK() printSdlError(__FILE__, __LINE__)

void cleanup(SDL_Window* window) {
	if (window != nullptr) {
		SDL_DestroyWindow(window);
	}

	SDL_Quit();
}

#define CLEANUP() cleanup(window)

GraphicsPipeline createFinalDrawPipeline(const std::shared_ptr<Instance>& instance, const Swapchain& swapchain, const std::filesystem::path& baseDirectory) {
	auto vsPath = baseDirectory / "shaders" / "main.vert.spv";
	auto fsPath = baseDirectory / "shaders" / "main.frag.spv";

	std::vector<GraphicsPipelineAttachment> color_formats { GraphicsPipelineAttachment(swapchain.surface_format().format, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR ) };
	std::optional<GraphicsPipelineAttachment> depth_format = std::nullopt;

	std::vector<vk::DescriptorSetLayoutBinding> bindings {
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1,
									   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
									   nullptr),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1,
									  vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
									  nullptr),
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eUniformBuffer, 1,
									   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
									   nullptr)
	};

	std::vector<vk::SubpassDependency> dependencies {
			{VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
			 vk::PipelineStageFlagBits::eColorAttachmentOutput, {},
			 vk::AccessFlagBits::eColorAttachmentWrite}
	};

	return { instance, vsPath, fsPath, bindings, dependencies, color_formats, depth_format };
}

GraphicsPipeline createZGraphicsPipeline(const std::shared_ptr<Instance>& instance, const vk::Format& depthFormat, const std::filesystem::path& baseDirectory) {
	auto vsPath = baseDirectory / "shaders" / "main.vert.spv";
	auto fsPath = baseDirectory / "shaders" / "main.frag.spv";

	std::vector<vk::DescriptorSetLayoutBinding> bindings {
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1,
										   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
										   nullptr),
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1,
										   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
										   nullptr),
			vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eUniformBuffer, 1,
										   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
										   nullptr)
	};

	std::vector<GraphicsPipelineAttachment> attachments;
	std::optional<GraphicsPipelineAttachment> depth = GraphicsPipelineAttachment(depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal);

	std::vector<vk::SubpassDependency> dependencies;

	return {instance, vsPath, fsPath, bindings, dependencies, attachments, depth };
}

ComputePipeline createDownsamplePipeline(const std::shared_ptr<Instance>& instance, const vk::Format& imageFormat, const std::filesystem::path& baseDirectory) {
	auto shaderPath = baseDirectory / "shaders" / "downsample.comp.spv";

	std::vector<vk::DescriptorSetLayoutBinding> bindings {
		vk::DescriptorSetLayoutBinding( 0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute),
		vk::DescriptorSetLayoutBinding( 1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute)
	};

	return { instance, shaderPath, bindings };
}

ComputePipeline createCopyPipeline(const std::shared_ptr<Instance>& instance, const vk::Format& zFormat, const std::filesystem::path& baseDirectory) {
	auto shaderPath = baseDirectory / "shaders" / "copy.comp.spv";

	std::vector<vk::DescriptorSetLayoutBinding> bindings {
		vk::DescriptorSetLayoutBinding( 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute),
		vk::DescriptorSetLayoutBinding( 1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute)
	};

	return { instance, shaderPath, bindings };
}

int main() {
	SDL_Init(SDL_INIT_VIDEO);

	auto *window = SDL_CreateWindow("vkOcclusionTest", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
									SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (window == nullptr) {
		SDL_CHECK();

		CLEANUP();
		return 1;
	}
	{
		auto instance = std::make_shared<Instance>(window);
		auto swapchain = Swapchain(window, instance);
		instance->create_device(swapchain);

		glm::ivec2 hzbSize = { 1024, 512 };

		int width = 0, height = 0;
		SDL_Vulkan_GetDrawableSize(window, &width, &height);
		swapchain.create_swapchain({width, height});

		bool alive = true;

		auto base_path_str = SDL_GetBasePath();
		auto base_path = std::filesystem::path(base_path_str);
		SDL_free(base_path_str);

		auto pipeline = createFinalDrawPipeline(instance, swapchain, base_path);
		swapchain.create_framebuffers(pipeline.render_pass());

		auto allNearestSampler = Sampler(instance, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest);
		auto trilinearSampler = Sampler(instance, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear);

		auto zPassPipeline = createZGraphicsPipeline(instance, vk::Format::eD32Sfloat, base_path);
		auto copyPipeline = createCopyPipeline(instance, vk::Format::eD32Sfloat, base_path);
		auto downsamplePipeline = createDownsamplePipeline(instance, vk::Format::eD32Sfloat, base_path);

		auto cmdBuff = instance->device().allocateCommandBuffers({instance->graphics_command_pool(), vk::CommandBufferLevel::ePrimary, 1})[0];
		auto imageAvailableSemaphore = instance->device().createSemaphore({});

		std::vector<Vertex_Data> vertices = {
				Vertex_Data(glm::vec3(-0.5f, 0.5f, 0.0f)),
				Vertex_Data(glm::vec3(0.5f, 0.5f, 0.0f)),
				Vertex_Data(glm::vec3(0.0f, -0.5f, 0.0f))
		};

		vertices[0].vertexColor = {1.0f, 0.0f, 0.0f, 1.0f};
		vertices[1].vertexColor = {0.0f, 1.0f, 0.0f, 1.0f};
		vertices[2].vertexColor = {0.0f, 0.0f, 1.0f, 1.0f};

		std::vector<Instance_Data> instanceData{
				{glm::translate(glm::mat4(1.0), glm::vec3(0, 0, -1))},
				{glm::translate(glm::mat4(1.0), glm::vec3(1, 0, -1))},
				{glm::translate(glm::mat4(1.0), glm::vec3(-1, 0, -1))},
		};

		Push_Constants push_constants(glm::lookAt(glm::vec3(0, 0, 0), {0, 0, -1}, {0, 1, 0}),
									  glm::perspectiveFov(glm::radians(70.0f), 1280.0f, 720.0f, 0.01f, 1000.0f));

		auto tmpVertexBuffer = Buffer(instance, sizeof(Vertex_Data) * vertices.size(), vk::BufferUsageFlagBits::eTransferSrc,
									  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		auto instanceBuffer = Buffer(instance, sizeof(Instance_Data) * instanceData.size(), vk::BufferUsageFlagBits::eStorageBuffer,
									 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		auto uniformBuffer0 = Buffer(instance, sizeof(Push_Constants), vk::BufferUsageFlagBits::eUniformBuffer,
									 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
		auto uniformBuffer1 = Buffer(instance, sizeof(Push_Constants), vk::BufferUsageFlagBits::eUniformBuffer,
									 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		{
			auto mapping = tmpVertexBuffer.map();
			std::memcpy(static_cast<void *>(mapping), (void *) vertices.data(), sizeof(Vertex_Data) * vertices.size());
		}

		{
			auto mapping = instanceBuffer.map();
			std::memcpy(static_cast<void *>(mapping), (void *) instanceData.data(),
						sizeof(Instance_Data) * instanceData.size());
		}

		{
			auto mapping = uniformBuffer0.map();
			std::memcpy(static_cast<void *>(mapping), (void *) &push_constants, sizeof(Push_Constants));
		}

		{
			auto mapping = uniformBuffer1.map();
			std::memcpy(static_cast<void *>(mapping), (void *) &push_constants, sizeof(Push_Constants));
		}

		auto vertexBuffer = Buffer(instance, tmpVertexBuffer.size(),
								   vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
								   vk::MemoryPropertyFlagBits::eDeviceLocal);
		tmpVertexBuffer.copy_to(vertexBuffer, cmdBuff);

		auto loadFence = instance->device().createFence({});

		instance->graphics_queue().submit({{{}, {}, cmdBuff, {}}}, loadFence);
		instance->device().waitForFences(loadFence, true, UINT64_MAX);

		std::vector<std::unique_ptr<FrameData>> frames;
		frames.push_back( std::move(std::make_unique<FrameData>(instance, 0, swapchain, hzbSize, zPassPipeline.render_pass(), downsamplePipeline.descriptor_set_layout(), allNearestSampler)));
		frames.push_back( std::move(std::make_unique<FrameData>(instance, 1, swapchain, hzbSize, zPassPipeline.render_pass(), downsamplePipeline.descriptor_set_layout(), allNearestSampler)));

		std::vector<vk::DescriptorSetLayout> layouts{
				pipeline.descriptor_set_layout(),
				pipeline.descriptor_set_layout(),
				copyPipeline.descriptor_set_layout(),
				copyPipeline.descriptor_set_layout()
		};

		auto descriptorSets = instance->create_descriptor_sets(layouts);

		std::array<vk::DescriptorBufferInfo, 4> descriptorBufferInfos = {
				vk::DescriptorBufferInfo{static_cast<vk::Buffer>(uniformBuffer0), 0, VK_WHOLE_SIZE},
				vk::DescriptorBufferInfo{static_cast<vk::Buffer>(uniformBuffer1), 0, VK_WHOLE_SIZE},
				vk::DescriptorBufferInfo{static_cast<vk::Buffer>(vertexBuffer), 0, VK_WHOLE_SIZE},
				vk::DescriptorBufferInfo{static_cast<vk::Buffer>(instanceBuffer), 0, VK_WHOLE_SIZE}
		};

		auto frame0CopySourceInfo = vk::DescriptorImageInfo( allNearestSampler, frames[0]->hz_buffer().depth_view(), vk::ImageLayout::eShaderReadOnlyOptimal);
		auto frame1CopySourceInfo = vk::DescriptorImageInfo( allNearestSampler, frames[1]->hz_buffer().depth_view(), vk::ImageLayout::eShaderReadOnlyOptimal);
		auto frame0CopyTargetInfo = vk::DescriptorImageInfo( nullptr, frames[0]->hz_buffer().level_views()[0], vk::ImageLayout::eGeneral);
		auto frame1CopyTargetInfo = vk::DescriptorImageInfo( nullptr, frames[1]->hz_buffer().level_views()[0], vk::ImageLayout::eGeneral);

		std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
				vk::WriteDescriptorSet{descriptorSets[0], 2, 0, vk::DescriptorType::eUniformBuffer, nullptr,
									   descriptorBufferInfos[0], nullptr, nullptr},
				vk::WriteDescriptorSet{descriptorSets[1], 2, 0, vk::DescriptorType::eUniformBuffer, nullptr,
									   descriptorBufferInfos[1], nullptr, nullptr},

				vk::WriteDescriptorSet(descriptorSets[0], 0, 0, vk::DescriptorType::eStorageBuffer, nullptr,
									   descriptorBufferInfos[2], nullptr, nullptr),
				vk::WriteDescriptorSet(descriptorSets[1], 0, 0, vk::DescriptorType::eStorageBuffer, nullptr,
									   descriptorBufferInfos[2], nullptr, nullptr),
				vk::WriteDescriptorSet(descriptorSets[0], 1, 0, vk::DescriptorType::eStorageBuffer, nullptr,
									   descriptorBufferInfos[3], nullptr, nullptr),
				vk::WriteDescriptorSet(descriptorSets[1], 1, 0, vk::DescriptorType::eStorageBuffer, nullptr,
									   descriptorBufferInfos[3], nullptr, nullptr),

				vk::WriteDescriptorSet(descriptorSets[2], 0, 0, vk::DescriptorType::eCombinedImageSampler, frame0CopySourceInfo, nullptr, nullptr),
				vk::WriteDescriptorSet(descriptorSets[3], 0, 0, vk::DescriptorType::eCombinedImageSampler, frame1CopySourceInfo, nullptr, nullptr),
				vk::WriteDescriptorSet(descriptorSets[2], 1, 0, vk::DescriptorType::eStorageImage, frame0CopyTargetInfo, nullptr, nullptr),
				vk::WriteDescriptorSet(descriptorSets[3], 1, 0, vk::DescriptorType::eStorageImage, frame1CopyTargetInfo, nullptr, nullptr),
		};

		instance->device().updateDescriptorSets(writeDescriptorSets, nullptr);


		auto dummyBuffer = Buffer(instance, 16, vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);

		uint32_t imageIndex = 0;
		while (alive) {

			SDL_Event event = {};
			while (SDL_PollEvent(&event)) {
				if (event.type == SDL_EventType::SDL_QUIT) {
					alive = false;
					break;
				}
			}
			auto device = instance->device();

			SDL_Vulkan_GetDrawableSize(window, &width, &height);
			glm::ivec2 nSize = {width, height};


			bool resizeMatrix = false;

			if (nSize != swapchain.size()) {
				swapchain.create_swapchain(nSize);
				swapchain.create_framebuffers(pipeline.render_pass());

				for(auto& frame : frames) {
					frame->update_framebuffer(swapchain);
				}

				resizeMatrix = true;
			}

			imageIndex = device.acquireNextImageKHR(swapchain.swapchain(), UINT64_MAX, imageAvailableSemaphore,
													nullptr).value;
			auto& frame = frames[imageIndex];
			auto commandBuffer = frame->command_buffer();

			device.waitForFences({ frame->in_flight_fence() }, true, UINT64_MAX);
			device.resetFences({ frame->in_flight_fence() });

			commandBuffer.reset();

			if (resizeMatrix) {
				push_constants.projection = glm::perspectiveFov(glm::radians(70.0f), (float) width, (float) height, 0.01f,
																1000.0f);
				for (int i = 0; i < 2; i++) {
					auto map = (i == 0 ? uniformBuffer0 : uniformBuffer1).map();
					std::memcpy(static_cast<void *>(map), &push_constants, sizeof(Push_Constants));
				}
			}

			vk::CommandBufferBeginInfo beginInfo({}, nullptr);
			commandBuffer.begin(beginInfo);

			auto clearValue = vk::ClearValue({0.0f, 0.0f, 0.0f, 1.0f});
			auto clearDepth = vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0));
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.pipeline_layout(), 0,
											 descriptorSets[imageIndex], nullptr);

			auto hzbSize = frame->hz_buffer().sizes()[0];

			commandBuffer.beginRenderPass({zPassPipeline.render_pass(), frame->z_framebuffer(),
										   {{ 0, 0 }, {(uint32_t)hzbSize.x, (uint32_t)hzbSize.y}},
										   clearDepth}, vk::SubpassContents::eInline);
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, zPassPipeline);
			commandBuffer.setViewport(0, {{0, 0, (float) hzbSize.x, (float)hzbSize.y, 0.0f, 1.0f}});
			commandBuffer.setScissor(0, {{{0, 0}, {(uint32_t) hzbSize.x, (uint32_t)hzbSize.y}}});
			commandBuffer.setCullMode(vk::CullModeFlagBits::eNone);

			commandBuffer.draw(3, instanceData.size(), 0, 0);

			commandBuffer.endRenderPass();

			vk::ImageMemoryBarrier copyRedBarrier(vk::AccessFlagBits::eDepthStencilAttachmentWrite,
													 vk::AccessFlagBits::eShaderRead,
													 vk::ImageLayout::eTransferSrcOptimal,
													 vk::ImageLayout::eShaderReadOnlyOptimal,
													 VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
													 frame->hz_buffer().depth_texture().image(),
													 {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1});
			vk::ImageMemoryBarrier copyWriteBarrier(vk::AccessFlagBits::eNone,
													vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead,
													vk::ImageLayout::eUndefined,
													vk::ImageLayout::eGeneral,
													VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
													frame->hz_buffer().texture().image(),
													{vk::ImageAspectFlagBits::eColor, 0, frame->hz_buffer().texture().levels(), 0, 1});

			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eComputeShader, {}, nullptr, nullptr, {{ copyRedBarrier, copyWriteBarrier }});

			auto hz_size = frame->hz_buffer().depth_texture().size();

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, copyPipeline);
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, copyPipeline.pipeline_layout(), 0, descriptorSets[2 + imageIndex], nullptr);

			commandBuffer.dispatch(glm::max(1, hz_size.x / 16), glm::max(1, hz_size.y / 16), 1);

			commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, downsamplePipeline);
			for (uint32_t level = 1; level < frame->hz_buffer().texture().levels(); level++) {
				vk::MemoryBarrier barrier { vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead };

				/*
				vk::ImageMemoryBarrier reLayout(vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead,
												vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead,
												vk::ImageLayout::eGeneral,
												vk::ImageLayout::eGeneral,
												VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
												frame->hz_buffer().texture().image(),
												{vk::ImageAspectFlagBits::eColor, level, frame->hz_buffer().texture().levels() - level, 0, 1});
												*/

				commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
											  vk::PipelineStageFlagBits::eComputeShader, {}, {{ barrier }}, nullptr, nullptr);


				commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, downsamplePipeline.pipeline_layout(), 0, frame->hz_buffer().downsample_descriptor_sets()[level - 1], nullptr);

				auto size = frame->hz_buffer().sizes()[level];

				commandBuffer.dispatch(glm::max(1, size.x / 16), glm::max(1, size.y / 16), 1);
			}

			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.pipeline_layout(), 0,
											 descriptorSets[imageIndex], nullptr);
			commandBuffer.beginRenderPass({pipeline.render_pass(), swapchain.framebuffers()[imageIndex],
										   {{0, 0}, {(uint32_t) width, (uint32_t) height}}, clearValue},
										  vk::SubpassContents::eInline);
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
			commandBuffer.setViewport(0, {{0.0f, 0.0f, (float) width, (float) height, 0.0f, 1.0f}});
			commandBuffer.setScissor(0, {{{0, 0}, {(uint32_t) width, (uint32_t) height}}});
			commandBuffer.setCullMode(vk::CullModeFlagBits::eNone);

			commandBuffer.draw(3, instanceData.size(), 0, 0);

			commandBuffer.endRenderPass();
			commandBuffer.end();

			vk::PipelineStageFlags waitFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eComputeShader;
			instance->graphics_queue().submit(
					{{imageAvailableSemaphore, waitFlags, commandBuffer, frame->render_in_progress_semaphore()}}, frame->in_flight_fence());

			instance->present_queue().presentKHR({frame->render_in_progress_semaphore(), swapchain.swapchain(), imageIndex});
		}

		instance->device().waitIdle();

		instance->device().destroyFence(loadFence);
		instance->device().destroySemaphore(imageAvailableSemaphore);

		instance = nullptr;
	}
	CLEANUP();
	return 0;
}
