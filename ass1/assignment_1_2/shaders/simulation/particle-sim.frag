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
uniform int bounceThreshold;
uniform int bounceFrames;

layout(location = 0) out vec3 finalPosition;
layout(location = 1) out vec3 finalVelocity;
layout(location = 2) out vec3 finalBounceData;

void main() {
    // ===== Task 1.1 Verlet Integration =====
    int particleIndex = int(gl_FragCoord.x - 0.5);

    // Fetch the previous position and velocity from textures
    vec3 previousPosition = texelFetch(previousPositions, ivec2(particleIndex, 0), 0).rgb;
    vec3 previousVelocity = texelFetch(previousVelocities, ivec2(particleIndex, 0), 0).rgb;

    // Acceleration due to gravity
    vec3 acceleration = vec3(0.0, -9.81, 0.0);

    // Velocity Verlet Integration
    vec3 newVelocity = previousVelocity + acceleration * timestep;
    vec3 newPosition = previousPosition + previousVelocity * timestep + 0.5 * acceleration * timestep * timestep;

    vec3 previouseBounceData = texelFetch(previousBounceData, ivec2(particleIndex, 0), 0).rgb;
    float collisionCount = previouseBounceData.r;
    float frameCount = previouseBounceData.g;

    float newCollisionCount = collisionCount;
    float newFrameCount = frameCount;

    // ===== Task 1.3 Inter-particle Collision =====
    if (interParticleCollision) {
        for (uint i = 0u; i < numParticles; i++ ) {
            if(i == particleIndex) continue;
            vec3 otherPosition = texelFetch(previousPositions, ivec2(int(i), 0), 0).rgb;
            vec3 delta = newPosition - otherPosition;
            float distance = length(delta);
            float minDistance = 2.0 * particleRadius;
            if(distance < minDistance) {
                //Collision detected
                float overlap = (minDistance - distance) * 0.5;
                vec3 normal = delta / distance;
                float eps = 0.001;
                newPosition += normal * (overlap + eps);
                float velocityAlongNormal = dot(newVelocity, normal);
                newVelocity -= 2.0 * normal * velocityAlongNormal;
                newCollisionCount++;
            }
        }
    }

    // ===== Task 1.2 Container Collision =====
    vec3 centerToParticle = newPosition - containerCenter ;
    float distance = length(centerToParticle);
    if (distance + particleRadius > containerRadius) {
        float overlap = distance + particleRadius - containerRadius;
        vec3 normal = centerToParticle / distance;
        float eps = 0.001;
        newPosition -= normal * (overlap + eps);
        float velocityAlongNormal = dot(newVelocity, normal);
        newVelocity -= 2.0 * normal * velocityAlongNormal;
        newCollisionCount++;
    }

    if (newCollisionCount > bounceThreshold) {
     newCollisionCount = 0;
     newFrameCount = float(bounceFrames);
    }
    newFrameCount = max(newFrameCount - 1.0, 0.0);

    finalPosition = newPosition;
    finalVelocity = newVelocity;
    finalBounceData = vec3(newCollisionCount, newFrameCount, 0.0);

}
