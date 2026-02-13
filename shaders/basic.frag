#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;

uniform vec3 uLightDir;     // Directional light direction (normalized)
uniform vec3 uLightColor;   // Light color
uniform vec3 uAmbient;      // Ambient light color
uniform vec3 uCameraPos;    // Camera position for specular

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);

    // Diffuse lighting (Lambert)
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;

    // Specular lighting (Blinn-Phong)
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = spec * uLightColor * 0.5;

    // Final color
    vec3 lighting = uAmbient + diffuse + specular;
    vec3 result = vColor * lighting;

    // Fog: fade to dark blue-grey at distance
    float fogDist = length(uCameraPos - vWorldPos);
    float fogFactor = clamp(1.0 - (fogDist - 20.0) / 40.0, 0.0, 1.0);
    vec3 fogColor = vec3(0.05, 0.05, 0.1);
    result = mix(fogColor, result, fogFactor);

    FragColor = vec4(result, 1.0);
}
