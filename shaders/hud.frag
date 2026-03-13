#version 330 core

in vec3 vColor;
out vec4 FragColor;

uniform float uAlpha;

void main() {
    float alpha = uAlpha > 0.0 ? uAlpha : 0.8;
    FragColor = vec4(vColor, alpha);
}
