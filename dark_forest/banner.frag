#version 140

uniform sampler2D texSampler;  
smooth in vec2 texCoord_v;     
out vec4 color_f;            

void main() {
  color_f = texture(texSampler, texCoord_v);
}
