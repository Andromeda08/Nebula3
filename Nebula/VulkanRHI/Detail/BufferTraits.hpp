#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace RHI
{
    enum class BufferUsage
    {
        Compute,
        Compute_Read,
        Compute_Write,
        Fragment,
        Fragment_ReadWrite,
        AS_BuildUpdate,
        AS_Traverse,
        SBT_Read,
        TransferSrc,
        TransferDst,
        DrawIndirect,
        Vertex,
        Index,
        StorageRead,
        All,
    };

    struct BufferState
    {
        vk::AccessFlags2        access;
        vk::PipelineStageFlags2 stage;
    };

    [[nodiscard]] constexpr BufferState getBarrierFlagsForBufferUsage(const BufferUsage bufferUsage)
    {
        using A = vk::AccessFlagBits2;
        using S = vk::PipelineStageFlagBits2;

        using enum BufferUsage;
        switch (bufferUsage)
        {
            case Compute: {
                return { A::eShaderRead | A::eShaderWrite, S::eComputeShader };
            }
            case Compute_Read: {
                return { A::eShaderRead, S::eComputeShader };
            }
            case Compute_Write: {
                return { A::eShaderWrite, S::eComputeShader };
            }
            case Fragment: {
                return { A::eShaderRead, S::eFragmentShader};
            }
            case Fragment_ReadWrite: {
                return { A::eShaderRead | A::eShaderWrite, S::eFragmentShader};
            }
            case AS_BuildUpdate: {
                return { A::eAccelerationStructureWriteKHR, S::eAccelerationStructureBuildKHR | S::eAccelerationStructureCopyKHR };
            }
            case AS_Traverse: {
                return { A::eAccelerationStructureReadKHR, S::eAllGraphics };
            }
            case SBT_Read: {
                return { A::eShaderBindingTableReadKHR, S::eAllGraphics };
            }
            case TransferSrc: {
                return { A::eTransferRead, S::eAllTransfer };
            }
            case TransferDst: {
                return { A::eTransferWrite, S::eAllTransfer };
            }
            case DrawIndirect: {
                return { A::eIndirectCommandRead, S::eDrawIndirect };
            }
            case Vertex: {
                return { A::eVertexAttributeRead, S::eVertexInput };
            }
            case Index: {
                return { A::eIndexRead, S::eVertexInput };
            }
            case StorageRead: {
                return { A::eShaderStorageRead, S::eAllGraphics };
            }
            case All: {
                return { A::eShaderWrite | A::eShaderWrite, S::eAllCommands };
            }
            default: {
                exitWithError("Unhandled BufferUsage case");
            }
        }
    }

    enum class BufferType
    {
        Index,
        Vertex,
        Indirect,
        Storage,
        Uniform,
        AccelerationStructure,
        ShaderBindingTable,
        Staging,
    };

    /**
     * Get the buffer usage flags for a specific BufferType.
     * @param bufferType
     * @param hasAccelerationStructureFeatures For marking Vertex,Index and Storage buffers for ASBuildInput usage.
     */
    [[nodiscard]] inline vk::BufferUsageFlags getBufferUsageFlags(const BufferType bufferType, const bool hasAccelerationStructureFeatures = false) noexcept
    {
        using enum vk::BufferUsageFlagBits;
        vk::BufferUsageFlags result = eTransferSrc | eTransferDst | eShaderDeviceAddress;

        switch (bufferType)
        {
            case BufferType::Index:{
                result |= eIndexBuffer | eStorageBuffer;
                if (hasAccelerationStructureFeatures)
                {
                    result |= eAccelerationStructureBuildInputReadOnlyKHR;
                }
                break;
            }
            case BufferType::Vertex:{
                result |= eVertexBuffer | eStorageBuffer;
                if (hasAccelerationStructureFeatures)
                {
                    result |= eAccelerationStructureBuildInputReadOnlyKHR;
                }
                break;
            }
            case BufferType::Indirect:{
                result |= eIndirectBuffer;
                break;
            }
            case BufferType::Storage: {
                result |= eStorageBuffer;
                if (hasAccelerationStructureFeatures)
                {
                    result |= eAccelerationStructureBuildInputReadOnlyKHR;
                }
                break;
            }
            case BufferType::Uniform: {
                result |= eUniformBuffer;
                break;
            }
            case BufferType::AccelerationStructure: {
                result |= eAccelerationStructureStorageKHR;
                break;
            }
            case BufferType::ShaderBindingTable: {
                result |= eShaderBindingTableKHR;
                break;
            }
            case BufferType::Staging: {
                break;
            }
        }

        return result;
    }

    // Get the VMA memory allocation flags for a specific BufferType.
    [[nodiscard]] constexpr int32_t getBufferMemoryFlags(const BufferType bufferType)
    {
        switch (bufferType)
        {
            case BufferType::ShaderBindingTable:
            case BufferType::Staging:
            case BufferType::Uniform:
                return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                       | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
                       | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            default:
                return 0;
        }
    }

    [[nodiscard]] constexpr bool isMappableBufferMemory(const BufferType bufferType)
    {
        return bufferType == BufferType::ShaderBindingTable
            || bufferType == BufferType::Staging
            || bufferType == BufferType::Uniform;
    }

    struct BufferProperties
    {
        vk::DeviceSize  size;
        BufferType      type;
        bool            isMappable;
    };
}
