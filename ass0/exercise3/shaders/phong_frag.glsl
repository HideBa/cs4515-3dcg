#version 410

// Global variables for lighting calculations
uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 ks;
uniform vec3 kd;
uniform float shininess;

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    vec3 normal = normalize(fragNormal);

    vec3 lightDir = normalize(lightPos - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * kd * lightColor;

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    float specAngle = max(dot(reflectDir, viewDir), 0.0);
    float specularStrength = pow(specAngle, shininess);
    vec3 specular = specularStrength * lightColor * ks;

    vec3 finalColor = diffuse + specular;
    outColor = vec4(finalColor, 1.0);
}