#version 410 core

uniform vec3 viewPos;
uniform sampler2D texShadow;

uniform mat4 lightMVP;
uniform vec3 lightPos;

uniform sampler2D texLight;

// Output for on-screen color.
layout(location = 0) out vec4 outColor;

// Interpolated output data from vertex shader.
in vec3 fragPos; // World-space position
in vec3 fragNormal; // World-space normal

void main()
{
    // Output the normal as color.
    vec3 lightDir = normalize(lightPos - fragPos);

    vec4 fragLightCoord = lightMVP * vec4(fragPos, 1.0);

    // Divide by w because fragLightCoord are homogeneous coordinates
    fragLightCoord.xyz /= fragLightCoord.w;

    // The resulting value is in NDC space (-1 to +1),
    //  we transform them to texture space (0 to 1).
    fragLightCoord.xyz = fragLightCoord.xyz * 0.5 + 0.5;

    // Depth of the fragment with respect to the light
    float fragLightDepth = fragLightCoord.z;

    // Shadow map coordinate corresponding to this fragment
    vec2 shadowMapCoord = fragLightCoord.xy;

    // Shadow map value from the corresponding shadow map position
    float shadowMapDepth = texture(texShadow, shadowMapCoord).x;

    // Shadow bias
    float bias = 0.005;

    // PCF: Percentage-closer filtering
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(texShadow, 0);
    for (int x = -5; x <= 5; ++x)
    {
        for (int y = -5; y <= 5; ++y)
        {
            float pcfDepth = texture(texShadow, shadowMapCoord + vec2(x, y) * texelSize).r;
            shadow += fragLightDepth - bias > pcfDepth ? 0.0 : 1.0;
        }
    }

    shadow /= 121.0;

    // Calculate the distance from the coordinate to the center of the shadow map
    float distanceToCenter = length(shadowMapCoord - vec2(0.5, 0.5));

    // Set a linear light multiplier that is 1 at the center and decreases to 0 at a distance of 0.5 or more
    float lightMultiplier = 1.0 - clamp(distanceToCenter / 0.5, 0.0, 1.0);

    // Apply the light multiplier to the shadow value to achieve the gradual dimming effect
    shadow *= lightMultiplier;

    // Apply lighting and shadow to the output color
    outColor = vec4(vec3(max(dot(fragNormal, lightDir), 0.0)), 1.0) * shadow;
}
