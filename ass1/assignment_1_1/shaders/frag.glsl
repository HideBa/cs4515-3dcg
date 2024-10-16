#version 410 core

// Global variables for lighting calculations
uniform vec3 viewPos;                // Camera position in world space
uniform vec3 lightPos;               // Light position in world space
uniform vec3 lightColor;             // Light color
uniform vec3 kd;                     // Diffuse reflectivity
uniform vec3 ks;                     // Specular reflectivity
uniform float shininess;             // Shininess factor for specular highlight
uniform int toonDiscretize;          // Number of levels for toon shading
uniform float toonSpecularThreshold; // Threshold for toon specular highlight

uniform int diffuseMode;             // Diffuse shading mode: 0 - debug, 1 - lambert, 2 - toon, 3 - x-toon
uniform int specularMode;            // Specular shading mode: 0 - none, 1 - phong, 2 - blinn-phong, 3 - toon

// Shadow mapping uniforms
uniform sampler2D shadowMap;
uniform mat4 lightMVP;
uniform int shadows; // 0 or 1
uniform int pcf;     // 0 or 1
uniform int lightMode; // 0 or 1

// Output for on-screen color
out vec4 outColor;

// Interpolated output data from vertex shader
in vec3 fragPos;     // World-space position
in vec3 fragNormal;  // World-space normal
void computeShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, out float shadow, out float attenuation) {
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Check if fragment is outside the light's view
    if (projCoords.z > 1.0) {
        shadow = 0.0;
        attenuation = 1.0;
        return;
    }

    // Get depth from shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // Get current fragment depth
    float currentDepth = projCoords.z;

    // Shadow map coordinate
    vec2 shadowMapCoord = projCoords.xy;

    // Bias to prevent shadow acne
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // Initialize shadow and attenuation
    shadow = 0.0;
    attenuation = 1.0;

    // Spotlight attenuation
    if (lightMode == 1) {
        float distFromCenter = distance(shadowMapCoord, vec2(0.5));
        float radius = 0.5;
        attenuation = clamp(1.0 - distFromCenter / radius, 0.0, 1.0);
        // Apply a falloff curve
        attenuation = pow(attenuation, 2.0); // Adjust exponent for sharper edges
    }

    if (pcf == 1) {
        // PCF
        float shadowSamples = 4.0;
        float offset = 1.0 / 1024.0; // Assuming shadow map size is 1024x1024
        float shadowSum = 0.0;
        for (float x = -1.5; x <= 1.5; x += 1.0)
        {
            for (float y = -1.5; y <= 1.5; y += 1.0)
            {
                float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * offset).r;
                shadowSum += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow = shadowSum / (shadowSamples * shadowSamples);
    }
    else {
        // Basic shadow
        shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    }
}
void main()
{
    vec3 color = vec3(0.0);

    // Normalize the input vectors
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    vec3 viewDir = normalize(viewPos - fragPos);

    // Early exit for debug mode
    if (diffuseMode == 0)
    {
        // Output the normal vector as color (mapped from [-1,1] to [0,1])
        color = normal * 0.5 + 0.5;
        outColor = vec4(color, 1.0);
        return;
    }

    // Compute shadow factor and attenuation
    float shadow = 0.0;
    float attenuation = 1.0;
    if (shadows == 1) {
        vec4 fragPosLightSpace = lightMVP * vec4(fragPos, 1.0);
        computeShadow(fragPosLightSpace, normal, lightDir, shadow, attenuation);
    }

    // Compute diffuse component
    vec3 diffuse = vec3(0.0);
    if (diffuseMode == 1) // Lambert
    {
        float diff = max(dot(normal, lightDir), 0.0);
        diffuse = diff * kd * lightColor;
    }
    else if (diffuseMode == 2) // Toon
    {
        float diff = max(dot(normal, lightDir), 0.0);
        diff = floor(diff * float(toonDiscretize)) / float(toonDiscretize);
        diffuse = diff * kd * lightColor;
    }
    else if (diffuseMode == 3) // X-Toon
    {
        // X-Toon shading computes both diffuse and specular components differently
        float diff = max(dot(normal, lightDir), 0.0);
        float toonShade = pow(floor(diff * float(toonDiscretize)) / float(toonDiscretize), 0.5);
        diffuse = toonShade * kd * lightColor;

        // Compute X-Toon specular component
        vec3 halfDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(normal, halfDir), 0.0);

        // Apply a threshold for specular highlight
        float specularStep = step(toonSpecularThreshold, specAngle);
        // Enhance the specular highlight in x-toon shading
        vec3 specular = pow(specularStep, 2.0) * ks * lightColor;

        // Combine diffuse and specular, apply shadow and attenuation
        color = (diffuse + specular) * attenuation * (1.0 - shadow);
        outColor = vec4(color, 1.0);
        return;
    }

    // Compute specular component
    vec3 specular = vec3(0.0);
    if (specularMode == 1) // Phong
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        float specAngle = max(dot(reflectDir, viewDir), 0.0);
        float specularStrength = pow(specAngle, shininess);
        specular = specularStrength * ks * lightColor;
    }
    else if (specularMode == 2) // Blinn-Phong
    {
        vec3 halfDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(normal, halfDir), 0.0);
        float specularStrength = pow(specAngle, shininess);
        specular = specularStrength * ks * lightColor;
    }
    else if (specularMode == 3) // Toon Specular
    {
        vec3 halfDir = normalize(lightDir + viewDir);
        float specAngle = max(dot(normal, halfDir), 0.0);

        // Apply threshold to create sharp specular highlights
        float specularStep = step(toonSpecularThreshold, specAngle);
        specular = specularStep * ks * lightColor;
    }

    // Combine diffuse and specular, apply shadow and attenuation
    color = (diffuse + specular) * attenuation * (1.0 - shadow);

    outColor = vec4(color, 1.0);
}
