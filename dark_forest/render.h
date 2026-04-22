//----------------------------------------------------------------------------------------
/**
 * \file    render.h
 * \author  bielevan
 * \date    2025
 * \brief   Declarations for rendering scene objects and managing shaders.
 */
//----------------------------------------------------------------------------------------

#ifndef __RENDER_STUFF_H
#define __RENDER_STUFF_H

#include "data.h"
#include "pgr_mesh.h"

// MeshGeometry represents shared mesh data across multiple instances of the same object
typedef struct _MeshGeometry {
  GLuint vertexBufferObject; 
  GLuint elementBufferObject;
  GLuint vertexArrayObject; 
  unsigned int numTriangles;  

  // Material properties
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
  float shininess;

  GLuint texture;
} MeshGeometry;

// Object base struct, stores common transform and state attributes
typedef struct _Object {
  glm::vec3 position;
  glm::vec3 direction;
  float speed;
  float size;
  bool destroyed;
  float startTime;
  float currentTime;

} Object;

// Specialized object types with additional attributes

typedef struct _PlayerObject : public Object {

  float viewAngle; // in degrees
  bool isFree; 

} PlayerObject;

typedef struct _SlendermanObject : public Object {

	glm::vec3 initPosition;
    float pitch;

} SlendermanObject;

typedef struct _PineForestObject : public Object {

	glm::vec3 initPosition;

} PineForestObject;

//typedef struct _PlaneObject : public Object {
//
//	glm::vec3 initPosition;
//
//} PlaneObject;

typedef struct _GroundObject : public Object {

	glm::vec3 initPosition;

} GroundObject;

typedef struct _MountainObject : public Object {

	glm::vec3 initPosition;

} MountainObject;

typedef struct _CabinObject : public Object {

	glm::vec3 initPosition;

} CabinObject;

typedef struct _GrassObject : public Object {

	glm::vec3 initPosition;

} GrassObject;

typedef struct _BushObject : public Object {

	glm::vec3 initPosition;

} BushObject;

typedef struct _CoinObject : public Object {

	int id;

	glm::vec3 initPosition;
	float rotationAngle = 0.0f; // in degrees

} CoinObject;

typedef struct _CrowObject : public Object {

	glm::vec3 initPosition;

} CrowObject;

typedef struct _ChairObject : public Object {

	glm::vec3 initPosition;

} ChairObject;

typedef struct _BannerObject : public Object {
	// Uses base Object structure
} BannerObject;

typedef struct _SparkleObject : public Object {

	int    textureFrames;
	float  frameDuration;

} SparkleObject;

//struct StaticModel {
//	GLuint vao;
//	GLuint vbo;
//	GLuint ebo;
//};
//StaticModel chairModel;


// Shader program struct, stores locations for commonly used uniforms and attributes
typedef struct _commonShaderProgram {
  GLuint program;    
  // Attributes
  GLint posLocation; 
  GLint colorLocation; 
  GLint normalLocation;  
  GLint texCoordLocation;

  // Matrices
  GLint PVMmatrixLocation;   
  GLint VmatrixLocation;  
  GLint MmatrixLocation;  
  GLint normalMatrixLocation;

  // Time uniform
  GLint timeLocation;     

  // Material properties
  GLint diffuseLocation; 
  GLint ambientLocation;  
  GLint specularLocation; 
  GLint shininessLocation; 

  // Texture uniforms
  GLint useTextureLocation;
  GLint texSamplerLocation; 

  // Light uniforms
  GLint flashlightOn;
  GLint pointlightOn;
  GLint flashlightColor;
  GLint spotLightPositionLocation;
  GLint spotLightDirectionLocation; 
} SCommonShaderProgram;


// Object drawing function declarations
void drawPlayer(PlayerObject* player, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix);
void drawSlenderman(SlendermanObject* slenderman, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix);
void drawPineForest(PineForestObject* pineForest, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
void drawGround(GroundObject* ground, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
void drawMountain(MountainObject* mountain, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
void drawCabin(CabinObject* cabin, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
void drawCrow(CrowObject* crow, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
void drawGrass(GrassObject* grass, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
void drawBush(BushObject* bush, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
void drawChair(ChairObject* chair, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
void drawCoin(CoinObject* coin, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
void drawSkybox(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
void drawBanner(BannerObject* banner, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
void drawSparkle(SparkleObject* sparkle, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

// Shader and Model Initialization / Cleanup
void initializeShaderPrograms();
void cleanupShaderPrograms();

void initializeModels();
void cleanupModels();

#endif // __RENDER_STUFF_H
