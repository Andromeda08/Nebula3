#version 460

const ivec2 edges[12] = ivec2[](
    ivec2(0,1), ivec2(1,3), ivec2(3,2), ivec2(2,0), // bottom
    ivec2(4,5), ivec2(5,7), ivec2(7,6), ivec2(6,4), // top
    ivec2(0,4), ivec2(1,5), ivec2(3,7), ivec2(2,6)  // verticals
);

layout (set = 0, binding = 0) uniform CameraUniform {
    mat4  view;
    mat4  proj;
    mat4  viewInverse;
    mat4  projInverse;
    vec4  eye;
    vec4  frustumPlanes[6];
    float nearPlane;
    float farPlane;
} camera;

layout(push_constant) uniform PC {
    vec4 aabbMin;
    vec4 aabbMax;
};

void main() {
    int edgeIdx = gl_VertexIndex / 2;
    int vertIdx = (gl_VertexIndex % 2 == 0) ? edges[edgeIdx].x : edges[edgeIdx].y;

    vec3 pos = mix(aabbMin.xyz, aabbMax.xyz, vec3(
        (vertIdx & 1) != 0,
        (vertIdx & 2) != 0,
        (vertIdx & 4) != 0
    ));

    gl_Position = camera.proj * camera.view * vec4(pos, 1.0);
}
