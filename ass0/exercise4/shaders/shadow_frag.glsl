#version 410 core

// Model/view/projection matrix
uniform mat4 mvp;
uniform mat4 lightSpaceMatrix;

// Per-vertex attributes
in vec3 pos; // World-space position
in vec3 normal; // World-space normal

void main() {
	// Transform 3D position into on-screen position
    // gl_Position = mvp * vec4(pos, 1.0);
    gl_Position = lightSpaceMatrix * vec4(pos,1.0);

}