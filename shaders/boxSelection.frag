#version 410 core

out vec4 FragColor;

uniform vec4 boxColor;

void main()
{
    // Setam culoarea primita prin uniform
    FragColor = boxColor;
}
