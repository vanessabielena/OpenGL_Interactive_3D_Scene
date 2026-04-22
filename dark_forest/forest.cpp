//----------------------------------------------------------------------------------------
/**
 * \file    forest.cpp
 * \author  bielevan
 * \date    2025/05/04
 * \brief   A dark forest scene inspired by the old Slenderman game
 */
//----------------------------------------------------------------------------------------

#include <time.h>
#include <list>s
#include "pgr.h"
#include "render.h"
#include "spline.h"
#include "pgr_mesh.h"
#include <iostream>


extern SCommonShaderProgram shaderProgram;
extern bool useLighting;

// List of flashlight colors
std::vector<glm::vec3> flashlightColors = {
    glm::vec3(1.0f), // Basic white
    glm::vec3(1.0f, 0.0f, 0.0f), // Red
    glm::vec3(0.0f, 1.0f, 0.0f), // Green
    glm::vec3(0.0f, 0.0f, 1.0f), // Blue
    glm::vec3(1.0f, 1.0f, 1.0f), // White
    glm::vec3(1.0f, 1.0f, 0.0f), // Yellow
    glm::vec3(0.0f, 1.0f, 1.0f), // Cyan
    glm::vec3(1.0f, 0.0f, 1.0f)  // Magenta
};
int currentColorIndex = 0;


typedef std::list<void *> GameObjectsList; 

struct GameState {

  int windowWidth;    // set by reshape callback
  int windowHeight;   // set by reshape callback

  bool cameraFollowsMouseMode;        // false;
  float cameraElevationAngle; // in degrees = initially 0.0f
  float cameraAzimuthAngle;

  float elapsedTime;

  bool flashlightOn = false;
  glm::vec3 flashlightColor = flashlightColors[currentColorIndex];
  bool pointlightOn = false;

  bool slendermanClicked = false;

} gameState;

int mouseX;
int mouseY;
unsigned char objectId = 0;
int coinNumber = 8;
bool displayBanner = true;


//Static cameras
glm::vec3 cam1Position = glm::vec3(-0.314527f, 0.523812f, 0.291453f);
glm::vec3 cam1Direction = glm::vec3(0.781831f, -0.623490f, -0.000000f);
float cam1ElevationAngle = -18.0000f;
float cam1Angle = 330.0000f;

glm::vec3 cam2Position = glm::vec3(0.702181f, 0.741275f, 0.03f);
glm::vec3 cam2Direction = glm::vec3(-0.707107f, -0.307107f, 0.000000f);
float cam2ElevationAngle = 0.5000f;
float cam2Angle = 225.0000f;

glm::vec3 cam3Position = glm::vec3(-0.752734f, 0.998000f, 0.601845f);
glm::vec3 cam3Direction = glm::vec3(0.42020f, -0.439693f, 0.000000f);
float cam3ElevationAngle = -20.0000f;
float cam3Angle = 200.0000f;




struct GameObjects {

  PlayerObject *player; // NULL

  SlendermanObject* slenderman;
  PineForestObject* pineForest;
  GroundObject* ground;
  MountainObject* mountain;
  CabinObject* cabin;
  CrowObject* crow;
  ChairObject* chair;
  GameObjectsList grass;
  GameObjectsList bush;
  GameObjectsList coin;

  BannerObject* bannerObject; // NULL;
  GameObjectsList sparkle;

} gameObjects;


glm::vec3 boundariesCheck(const glm::vec3& position, float objectSize) {
    glm::vec3 newPosition = position;

    // Clamp X coordinate
    if (position.x < -2*SCENE_WIDTH + objectSize) {
        newPosition.x = -2*SCENE_WIDTH + objectSize;
    }
    if (position.x > 2*SCENE_WIDTH - objectSize) {
        newPosition.x = 2*SCENE_WIDTH - objectSize;
    }

    // Clamp Y coordinate
    if (position.y < -2*SCENE_HEIGHT + objectSize) {
        newPosition.y = -2*SCENE_HEIGHT + objectSize;
    }
    if (position.y > 2*SCENE_HEIGHT - objectSize) {
        newPosition.y = 2*SCENE_HEIGHT - objectSize;
    }

    // Z coordinate remains unchanged
    return newPosition;
}


void moveForward() {
    const float moveStep = 0.01f; // Adjusting step size as needed
    if (gameObjects.player->isFree) {
        glm::vec3 forward = glm::normalize(gameObjects.player->direction); // This stores where the player is facing
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0))); // Right direction based on world up (Y-axis)
        gameObjects.player->position += forward * moveStep;
    }
}

void moveRight() {
    const float moveStep = 0.01f; // Adjust step size as needed
    if (gameObjects.player->isFree) {
        glm::vec3 forward = glm::normalize(gameObjects.player->direction);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 0, 1)));
        gameObjects.player->position += right * moveStep;
    }
}

void moveLeft() {
    const float moveStep = 0.01f; // Adjust step size as needed
    if (gameObjects.player->isFree) {
        glm::vec3 forward = glm::normalize(gameObjects.player->direction);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 0, 1)));
        gameObjects.player->position -= right * moveStep;
    }
}

void moveBackward() {
    const float moveStep = 0.01f; // Adjusting step size as needed
    if (gameObjects.player->isFree) {
        glm::vec3 forward = glm::normalize(gameObjects.player->direction); // This stores where the player is facing
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0))); // Right direction based on world up (Y-axis)
        gameObjects.player->position -= forward * moveStep;
    }
}


void turnPlayerLeft(float deltaAngle) {

  gameObjects.player->viewAngle += deltaAngle;

  if(gameObjects.player->viewAngle > 360.0f)
    gameObjects.player->viewAngle -= 360.0f;

  const float angle = glm::radians(gameObjects.player->viewAngle);

  gameObjects.player->direction.x = cos(angle);
  gameObjects.player->direction.y = sin(angle);
}

void turnPlayerRight(float deltaAngle) {

  gameObjects.player->viewAngle -= deltaAngle;

  if(gameObjects.player->viewAngle < 0.0f)
    gameObjects.player->viewAngle += 360.0f;

  const float angle = glm::radians(gameObjects.player->viewAngle);

  gameObjects.player->direction.x = cos(angle);
  gameObjects.player->direction.y = sin(angle);
}

void turnPlayerUp(float deltaAngle) {
    gameState.cameraElevationAngle -= deltaAngle;

    const float angle = glm::radians(gameState.cameraElevationAngle);

    gameObjects.player->direction.z = angle; //sin(angle);
}

void turnPlayerDown(float deltaAngle) {
    gameState.cameraElevationAngle -= deltaAngle;

    const float angle = glm::radians(gameState.cameraElevationAngle);

    gameObjects.player->direction.z = angle; //sin(angle);
}

void insertSparkle(const glm::vec3& position) {

    SparkleObject* newSparkle = new SparkleObject;

    newSparkle->speed = 0.0f;
    newSparkle->destroyed = false;

    newSparkle->startTime = gameState.elapsedTime;
    newSparkle->currentTime = newSparkle->startTime;

    newSparkle->size = BILLBOARD_SIZE;
    newSparkle->direction = glm::vec3(0.0f, 0.0f, 1.0f);

    newSparkle->frameDuration = 0.1f;
    newSparkle->textureFrames = 8;

    newSparkle->position = position - glm::vec3(0.0f, 0.0f, 0.04f);

    gameObjects.sparkle.push_back(newSparkle);
}

void cleanUpObjects(void) {

  // delete slenderman
  if (gameObjects.slenderman != nullptr) {
      delete gameObjects.slenderman;
      gameObjects.slenderman = nullptr;
  }

  // delete forest
  if (gameObjects.pineForest != nullptr) {
      delete gameObjects.pineForest;
      gameObjects.pineForest = nullptr;
  }

  // delete ground
  if (gameObjects.ground != nullptr) {
      delete gameObjects.ground;
      gameObjects.ground = nullptr;
  }

  // delete mountain
  if (gameObjects.mountain != nullptr) {
      delete gameObjects.mountain;
      gameObjects.mountain = nullptr;
  }

  // delete cabin
  if (gameObjects.cabin != nullptr) {
      delete gameObjects.cabin;
      gameObjects.cabin = nullptr;
  }

  // delete chair
  if (gameObjects.chair != nullptr) {
      delete gameObjects.chair;
      gameObjects.chair = nullptr;
  }

  // delete crow
  if (gameObjects.crow != nullptr) {
      delete gameObjects.crow;
      gameObjects.crow = nullptr;
  }

  // delete grass
  while (!gameObjects.grass.empty()) {
      delete gameObjects.grass.back();
      gameObjects.grass.pop_back();
  }

  // delete bush
  while (!gameObjects.bush.empty()) {
      delete gameObjects.bush.back();
      gameObjects.bush.pop_back();
  }

  // delete coin
  while (!gameObjects.coin.empty()) {
      delete gameObjects.coin.back();
      gameObjects.coin.pop_back();
  }

  // delete sparkle
  while (!gameObjects.sparkle.empty()) {
      delete gameObjects.sparkle.back();
      gameObjects.sparkle.pop_back();
  }
}

// Generates random position that does not collide with the player
glm::vec3 generateRandomPosition(void) {
 glm::vec3 newPosition;

    newPosition = glm::vec3(
      (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
      (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
      0.0f
    );

  return newPosition;
}

// Generates random position that does not collide with the Cabin
glm::vec3 generateRandomPositionOutsideCabin(void) {
    glm::vec3 newPosition;

    do {
        newPosition = glm::vec3(
            (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
            (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
            0.0f
        );
    } while (newPosition.x > -0.5f && newPosition.x < 0.5f &&
        newPosition.y > -0.5f && newPosition.y < 0.5f);

    return newPosition;
}


SlendermanObject* createSlenderman(void) {
 SlendermanObject* newSlenderman = new SlendermanObject;

  newSlenderman->destroyed = false;

  newSlenderman->startTime = gameState.elapsedTime;
  newSlenderman->currentTime = newSlenderman->startTime;

  newSlenderman->size = SLENDERMAN_SIZE;

  // Generate motion direction randomly in range -1.0f ... 1.0f
  newSlenderman->direction = glm::vec3(
    (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
    (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
    0.0f
  );

  // Generate a random pitch angle between 0 and 360 degrees
  newSlenderman->pitch = static_cast<float>(rand() % 360);

  newSlenderman->direction = glm::normalize(newSlenderman->direction);

  newSlenderman->position = generateRandomPositionOutsideCabin();

  newSlenderman->speed = 0.0f;

  return newSlenderman;
}

PineForestObject* createPineForest(void) {
    PineForestObject* newPineForest = new PineForestObject;

    newPineForest->destroyed = false;

    newPineForest->startTime = gameState.elapsedTime;
    newPineForest->currentTime = newPineForest->startTime;

    newPineForest->size = 2.2f;

    // generate initial position randomly
    newPineForest->initPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    newPineForest->position = newPineForest->initPosition;

    newPineForest->direction = glm::vec3(
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        0.0f
    );
    newPineForest->direction = glm::normalize(newPineForest->direction);

    return newPineForest;
}

GroundObject* createGround(void) {
    GroundObject* newGround = new GroundObject;

    newGround->destroyed = false;

    newGround->startTime = gameState.elapsedTime;
    newGround->currentTime = newGround->startTime;

    newGround->size = 2.0f;

    // generate initial position randomly
    newGround->initPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    newGround->position = newGround->initPosition;

    newGround->direction = glm::vec3(
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        0.0f
    );
    newGround->direction = glm::normalize(newGround->direction);

    return newGround;
}

MountainObject* createMountain(void) {
    MountainObject* newMountain = new MountainObject;

    newMountain->destroyed = false;

    newMountain->startTime = gameState.elapsedTime;
    newMountain->currentTime = newMountain->startTime;

    newMountain->size = 5.5f;

    // Generate initial position randomly
    newMountain->initPosition = glm::vec3(0.0f, 0.0f, 0.5f);
    newMountain->position = newMountain->initPosition;

    newMountain->direction = glm::vec3(
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        0.0f
    );
    newMountain->direction = glm::normalize(newMountain->direction);

    return newMountain;
}

CabinObject* createCabin(void) {
    CabinObject* newCabin = new CabinObject;

    newCabin->destroyed = false;

    newCabin->startTime = gameState.elapsedTime;
    newCabin->currentTime = newCabin->startTime;

    newCabin->size = 0.3f;

    // Generate initial position randomly
    newCabin->initPosition = glm::vec3(0.0f, 0.0f, 0.2f);
    newCabin->position = newCabin->initPosition;

    newCabin->direction = glm::vec3(
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        0.0f
    );
    newCabin->direction = glm::normalize(newCabin->direction);

    return newCabin;
}

ChairObject* createChair(void) {
    ChairObject* newChair = new ChairObject;

    newChair->destroyed = false;

    newChair->startTime = gameState.elapsedTime;
    newChair->currentTime = newChair->startTime;

    newChair->size = 0.2f;

    // Generate initial position randomly
    newChair->initPosition = glm::vec3(0.0f, 0.0f, -0.18f);
    newChair->position = newChair->initPosition;

    newChair->direction = glm::vec3(0.f, 0.0f, 0.0f);
    newChair->direction = glm::normalize(newChair->direction);

    return newChair;
}

CrowObject* createCrow(void) {
    CrowObject* newCrow = new CrowObject;

    newCrow->destroyed = false;

    newCrow->startTime = gameState.elapsedTime;
    newCrow->currentTime = newCrow->startTime;
    newCrow->speed = 0.5f;

    newCrow->size = 0.1f;

    // Generate initial position randomly
    newCrow->initPosition = glm::vec3(0.0f, 0.0f, 0.2f);
    newCrow->position = newCrow->initPosition;

    newCrow->direction = glm::vec3(
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        0.0f
    );
    newCrow->direction = glm::normalize(newCrow->direction);

    return newCrow;
}

GrassObject* createGrass(void) {
    GrassObject* newGrass = new GrassObject;

    newGrass->destroyed = false;

    newGrass->startTime = gameState.elapsedTime;
    newGrass->currentTime = newGrass->startTime;

    newGrass->size = 0.04f;

    // Generate initial position randomly
    newGrass->initPosition = generateRandomPositionOutsideCabin();
    
    newGrass->position = newGrass->initPosition;

    newGrass->direction = glm::vec3(
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        0.0f
    );
    newGrass->direction = glm::normalize(newGrass->direction);

    return newGrass;
}

BushObject* createBush(void) {
    BushObject* newBush = new BushObject;

    newBush->destroyed = false;

    newBush->startTime = gameState.elapsedTime;
    newBush->currentTime = newBush->startTime;

    newBush->size = 0.04f;

    // Generate initial position randomly
    newBush->initPosition = generateRandomPositionOutsideCabin();

    newBush->position = newBush->initPosition;

    newBush->direction = glm::vec3(
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        0.0f
    );
    newBush->direction = glm::normalize(newBush->direction);

    return newBush;
}

CoinObject* createCoin(int id) {
    CoinObject* newCoin = new CoinObject;

    newCoin->destroyed = false;

    newCoin->startTime = gameState.elapsedTime;
    newCoin->currentTime = newCoin->startTime;

    newCoin->size = 0.03f;

    newCoin->id = id;

    // Generate initial position randomly
    newCoin->initPosition = generateRandomPositionOutsideCabin();
    newCoin->position = newCoin->initPosition;

    newCoin->direction = glm::vec3(
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        (float)(2.0 * (rand() / (double)RAND_MAX) - 1.0),
        0.0f
    );
    newCoin->direction = glm::normalize(newCoin->direction);

    return newCoin;
}

BannerObject* createBanner(void) {
    BannerObject* newBanner = new BannerObject;

    newBanner->size = BANNER_SIZE;
    newBanner->position = glm::vec3(0.5f, -0.5f, 0.0f);
    newBanner->direction = glm::vec3(0.0f, 1.0f, 0.0f);
    newBanner->speed = 0.0f;
    newBanner->size = 2.0f;

    newBanner->destroyed = false;

    newBanner->startTime = gameState.elapsedTime;
    newBanner->currentTime = newBanner->startTime;

    return newBanner;
}



void teleport(void) {
    // Delete slenderman
    if (gameObjects.slenderman != nullptr) {
        delete gameObjects.slenderman;
        gameObjects.slenderman = nullptr;
    }

    SlendermanObject* newSlenderman = createSlenderman();

    gameObjects.slenderman = newSlenderman;
    
}



void restartGame(void) {

  cleanUpObjects();

  gameState.elapsedTime = 0.001f * (float)glutGet(GLUT_ELAPSED_TIME); 

  // Initialize player
  if(gameObjects.player == NULL)
    gameObjects.player = new PlayerObject;

  gameObjects.player->position = glm::vec3(0.0f, 0.0f, 0.0f);
  gameObjects.player->viewAngle = 90.0f; // degrees
  gameObjects.player->direction = glm::vec3(cos(glm::radians(gameObjects.player->viewAngle)), sin(glm::radians(gameObjects.player->viewAngle)), 0.0f);
  gameObjects.player->speed = 0.0f;
  gameObjects.player->size = PLAYER_SIZE;
  gameObjects.player->isFree = true;
  gameObjects.player->destroyed = false;
  gameObjects.player->startTime = gameState.elapsedTime;
  gameObjects.player->currentTime = gameObjects.player->startTime;

  SlendermanObject* newSlenderman = createSlenderman();
    
  gameObjects.slenderman = newSlenderman; 

  gameState.cameraElevationAngle = 0.0f;
  gameState.cameraAzimuthAngle = 0.0f;

}

void drawWindowContents() {

  // Parallel projection
  const glm::mat4 orthoProjectionMatrix = glm::ortho(
    -SCENE_WIDTH, SCENE_WIDTH,
    -SCENE_HEIGHT, SCENE_HEIGHT,
    -10.0f*SCENE_DEPTH, 10.0f*SCENE_DEPTH
  );
  // static viewpoint - top view
  const glm::mat4 orthoViewMatrix = glm::lookAt(
    glm::vec3(0.0f, 0.0f, 1.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f)
  );

    // Compute camera position and direction
    glm::vec3 cameraPosition = gameObjects.player->position;
    glm::vec3 cameraUpVector = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 cameraViewDirection = gameObjects.player->direction;

    // Apply camera elevation angle
    glm::vec3 rotationAxis = glm::cross(cameraViewDirection, cameraUpVector);
    glm::mat4 cameraTransform = glm::rotate(glm::mat4(1.0f), glm::radians(gameState.cameraElevationAngle), rotationAxis);

    cameraUpVector = glm::vec3(cameraTransform * glm::vec4(cameraUpVector, 0.0f));
    cameraViewDirection = glm::vec3(cameraTransform * glm::vec4(cameraViewDirection, 0.0f));

    glm::vec3 cameraCenter = cameraPosition + cameraViewDirection;

    glm::mat4 viewMatrix = glm::lookAt(cameraPosition, cameraCenter, cameraUpVector);
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f), gameState.windowWidth / (float)gameState.windowHeight, 0.1f, 100.0f);


  glUseProgram(shaderProgram.program);
  glUniform1f(shaderProgram.timeLocation, gameState.elapsedTime);

  glUniform3fv(shaderProgram.spotLightPositionLocation, 1, glm::value_ptr(gameObjects.player->position));
  glUniform3fv(shaderProgram.spotLightDirectionLocation, 1, glm::value_ptr(gameObjects.player->direction));
  glUniform1i(shaderProgram.flashlightOn, gameState.flashlightOn);
  
  glUniform3fv(shaderProgram.flashlightColor, 1, glm::value_ptr(gameState.flashlightColor));
  glUniform1i(shaderProgram.pointlightOn, gameState.pointlightOn);
  glUseProgram(0);
  glEnable(GL_STENCIL_TEST);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

  // draw player
  drawPlayer(gameObjects.player, viewMatrix, projectionMatrix);

  CHECK_GL_ERROR(); 
 

  drawPineForest(gameObjects.pineForest, viewMatrix, projectionMatrix);

  // draw mountains
  drawMountain(gameObjects.mountain, viewMatrix, projectionMatrix);

  // draw cabins
  drawCabin(gameObjects.cabin, viewMatrix, projectionMatrix);

  // draw chair
  drawChair(gameObjects.chair, viewMatrix, projectionMatrix);

  // draw crow
  drawCrow(gameObjects.crow, viewMatrix, projectionMatrix);

  // draw ground
  drawGround(gameObjects.ground, viewMatrix, projectionMatrix);

  glm::mat4 modelMatrix = glm::mat4(1.0f); // identity matrix
  glm::mat4 pvmMatrix = projectionMatrix * viewMatrix * modelMatrix;


  // draw grass
  for (GameObjectsList::const_iterator it = gameObjects.grass.cbegin(); it != gameObjects.grass.cend(); ++it) {
      GrassObject* grass = (GrassObject*)(*it);
    drawGrass(grass, viewMatrix, projectionMatrix);
  }

  // draw bush
  for (GameObjectsList::const_iterator it = gameObjects.bush.cbegin(); it != gameObjects.bush.cend(); ++it) {
      BushObject* bush = (BushObject*)(*it);
      drawBush(bush, viewMatrix, projectionMatrix);
  }

  for (GameObjectsList::const_iterator it = gameObjects.coin.cbegin(); it != gameObjects.coin.cend(); ++it) {
      CoinObject* coin = static_cast<CoinObject*>(*it);
      glStencilFunc(GL_ALWAYS, coin->id, 0xFF);  // Use coin->id as the stencil value
      drawCoin(coin, viewMatrix, projectionMatrix);
  }

  glStencilFunc(GL_ALWAYS, 8, 0x11);
  drawSlenderman(gameObjects.slenderman, viewMatrix, projectionMatrix);

  // draw skybox
  drawSkybox(viewMatrix, projectionMatrix);

  // draw sparkle with depth test disabled
  glDisable(GL_DEPTH_TEST);

  for (GameObjectsList::const_iterator it = gameObjects.sparkle.cbegin(); it != gameObjects.sparkle.cend(); ++it) {
      SparkleObject* sparkle = (SparkleObject*)(*it);
      drawSparkle(sparkle, viewMatrix, projectionMatrix);
  }
  glEnable(GL_DEPTH_TEST);

  // draw banner
  if ((gameObjects.bannerObject != NULL) && (displayBanner == true))
  drawBanner(gameObjects.bannerObject, orthoViewMatrix, orthoProjectionMatrix);

}

// Called to update the display
void displayCallback() {
    // Clear color, depth, and stencil buffers
  GLbitfield mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;

  mask |= GL_STENCIL_BUFFER_BIT;
  glClear(mask);

  // Draw the game scene
  drawWindowContents();

  // Swap buffers to display the rendered image
  glutSwapBuffers();
}

// Responds to resiying the window
void reshapeCallback(int newWidth, int newHeight) {

  gameState.windowWidth = newWidth;
  gameState.windowHeight = newHeight;

  // Set the new viewport to match window size
  glViewport(0, 0, (GLsizei) newWidth, (GLsizei) newHeight);
}

void updateObjects(float elapsedTime) {

    // Update player position based on time and movement
  const float timeDelta = elapsedTime - gameObjects.player->currentTime;
  gameObjects.player->currentTime = elapsedTime;
  gameObjects.player->position += timeDelta * gameObjects.player->speed * gameObjects.player->direction;

  // update slendermans
  GameObjectsList::iterator it;

  // Keep player within bounds
  gameObjects.player->position = boundariesCheck(gameObjects.player->position, gameObjects.player->size);

  // Update slenderman logic
  SlendermanObject* slenderman = gameObjects.slenderman;

  if (slenderman) {
      slenderman->currentTime = elapsedTime;

      float aliveTime = slenderman->currentTime - slenderman->startTime;

      if (aliveTime >= 8.0f) {
          // Teleport (delete and create a new slenderman)
          teleport();
      }
  }
  
  // Move crow along a curve if it's present
  if (gameObjects.crow && flyingCurve && curveSize > 0) {
      gameObjects.crow->currentTime = elapsedTime;

      float curveParam = gameObjects.crow->speed * (elapsedTime - gameObjects.crow->startTime);

      gameObjects.crow->position = evaluateClosedCurve(flyingCurve, curveSize, curveParam);

      // Update crow's direction along the curve
      glm::vec3 dir = evaluateClosedCurve_1stDerivative(flyingCurve, curveSize, curveParam);
      if (glm::length(dir) > 0.001f) {
          gameObjects.crow->direction = glm::normalize(dir);
      }
      else {
          gameObjects.crow->direction = glm::vec3(1, 0, 0);
      }
  }

  // Update sparkle animations
  it = gameObjects.sparkle.begin();
  while (it != gameObjects.sparkle.end()) {
      SparkleObject* sparkle = (SparkleObject*)(*it);
      sparkle->currentTime = elapsedTime;
      // Check if sparkle animation finished
      if (sparkle->currentTime > sparkle->startTime + sparkle->textureFrames * sparkle->frameDuration)
          sparkle->destroyed = true;
      // Remove finished sparkles
      if (sparkle->destroyed == true) {
          it = gameObjects.sparkle.erase(it);
      }
      else {
          ++it;
      }
  }

  // Update coin rotation
  it = gameObjects.coin.begin();
  while (it != gameObjects.coin.end()) {
      CoinObject* coin = (CoinObject*)(*it);

      float delta = elapsedTime - coin->currentTime;
      coin->currentTime = elapsedTime;

      coin->rotationAngle += 60.0f * delta;  // 60 degrees per second
      if (coin->rotationAngle > 360.0f) {
          coin->rotationAngle -= 360.0f;
      }

      ++it;
  }


}

// Responsible for the scene update
void timerCallback(int) {
  // Get the current time in seconds
  gameState.elapsedTime = 0.001f * (float)glutGet(GLUT_ELAPSED_TIME); // milliseconds => seconds
  // Update banner time if it exists
  if (gameObjects.bannerObject != NULL) {
      gameObjects.bannerObject->currentTime = gameState.elapsedTime;
  }

  // Update positions and animations
  updateObjects(gameState.elapsedTime);


  // Create default objects if they don't exist
  if (gameObjects.pineForest == nullptr) gameObjects.pineForest = createPineForest();
  if (gameObjects.ground == nullptr) gameObjects.ground = createGround();
  if (gameObjects.mountain == nullptr) gameObjects.mountain = createMountain();
  if (gameObjects.cabin == nullptr) gameObjects.cabin = createCabin();
  if (gameObjects.chair == nullptr) gameObjects.chair = createChair();
  if (gameObjects.crow == nullptr) gameObjects.crow = createCrow();
  
  // generate new grass patches randomly
  if (gameObjects.grass.size() < GRASS_COUNT_MIN) {
      int howManyGrasses = rand() % (GRASS_COUNT_MAX - GRASS_COUNT_MIN + 1);

      for (int i = 0; i < howManyGrasses; i++) {
          GrassObject* newGrass = createGrass();

          gameObjects.grass.push_back(newGrass);
      }
  }

  // generate new bush patches randomly
  if (gameObjects.bush.size() < BUSH_COUNT_MIN) {
      int howManyBushes = rand() % (BUSH_COUNT_MAX - BUSH_COUNT_MIN + 1);

      for (int i = 0; i < howManyBushes; i++) {
          BushObject* newBush = createBush();

          gameObjects.bush.push_back(newBush);
      }
  }

  // Spawn new coins if none are present
  if (gameObjects.coin.size() == 0) {
      int howManyCoins = coinNumber;
      for (int i = 0; i < howManyCoins; i++) {
          CoinObject* newCoin = createCoin(i);
          gameObjects.coin.push_back(newCoin);
      }
  }

  // Create slenderman if it doesn't exist
  if (gameObjects.slenderman == nullptr) {
      gameObjects.slenderman = createSlenderman();
  }
  // Show game over banner if necessary
  if ((gameObjects.bannerObject == NULL) && (displayBanner == true)) {
      gameObjects.bannerObject = createBanner();
  }

  // Schedule next update
  glutTimerFunc(33, timerCallback, 0);

  glutPostRedisplay();
}

void passiveMouseMotionCallback(int mouseX, int mouseY) {
    // Get center of the screen
    int centerX = gameState.windowWidth / 2;
    int centerY = gameState.windowHeight / 2;

    // Calculate movement deltas
    float deltaX = mouseX - centerX;
    float deltaY = mouseY - centerY;

    // Horizontal camera rotation (left/right)
    const float cameraAzimuthAngleDelta = 0.2f * deltaX;
    gameState.cameraAzimuthAngle += cameraAzimuthAngleDelta;
    if (deltaX <= 0) {
        turnPlayerLeft(-cameraAzimuthAngleDelta);
    }
    else {
        turnPlayerRight(cameraAzimuthAngleDelta);
    }
    

    // Vertical camera rotation (up/down)
    const float cameraElevationAngleDelta = 0.05f * deltaY;

    if (deltaY < 0) {  // looking up
        gameState.cameraElevationAngle -= cameraElevationAngleDelta;
        gameState.cameraElevationAngle = glm::clamp(gameState.cameraElevationAngle, CAMERA_ELEVATION_MIN, CAMERA_ELEVATION_MAX);
        turnPlayerUp(cameraElevationAngleDelta);
    }
    else if (deltaY > 0) {  // looking down
        gameState.cameraElevationAngle -= cameraElevationAngleDelta;
        gameState.cameraElevationAngle = glm::clamp(gameState.cameraElevationAngle, CAMERA_ELEVATION_MIN, CAMERA_ELEVATION_MAX);
        turnPlayerDown(cameraElevationAngleDelta);
    }


    // Reset mouse position to center
    glutWarpPointer(centerX, centerY);

    // Request a redisplay
    glutPostRedisplay();

}

void keyboardCallback(unsigned char keyPressed, int mouseX, int mouseY) {
  
  switch(keyPressed) {
    case 27: // escape
#ifndef __APPLE__
        glutLeaveMainLoop();
#else
        exit(0);
#endif
      break;
    case 'w': // move forward 
        moveForward();
        break;
    case 'a': // move left
        moveLeft();
        break;
    case 'd': // move right
        moveRight();
        break;
    case 's': // move backward
        moveBackward();
        break;
    case 'f': // switch on flashlight
        gameState.flashlightOn = !gameState.flashlightOn;
        break;
    case 'p': // switch on pointlight
        gameState.pointlightOn = !gameState.pointlightOn;
        break;
    case 'c': // switch flashlight color
        currentColorIndex = (currentColorIndex + 1) % flashlightColors.size();
        gameState.flashlightColor = flashlightColors[currentColorIndex];
        break;
    case '1': // switch to first static camera
        gameObjects.player->isFree = false;
        gameObjects.player->position = cam1Position;
        gameObjects.player->direction = cam1Direction;
        gameState.cameraElevationAngle = cam1ElevationAngle;
        gameObjects.player->viewAngle = cam1Angle;
        displayBanner = false;
        glutPassiveMotionFunc(NULL);
        break;
    case '2': // switch to second static camera
        gameObjects.player->isFree = false;
        gameObjects.player->position = cam2Position;
        gameObjects.player->direction = cam2Direction;
        gameState.cameraElevationAngle = cam2ElevationAngle;
        gameObjects.player->viewAngle = cam2Angle;
        displayBanner = false;
        glutPassiveMotionFunc(NULL);
        break;
    case '3': // switch to third static camera
        gameObjects.player->isFree = false;
        gameObjects.player->position = cam3Position;
        gameObjects.player->direction = cam3Direction;
        gameState.cameraElevationAngle = cam3ElevationAngle;
        gameObjects.player->viewAngle = cam3Angle;
        displayBanner = false;
        glutPassiveMotionFunc(NULL);
        break;
    case '0': // switch on pointlight
        displayBanner = true;
        glutPassiveMotionFunc(passiveMouseMotionCallback);
        glutWarpPointer(gameState.windowWidth / 2, gameState.windowHeight / 2);
        gameObjects.player->isFree = true;
        gameObjects.player->position = glm::vec3(0.0f, 0.0f, 0.0f);
        gameObjects.player->viewAngle = 90.0f; // degrees
        gameObjects.player->direction = glm::vec3(cos(glm::radians(gameObjects.player->viewAngle)), sin(glm::radians(gameObjects.player->viewAngle)), 0.0f);
        gameState.cameraElevationAngle = 0.0f;
        gameState.cameraAzimuthAngle = 0.0f;
        break;
    case ' ': // turn off/on camera movement
      gameState.cameraFollowsMouseMode = !gameState.cameraFollowsMouseMode;
      if(gameState.cameraFollowsMouseMode == true) {
        glutPassiveMotionFunc(passiveMouseMotionCallback);
        glutWarpPointer(gameState.windowWidth/2, gameState.windowHeight/2);
      }
      else {
        glutPassiveMotionFunc(NULL);
      }
      break;
    default:
      ;
  }
}

// Triggered when keyboard function or directional keys are pressed.
void specialKeyboardCallback(int specKeyPressed, int mouseX, int mouseY) {
    switch (specKeyPressed) {
    case GLUT_KEY_RIGHT: moveRight(); break;
    case GLUT_KEY_LEFT: moveLeft(); break;
    case GLUT_KEY_UP: moveForward(); break;
    case GLUT_KEY_DOWN: moveBackward(); break;
    default: break;
    }
}



void onMouseClick(int button, int state, int mouseX, int mouseY) {

    if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN)) {
        glReadPixels(mouseX, WINDOW_HEIGHT - mouseY - 1, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &objectId);

        // Search and remove the coin with matching ID
        if ((objectId <= 7) && (coinNumber != 0)) {
            std::cout << "Mouse x: " << mouseX << ", y: " << mouseY << ", stencil id: " << (int)objectId << "\n";
            for (GameObjectsList::iterator it = gameObjects.coin.begin(); it != gameObjects.coin.end(); ++it) {
                CoinObject* coin = static_cast<CoinObject*>(*it);
                if ((GLuint)coin->id == objectId) {
                    // Coin matched — remove it from the list
                    insertSparkle(coin->position);
                    gameObjects.coin.erase(it);
                    delete coin; // Free memory if dynamically allocated
                    coinNumber--;
                    break;
                }
            }
        }
    }
}

// Called after the window and OpenGL are initialized. Called exactly once, before the main loop.
void initializeApplication() {
  srand ((unsigned int)time(NULL));
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClearStencil(0);
  glEnable(GL_DEPTH_TEST);
  useLighting = true;

  initializeShaderPrograms();
  initializeModels();

  gameObjects.player = NULL;
  restartGame();
}

void finalizeApplication(void) {
  cleanUpObjects();
  delete gameObjects.player;
  gameObjects.player = NULL;
 
  cleanupModels();
  cleanupShaderPrograms();
}

int main(int argc, char** argv) {
  glutInit(&argc, argv);

#ifndef __APPLE__
  glutInitContextVersion(pgr::OGL_VER_MAJOR, pgr::OGL_VER_MINOR);
  glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);

  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
#else
  glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
#endif

  // Initial window size
  glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
  glutCreateWindow(WINDOW_TITLE);
  glutDisplayFunc(displayCallback);
  glutReshapeFunc(reshapeCallback);
  glutKeyboardFunc(keyboardCallback);
  glutMouseFunc(onMouseClick);
  glutSpecialFunc(specialKeyboardCallback);     // key pressed
  glutPassiveMotionFunc(passiveMouseMotionCallback);
  glutWarpPointer(gameState.windowWidth / 2, gameState.windowHeight / 2);


  glutTimerFunc(33, timerCallback, 0);

  if(!pgr::initialize(pgr::OGL_VER_MAJOR, pgr::OGL_VER_MINOR))
    pgr::dieWithError("pgr init failed, required OpenGL not supported?");

  initializeApplication();

#ifndef __APPLE__
  glutCloseFunc(finalizeApplication);
#else
  glutWMCloseFunc(finalizeApplication);
#endif

  glutMainLoop();

  return 0;
}
