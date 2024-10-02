#version 410

// Global variables for lighting calculations
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 kd;
uniform vec3 ks;
uniform vec3 lightPos;
uniform float shininess;
// uniform float toonSpecularThreshold;

uniform int toonDiscretize;

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    // vec3 norm = normalize(fragNormal);
    // vec3 lightDir = normalize(lightPos - fragPos);
    // vec3 viewDir = normalize(viewPos - fragPos);

    // float diff = max(dot(norm, lightDir), 0.0);
    // diff = floor(diff * toonDiscretize) / toonDiscretize;
    // vec3 diffuse = diff * kd * lightColor;

    // vec3 halfwayDir = normalize(lightDir + viewDir);

    // float specAngle = max(dot(norm, halfwayDir), 0.0);
    // float spec = pow(specAngle, shininess);
    // spec = floor(spec * toonDiscretize) / toonDiscretize;  // Apply quantization to specular
    // vec3 specular = spec * lightColor * ks;
    // Apply toon shading to specular: white if above threshold, otherwise black
    // vec3 specular = vec3(0.0);
    // if (spec >= toonSpecularThreshold)
    // {
    //     specular = vec3(1.0); // Set to white if above threshold
    // }
    // else
    // {
    //     specular = vec3(0.0); // Set to black if below threshold
    // }


    // vec3 result = diffuse + specular;
    // outColor = vec4(result, 1.0);



    vec3 normal = normalize(fragNormal);

    vec3 lightDir = normalize(lightPos - fragPos);

    vec3 viewDir = normalize(viewPos - fragPos);

    vec3 halfDir = normalize(lightDir + viewDir);

    float diff = max(dot(normal, lightDir), 0.0);
    // diff = floor(diff * toonDiscretize) / toonDiscretize;

    vec3 diffuse = diff * kd * lightColor;

    float specAngle = max(dot(normal, halfDir), 0.0);
    float specularStrength = pow(specAngle, shininess);
    vec3 specular = specularStrength * lightColor * ks;

    vec3 finalColor = diffuse + specular;
    outColor = vec4(finalColor, 1.0);
}
