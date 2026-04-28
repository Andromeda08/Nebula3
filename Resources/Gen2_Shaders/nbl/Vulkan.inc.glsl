#ifndef nbl_VULKAN_INC_GLSL
#define nbl_VULKAN_INC_GLSL

// Vulkan struct equivalent
struct DrawIndexedIndirectCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int  vertexOffset;
    uint firstInstance;
};

struct AccelerationStructureInstanceKHR
{
    float    transform[12];             // vk::TransformMatrixKHR
    uint     instanceCustomIndex_Mask;  // (24 bits, 8 bits)
    uint     sbtRecordOffset_Flags;     // (24 bits, 8 bits)
    uint64_t blasReference;
};

#endif
