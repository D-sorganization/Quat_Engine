#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;
in vec2 vUV;

// Directional light
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform vec3 uCameraPos;

// Texture
uniform sampler2D uTexture0;
uniform int uUseTexture;

// Point lights (up to 8 for dynamic effects)
#define MAX_POINT_LIGHTS 8
uniform vec3 uPointLightPos[MAX_POINT_LIGHTS];
uniform vec3 uPointLightColor[MAX_POINT_LIGHTS];
uniform float uPointLightRadius[MAX_POINT_LIGHTS];
uniform int uPointLightCount;

// Emission (for glowing objects: power-ups, projectiles, hit flash)
uniform vec3 uEmission;
uniform float uEmissionStrength;

// Fog
uniform float uFogNear;
uniform float uFogFar;
uniform vec3 uFogColor;

// Time (for animated effects)
uniform float uTime;

// Fresnel rim light
uniform float uRimPower;
uniform vec3 uRimColor;

out vec4 FragColor;

vec3 compute_point_light(vec3 lightPos, vec3 lightColor, float lightRadius,
                         vec3 normal, vec3 worldPos, vec3 viewDir, vec3 baseColor) {
    vec3 toLight = lightPos - worldPos;
    float dist = length(toLight);
    if (dist > lightRadius) return vec3(0.0);

    vec3 lightDir = toLight / dist;
    float attenuation = 1.0 - smoothstep(0.0, lightRadius, dist);
    attenuation *= attenuation; // Quadratic falloff

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);

    // Specular (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 64.0);

    return (diff * baseColor + spec * 0.6) * lightColor * attenuation;
}

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);

    // Diffuse (Lambert)
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;

    // Specular (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = spec * uLightColor * 0.5;

    // Base color: texture or vertex color
    vec3 baseColor = vColor;
    if (uUseTexture > 0) {
        vec3 texColor = texture(uTexture0, vUV).rgb;
        baseColor = texColor * vColor;
    }

    // Directional lighting
    vec3 lighting = uAmbient + diffuse + specular;
    vec3 result = baseColor * lighting;

    // Point lights contribution
    for (int i = 0; i < uPointLightCount && i < MAX_POINT_LIGHTS; ++i) {
        result += compute_point_light(
            uPointLightPos[i], uPointLightColor[i], uPointLightRadius[i],
            normal, vWorldPos, viewDir, baseColor);
    }

    // Emission (additive glow)
    result += uEmission * uEmissionStrength;

    // Fresnel rim lighting (highlights edges for sci-fi feel)
    if (uRimPower > 0.0) {
        float rim = 1.0 - max(dot(viewDir, normal), 0.0);
        rim = pow(rim, uRimPower);
        result += uRimColor * rim * 0.5;
    }

    // Distance fog
    float fogDist = length(uCameraPos - vWorldPos);
    float fogNear = uFogNear > 0.0 ? uFogNear : 25.0;
    float fogFar = uFogFar > 0.0 ? uFogFar : 75.0;
    float fogFactor = clamp(1.0 - (fogDist - fogNear) / (fogFar - fogNear), 0.0, 1.0);
    vec3 fogColor = uFogColor.r > 0.001 ? uFogColor : vec3(0.03, 0.03, 0.08);
    result = mix(fogColor, result, fogFactor);

    // HDR-ish tone mapping (simple Reinhard)
    result = result / (result + vec3(1.0));

    FragColor = vec4(result, 1.0);
}
