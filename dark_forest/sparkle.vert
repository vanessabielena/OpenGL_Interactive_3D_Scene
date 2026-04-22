#version 140

// ----- Uniforms
uniform mat4 PVMmatrix;


// Vertex Attributes, from VBO
in vec3 position; 
in vec2 texCoord; 

// Varyings, sent to fragment shader
smooth out vec2 texCoord_v;

void main() {
  gl_Position = PVMmatrix * vec4(position, 1.0);  

  texCoord_v = texCoord;
}
