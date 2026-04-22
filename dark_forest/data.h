//----------------------------------------------------------------------------------------
/**
 * \file    data.h
 * \author  bielevan
 * \date    2025
 * \brief   Constants, definitions, and essential geometry data.
 */
//----------------------------------------------------------------------------------------

#ifndef __DATA_H
#define __DATA_H

// Window and scene configuration
#define WINDOW_WIDTH   750
#define WINDOW_HEIGHT  750
#define WINDOW_TITLE   "Slenderman Game"
#define SCENE_WIDTH  1.0f
#define SCENE_HEIGHT 1.0f
#define SCENE_DEPTH  1.0f

// Entity sizes
#define PLAYER_SIZE   0.1f
#define SLENDERMAN_SIZE 0.1f
#define BILLBOARD_SIZE 0.05f
#define BANNER_SIZE 1.0f

// Object generation parameters
#define GRASS_COUNT_MIN 30
#define GRASS_COUNT_MAX 35
#define BUSH_COUNT_MIN 15
#define BUSH_COUNT_MAX 20
#define COUNT_MIN 1
#define COUNT_MAX 3

// Key bindings enumeration
enum { 
    KEY_LEFT_ARROW, 
    KEY_RIGHT_ARROW, 
    KEY_UP_ARROW, 
    KEY_DOWN_ARROW, 
    KEY_SPACE, 
    KEYS_COUNT 
};

// Player control constants
#define PLAYER_SPEED_INCREMENT  0.025f
#define PLAYER_SPEED_MAX        1.0f
#define PLAYER_VIEW_ANGLE_DELTA 2.0f // in degrees

// Camera elevation clamp
#define CAMERA_ELEVATION_MAX 45.0f
#define CAMERA_ELEVATION_MIN -45.0f


// Vertex shader, colors per vertex with transformation
const std::string colorVertexShaderSrc(
    "#version 140\n"
    "uniform mat4 PVMmatrix;\n"
    "in vec3 position;\n"
    "in vec3 color;\n"
    "smooth out vec4 theColor;\n"
    "void main() {\n"
	"  gl_Position = PVMmatrix * vec4(position, 1.0);\n"
	"  theColor = vec4(color, 1.0);\n"
    "}\n"
);

// Fragment shader, simple color passthrough
const std::string colorFragmentShaderSrc(
    "#version 140\n"
    "smooth in vec4 theColor;\n"
    "out vec4 outputColor;\n"
    "void main() {\n"
    "  outputColor = theColor;\n"
    "}\n"
);

// Vertex shader, textured geometry transformation
const std::string textureVertexShaderSrc(
    "#version 140\n"
    "uniform mat4 PVMmatrix;\n"
    "in vec3 position;\n"
    "in vec2 texCoord;\n"
    "out vec2 texCoord_v;\n"
    "void main() {\n"
    "  gl_Position = PVMmatrix * vec4(position, 1.0);\n"
    "  texCoord_v = texCoord;\n"
    "}\n"
);

// Fragment shader, texture with alpha masking
const std::string textureFragmentShaderSrc(
    "#version 140\n"
    "uniform sampler2D texSampler;\n"
    "in vec2 texCoord_v;\n"
    "out vec4 outputColor;\n"
    "void main() {\n"
    "  vec4 texColor = texture(texSampler, texCoord_v);\n"
    "  if (texColor.a < 0.1)\n"
    "    discard;\n"
    "  outputColor = texColor;\n"
    "}\n"
);


// Vertex shader, converts screen coords into cube map directions
const std::string skyboxVertexShaderSrc(
    "#version 140\n"
    "\n"
    "uniform mat4 inversePVmatrix;\n"
    "in vec2 screenCoord;\n"
    "out vec3 texCoord_v;\n"
    "\n"
    "void main() {\n"
    "  vec4 farplaneCoord = vec4(screenCoord, 0.9999, 1.0);\n"
    "  vec4 worldViewCoord = inversePVmatrix * farplaneCoord;\n"
    "  texCoord_v = worldViewCoord.xyz / worldViewCoord.w;\n"
    "  gl_Position = farplaneCoord;\n"
    "}\n"
);

// Fragment shader, samples color from cube map using direction
const std::string skyboxFragmentShaderSrc(
    "#version 140\n"
    "\n"
    "uniform samplerCube skyboxSampler;\n"
    "in vec3 texCoord_v;\n"
    "out vec4 color_f;\n"
    "\n"
    "void main() {\n"
    "  color_f = texture(skyboxSampler, texCoord_v);\n"
    "}\n"
);

// Geometry Data

// Hand holding flashlight banner quad vertices: X, Y, Z, U, V
const int bannerNumQuadVertices = 4;
const float bannerVertexData[20] = {
    -0.55f,  0.55f, 0.0f, 0.0f, 1.0f,
    -0.55f, -0.55f, 0.0f, 0.0f, 0.0f,
     0.55f,  0.55f, 0.0f, 1.0f, 1.0f,
     0.55f, -0.55f, 0.0f, 1.0f, 0.0f
};


// Sparkle billboard geometry: X, Y, Z, U, V 
const int sparkleNumQuadVertices = 4;
const float sparkleVertexData[20] = {
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
};

#endif // __DATA_H
