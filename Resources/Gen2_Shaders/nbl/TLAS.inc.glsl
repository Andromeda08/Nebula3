#ifndef nbl_TLAS_INC_GLSL
#define nbl_TLAS_INC_GLSL

#ifdef nbl_RT
    #ifndef nbl_TLAS_SET
        #error nbl_TLAS_SET must be defined
    #endif

    layout (set = nbl_TLAS_SET, binding = 0) uniform accelerationStructureEXT topLevelAS;
#endif

#endif
