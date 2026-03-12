#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
layout(location = 3) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uViewProjection;

out vec3 vColor;
out float vAlpha;

uniform float uAlpha;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    vColor = aColor;
    vAlpha = uAlpha;
    gl_Position = uViewProjection * worldPos;
}
