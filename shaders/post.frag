#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;

uniform float uTime;
uniform int uAberrationEnabled;
uniform float uAberrationOffset;
uniform int uCRTEnabled;
uniform int uVignetteEnabled;
uniform int uGrainEnabled;

// Generic random function for grain
float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec2 uv = TexCoords;

    // 1. CRT Screen space curving
    if (uCRTEnabled > 0) {
        uv = (uv - 0.5) * 2.0;
        uv.x *= 1.0 + pow(abs(uv.y) / 4.0, 2.0);
        uv.y *= 1.0 + pow(abs(uv.x) / 4.0, 2.0);
        uv = (uv / 2.0) + 0.5;
    }

    // Screen bound check if CRT enabled
    if (uCRTEnabled > 0 && (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 color = vec3(0.0);

    // 2. Chromatic Aberration
    if (uAberrationEnabled > 0) {
        float offset = uAberrationOffset;
        color.r = texture(screenTexture, vec2(uv.x + offset, uv.y)).r;
        color.g = texture(screenTexture, uv).g;
        color.b = texture(screenTexture, vec2(uv.x - offset, uv.y)).b;
    } else {
        color = texture(screenTexture, uv).rgb;
    }

    // 3. Scanlines
    if (uCRTEnabled > 0) {
        float scanline = sin(uv.y * 800.0 * 3.14159) * 0.04;
        color -= scanline;
    }

    // 4. Vignette
    if (uVignetteEnabled > 0) {
        float dist = distance(uv, vec2(0.5));
        dist = smoothstep(0.4, 0.9, dist);
        color *= 1.0 - (dist * 0.8);
    }

    // 5. Film Grain
    if (uGrainEnabled > 0) {
        float grain = rand(uv * uTime) * 0.05;
        color += grain;
    }

    FragColor = vec4(color, 1.0);
}
