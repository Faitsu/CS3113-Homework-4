//Erica Chou 03/24/2020
//Entity Header File
#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

#include <vector>

//The different types of entities in my game
enum EntityType{PLAYER,PROJECTILE,PLATFORM,ENEMY,BACKGROUND};
enum EnemyType{NONE,SLEEPER,JUMPER,STALKER};

class Entity {
	public:  
		
		//important variables
		int *animRight = NULL;
		int *animLeft = NULL;
		int *animRightFly = NULL;
		int *animLeftFly = NULL;
		int *animIdle = NULL;
		int *animIndices = NULL;

		//some of these are not needed but I'll keep them here for reuseability
		float animTimer = 0.25f;
		int animFrames = 0;
		int animIndex = 0;
		float animTime = 0.0f;

		int animCols = 0;
		int animRows = 0;

		float width = 1;
		float height = 1;

		bool isActive = true;
		bool idle = true;
		
		bool goLeft = false;

		bool fly = false;
		bool airborne = false;
		float flyheight = 0;
		int maxjumpcount = 2;
		int jumpcount = 0;

		//other logic for collision
		bool collidedTop = false;
		bool collidedBottom = false;
		bool collidedLeft = false;
		bool collidedRight = false;

		//Entity* collisionObject = new Entity[4];


		//states for fail and success
		bool fail = false;
		bool success = false;

		int hp = 2;
		bool chase = false;
		float counter = 0.0f;

		//Type of Entity (determines how we update it and such)
		EntityType entityType;
		EnemyType enemyType = NONE;
		//mostly for player, holds values for speed and such
		glm::vec3 position;   
		glm::vec3 movement;
		glm::vec3 velocity;
		glm::vec3 acceleration;
		float speed;      
		
		//textureID for loading outside textures
		GLuint textureID; 
		
		glm::mat4 modelMatrix;
		
		//constructor
		Entity();       
		//functions inside entity
		bool CheckCollision(Entity* other);
		void CheckCollisionsY(std::vector<Entity*> objects, int objectCount);
		void CheckCollisionsX(std::vector<Entity*> objects, int objectCount);

		void Update(float deltaTime,Entity* player, std::vector<Entity*> platform, int platform_count); 
		void DrawSpriteFromTextureAtlas(ShaderProgram *program, GLuint textureID, int index);
		void Render(ShaderProgram *program);
		void Shoot(Entity *player);
		void AI(Entity* player);
		void AIJumper();
		void AISleeper();
		void AIStalker(Entity* player);
};
		