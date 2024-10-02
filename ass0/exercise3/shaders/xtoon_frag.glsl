#version 410

in vec3 fragNormal;
in vec3 fragPos;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform sampler2D texToon;
uniform vec3 kd;
uniform vec3 ks;
uniform float shininess;

out vec4 outColor;

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    vec3 viewDir = normalize(viewPos - fragPos);


    float diff = max(dot(normal, lightDir), 0.0);

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specAngle = max(dot(normal, halfwayDir), 0.0);
    float spec = pow(specAngle, shininess);


    vec3 diffuseColor = texture(texToon, vec2(diff, 0.0)).rgb;

    vec3 specular = spec * lightPos * ks;

    vec3 resultColor = diffuseColor * kd + specular;

    // Output the final color
    outColor = vec4(resultColor, 1.0);
}