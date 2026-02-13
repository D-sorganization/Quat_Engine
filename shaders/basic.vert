#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uViewProjection;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(uModel) * aNormal;  // Simplified normal transform (ok for uniform scale)
    vColor = aColor;
    gl_Position = uViewProjection * worldPos;
}
