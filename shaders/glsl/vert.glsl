#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in float inFaceId;

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out float vFaceId;
layout(location = 3) out vec3 vViewDir;

// known issue: i went above the 128-byte limit for push constants.
// shouldn't work on old machine or mobile.
layout(push_constant) uniform Push {
    mat4 mvp;
    mat4 model;
    vec4 camPos;
} pc;

void main() {
    gl_Position = pc.mvp * vec4(inPos, 1.0);
    vUV         = inUV;
    vNormal     = mat3(pc.model) * inNormal;
    vFaceId     = inFaceId;

    vec3 worldPos = (pc.model * vec4(inPos, 1.0)).xyz;
    vViewDir      = pc.camPos.xyz - worldPos;
}
