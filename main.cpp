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
#include "PipelineCollection.h"
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

		auto allNearestSampler = Sampler(instance, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest);
		auto trilinearSampler = Sampler(instance, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear);

		PipelineCollection pipelines(instance, base_path);

		Scene scene(instance, 1024 * 12, 50 * 1024);

		std::vector<Vertex> quad_vertices {
			Vertex({-0.5, -0.5, 0}),
			Vertex({0.5, -0.5, 0}),
			Vertex({0.5, 0.5, 0}),

			Vertex({-0.5, -0.5, 0}),
			Vertex({0.5, 0.5, 0}),
			Vertex({-0.5, 0.5, 0})
		};

		for(auto& v : quad_vertices) {
			v.vertexColor = { 1, 1, 1, 1};
		}

		auto cmd_buffer = instance->device().allocateCommandBuffers(vk::CommandBufferAllocateInfo(instance->graphics_command_pool(), vk::CommandBufferLevel::ePrimary, 1))[0];
		auto load_fence = instance->device().createFence({});
		cmd_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
		auto cube_id = scene.meshes()->append(quad_vertices, (int)vk::PrimitiveTopology::eTriangleList, {0, 0, 0}, {0.5, 0.5, 0.0001f}, cmd_buffer);
		cmd_buffer.end();
		instance->graphics_queue().submit(vk::SubmitInfo(nullptr, nullptr, cmd_buffer, nullptr), load_fence);

		auto objectId = scene.addObject(cube_id, 0);
		auto& obj = scene.get_object(objectId);
		obj.transform.position({ 0, 0, -2});
		obj.transform.scale({0.9, 0.9, 0.9});

		objectId = scene.addObject(cube_id, 0);
		auto& obj2 = scene.get_object(objectId);
		obj2.transform.position({ -1, 0, -2});
		obj2.transform.scale({0.9, 0.9, 0.9});

		objectId = scene.addObject(cube_id, 0);
		auto& obj3 = scene.get_object(objectId);
		obj3.transform.position({ 1, 0, -2});
		obj3.transform.scale({0.9, 0.9, 0.9});

		objectId = scene.addObject(cube_id, 0);
		auto& obj4 = scene.get_object(objectId);
		obj4.transform.position({ 0, 0, -3});
		obj4.transform.scale({0.9, 0.9, 0.9});

		auto imageAvailableSemaphore = instance->device().createSemaphore({});

		std::vector<std::unique_ptr<FrameData>> frames;
		frames.push_back( std::move(std::make_unique<FrameData>(instance, 0, swapchain, hzbSize, pipelines, allNearestSampler)));
		frames.push_back( std::move(std::make_unique<FrameData>(instance, 1, swapchain, hzbSize, pipelines, allNearestSampler)));


		UniformData uniformData(glm::lookAt(glm::vec3(0, 0, 0), {0, 0, -1}, {0, 1, 0}),
								glm::perspectiveFov(glm::radians(70.0f), 1280.0f, 720.0f, 0.01f, 1000.0f));

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

			//Resize swapchain and framebuffers if necessary
			SDL_Vulkan_GetDrawableSize(window, &width, &height);
			glm::ivec2 nSize = {width, height};

			if (nSize != swapchain.size()) {
				swapchain.create_swapchain(nSize);
			}

			imageIndex = device.acquireNextImageKHR(swapchain.swapchain(), UINT64_MAX, imageAvailableSemaphore,
													nullptr).value;
			auto& frame = frames[imageIndex];
			auto commandBuffer = frame->command_buffer();

			device.waitForFences({ frame->in_flight_fence() }, true, UINT64_MAX);
			device.resetFences({ frame->in_flight_fence() });


			auto imgs = device.getSwapchainImagesKHR(swapchain.swapchain());
			commandBuffer.reset();

			uniformData.projection = glm::perspectiveFov(glm::radians(70.0f), (float) width, (float) height, 0.01f,
																1000.0f);
			vk::CommandBufferBeginInfo beginInfo({}, nullptr);
			commandBuffer.begin(beginInfo);

			frame->draw(commandBuffer, scene, uniformData, pipelines, imgs[imageIndex], { width, height });

			commandBuffer.end();

			vk::PipelineStageFlags waitFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eTransfer;
			instance->graphics_queue().submit(
					{{imageAvailableSemaphore, waitFlags, commandBuffer, frame->render_in_progress_semaphore()}}, frame->in_flight_fence());

			instance->present_queue().presentKHR({frame->render_in_progress_semaphore(), swapchain.swapchain(), imageIndex});
		}

		instance->device().waitIdle();

		instance->device().destroySemaphore(imageAvailableSemaphore);

		instance = nullptr;
	}
	CLEANUP();
	return 0;
}
