#version 410

// Global variables for lighting calculations
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 kd;

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    vec3 lightDir = normalize(lightPos - fragPos);

    float diff = max(dot(fragNormal, lightDir), 0.0);

    vec3 diffuse = diff * kd * lightColor; // Lambertian reflectance
    outColor = vec4(diffuse, 1.0);
}