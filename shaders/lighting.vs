#version 330 core

// === ATRYBUTY Z RAYLIB ===
in vec3 vertexPosition;
in vec3 vertexNormal;

// === MACIERZE Z RAYLIB ===
uniform mat4 mvp;
uniform mat4 matModel;

// === DO FRAGMENT SHADERA ===
out vec3 fragPosition;
out vec3 fragNormal;

void main()
{
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragNormal = mat3(transpose(inverse(matModel))) * vertexNormal;

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}