#version 410 core
layout(location = 0) in vec3 position;

uniform float time;
uniform mat4 model;
uniform mat4 view; 
uniform mat4 projection;

uniform float amplitude1;
uniform float frequency1;
uniform vec2 direction1;
uniform float speed1;

uniform float amplitude2;
uniform float frequency2;
uniform vec2 direction2;
uniform float speed2;

void main() {
    vec3 pos = position;

    // Calculate displacement for each wave
    float wave1 = amplitude1 * sin(dot(direction1, pos.xz) * frequency1 + time * speed1);
    float wave2 = amplitude2 * sin(dot(direction2, pos.xz) * frequency2 + time * speed2);

    pos.y += wave1 + wave2;


    gl_Position = projection * view * model * vec4(pos, 1.0);
}
