#include "PipelineCollection.h"

#include <memory>

PipelineCollection::PipelineCollection(std::shared_ptr<Instance> inst, std::filesystem::path basePath) : instance(std::move(inst)), base_path(std::move(basePath)) {

}

const std::unique_ptr<GraphicsPipeline>& PipelineCollection::z_pass() {
	if (zPass == nullptr) {
		auto vsPath = base_path / "shaders" / "main.vert.spv";
		auto fsPath = base_path / "shaders" / "main.frag.spv";


		std::vector<vk::DescriptorSetLayoutBinding> bindings {
				vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1,
											   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
											   nullptr),
				vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBuffer, 1,
											   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
											   nullptr)
		};
		std::vector<vk::DescriptorSetLayoutBinding> bindings2 {
				vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1,
											   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
											   nullptr)
		};

		std::vector<GraphicsPipelineAttachment> attachments;
		std::optional<GraphicsPipelineAttachment> depth = GraphicsPipelineAttachment(PIPELINE_DEPTH_FORMAT, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal);
		std::vector<vk::SubpassDependency> dependencies;

		std::vector<std::vector<vk::DescriptorSetLayoutBinding>> bindingsVector = {{ bindings, bindings2 }};

		zPass = std::make_unique<GraphicsPipeline>(instance, vsPath, fsPath, bindingsVector, dependencies, attachments, depth);
	}

	return zPass;
}

const std::unique_ptr<GraphicsPipeline> &PipelineCollection::draw_pass() {
	if (drawPass == nullptr) {
		auto vsPath = base_path / "shaders" / "full.vert.spv";
		auto fsPath = base_path / "shaders" / "full.frag.spv";

		std::vector<vk::DescriptorSetLayoutBinding> bindings0{
				vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1,
											   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
											   nullptr),
				vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eUniformBuffer, 1,
											   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
											   nullptr)
		};

		std::vector<vk::DescriptorSetLayoutBinding> bindings1{
				vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1,
											   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
											   nullptr),
				vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1,
												vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
												nullptr)
		};

		std::vector<vk::DescriptorSetLayoutBinding> bindings2 {
				vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1,
											   vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
											   nullptr),
				vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 4,
											   vk::ShaderStageFlagBits::eFragment, nullptr)
		};

		std::vector<GraphicsPipelineAttachment> color_formats { GraphicsPipelineAttachment(PIPELINE_COLOR_FORMAT, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal) };
		std::optional<GraphicsPipelineAttachment> depth_format = GraphicsPipelineAttachment(PIPELINE_DEPTH_FORMAT, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

		std::vector<vk::SubpassDependency> dependencies {
				/*{VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
				 vk::PipelineStageFlagBits::eColorAttachmentOutput, {},
				 vk::AccessFlagBits::eColorAttachmentWrite}*/
		};

		std::vector<std::vector<vk::DescriptorSetLayoutBinding>> bindingsVector = {{ bindings0, bindings1, bindings2 }};

		drawPass = std::make_unique<GraphicsPipeline>(instance, vsPath, fsPath, bindingsVector, dependencies, color_formats, depth_format);
	}

	return drawPass;
}

const std::unique_ptr<ComputePipeline> &PipelineCollection::copy_pass() {
	if(copyPass == nullptr) {
		auto shaderPath = base_path / "shaders" / "copy.comp.spv";

		std::vector<vk::DescriptorSetLayoutBinding> bindings {
				vk::DescriptorSetLayoutBinding( 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute),
				vk::DescriptorSetLayoutBinding( 1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute)
		};

		std::vector<std::vector<vk::DescriptorSetLayoutBinding>> bindingsVector = {{ bindings  }};

		copyPass = std::make_unique<ComputePipeline>(instance, shaderPath, bindingsVector);
	}

	return copyPass;
}

const std::unique_ptr<ComputePipeline> &PipelineCollection::downsample_pass() {
	if (downsamplePass == nullptr) {
		auto shaderPath = base_path / "shaders" / "downsample.comp.spv";

		std::vector<vk::DescriptorSetLayoutBinding> bindings{
				vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute),
				vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute)
		};

		std::vector<std::vector<vk::DescriptorSetLayoutBinding>> bindingsVector = {{bindings}};

		downsamplePass = std::make_unique<ComputePipeline>(instance, shaderPath, bindingsVector);
	}

	return downsamplePass;
}

const std::unique_ptr<ComputePipeline> &PipelineCollection::query_pass() {
	if (queryPass == nullptr) {
		auto shaderPath = base_path / "shaders" / "query.comp.spv";

		std::vector<vk::DescriptorSetLayoutBinding> matricesBindings {
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute)
		};

		std::vector<vk::DescriptorSetLayoutBinding> instanceBindings {
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute),
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute)
		};

		std::vector<vk::DescriptorSetLayoutBinding> queryBindings {
			vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eCompute),
			vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute),
		};

		std::vector<std::vector<vk::DescriptorSetLayoutBinding>> bindingsVector = {{ matricesBindings, instanceBindings, queryBindings }};

		std::vector<vk::PushConstantRange> pushConstants {
			vk::PushConstantRange( vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t))
		};

		queryPass = std::make_unique<ComputePipeline>(instance, shaderPath, bindingsVector, pushConstants);
	}

	return queryPass;
}
