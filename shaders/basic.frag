#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;
in vec2 vUV;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform vec3 uCameraPos;
uniform sampler2D uTexture0;
uniform int uUseTexture;    // 0 = vertex color only, 1 = texture * color

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);

    // Diffuse (Lambert)
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;

    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = spec * uLightColor * 0.5;

    // Base color: texture or vertex color
    vec3 baseColor = vColor;
    if (uUseTexture > 0) {
        vec3 texColor = texture(uTexture0, vUV).rgb;
        baseColor = texColor * vColor;
    }

    // Lighting
    vec3 lighting = uAmbient + diffuse + specular;
    vec3 result = baseColor * lighting;

    // Distance fog
    float fogDist = length(uCameraPos - vWorldPos);
    float fogFactor = clamp(1.0 - (fogDist - 25.0) / 50.0, 0.0, 1.0);
    vec3 fogColor = vec3(0.03, 0.03, 0.08);
    result = mix(fogColor, result, fogFactor);

    FragColor = vec4(result, 1.0);
}
