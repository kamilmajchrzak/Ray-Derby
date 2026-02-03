#version 330 core

in vec3 fragPosition;
in vec3 fragNormal;

uniform vec3 viewPos;
uniform vec3 lightPos[5];
uniform vec3 lightColor[5];
uniform vec4 colDiffuse;
uniform vec4 ambient;

out vec4 finalColor;

void main()
{
    vec3 norm = normalize(fragNormal);
    vec3 result = ambient.rgb * colDiffuse.rgb;

    for (int i = 0; i < 5; i++)
    {
        vec3 lightDir = normalize(lightPos[i] - fragPosition);
        float diff = max(dot(norm, lightDir), 0.0);
        result += diff * lightColor[i] * colDiffuse.rgb;
    }

    finalColor = vec4(result, colDiffuse.a);
}