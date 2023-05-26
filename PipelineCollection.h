#ifndef VKOCCLUSIONTEST_PIPELINECOLLECTION_H
#define VKOCCLUSIONTEST_PIPELINECOLLECTION_H

#include "GraphicsPipeline.h"
#include "ComputePipeline.h"
#include "Instance.h"
#include <memory>
#include <filesystem>

#define PIPELINE_DEPTH_FORMAT vk::Format::eD32Sfloat
#define PIPELINE_COLOR_FORMAT vk::Format::eR8G8B8A8Unorm

class PipelineCollection {
private:
	std::shared_ptr<Instance> instance;
	std::filesystem::path base_path;
	std::unique_ptr<GraphicsPipeline> zPass;
	std::unique_ptr<GraphicsPipeline> drawPass;
	std::unique_ptr<ComputePipeline> copyPass;
	std::unique_ptr<ComputePipeline> downsamplePass;
	std::unique_ptr<ComputePipeline> queryPass;
public:
	PipelineCollection(std::shared_ptr<Instance> inst, std::filesystem::path basePath);

	const std::unique_ptr<GraphicsPipeline>& z_pass();
	const std::unique_ptr<GraphicsPipeline>& draw_pass();
	const std::unique_ptr<ComputePipeline>& copy_pass();
	const std::unique_ptr<ComputePipeline>& downsample_pass();
	const std::unique_ptr<ComputePipeline>& query_pass();
};

#endif //VKOCCLUSIONTEST_PIPELINECOLLECTION_H
