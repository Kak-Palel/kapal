#version 330 core

layout(location = 0) in vec3 vertexPosition;

uniform mat4 mvp;
uniform mat4 matView;
uniform mat4 matProjection;
uniform mat4 matModel;

void main()
{
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
