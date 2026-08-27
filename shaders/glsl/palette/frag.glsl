#version 450

layout(location = 2) in float vFaceId;
layout(location = 0) out vec4 outColor;

void main() {
    float shade = 0.25 + 0.3 * fract(vFaceId * (0.6180339887)); // golden ratio
    outColor = vec4(vec3(shade), 1.0);
}
