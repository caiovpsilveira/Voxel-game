#ifndef RENDERER_UTILS_HPP
#define RENDERER_UTILS_HPP

// libs
#include <vulkan/vulkan.hpp>

// std
#include <filesystem>
#include <span>
#include <vector>

namespace renderer::utils
{

bool containsExtension(std::span<const char* const> extensionsList, const char* extensionName) noexcept;

bool containsExtension(std::span<const vk::ExtensionProperties> extensionsPropertiesList,
                       const char* extensionName) noexcept;

std::vector<char> readFile(const std::filesystem::path& path);

vk::UniqueShaderModule createUniqueShaderModule(vk::Device device, std::span<const char> code);

void transitionImage(vk::CommandBuffer cmd,
                     vk::Image image,
                     vk::ImageLayout currentLayout,
                     vk::ImageLayout newLayout) noexcept;

}   // namespace renderer::utils

#endif
