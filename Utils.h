#ifndef VKOCCLUSIONTEST_UTILS_H
#define VKOCCLUSIONTEST_UTILS_H

#include <filesystem>
#include <vector>
#include <vulkan/vulkan.hpp>

std::vector<char> readFile(const std::filesystem::path& path);

template<typename SourceT, typename TargetT>
std::vector<TargetT> reinterpret_data(const std::vector<SourceT>& data) {
	if ((data.size() * sizeof(SourceT)) % sizeof(TargetT) != 0) {
		throw std::runtime_error("Could not convert _data type");
	}

	auto nextSize = (data.size() * sizeof(SourceT));

	std::vector<uint32_t> rvl(nextSize / sizeof(TargetT));
	std::memcpy(rvl.data(), data.data(), nextSize);

	return rvl;
}

vk::ShaderModule createShaderModule(const std::filesystem::path& path, const vk::Device& device);

#endif //VKOCCLUSIONTEST_UTILS_H
