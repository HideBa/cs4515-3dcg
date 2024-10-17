#version 410

uniform vec3 containerCenter;

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragVelocity;
layout(location = 3) in vec3 fragBounceData;

uniform vec3 particleColorMin;
uniform vec3 particleColorMax;
uniform bool doSpeedBasedColor;
uniform float maxSpeed;
uniform bool shading;
uniform float ambientCoefficient;
uniform vec3 lightPos;

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 baseColor = vec3(1.0);

    // ===== Task 2.1 Speed-based Colors =====
    vec3 finalColor = baseColor;
    if (doSpeedBasedColor) {
        float speed = length(fragVelocity);
        float t = clamp(speed / maxSpeed, 0.0, 1.0);
        finalColor = mix(particleColorMin, particleColorMax, t);
    }

    // ===== Task 2.2 Shading =====
    if (shading) {
        vec3 ambient = ambientCoefficient * finalColor;
        vec3 normal = normalize(fragNormal);
        vec3 lightDir = normalize(lightPos - fragPosition);
        float diffuseIntensity = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diffuseIntensity * finalColor;
        finalColor = ambient + diffuse;
    }

    fragColor = vec4(finalColor, 1.0);
}
