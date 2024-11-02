#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 color;

out vec3 FragColor;
// out vec3 FragPosition;
// out vec3 FragNormal;

void main() {
    gl_Position = projection * view * model * vec4( position, 1.0);
    // FragPosition = vec3(model * vec4(position, 1.0));
    // FragNormal = mat3(transpose(inverse(model))) * aNormal;
    FragColor = color;
}
