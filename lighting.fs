#version 330

in vec3 fragPosition;
in vec3 fragNormal;

out vec4 finalColor;

uniform vec3 viewPos;

#define MAX_LIGHTS 5

uniform vec3 lightPos[MAX_LIGHTS];
uniform vec3 lightColor[MAX_LIGHTS];

uniform vec4 colDiffuse;

void main() {
    vec3 norm = normalize(fragNormal);
    vec3 result = vec3(0.0);

    for (int i = 0; i < MAX_LIGHTS; i++) {
        vec3 lightDir = normalize(lightPos[i] - fragPosition);
        float diff = max(dot(norm, lightDir), 0.0);
        result += diff * lightColor[i];
    }

    vec3 final = result * colDiffuse.rgb;
    finalColor = vec4(final, 1.0);
}