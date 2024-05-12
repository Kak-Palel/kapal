#version 330 core

out vec4 fragColor;

uniform sampler2D texture0; // You may have more textures here

void main()
{
    fragColor = texture(texture0, vec2(0.0, 0.0)); // Example texture sampling
}