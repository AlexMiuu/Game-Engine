#version 410 core

// Input attributes from the vertex buffer
layout(location = 0) in vec3 vPosition;   // Vertex position
layout(location = 1) in vec3 vNormal;     // Vertex normal
layout(location = 2) in vec2 vTexCoords; // Texture coordinates

// Outputs to the fragment shader
out vec3 fNormal;       // Transformed normal
out vec4 fPosEye;       // Vertex position in eye space
out vec2 fTexCoords;    // Texture coordinates

// Uniforms for transformations
uniform mat4 model;         // Model transformation
uniform mat4 view;          // View transformation
uniform mat4 projection;    // Projection transformation
uniform mat3 normalMatrix;  // Normal matrix for normal transformation



out vec4 FragPosLightSpace; // NEW: pass to fragment shader

uniform mat4 lightSpaceMatrix; // NEW: for shadow mapping


void main() 
{

    
  
    // Transform the vertex position to eye space
    fPosEye = view * model * vec4(vPosition, 1.0f);

    // Transform the normal vector to eye space
    fNormal = normalize(normalMatrix * vNormal);

    // Pass through texture coordinates to the fragment shader
    fTexCoords = vTexCoords;

    FragPosLightSpace = lightSpaceMatrix * model* vec4(vPosition, 1.0); // Transform to light space 

    // Compute the final vertex position in clip space
    gl_Position = projection * view* model *vec4(vPosition,1.0f);

    
}
