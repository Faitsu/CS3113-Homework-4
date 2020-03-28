//Erica Chou 03/15/20
//Entity.cpp file, sets up functions for various entities used within the game

#include "Entity.h"
Entity::Entity() { //constructor
	position = glm::vec3(0);   
	movement = glm::vec3(0);
	acceleration = glm::vec3(0);
	velocity = glm::vec3(0);
	speed = 0;        
	modelMatrix = glm::mat4(1.0f); 
}

bool Entity::CheckCollision(Entity* other) {
	//checks if object collided with another object and returns a bool
	float xdist = fabs(position.x - other->position.x) - ((width + other->width) / 2.0f);
	float ydist = fabs(position.y - other->position.y) - ((height + other->height) / 2.0f);

	if (xdist < 0 && ydist < 0) { 
		
		return true;
	}

	return false;
}

void Entity::CheckCollisionsY(std::vector<Entity*> objects, int objectCount) { 
	//Fixs entity position if we do have a collision in the y axis
	float penetrationY = 0;
	for (int i = 0; i < objectCount; i++) { 
		Entity *object = objects[i];     
		if (CheckCollision(object) && object->isActive) {
			float ydist = fabs(position.y - object->position.y);   //for figuring out the difference of the penetration
			float temp = fabs(ydist - (height / 2.0f) - (object->height / 2.0f));
			if (penetrationY < temp) {
				penetrationY = temp;
			}
			if (velocity.y > 0) {//collision between an object occured on its top
				if (object->entityType == PLATFORM) {
					position.y -= penetrationY;
					velocity.y = 0;
				}
				collidedTop = true;
			}
			else if (velocity.y < 0) { //collision happened on bottom
				if (object->entityType == PLATFORM) {
					position.y += penetrationY;
					velocity.y = 0;
				}
				collidedBottom = true;
				if (entityType == PLAYER || enemyType == JUMPER) {
					jumpcount = 0;
					airborne = false;
				}
				if (entityType == PLAYER && object->entityType == ENEMY) {
					object->isActive = false;
					collidedBottom = false;
					return;
				}
			}
		}
	}
}

void Entity::CheckCollisionsX(std::vector<Entity*> objects, int objectCount) {
	//fixes position of object from the x-axis
	for (int i = 0; i < objectCount; i++) { 
		Entity* object = objects[i];    
		if (CheckCollision(object) && object->isActive) {
			float xdist = fabs(position.x - object->position.x);     
			float penetrationX = fabs(xdist - (width / 2.0f) - (object->width / 2.0f));        
			if (velocity.x > 0 ) { //collision to the right
				if (object->entityType ==PLATFORM) {
					if (entityType != PROJECTILE ) {
						position.x -= penetrationX;
						velocity.x = 0;
					}
					else {
						isActive = false;
					}
				}
				collidedRight = true;
			} 
			else if (velocity.x < 0 ) { //collision to the left
				if (object->entityType == PLATFORM) {
					if (entityType != PROJECTILE) {
						position.x += penetrationX;
						velocity.x = 0;
					}
					else {
						isActive = false;
					}
				}
				collidedLeft = true;
			} 
			if (collidedRight || collidedLeft) {//logic for enemy collisions with enemy and player or projectile
				if ((entityType == PLAYER && object->entityType ==ENEMY) ) {
					fail = true;
					isActive = false;
				}
				else if (entityType == ENEMY && object->entityType == PLAYER) {
					object->fail = true;
					object->isActive = false;
				}
				else if (entityType == PROJECTILE && object->entityType == ENEMY){
					isActive = false;
					if (object->enemyType != SLEEPER || object->hp <= 0 ) {//because of how the algorithm works
						object->isActive = false;
					}
					else {
						object->hp--;
					}
				}
			}
		}
	}
}



void Entity::AISleeper() {
	//do nothing for now
}

void Entity::AIJumper() {
	
	if (!fly && !airborne && counter >= 2.0f) {//fly is for pushing off the ground itself, airborne is for when the object is in the air
 		airborne = true;
		fly = true;
		counter = 0;
	}
}

void Entity::AIStalker(Entity* player) {
	if (!player->success && !player->fail) {
		float ydist = fabs(position.y - player->position.y) - ((height + player->height) / 2.0f);
		if (ydist <= 0) {
			chase = true;
		}
		else {
			chase = false;
		}
		if (chase) {
			if (position.x - player->position.x > 0) { //to the right
				movement.x = -1.0f;
			}
			else if (position.x - player->position.x < 0) {
				movement.x = 1.0f;
			}
		}
	}
	else {
		movement.x = 0;
	}
}

void Entity::AI(Entity* player) {
		if (entityType == ENEMY) {
			switch (enemyType) {
			case(SLEEPER):
				AISleeper(); //does nothing for now
				break;
			case(JUMPER):
				if (!player->success && !player->fail) {
					AIJumper();
				}
				break;
			case(STALKER):
				AIStalker(player);
				break;
			}

	}
}

void Entity::Shoot(Entity* player) {
	isActive = true;
	position = player->position;
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, position);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.0f));
	if (player->goLeft) {
		movement.x = -1.0f;
	}
	else {
		movement.x = 1.0f;
	}
}

void Entity::Update(float deltaTime, Entity* player, std::vector<Entity*>platforms, int platformCount) { 
	//updates entity
	collidedTop = false;
	collidedBottom = false;
	collidedLeft = false;
	collidedRight = false;
	if (success || fail || !isActive) { 
		//end of game if success or fail so we do not want to update positions and such
		AI(player);
		return;
	}

	if (animIndices != NULL) {
		if (glm::length(movement) == 0 && entityType == PLAYER) {
			idle = true;
		}
		else {
			idle = false;
		}

		animTime += deltaTime;

		if (animTime >= animTimer) {

			animTime = 0.0f;
			animIndex++;

			if (animIndex >= animFrames) {
				animIndex = 0;
			}

		}
	}

	if (entityType == BACKGROUND) {//we only need this to happen to background to make this bigger
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::scale(modelMatrix, glm::vec3(10.0f, 7.5f, 0.0f));
		return;
	}
	if (entityType == PLAYER || entityType == ENEMY) {
		//movement logic
		if (entityType == ENEMY) {
			AI(player);
		}
		if (fly) {
			fly = false;
			velocity.y = flyheight;
		}

		velocity.x = movement.x*speed;
		velocity += acceleration * deltaTime;

		position.x += velocity.x * deltaTime;
		CheckCollisionsX(platforms, platformCount);

		position.y += velocity.y*deltaTime;
		CheckCollisionsY(platforms, platformCount);

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);

	}
	if (entityType == PROJECTILE) {
		if (isActive) {
			velocity.x = movement.x*speed;

			position.x += velocity.x * deltaTime;
			CheckCollisionsX(platforms, platformCount);

		}
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.0f));
	}
	else {
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);
	}
	/*if (entityType == ENEMY ) {
		modelMatrix = glm::scale(modelMatrix, glm::vec3(1.5f, 1.5f, 0));
	}
	*/
}

void Entity::DrawSpriteFromTextureAtlas(ShaderProgram *program, GLuint textureID, int index) {
	//draws sprite from a sprite sheet
	float u = (float)(index % animCols) / (float)animCols;
	float v = (float)(index / animCols) / (float)animRows;
	
	float width = 1.0f / (float)animCols;
	float height = 1.0f / (float)animRows;
	
	float texCoords[] = { u, v + height, u + width, v + height, u + width, v, u, v + height, u + width, v, u, v };
	float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
	
	glBindTexture(GL_TEXTURE_2D, textureID);
	
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void Entity::Render(ShaderProgram* program) { 
	//renders an entity
	if (!isActive) {
		return;
	}
	program->SetModelMatrix(modelMatrix);    
	if (animIndices != NULL) {
		if (animIdle != NULL) {
			if (idle && !goLeft && !airborne) {
				DrawSpriteFromTextureAtlas(program, textureID, animIdle[0]);
				return;
			}
			else if (idle && goLeft && !airborne) {
				DrawSpriteFromTextureAtlas(program, textureID, animIdle[1]);
				return;
			}

		}
		if (velocity.y < 0 && animRightFly != NULL && animLeftFly != NULL) {
			if (!goLeft) {
				DrawSpriteFromTextureAtlas(program, textureID, animRightFly[0]);
				return;
			}
			else {
				DrawSpriteFromTextureAtlas(program, textureID, animLeftFly[0]);
				return;
			}
			return;
		}
		if (enemyType==JUMPER && airborne) {
			DrawSpriteFromTextureAtlas(program, textureID, animIndices[0]);
			return;
		}
		if (enemyType == JUMPER && !airborne) {
			DrawSpriteFromTextureAtlas(program, textureID, animIndices[1]);
			return;
		}
		if (enemyType == STALKER && glm::length(movement) == 0) {
			DrawSpriteFromTextureAtlas(program, textureID, animIndices[1]);
			return;
		}
		DrawSpriteFromTextureAtlas(program, textureID, animIndices[animIndex]);
		return;
	}
	float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };   
	float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };    
	glBindTexture(GL_TEXTURE_2D, textureID);  

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);  
	glEnableVertexAttribArray(program->positionAttribute);   

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);  
	glEnableVertexAttribArray(program->texCoordAttribute);      

	glDrawArrays(GL_TRIANGLES, 0, 6);     

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute); 
}


