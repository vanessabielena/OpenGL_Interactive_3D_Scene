//----------------------------------------------------------------------------------------
/**
 * \file    render.cpp
 * \author  bielevan
 * \date    2025
 * \brief   Core rendering routines for all 3D models in the scene
 */
//----------------------------------------------------------------------------------------

#include <iostream>
#include "pgr.h"
#include "render.h"
#include "data.h"
#include "spline.h"
#include "pgr_mesh.h"

// Scene geometry references; each pointer will later refer to the loaded mesh data.
MeshGeometry* slendermanGeometry = NULL;
MeshGeometry* playerGeometry = NULL;
MeshGeometry* pineForestGeometry = NULL;
MeshGeometry* groundGeometry = NULL;
MeshGeometry* mountainGeometry = NULL;
MeshGeometry* cabinGeometry = NULL;
MeshGeometry* crowGeometry = NULL;
MeshGeometry* grassGeometry = NULL;
MeshGeometry* bushGeometry = NULL;
MeshGeometry* coinGeometry = NULL;
MeshGeometry* chairGeometry = NULL;
MeshGeometry* skyboxGeometry = NULL;
MeshGeometry* bannerGeometry = NULL;
MeshGeometry* sparkleGeometry = NULL;

// File paths to model data
const char* SLENDERMAN_MODEL_NAME = "data/slenderman/slenderman_1.obj";
const char* PLAYER_MODEL_NAME = "data/avatar/avatar2.obj";
const char* PINE_MODEL_NAME = "data/forest/forest.obj";
const char* GROUND_MODEL_NAME = "data/ground/ground.obj";
const char* MOUNTAIN_MODEL_NAME = "data/mountain/mountains.obj";
const char* CABIN_MODEL_NAME = "data/cabin/cabin1.obj";
const char* CROW_MODEL_NAME = "data/crow/bird3.obj";
const char* GRASS_MODEL_NAME = "data/grass/grass.obj";
const char* BUSH_MODEL_NAME = "data/bush/bush.obj";
const char* COIN_MODEL_NAME = "data/coin/coin.obj";
const char* SKYBOX_CUBE_TEXTURE_FILE_PREFIX = "data/skybox/skybox";
const char* BANNER_TEXTURE_NAME = "data/hand_flashlight_nb.png";
const char* SPARKLE_TEXTURE_NAME = "data/star/star.png";

// Global shader programs used throughout the rendering pipeline
SCommonShaderProgram shaderProgram;
// Global rendering flags
bool useLighting = false;

GLuint staticModelVAO, staticModelVBO, staticModelEBO;

// Shader setup for skybox rendering
struct SkyboxShaderProgram {
    GLuint program;  
    GLint screenCoordLocation; 
    GLint inversePVmatrixLocation; 
    GLint skyboxSamplerLocation; 
} skyboxFarPlaneShaderProgram;

// UI overlay rendering (e.g., HUD icons like flashlight)
struct BannerShaderProgram {
    GLuint program;  
    GLint posLocation;  
    GLint texCoordLocation; 
    GLint PVMmatrixLocation; 
    GLint timeLocation; 
    GLint texSamplerLocation;
} bannerShaderProgram;

// Animated texture rendering (sparkles effect)
struct SparkleShaderProgram {
    GLuint program;     
    GLint posLocation; 
    GLint texCoordLocation; 
    GLint PVMmatrixLocation;    
    GLint VmatrixLocation; 
    GLint timeLocation;  
    GLint texSamplerLocation; 
    GLint frameDurationLocation;

} sparkleShaderProgram;

// Sends transformation matrices to the GPU
void setTransformUniforms(const glm::mat4 &modelMatrix, const glm::mat4 &viewMatrix, const glm::mat4 &projectionMatrix) {
    
  // Calculate the PVM matrix that combines projection, view, and model transforms
  const glm::mat4 PVM = projectionMatrix * viewMatrix * modelMatrix;
  glUniformMatrix4fv(shaderProgram.PVMmatrixLocation, 1, GL_FALSE, glm::value_ptr(PVM));

  // Send individual view and model matrices as well
  glUniformMatrix4fv(shaderProgram.VmatrixLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));
  glUniformMatrix4fv(shaderProgram.MmatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));

  const glm::mat4 modelRotationMatrix = glm::mat4(
    modelMatrix[0],
    modelMatrix[1],
    modelMatrix[2],
    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
  );
  const glm::mat4 normalMatrix = glm::transpose(glm::inverse(modelRotationMatrix));

  glUniformMatrix4fv(shaderProgram.normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMatrix));  // correct matrix for non-rigid transform
}

// Configures material properties and texture for rendering
void setMaterialUniforms(const glm::vec3 &ambient, const glm::vec3 &diffuse, const glm::vec3 &specular, float shininess, GLuint texture) {

  // Set color-related properties
  glUniform3fv(shaderProgram.diffuseLocation,  1, glm::value_ptr(diffuse)); 
  glUniform3fv(shaderProgram.ambientLocation,  1, glm::value_ptr(ambient));
  glUniform3fv(shaderProgram.specularLocation, 1, glm::value_ptr(specular));
  glUniform1f(shaderProgram.shininessLocation, shininess);

  // Texture usage toggle
  if(texture != 0) {
    glUniform1i(shaderProgram.useTextureLocation, 1);  
    glUniform1i(shaderProgram.texSamplerLocation, 0); 
    glActiveTexture(GL_TEXTURE0 + 0);                
    glBindTexture(GL_TEXTURE_2D, texture);
  }
  else {
      // No texture present; skip sampling in shader
    glUniform1i(shaderProgram.useTextureLocation, 0);  // do not sample the texture
  }
}

void drawPlayer(PlayerObject *player, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix) {

  glUseProgram(shaderProgram.program);

  // Construct model transformation matrix:
  glm::mat4 modelMatrix = glm::mat4(1.0f);
  modelMatrix = glm::translate(modelMatrix, player->position);
  modelMatrix = glm::rotate(modelMatrix, glm::radians(player->viewAngle), glm::vec3(0, 0, 1));
  modelMatrix = glm::scale(modelMatrix, glm::vec3(player->size, player->size, player->size));

  setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

  // Setup material properties
  setMaterialUniforms(
    playerGeometry->ambient,
    playerGeometry->diffuse,
    playerGeometry->specular,
    playerGeometry->shininess,
    playerGeometry->texture
  );

  // Bind and render the geometry
  glBindVertexArray(playerGeometry->vertexArrayObject);
  glDrawElements(GL_TRIANGLES, playerGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);

  // Clean up
  glUseProgram(0);

  return;
}

void drawPineForest(PineForestObject* pineForest, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {

    glUseProgram(shaderProgram.program);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Construct model transformation matrix:
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), pineForest->position + glm::vec3(0.0f, 0.0f, 0.42f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(3.1f));
    modelMatrix = glm::rotate(modelMatrix, 1.5708f, glm::vec3(1, 0, 0));

    setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

    // Setup material properties
    setMaterialUniforms(
        pineForestGeometry->ambient,
        pineForestGeometry->diffuse,
        pineForestGeometry->specular,
        pineForestGeometry->shininess,
        pineForestGeometry->texture
    );

    // Bind and render the geometry
    glBindVertexArray(pineForestGeometry->vertexArrayObject);
    glDrawElements(GL_TRIANGLES, pineForestGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    //  Disable blending after drawing, restore original pipeline state
    glDisable(GL_BLEND);
    glUseProgram(0);

    return;
}

void drawSlenderman(SlendermanObject* slenderman, const glm::mat4 & viewMatrix, const glm::mat4 & projectionMatrix) {
  glUseProgram(shaderProgram.program);

  // Construct model transformation matrix:
  glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), slenderman->position + glm::vec3(0.0f, 0.0f, -0.05f));

  // Generate a random pitch angle each frame (between 0 and 360 degrees)
  float randomPitch = glm::radians(static_cast<float>(rand() % 360));

  modelMatrix = glm::scale(modelMatrix, glm::vec3(slenderman->size));
  modelMatrix = glm::rotate(modelMatrix, 1.5708f, glm::vec3(1, 0, 0));
  modelMatrix = glm::rotate(modelMatrix, glm::radians(slenderman->pitch), glm::vec3(0, 1, 0));

  setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix); 

  // Setup material properties
  setMaterialUniforms(
    slendermanGeometry->ambient,
    slendermanGeometry->diffuse,
    slendermanGeometry->specular,
    slendermanGeometry->shininess,
    slendermanGeometry->texture
  );

  // Bind and render the geometry
  glBindVertexArray(slendermanGeometry->vertexArrayObject);
  glDrawElements(GL_TRIANGLES, slendermanGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);

  glUseProgram(0);

  return;
}

void drawGround(GroundObject* ground, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {

    glUseProgram(shaderProgram.program);

    // Construct model transformation matrix:
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), ground->position + glm::vec3(0.0f, 0.0f, -0.13f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(ground->size));
    modelMatrix = glm::rotate(modelMatrix, 1.5708f, glm::vec3(1, 0, 0));

    setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

    // Setup material properties
    setMaterialUniforms(
        groundGeometry->ambient,
        groundGeometry->diffuse,
        groundGeometry->specular,
        groundGeometry->shininess,
        groundGeometry->texture
    );

    // Bind and render the geometry
    glBindVertexArray(groundGeometry->vertexArrayObject);
    glDrawElements(GL_TRIANGLES, groundGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glUseProgram(0);

    return;
}

void drawMountain(MountainObject* mountain, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {

    glUseProgram(shaderProgram.program);

    // Construct model transformation matrix:
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), mountain->position + glm::vec3(0.0f, 0.0f, -0.2f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(mountain->size));
    modelMatrix = glm::rotate(modelMatrix, 1.5708f, glm::vec3(1, 0, 0));

    setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

    // Setup material properties
    setMaterialUniforms(
        mountainGeometry->ambient,
        mountainGeometry->diffuse,
        mountainGeometry->specular,
        mountainGeometry->shininess,
        mountainGeometry->texture
    );

    // Bind and render the geometry
    glBindVertexArray(mountainGeometry->vertexArrayObject);
    glDrawElements(GL_TRIANGLES, mountainGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glUseProgram(0);

    return;
}

void drawCabin(CabinObject* cabin, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {

    glUseProgram(shaderProgram.program);

    // Construct model transformation matrix:
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), cabin->position + glm::vec3(0.0f, 0.0f, -0.13f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(cabin->size));
    modelMatrix = glm::rotate(modelMatrix, 1.5708f, glm::vec3(1, 0, 0));

    setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

    // Setup material properties
    setMaterialUniforms(
        cabinGeometry->ambient,
        cabinGeometry->diffuse,
        cabinGeometry->specular,
        cabinGeometry->shininess,
        cabinGeometry->texture
    );

    // Bind and render the geometry
    glBindVertexArray(cabinGeometry->vertexArrayObject);
    glDrawElements(GL_TRIANGLES, cabinGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glUseProgram(0);

    return;
}

void drawCrow(CrowObject* crow, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {

    glUseProgram(shaderProgram.program);

    // Construct model transformation matrix:
    glm::mat4 modelMatrix = alignObject(crow->position, crow->direction, glm::vec3(0.0f, 0.0f, 1.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(crow->size));
    modelMatrix = glm::rotate(modelMatrix, 0.0f, glm::vec3(1, 0, 0));

    setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

    // Setup material properties
    setMaterialUniforms(
        crowGeometry->ambient,
        crowGeometry->diffuse,
        crowGeometry->specular,
        crowGeometry->shininess,
        crowGeometry->texture
    );

    // Bind and render the geometry
    glBindVertexArray(crowGeometry->vertexArrayObject);
    glDrawElements(GL_TRIANGLES, crowGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glUseProgram(0);

    return;
}

void drawGrass(GrassObject* grass, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {

    glUseProgram(shaderProgram.program);

    // Construct model transformation matrix:
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), grass->position + glm::vec3(0.0f, 0.0f, -0.1f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(grass->size));
    modelMatrix = glm::rotate(modelMatrix, 1.5708f, glm::vec3(1, 0, 0));

    setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

    // Setup material properties
    setMaterialUniforms(
        grassGeometry->ambient,
        grassGeometry->diffuse,
        grassGeometry->specular,
        grassGeometry->shininess,
        grassGeometry->texture
    );

    // Bind and render the geometry
    glBindVertexArray(grassGeometry->vertexArrayObject);
    glDrawElements(GL_TRIANGLES, grassGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glUseProgram(0);

    return;
}

void drawBush(BushObject* bush, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {

    glUseProgram(shaderProgram.program);

    // Construct model transformation matrix:
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), bush->position + glm::vec3(0.0f, 0.0f, -0.12f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(bush->size + 0.05f));
    modelMatrix = glm::rotate(modelMatrix, 1.5708f, glm::vec3(1, 0, 0));

    setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

    // Setup material properties
    setMaterialUniforms(
        bushGeometry->ambient,
        bushGeometry->diffuse,
        bushGeometry->specular,
        bushGeometry->shininess,
        bushGeometry->texture
    );

    // Bind and render the geometry
    glBindVertexArray(bushGeometry->vertexArrayObject);
    glDrawElements(GL_TRIANGLES, bushGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glUseProgram(0);

    return;
}


void drawCoin(CoinObject* coin, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {

    glUseProgram(shaderProgram.program);

    // Construct model transformation matrix:
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), coin->position + glm::vec3(0.0f, 0.0f, -0.06f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(coin->size));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(coin->rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)); // or Y axis if upright
    
    setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

    // Setup material properties
    setMaterialUniforms(
        coinGeometry->ambient,
        coinGeometry->diffuse,
        coinGeometry->specular,
        coinGeometry->shininess,
        coinGeometry->texture
    );

    // Bind and render the geometry
    glBindVertexArray(coinGeometry->vertexArrayObject);
    glDrawElements(GL_TRIANGLES, coinGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glUseProgram(0);

    return;
}
void drawChair(ChairObject* chair, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {

    glUseProgram(shaderProgram.program);

    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), chair->position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(-90.0f), glm::vec3(1, 0, 0));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(chair->size));
    setTransformUniforms(modelMatrix, viewMatrix, projectionMatrix);

    setMaterialUniforms(
        chairGeometry->ambient,
        chairGeometry->diffuse,
        chairGeometry->specular,
        chairGeometry->shininess,
        chairGeometry->texture
    );

    glBindVertexArray(chairGeometry->vertexArrayObject);
    glDrawElements(GL_TRIANGLES, chairGeometry->numTriangles * 3, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glUseProgram(0);

    return;
}

void drawBanner(BannerObject* banner, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);

    glUseProgram(bannerShaderProgram.program);

    glm::mat4 matrix = glm::translate(glm::mat4(3.0f), banner->position);
    matrix = glm::scale(matrix, glm::vec3(1.0f, 1.0f, 1.0f));

    glm::mat4 PVMmatrix = projectionMatrix * viewMatrix * matrix;
    glUniformMatrix4fv(bannerShaderProgram.PVMmatrixLocation, 1, GL_FALSE, glm::value_ptr(PVMmatrix));        // model-view-projection
    glUniform1f(bannerShaderProgram.timeLocation, banner->currentTime - banner->startTime);
    glUniform1i(bannerShaderProgram.texSamplerLocation, 0);

    glBindTexture(GL_TEXTURE_2D, bannerGeometry->texture);
    glBindVertexArray(bannerGeometry->vertexArrayObject);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, bannerGeometry->numTriangles);

    CHECK_GL_ERROR();

    glBindVertexArray(0);
    glUseProgram(0);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    return;
}

void drawSparkle(SparkleObject* sparkle, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    glUseProgram(sparkleShaderProgram.program);

    // version 1: inversion of the rotation part of the view matrix

      // just take 3x3 rotation part of the view transform
    glm::mat4 billboardRotationMatrix = glm::mat4(
        viewMatrix[0],
        viewMatrix[1],
        viewMatrix[2],
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
    );
    // inverse view rotation
    billboardRotationMatrix = glm::transpose(billboardRotationMatrix);

    glm::mat4 matrix = glm::mat4(1.0f);
    matrix = glm::translate(matrix, sparkle->position);
    matrix = glm::scale(matrix, glm::vec3(sparkle->size));

    matrix = matrix * billboardRotationMatrix; // make billboard to face the camera
    glm::mat4 PVMmatrix = projectionMatrix * viewMatrix * matrix;

    glUniformMatrix4fv(sparkleShaderProgram.VmatrixLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));   // view

    glUniformMatrix4fv(sparkleShaderProgram.PVMmatrixLocation, 1, GL_FALSE, glm::value_ptr(PVMmatrix));  // model-view-projection
    glUniform1f(sparkleShaderProgram.timeLocation, sparkle->currentTime - sparkle->startTime);
    glUniform1i(sparkleShaderProgram.texSamplerLocation, 0);
    glUniform1f(sparkleShaderProgram.frameDurationLocation, sparkle->frameDuration);

    glBindVertexArray(sparkleGeometry->vertexArrayObject);
    glBindTexture(GL_TEXTURE_2D, sparkleGeometry->texture);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, sparkleGeometry->numTriangles);

    glBindVertexArray(0);
    glUseProgram(0);

    glDisable(GL_BLEND);

    return;
}


void drawSkybox(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {

    glUseProgram(skyboxFarPlaneShaderProgram.program);

    // compose transformations
    const glm::mat4 matrix = projectionMatrix * viewMatrix;

    // create view rotation matrix by using view matrix with cleared translation
    glm::mat4 viewRotation = viewMatrix;
    viewRotation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    // vertex shader will translate screen space coordinates (NDC) using inverse PV matrix
    const glm::mat4 inversePVmatrix = glm::inverse(projectionMatrix * viewRotation);

    glUniformMatrix4fv(skyboxFarPlaneShaderProgram.inversePVmatrixLocation, 1, GL_FALSE, glm::value_ptr(inversePVmatrix));
    glUniform1i(skyboxFarPlaneShaderProgram.skyboxSamplerLocation, 0);

    // draw "skybox" rendering 2 triangles covering the far plane
    glBindVertexArray(skyboxGeometry->vertexArrayObject);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxGeometry->texture);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, skyboxGeometry->numTriangles + 2);

    glBindVertexArray(0);
    glUseProgram(0);
}


void cleanupShaderPrograms(void) {

  pgr::deleteProgramAndShaders(shaderProgram.program);
  pgr::deleteProgramAndShaders(bannerShaderProgram.program);
  pgr::deleteProgramAndShaders(skyboxFarPlaneShaderProgram.program);
  pgr::deleteProgramAndShaders(sparkleShaderProgram.program);
}

void initializeShaderPrograms(void) {

  std::vector<GLuint> shaderList;

  if(useLighting == true) {
    // load and compile shader for lighting (lights & materials)

    // push vertex shader and fragment shader
    shaderList.push_back(pgr::createShaderFromFile(GL_VERTEX_SHADER, "vertexShader.vert"));
    shaderList.push_back(pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "fragmentShader.frag"));

    // create the shader program with two shaders
    shaderProgram.program = pgr::createProgram(shaderList);

    // get vertex attributes locations, if the shader does not have this uniform -> return -1
    shaderProgram.posLocation      = glGetAttribLocation(shaderProgram.program, "position");
    shaderProgram.normalLocation   = glGetAttribLocation(shaderProgram.program, "normal");
    shaderProgram.texCoordLocation = glGetAttribLocation(shaderProgram.program, "texCoord");
    // get uniforms locations
    shaderProgram.PVMmatrixLocation    = glGetUniformLocation(shaderProgram.program, "PVMmatrix");
    shaderProgram.VmatrixLocation      = glGetUniformLocation(shaderProgram.program, "Vmatrix");
    shaderProgram.MmatrixLocation      = glGetUniformLocation(shaderProgram.program, "Mmatrix");
    shaderProgram.normalMatrixLocation = glGetUniformLocation(shaderProgram.program, "normalMatrix");
    shaderProgram.timeLocation         = glGetUniformLocation(shaderProgram.program, "time");
    // material
    shaderProgram.ambientLocation      = glGetUniformLocation(shaderProgram.program, "material.ambient");
    shaderProgram.diffuseLocation      = glGetUniformLocation(shaderProgram.program, "material.diffuse");
    shaderProgram.specularLocation     = glGetUniformLocation(shaderProgram.program, "material.specular");
    shaderProgram.shininessLocation    = glGetUniformLocation(shaderProgram.program, "material.shininess");
    // texture
    shaderProgram.texSamplerLocation   = glGetUniformLocation(shaderProgram.program, "texSampler");
    shaderProgram.useTextureLocation   = glGetUniformLocation(shaderProgram.program, "material.useTexture");
    // spotlight
    shaderProgram.spotLightPositionLocation = glGetUniformLocation(shaderProgram.program, "spotLightPosition");
    shaderProgram.spotLightDirectionLocation = glGetUniformLocation(shaderProgram.program, "spotLightDirection");
    shaderProgram.flashlightOn = glGetUniformLocation(shaderProgram.program, "flashlightOn");
    // pointlight
    shaderProgram.pointlightOn = glGetUniformLocation(shaderProgram.program, "pointlightOn");
    shaderProgram.flashlightColor = glGetUniformLocation(shaderProgram.program, "flashlightColor");
    


  }
  else {
      // load and compile simple shader (colors only, no lights at all)

      // push vertex shader and fragment shader
      shaderList.push_back(pgr::createShaderFromSource(GL_VERTEX_SHADER, colorVertexShaderSrc));
      shaderList.push_back(pgr::createShaderFromSource(GL_FRAGMENT_SHADER, colorFragmentShaderSrc));

      // create the program with two shaders (fragment and vertex)
      shaderProgram.program = pgr::createProgram(shaderList);
      // get position and color attributes locations
      shaderProgram.posLocation = glGetAttribLocation(shaderProgram.program, "position");
      shaderProgram.colorLocation = glGetAttribLocation(shaderProgram.program, "color");
      // get uniforms locations
      shaderProgram.PVMmatrixLocation = glGetUniformLocation(shaderProgram.program, "PVMmatrix");
  }

  // load and compile shader for banner (translation of texture coordinates)

  shaderList.clear();

  // push vertex shader and fragment shader
  shaderList.push_back(pgr::createShaderFromFile(GL_VERTEX_SHADER, "banner.vert"));
  shaderList.push_back(pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "banner.frag"));

  // Create the program with two shaders
  bannerShaderProgram.program = pgr::createProgram(shaderList);

  // get position and color attributes locations
  bannerShaderProgram.posLocation = glGetAttribLocation(bannerShaderProgram.program, "position");
  bannerShaderProgram.texCoordLocation = glGetAttribLocation(bannerShaderProgram.program, "texCoord");
  // get uniforms locations
  bannerShaderProgram.PVMmatrixLocation = glGetUniformLocation(bannerShaderProgram.program, "PVMmatrix");
  bannerShaderProgram.timeLocation = glGetUniformLocation(bannerShaderProgram.program, "time");
  bannerShaderProgram.texSamplerLocation = glGetUniformLocation(bannerShaderProgram.program, "texSampler");


  // load and compile shader for sparkle (dynamic texture)
  shaderList.clear();

  // push vertex shader and fragment shader
  shaderList.push_back(pgr::createShaderFromFile(GL_VERTEX_SHADER, "sparkle.vert"));
  shaderList.push_back(pgr::createShaderFromFile(GL_FRAGMENT_SHADER, "sparkle.frag"));

  // create the program with two shaders
  sparkleShaderProgram.program = pgr::createProgram(shaderList);

  // get position and texture coordinates attributes locations
  sparkleShaderProgram.posLocation = glGetAttribLocation(sparkleShaderProgram.program, "position");
  sparkleShaderProgram.texCoordLocation = glGetAttribLocation(sparkleShaderProgram.program, "texCoord");
  // get uniforms locations
  sparkleShaderProgram.PVMmatrixLocation = glGetUniformLocation(sparkleShaderProgram.program, "PVMmatrix");
  sparkleShaderProgram.VmatrixLocation = glGetUniformLocation(sparkleShaderProgram.program, "Vmatrix");
  sparkleShaderProgram.timeLocation = glGetUniformLocation(sparkleShaderProgram.program, "time");
  sparkleShaderProgram.texSamplerLocation = glGetUniformLocation(sparkleShaderProgram.program, "texSampler");
  sparkleShaderProgram.frameDurationLocation = glGetUniformLocation(sparkleShaderProgram.program, "frameDuration");


  // load and compile shader for skybox (cube map)

  shaderList.clear();

  // push vertex shader and fragment shader
  shaderList.push_back(pgr::createShaderFromSource(GL_VERTEX_SHADER, skyboxVertexShaderSrc));
  shaderList.push_back(pgr::createShaderFromSource(GL_FRAGMENT_SHADER, skyboxFragmentShaderSrc));

  // create the program with two shaders
  skyboxFarPlaneShaderProgram.program = pgr::createProgram(shaderList);

  // handles to vertex attributes locations
  skyboxFarPlaneShaderProgram.screenCoordLocation = glGetAttribLocation(skyboxFarPlaneShaderProgram.program, "screenCoord");
  // get uniforms locations
  skyboxFarPlaneShaderProgram.skyboxSamplerLocation = glGetUniformLocation(skyboxFarPlaneShaderProgram.program, "skyboxSampler");
  skyboxFarPlaneShaderProgram.inversePVmatrixLocation = glGetUniformLocation(skyboxFarPlaneShaderProgram.program, "inversePVmatrix");
  
}

/** Load mesh using assimp library
 *  Vertex, normals and texture coordinates data are stored without interleaving |VVVVV...|NNNNN...|tttt
 * \param fileName [in] file to open/load
 * \param shader [in] vao will connect loaded data to shader
 * \param geometry
 */
bool loadSingleMesh(const std::string &fileName, SCommonShaderProgram& shader, MeshGeometry** geometry) {
  Assimp::Importer importer;

  // Unitize object in size (scale the model to fit into (-1..1)^3)
  importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, 1);

  // Load asset from the file - you can play with various processing steps
  const aiScene* scn = importer.ReadFile(fileName.c_str(), 0
      | aiProcess_Triangulate             // Triangulate polygons (if any).
      | aiProcess_PreTransformVertices    // Transforms scene hierarchy into one root with geometry-leafs only. For more see Doc.
      | aiProcess_GenSmoothNormals        // Calculate normals per vertex.
      | aiProcess_JoinIdenticalVertices);

  // abort if the loader fails
  if(scn == NULL) {
    std::cerr << "assimp error: " << importer.GetErrorString() << std::endl;
    *geometry = NULL;
    return false;
  }

  // some formats store whole scene (multiple meshes and materials, lights, cameras, ...) in one file, we cannot handle that in our simplified example
  if(scn->mNumMeshes != 1) {
    std::cerr << "this simplified loader can only process files with only one mesh" << std::endl;
    *geometry = NULL;
    return false;
  }

  // in this phase we know we have one mesh in our loaded scene, we can directly copy its data to OpenGL ...
  const aiMesh * mesh = scn->mMeshes[0];

  *geometry = new MeshGeometry;

  // vertex buffer object, store all vertex positions and normals
  glGenBuffers(1, &((*geometry)->vertexBufferObject));
  glBindBuffer(GL_ARRAY_BUFFER, (*geometry)->vertexBufferObject);
  glBufferData(GL_ARRAY_BUFFER, 8*sizeof(float)*mesh->mNumVertices, 0, GL_STATIC_DRAW); // allocate memory for vertices, normals, and texture coordinates
  // first store all vertices
  glBufferSubData(GL_ARRAY_BUFFER, 0, 3*sizeof(float)*mesh->mNumVertices, mesh->mVertices);
  // then store all normals
  glBufferSubData(GL_ARRAY_BUFFER, 3*sizeof(float)*mesh->mNumVertices, 3*sizeof(float)*mesh->mNumVertices, mesh->mNormals);
  
  // just texture 0 for now
  float *textureCoords = new float[2 * mesh->mNumVertices];  // 2 floats per vertex
  float *currentTextureCoord = textureCoords;

  // copy texture coordinates
  aiVector3D vect;
    
  if(mesh->HasTextureCoords(0) ) {
    // we use 2D textures with 2 coordinates and ignore the third coordinate
    for(unsigned int idx=0; idx<mesh->mNumVertices; idx++) {
      vect = (mesh->mTextureCoords[0])[idx];
      *currentTextureCoord++ = vect.x;
      *currentTextureCoord++ = vect.y;
    }
  }
    
  // finally store all texture coordinates
  glBufferSubData(GL_ARRAY_BUFFER, 6*sizeof(float)*mesh->mNumVertices, 2*sizeof(float)*mesh->mNumVertices, textureCoords);

  // copy all mesh faces into one big array (assimp supports faces with ordinary number of vertices, we use only 3 -> triangles)
  unsigned int *indices = new unsigned int[mesh->mNumFaces * 3];
  for(unsigned int f = 0; f < mesh->mNumFaces; ++f) {
    indices[f*3 + 0] = mesh->mFaces[f].mIndices[0];
    indices[f*3 + 1] = mesh->mFaces[f].mIndices[1];
    indices[f*3 + 2] = mesh->mFaces[f].mIndices[2];
  }

  // copy our temporary index array to OpenGL and free the array
  glGenBuffers(1, &((*geometry)->elementBufferObject));
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (*geometry)->elementBufferObject);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(unsigned) * mesh->mNumFaces, indices, GL_STATIC_DRAW);

  delete [] indices;

  // copy the material info to MeshGeometry structure
  const aiMaterial *mat  = scn->mMaterials[mesh->mMaterialIndex];
  aiColor4D color;
  aiString name;
  aiReturn retValue = AI_SUCCESS;

  // Get returns: aiReturn_SUCCESS 0 | aiReturn_FAILURE -1 | aiReturn_OUTOFMEMORY -3
  mat->Get(AI_MATKEY_NAME, name); // may be "" after the input mesh processing. Must be aiString type!

  if((retValue = aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &color)) != AI_SUCCESS)
    color = aiColor4D(0.0f, 0.0f, 0.0f, 0.0f);

  (*geometry)->diffuse = glm::vec3(color.r, color.g, color.b);

  if ((retValue = aiGetMaterialColor(mat, AI_MATKEY_COLOR_AMBIENT, &color)) != AI_SUCCESS)
    color = aiColor4D(0.0f, 0.0f, 0.0f, 0.0f);
  (*geometry)->ambient = glm::vec3(color.r, color.g, color.b);

  if ((retValue = aiGetMaterialColor(mat, AI_MATKEY_COLOR_SPECULAR, &color)) != AI_SUCCESS)
    color = aiColor4D(0.0f, 0.0f, 0.0f, 0.0f);
  (*geometry)->specular = glm::vec3(color.r, color.g, color.b);

  ai_real shininess, strength;
  unsigned int max;	// changed: to unsigned

  max = 1;	
  if ((retValue = aiGetMaterialFloatArray(mat, AI_MATKEY_SHININESS, &shininess, &max)) != AI_SUCCESS)
    shininess = 1.0f;
  max = 1;
  if((retValue = aiGetMaterialFloatArray(mat, AI_MATKEY_SHININESS_STRENGTH, &strength, &max)) != AI_SUCCESS)
    strength = 1.0f;
  (*geometry)->shininess = shininess * strength;

  (*geometry)->texture = 0;

  // load texture image
  if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
    // get texture name 
    aiString path; // filename

    aiReturn texFound = mat->GetTexture(aiTextureType_DIFFUSE, 0, &path);
    std::string textureName = path.data;

    size_t found = fileName.find_last_of("/\\");
    // insert correct texture file path 
    if(found != std::string::npos) { // not found
      //subMesh_p->textureName.insert(0, "/");
      textureName.insert(0, fileName.substr(0, found+1));
    }

    std::cout << "Loading texture file: " << textureName << std::endl;
    (*geometry)->texture = pgr::createTexture(textureName);
  }
  CHECK_GL_ERROR();

  glGenVertexArrays(1, &((*geometry)->vertexArrayObject));
  glBindVertexArray((*geometry)->vertexArrayObject);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (*geometry)->elementBufferObject); // bind our element array buffer (indices) to vao
  glBindBuffer(GL_ARRAY_BUFFER, (*geometry)->vertexBufferObject);

  glEnableVertexAttribArray(shader.posLocation);
  glVertexAttribPointer(shader.posLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

  if(useLighting == true) {
    glEnableVertexAttribArray(shader.normalLocation);
    glVertexAttribPointer(shader.normalLocation, 3, GL_FLOAT, GL_FALSE, 0, (void*)(3 * sizeof(float) * mesh->mNumVertices));
  }
  else {
	  glDisableVertexAttribArray(shader.colorLocation);
	  // following line is problematic on AMD/ATI graphic cards
	  // -> if you see black screen (no objects at all) than try to set color manually in vertex shader to see at least something
    glVertexAttrib3f(shader.colorLocation, color.r, color.g, color.b);
  }

  glEnableVertexAttribArray(shader.texCoordLocation);
  glVertexAttribPointer(shader.texCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, (void*)(6 * sizeof(float) * mesh->mNumVertices));
  CHECK_GL_ERROR();

  glBindVertexArray(0);

  (*geometry)->numTriangles = mesh->mNumFaces;

  return true;
}


void initSkyboxGeometry(GLuint shader, MeshGeometry** geometry) {

    *geometry = new MeshGeometry;

    // 2D coordinates of 2 triangles covering the whole screen (NDC), draw using triangle strip
    static const float screenCoords[] = {
      -1.0f, -1.0f,
       1.0f, -1.0f,
      -1.0f,  1.0f,
       1.0f,  1.0f
    };

    glGenVertexArrays(1, &((*geometry)->vertexArrayObject));
    glBindVertexArray((*geometry)->vertexArrayObject);

    // buffer for far plane rendering
    glGenBuffers(1, &((*geometry)->vertexBufferObject));\
        glBindBuffer(GL_ARRAY_BUFFER, (*geometry)->vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenCoords), screenCoords, GL_STATIC_DRAW);

    //glUseProgram(farplaneShaderProgram);

    glEnableVertexAttribArray(skyboxFarPlaneShaderProgram.screenCoordLocation);
    glVertexAttribPointer(skyboxFarPlaneShaderProgram.screenCoordLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindVertexArray(0);
    glUseProgram(0);
    CHECK_GL_ERROR();

    (*geometry)->numTriangles = 2;

    glActiveTexture(GL_TEXTURE0);

    glGenTextures(1, &((*geometry)->texture));
    glBindTexture(GL_TEXTURE_CUBE_MAP, (*geometry)->texture);

    const char* suffixes[] = { "posx", "negx", "posy", "negy", "posz", "negz" };
    GLuint targets[] = {
      GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    for (int i = 0; i < 6; i++) {
        std::string texName = std::string(SKYBOX_CUBE_TEXTURE_FILE_PREFIX) + "_" + suffixes[i] + ".jpg";
        std::cout << "Loading cube map texture: " << texName << std::endl;
        if (!pgr::loadTexImage2D(texName, targets[i])) {
            pgr::dieWithError("Skybox cube map loading failed!");
        }
    }

    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // unbind the texture (just in case someone will mess up with texture calls later)
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    CHECK_GL_ERROR();
}

void initBannerGeometry(GLuint shader, MeshGeometry** geometry) {

    *geometry = new MeshGeometry;

    (*geometry)->texture = pgr::createTexture(BANNER_TEXTURE_NAME);
    glBindTexture(GL_TEXTURE_2D, (*geometry)->texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);

    glGenVertexArrays(1, &((*geometry)->vertexArrayObject));
    glBindVertexArray((*geometry)->vertexArrayObject);

    glGenBuffers(1, &((*geometry)->vertexBufferObject));
    glBindBuffer(GL_ARRAY_BUFFER, (*geometry)->vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bannerVertexData), bannerVertexData, GL_STATIC_DRAW);

    glEnableVertexAttribArray(bannerShaderProgram.posLocation);
    glEnableVertexAttribArray(bannerShaderProgram.texCoordLocation);
    // vertices of triangles - start at the beginning of the interlaced array
    glVertexAttribPointer(bannerShaderProgram.posLocation, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
    // texture coordinates of each vertices are stored just after its position
    glVertexAttribPointer(bannerShaderProgram.texCoordLocation, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    (*geometry)->numTriangles = bannerNumQuadVertices;
}

void initSparkleGeometry(GLuint shader, MeshGeometry** geometry) {

    *geometry = new MeshGeometry;

    (*geometry)->texture = pgr::createTexture(SPARKLE_TEXTURE_NAME);

    glGenVertexArrays(1, &((*geometry)->vertexArrayObject));
    glBindVertexArray((*geometry)->vertexArrayObject);

    glGenBuffers(1, &((*geometry)->vertexBufferObject));\
        glBindBuffer(GL_ARRAY_BUFFER, (*geometry)->vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sparkleVertexData), sparkleVertexData, GL_STATIC_DRAW);

    glEnableVertexAttribArray(sparkleShaderProgram.posLocation);
    // vertices of triangles - start at the beginning of the array (interlaced array)
    glVertexAttribPointer(sparkleShaderProgram.posLocation, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);

    glEnableVertexAttribArray(sparkleShaderProgram.texCoordLocation);
    // texture coordinates are placed just after the position of each vertex (interlaced array)
    glVertexAttribPointer(sparkleShaderProgram.texCoordLocation, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    (*geometry)->numTriangles = sparkleNumQuadVertices;
}


void initializeChair(SCommonShaderProgram& shader, MeshGeometry** geometry) {
    *geometry = new MeshGeometry;

    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * object_2_n_vertices * object_2_n_attribs_per_vertex, object_2_vertices, GL_STATIC_DRAW);
    CHECK_GL_ERROR();

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * object_2_n_triangles * 3, object_2_triangles, GL_STATIC_DRAW);
    CHECK_GL_ERROR();

    const GLsizei stride = sizeof(float) * object_2_n_attribs_per_vertex;

    glEnableVertexAttribArray(shader.posLocation);
    glVertexAttribPointer(shader.posLocation, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));

    glEnableVertexAttribArray(shader.texCoordLocation);
    glVertexAttribPointer(shader.texCoordLocation, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 3));

    glEnableVertexAttribArray(shader.normalLocation);
    glVertexAttribPointer(shader.normalLocation, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(sizeof(float) * 6));

    (*geometry)->vertexArrayObject = vao;
    (*geometry)->vertexBufferObject = vbo;
    (*geometry)->elementBufferObject = ebo;

    (*geometry)->ambient = glm::vec3(1.0f);
    (*geometry)->diffuse = glm::vec3(0.5f);
    (*geometry)->specular = glm::vec3(0.5f);
    (*geometry)->shininess = 5.0f;
    (*geometry)->texture = pgr::createTexture("data/Material_baseColor.png");
    (*geometry)->numTriangles = object_2_n_triangles;

    glBindVertexArray(0);
}





void initializeModels() {

  // load slenderman model from external file
  if(loadSingleMesh(SLENDERMAN_MODEL_NAME, shaderProgram, &slendermanGeometry) != true) {
    std::cerr << "initializeModels(): Slenderman model loading failed." << std::endl;
  }
  CHECK_GL_ERROR();

  // load space ship model from external file
  if(loadSingleMesh(PLAYER_MODEL_NAME, shaderProgram, &playerGeometry) != true) {
    std::cerr << "initializeModels(): Space ship model loading failed." << std::endl;
  }
  CHECK_GL_ERROR();

  // load forest model from external file
  if (loadSingleMesh(PINE_MODEL_NAME, shaderProgram, &pineForestGeometry) != true) {
      std::cerr << "initializeModels(): Pines model loading failed." << std::endl;
  }
  CHECK_GL_ERROR();

  // load ground model from external file
  if (loadSingleMesh(GROUND_MODEL_NAME, shaderProgram, &groundGeometry) != true) {
      std::cerr << "initializeModels(): Ground model loading failed." << std::endl;
  }
  CHECK_GL_ERROR();

  // load mountain model from external file
  if (loadSingleMesh(MOUNTAIN_MODEL_NAME, shaderProgram, &mountainGeometry) != true) {
      std::cerr << "initializeModels(): Mountain model loading failed." << std::endl;
  }
  CHECK_GL_ERROR();

  // load cabin model from external file
  if (loadSingleMesh(CABIN_MODEL_NAME, shaderProgram, &cabinGeometry) != true) {
      std::cerr << "initializeModels(): Cabin model loading failed." << std::endl;
  }
  CHECK_GL_ERROR();

  // load crow model from external file
  if (loadSingleMesh(CROW_MODEL_NAME, shaderProgram, &crowGeometry) != true) {
      std::cerr << "initializeModels(): Crow model loading failed." << std::endl;
  }
  CHECK_GL_ERROR();

  // load grass model from external file
  if (loadSingleMesh(GRASS_MODEL_NAME, shaderProgram, &grassGeometry) != true) {
      std::cerr << "initializeModels(): Grass model loading failed." << std::endl;
  }
  CHECK_GL_ERROR();

  // load bush model from external file
  if (loadSingleMesh(BUSH_MODEL_NAME, shaderProgram, &bushGeometry) != true) {
      std::cerr << "initializeModels(): Bush model loading failed." << std::endl;
  }
  CHECK_GL_ERROR();

  // load coin model from external file
  if (loadSingleMesh(COIN_MODEL_NAME, shaderProgram, &coinGeometry) != true) {
      std::cerr << "initializeModels(): Coin model loading failed." << std::endl;
  }
  CHECK_GL_ERROR();

  //// fill MeshGeometry structure for plane object
  initializeChair(shaderProgram, &chairGeometry);

  // fill MeshGeometry structure for skybox object
  initSkyboxGeometry(skyboxFarPlaneShaderProgram.program, &skyboxGeometry);

  // fill MeshGeometry structure for banner object
  initBannerGeometry(bannerShaderProgram.program, &bannerGeometry);

  // fill MeshGeometry structure for sparkle object
  initSparkleGeometry(sparkleShaderProgram.program, &sparkleGeometry);
}

void cleanupGeometry(MeshGeometry *geometry) {

  glDeleteVertexArrays(1, &(geometry->vertexArrayObject));
  glDeleteBuffers(1, &(geometry->elementBufferObject));
  glDeleteBuffers(1, &(geometry->vertexBufferObject));

  if(geometry->texture != 0)
    glDeleteTextures(1, &(geometry->texture));
}

void cleanupModels() {

  cleanupGeometry(playerGeometry);
  cleanupGeometry(slendermanGeometry);
  cleanupGeometry(pineForestGeometry);
  cleanupGeometry(groundGeometry);
  cleanupGeometry(mountainGeometry);
  cleanupGeometry(cabinGeometry);
  cleanupGeometry(crowGeometry);
  cleanupGeometry(grassGeometry);
  cleanupGeometry(bushGeometry);
  cleanupGeometry(coinGeometry);

  cleanupGeometry(bannerGeometry);
  cleanupGeometry(skyboxGeometry);
  cleanupGeometry(sparkleGeometry);
}
