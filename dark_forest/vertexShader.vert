#version 140 //vert

// Lighting calculations in camera (eye) space


struct Material {     
  vec3  ambient;       
  vec3  diffuse;       
  vec3  specular;      
  float shininess;     
  bool  useTexture;    
};


// sampler for the texture access
uniform sampler2D texSampler;  

struct Light {         // structure describing light parameters
  vec3  ambient;       
  vec3  diffuse;       
  vec3  specular;     
  vec3  position;     
  vec3  spotDirection; 
  float spotCosCutOff; 
  float spotExponent;  
};

// Input attributes
in vec3 position;          
in vec3 normal;             
in vec2 texCoord;           

// Output to fragment shader
smooth out vec2 texCoord_v;  // outgoing texture coordinates
smooth out vec4 color_v;     // outgoing fragment color

out vec2 fragTexCoord;
out vec3 fragNorm;
out vec3 fragPos;

// Uniforms
uniform float time;         
uniform Material material;  
uniform mat4 PVMmatrix;     
uniform mat4 Vmatrix;       
uniform mat4 Mmatrix;       
uniform mat4 normalMatrix;  
uniform vec3 spotLightPosition;   
uniform vec3 spotLightDirection; 
uniform bool flashlightOn;
uniform bool pointlightOn;
uniform vec3 flashlightColor;


// Lighting setup hardcoded
Light sun;
Light playerReflector;
Light pointLight;



vec4 computeDirectionalLight(Light light, Material material, vec3 vertexPosition, vec3 vertexNormal) {
  vec3 result = material.ambient * light.ambient;

  vec3 viewDir = normalize(-vertexPosition);
  if (dot(viewDir, vertexNormal) < 0.0) return vec4(result, 1.0);

  vec3 lightDir = normalize(light.position);
  float NdotL = dot(vertexNormal, lightDir);
  if (NdotL > 0.0) {
    vec3 reflectDir = reflect(-lightDir, vertexNormal);
    float RdotV = max(dot(reflectDir, viewDir), 0.0);
    result += material.diffuse * light.diffuse * NdotL;
    result += material.specular * light.specular * pow(RdotV, material.shininess);
  }

  return vec4(result, 1.0);
}

vec4 computePointLight(Light light, Material material, vec3 vertexPosition, vec3 vertexNormal) {
  vec3 result = material.ambient * light.ambient;

  vec3 viewDir = normalize(-vertexPosition);
  if (dot(viewDir, vertexNormal) < 0.0) {
    return vec4(result, 1.0);
  }

  vec3 lightDir = normalize(light.position - vertexPosition);
  float NdotL = dot(vertexNormal, lightDir);

  if (NdotL > 0.0) {
    vec3 reflectorDir = reflect(-lightDir, vertexNormal);
    float RdotV = max(0.0, dot(reflectorDir, viewDir));

    float distance = length(light.position - vertexPosition);
    float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance); // tweakable

    result += attenuation * (
      material.diffuse * light.diffuse * NdotL +
      material.specular * light.specular * pow(RdotV, material.shininess)
    );
  }

  return vec4(result, 1.0);
}

vec4 computeSpotLight(Light light, Material material, vec3 vertexPosition, vec3 vertexNormal) {
  vec3 result = material.ambient * light.ambient;

  vec3 viewDir = normalize(-vertexPosition);
  if (dot(viewDir, vertexNormal) < 0.0) return vec4(result, 1.0);

  vec3 lightVec = normalize(light.position - vertexPosition);
  float NdotL = dot(vertexNormal, lightVec);
  if (NdotL < 0.0) return vec4(result, 1.0);

  float spotFactor = max(dot(-lightVec, light.spotDirection), 0.0);
  if (spotFactor > light.spotCosCutOff) {
    vec3 reflectDir = reflect(-lightVec, vertexNormal);
    float RdotV = max(dot(reflectDir, viewDir), 0.0);
    result += material.diffuse * light.diffuse * NdotL;
    result += material.specular * light.specular * pow(RdotV, material.shininess);
    result *= pow(spotFactor, light.spotExponent);
  }

  return vec4(result, 1.0);
}




void setupLights() {
  sun.ambient = vec3(0.015, 0.015, 0.035);
  sun.diffuse = vec3(0.1, 0.1, 0.2);
  sun.specular = vec3(0.0);
  sun.position = vec3(0.0, 1.0, 0.0);

  playerReflector.ambient = vec3(0.2);
  playerReflector.diffuse = vec3(1.0);
  playerReflector.specular = vec3(1.0);
  playerReflector.spotCosCutOff = 0.95f;
  playerReflector.spotExponent = 0.0;
  playerReflector.position = (Vmatrix * vec4(spotLightPosition, 1.0)).xyz;
  playerReflector.spotDirection = normalize((Vmatrix * vec4(spotLightDirection, 0.0)).xyz);
}

void main() {
  setupLights();

  vec3 posEyeSpace = (Vmatrix * Mmatrix * vec4(position, 1.0)).xyz;
  vec3 normEyeSpace = normalize((Vmatrix * normalMatrix * vec4(normal, 0.0)).xyz);

  fragNorm = normalize((normalMatrix * vec4(normal, 0.0)).xyz);
  fragTexCoord = texCoord;
  fragPos = posEyeSpace;

  vec3 ambientLight = vec3(0.2f);
  vec4 totalColor = vec4(material.ambient * ambientLight, 0.0);

  totalColor += computeDirectionalLight(sun, material, posEyeSpace, normEyeSpace);

  if (flashlightOn) {
    playerReflector.diffuse  = flashlightColor;
    playerReflector.specular = flashlightColor; // or a dimmer version
    playerReflector.ambient  = flashlightColor * 0.2; // optional

    totalColor += computeSpotLight(playerReflector, material, posEyeSpace, normEyeSpace);
  }

  if (pointlightOn) {
    float flicker = 0.6 +
        0.2 * sin(time * 9.0) +
        0.1 * sin(time * 23.0 + 1.0) +
        0.05 * sin(time * 37.0 + 2.5);

    flicker = clamp(flicker, 0.0, 1.0); // just in case

    pointLight.ambient = vec3(0.05f) * flicker;
    pointLight.diffuse = vec3(0.7f) * flicker;
    pointLight.specular = vec3(0.1f) * flicker;
    pointLight.position = (Vmatrix * vec4(vec3(0.0f), 1.0)).xyz;

    totalColor += computePointLight(pointLight, material, posEyeSpace, normEyeSpace);
  }

  gl_Position = PVMmatrix * vec4(position, 1.0);
  color_v = totalColor;
  texCoord_v = texCoord;
}
