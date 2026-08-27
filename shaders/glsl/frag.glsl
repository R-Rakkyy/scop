#version 450

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in float vFaceId; /* unused here but keeps the vertex-shader interface identical to the palette pipeline */
layout(location = 3) in vec3 vViewDir;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D diffuseSampler;
layout(set = 0, binding = 1) uniform sampler2D bumpSampler;
layout(set = 0, binding = 2) uniform Material {
    vec4 kd;
    vec4 ks_ns; /* .rgb = Ks, .a = Ns */
} mat;

/* Same range vert.glsl declares (both stages share one push-constant
 * range)
 * note: only camPos.w (g_lighting_enabled) is actually used in this fragment shader.*/
layout(push_constant) uniform Push {
    mat4 mvp;
    mat4 model;
    vec4 camPos;
} pc;

const vec3  LIGHT_DIR   = normalize(vec3(0.45, 0.85, 0.35));  // key light direction
const float AMBIENT     = 0.35; // ambient light fraction of the key light's intensity

void main() {
    vec3 albedo = texture(diffuseSampler, vUV).rgb * mat.kd.rgb;

    /* g_lighting_enabled is in the camPos.w component of the push-constant */
    if (pc.camPos.w < 0.5) {
        outColor = vec4(albedo, 1.0);
        return;
    }

    /* No tangent-space normal mapping (would need a tangent vertex
     * attribute): the bump/normal map's red channel is instead used as a
     * cheap surface-detail multiplier so it still visibly contributes. */
    float detail = mix(0.85, 1.15, texture(bumpSampler, vUV).r);

    vec3 n = normalize(vNormal);
    float diffuse = max(dot(n, LIGHT_DIR), 0.0);
    float lighting = AMBIENT + diffuse * (1.0 - AMBIENT);

    vec3 color = albedo * detail * lighting;
    if (diffuse > 0.0) {
        vec3 viewDir = normalize(vViewDir);
        vec3 halfVec = normalize(viewDir + LIGHT_DIR);

        float spec = pow(max(dot(n, halfVec), 0.0), max(mat.ks_ns.a, 1.0)); // specular highlight
        color += mat.ks_ns.rgb * spec;
    }

    outColor = vec4(color, 1.0);
}
