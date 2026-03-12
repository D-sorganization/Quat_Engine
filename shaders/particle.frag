#version 330 core

in vec3 vColor;
in float vAlpha;

out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, vAlpha);
}
