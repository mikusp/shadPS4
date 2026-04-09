// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/profile.h"
#include "shader_recompiler/runtime_info.h"
#include "video_core/renderer_vulkan/vk_common.h"

#include <boost/container/small_vector.hpp>

namespace Shader {
struct Info;
struct PushData;
} // namespace Shader

namespace Vulkan {

class Instance;
class Scheduler;
class DescriptorHeap;

class ShaderObject {
public:
    ShaderObject(const Instance& instance, Scheduler& scheduler, DescriptorHeap& desc_heap,
             const Shader::Profile& profile, vk::PipelineCache pipeline_cache,
             bool is_compute = false);
    virtual ~ShaderObject();

    vk::ShaderEXT Handle() const noexcept {
        return *shader_object;
    }

    vk::PipelineLayout GetLayout() const noexcept {
        return *pipeline_layout;
    }

    // auto GetStages() const {
    //     static_assert(static_cast<u32>(Shader::LogicalStage::Compute) == Shader::MaxStageTypes - 1);
    //     if (is_compute) {
    //         return std::span{stages.cend() - 1, stages.cend()};
    //     } else {
    //         return std::span{stages.cbegin(), stages.cend() - 1};
    //     }
    // }

    const Shader::Info& GetInfo() const noexcept {
        return *info;
    }

    bool IsCompute() const {
        return is_compute;
    }

    using DescriptorWrites = boost::container::small_vector<vk::WriteDescriptorSet, 16>;
    using BufferBarriers = boost::container::small_vector<vk::BufferMemoryBarrier2, 16>;

    void BindResources(DescriptorWrites& set_writes, const BufferBarriers& buffer_barriers,
                       const Shader::PushData& push_data) const;

protected:
    [[nodiscard]] std::string GetDebugString() const;

    const Instance& instance;
    Scheduler& scheduler;
    DescriptorHeap& desc_heap;
    const Shader::Profile& profile;
    vk::UniqueShaderEXT shader_object;
    vk::UniquePipelineLayout pipeline_layout;
    vk::UniqueDescriptorSetLayout desc_layout;
    const Shader::Info* info{};
    bool uses_push_descriptors{};
    bool is_compute;
};

} // namespace Vulkan
