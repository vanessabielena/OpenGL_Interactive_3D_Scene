#version 140

// Uniforms
uniform mat4 PVMmatrix;             

// Attributes, inputs from vertex buffer
in vec3 position;           
in vec2 texCoord;  

// Outputs, sent to fragment shader
smooth out vec2 texCoord_v;



void main() {
  gl_Position = PVMmatrix * vec4(position, 1.0);   

  texCoord_v = texCoord; //+ offset;
}
