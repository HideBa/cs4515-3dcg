#version 410 core

// Global variables for lighting calculations
uniform vec3 viewPos;          // Camera position in world space
uniform vec3 lightPos;         // Light position in world space
uniform vec3 lightColor;       // Light color
uniform vec3 kd;               // Diffuse reflectivity
uniform vec3 ks;               // Specular reflectivity
uniform float shininess;       // Shininess factor for specular highlight
uniform int toonDiscretize;    // Number of levels for toon shading (if needed)
uniform int mode;              // Shading mode: 0 - none, 1 - phong, 2 - blinn-phong, 3 - toon

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos;       // World-space position
in vec3 fragNormal;    // World-space normal

void main()
{
    vec3 color;

    // Normalize the input vectors
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    vec3 viewDir = normalize(viewPos - fragPos);

    // Common diffuse component
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * kd * lightColor;

    // Initialize specular component
    vec3 specular = vec3(0.0);

    // Switch between different shading modes
    switch (mode)
    {
        case 0:
        {
            color = normal;
          break;
        }
        case 1: // Phong Shading
        {
            // Calculate reflection vector
            vec3 reflectDir = reflect(-lightDir, normal);

            // Calculate specular component
            float specAngle = max(dot(reflectDir, viewDir), 0.0);
            float specularStrength = pow(specAngle, shininess);
            specular = specularStrength * ks * lightColor;

            // Final color
            color = diffuse + specular;
            break;
        }

        case 2: // Blinn-Phong Shading
        {
            // Calculate half-vector
            vec3 halfDir = normalize(lightDir + viewDir);

            // Calculate specular component
            float specAngle = max(dot(normal, halfDir), 0.0);
            float specularStrength = pow(specAngle, shininess);
            specular = specularStrength * ks * lightColor;

            // Final color
            color = diffuse + specular;
            break;
        }

        case 3: // Toon Shading
        {
            // Discretize the diffuse component
            float toonShade = floor(diff * float(toonDiscretize)) / float(toonDiscretize);
            vec3 toonDiffuse = toonShade * kd * lightColor;

            // Calculate specular component using Blinn-Phong model
            vec3 halfDir = normalize(lightDir + viewDir);
            float specAngle = max(dot(normal, halfDir), 0.0);
            float specularStrength = pow(specAngle, shininess);
            specular = specularStrength * ks * lightColor;

            // Combine diffuse and specular components
            color = toonDiffuse + specular;
            break;
        }

        default:
            color = vec3(1.0);
            break;
    }

    outColor = vec4(color, 1.0);
}
