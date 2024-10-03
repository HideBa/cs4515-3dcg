#version 410 core

// Global variables for lighting calculations
uniform vec3 viewPos;      // Camera position in world space
uniform vec3 lightPos;     // Light position in world space
uniform vec3 lightColor;   // Light color
uniform vec3 kd;           // Diffuse reflectivity
uniform vec3 ks;           // Specular reflectivity
uniform int toonDiscretize;     // Number of levels for toon shading
uniform float toonSpecularThreshold; // Threshold for toon specular highlight
uniform int mode;          // Shading mode: 0 - debug, 1 - lambert, 2 - toon, 3 - x-toon

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

    // Switch between different shading modes
    switch (mode)
    {

        case 1: // Lambert
        {
            // Lambertian diffuse shading
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = diff * kd * lightColor;
            color = diffuse;
            break;
        }

        case 2: // Toon
        {
            // Toon shading with discrete diffuse levels
            float diff = max(dot(normal, lightDir), 0.0);

            diff = floor(diff * toonDiscretize) / toonDiscretize;
            vec3 diffuse = diff * kd * lightColor;
            color = diffuse;
            break;
        }

        case 3: // X-Toon
        {
            // X-Toon shading combines toon diffuse and specular highlights

            // Toon diffuse component
            float diff = max(dot(normal, lightDir), 0.0);
            // Apply a non-linear scaling to enhance the diffuse in x-toon shading
            float toonShade = pow(floor(diff * float(toonDiscretize)) / float(toonDiscretize), 0.5);
            vec3 diffuse = toonShade * kd * lightColor;

            // Toon specular component
            vec3 halfDir = normalize(lightDir + viewDir);
            float specAngle = max(dot(normal, halfDir), 0.0);

            // Apply a threshold for specular highlight
            float specularStep = step(toonSpecularThreshold, specAngle);
            // Enhance the specular highlight in x-toon shading by raising it to the power of 2
            vec3 specular = pow(specularStep, 2.0) * ks * lightColor;

            // Combine diffuse and specular
            color = diffuse + specular;
            break;
        }

        default:
            color = vec3(1.0); // Default to white color if mode is invalid
            break;
    }

    outColor = vec4(color, 1.0);
}
