#version 460

layout (location = 0) out vec2 frUV;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    frUV        = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(frUV * 2.0f - 1.0f, 1.0f, 1.0f);
}
