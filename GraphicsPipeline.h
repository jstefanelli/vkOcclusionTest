#ifndef VKOCCLUSIONTEST_GRAPHICSPIPELINE_H
#define VKOCCLUSIONTEST_GRAPHICSPIPELINE_H

#include <filesystem>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include "Instance.h"
#include <filesystem>
#include <memory>
#include <optional>

std::vector<char> readFile(const std::filesystem::path& path);

template<typename SourceT, typename TargetT>
std::vector<TargetT> reinterpret_data(const std::vector<SourceT>& data);

struct GraphicsPipelineAttachment {
	vk::Format format;
	vk::ImageLayout startLayout;
	vk::ImageLayout endLayout;
};

class GraphicsPipeline {
private:
	std::shared_ptr<Instance> instance;
	vk::Pipeline _pipeline;
	vk::PipelineLayout _pipeline_layout;
	vk::RenderPass _render_pass;
	std::vector<vk::DescriptorSetLayout> _descriptor_set_layouts;

	std::vector<vk::ShaderModule> modules;
public:
	GraphicsPipeline(std::shared_ptr<Instance> instance, const std::filesystem::path& vsPath, const std::filesystem::path& fsPath,
					 const std::vector<std::vector<vk::DescriptorSetLayoutBinding>>& bindings, const std::vector<vk::SubpassDependency>& dependencies,
					 const std::vector<GraphicsPipelineAttachment>& colorAttachments, const std::optional<GraphicsPipelineAttachment>& depthFormat);

	~GraphicsPipeline();

	inline const vk::Pipeline& pipeline() const {
		return _pipeline;
	}

	inline operator const vk::Pipeline&() const {
		return _pipeline;
	}

	inline const vk::PipelineLayout& pipeline_layout() const {
		return _pipeline_layout;
	}

	inline const vk::RenderPass& render_pass() const {
		return _render_pass;
	}

	inline const std::vector<vk::DescriptorSetLayout>& descriptor_set_layouts() const {
		return _descriptor_set_layouts;
	}

};

#endif //VKOCCLUSIONTEST_GRAPHICSPIPELINE_H
