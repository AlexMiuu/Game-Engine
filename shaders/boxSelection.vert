#version 410 core

layout (location = 0) in vec2 aPos;

void main()
{
    // Coordonatele sunt deja in NDC (-1 to 1), deci le pasam direct
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}
