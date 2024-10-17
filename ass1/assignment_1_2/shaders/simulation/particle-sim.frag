#version 410
#extension GL_ARB_explicit_uniform_location : enable

uniform sampler2D previousPositions;
uniform sampler2D previousVelocities;
uniform sampler2D previousBounceData;
uniform float timestep;
uniform uint numParticles;
uniform float particleRadius;
uniform vec3 containerCenter;
uniform float containerRadius;
uniform bool interParticleCollision;

layout(location = 0) out vec3 finalPosition;
layout(location = 1) out vec3 finalVelocity;
layout(location = 2) out vec3 finalBounceData;

void main() {
    // ===== Task 1.1 Verlet Integration =====
    // Compute the particle index based on fragment coordinates
    int particleIndex = int(gl_FragCoord.x - 0.5);

    // Fetch the previous position and velocity from textures
    vec3 previousPosition = texelFetch(previousPositions, ivec2(particleIndex, 0), 0).rgb;
    vec3 previousVelocity = texelFetch(previousVelocities, ivec2(particleIndex, 0), 0).rgb;

    // Acceleration due to gravity
    vec3 acceleration = vec3(0.0, -9.81, 0.0);

    // Velocity Verlet Integration
    vec3 newVelocity = previousVelocity + acceleration * timestep;
    vec3 newPosition = previousPosition + previousVelocity * timestep + 0.5 * acceleration * timestep * timestep;

    finalPosition = newPosition;
    finalVelocity = newVelocity;
    finalBounceData = vec3(0.0); // Initialize bounce data to zero
    // ===== Task 1.3 Inter-particle Collision =====
    // if (interParticleCollision) {
    // }

    // ===== Task 1.2 Container Collision =====


}