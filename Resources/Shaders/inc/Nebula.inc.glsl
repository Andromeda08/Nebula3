struct CameraData
{
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  eye;
    vec4  frustumPlanes[6];
    float nearPlane;
    float farPlane;
};

struct GPUInstanceData
{
    mat4     model;
    vec4     solidColor;
    int      textureIndex;
    int      geometryIndex;
    uint64_t blasAddress;
    int      normalIndex;
    int      _p0, _p1, _p2;
    vec4     min;
    vec4     max;
};
