#version 140 //frag


// Material Definition
struct Material {           
  vec3  ambient;            
  vec3  diffuse;            
  vec3  specular;           
  float shininess;          
  bool  useTexture; // Indicates if a texture should be used        
};

// Uniforms
uniform Material material; 
uniform sampler2D texSampler; 
uniform bool fog = true;  // Fog toggle
   
// Varying inputs (from vertex shader)
smooth in vec4 color_v;       
smooth in vec2 texCoord_v;     
smooth in vec3 fragNorm;
smooth in vec3 fragPos;

// Output
out vec4 color_f;       




void main() {
  vec4 outputColor = color_v;

  // Apply texture color if enabled in material settings
  if (material.useTexture) {
    vec4 texColor = texture(texSampler, texCoord_v);

    // Discarding fully transparent fragments
    if (texColor.a == 0.0)
      discard;

    // Modulate lighting result with the texture color
    outputColor *= texColor;
  }

  // Fog Computation
  if (fog) {
    // Basic linear fog parameters
    float density = 0.3f;

    // Calculating the distance from the camera using Z in eye space
    float distanceToCamera = abs(fragPos.z);

    // Coloring the fog
    vec4 fogColor = vec4(0.3f, 0.3f, 0.3f, 0.95f);

    // Linear fog blend factor
    float fogFactor = (3.0f - distanceToCamera) / 3.0f;
    fogFactor = clamp(fogFactor, 0.0f, 1.0f);

    // Inverting the factor to blend correctly (0 = full fog, 1 = full scene)
    fogFactor = 1.0f - fogFactor;

    outputColor = mix(outputColor, fogColor, fogFactor);
  }

  color_f = outputColor;
}

