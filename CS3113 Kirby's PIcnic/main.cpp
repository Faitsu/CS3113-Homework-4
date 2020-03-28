//Erica Chou 03/15/20
//Kirby Vs Penguins
//Main Function
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

#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"

#include "Entity.h"

#include <SDL_mixer.h>

#include <vector>;

//Hardstuck Variables
#define PLATFORM_COUNT 22
#define FIXED_TIMESTEP .016666f
#define ENEMY_COUNT 3
#define RESET 3

//game will have 3 stages for the platformer, for now game is a shooter due to weird collision box errors
enum GameStage{START, STAGE_ONE, STAGE_TWO, SUCCESS, FAIL };

//Entities
struct GameState {
	Entity *player;
	Entity *background;
	Entity *platforms;
	//There's three enemies
	Entity *enemies; 
	/*Enemy Key: 
		Easiest Enemy: Snorlax, Sleeper, Doesn't do anything but takes two apples to kill
		Second Easiest: Penguin, Jumper
		Hardest Enemy: Mimikyu, Stalker
	*/
	Entity *projectiles;
};

//Globals
GameState state;
GLuint kirbyTextureID;
float LastTicks = 0.0f;
SDL_Window* displayWindow;
bool gameIsRunning = true;
GameStage stage;
float accumulator = 0.0f;
float resetTimer = 0;
float accelerationx = 2.0f;

float edgeLeft = -5.00f;
float edgeRight = 5.00f;

float edgeTop = 3.75f;
float edgeBottom = -3.75f;

int projectile_countmax = 3;
int projectile_count = 0;

Mix_Music* music;
Mix_Chunk* success;
Mix_Chunk* fail;

//Font Set Up and other Globals
GLuint fontTextureID;
glm::vec3 fontPos1 = glm::vec3(-1.5f, 2.0f, 0);
glm::vec3 fontPos2 = glm::vec3(-3.25f,0,0);
glm::vec3 fontPos3 = glm::vec3(-4.5f, 1.0f, 0);


//Vectors to keep Entities in, might change to Entity*
std::vector<Entity*> collisionChecksPlayer;
std::vector<Entity*> collisionChecksEnemy;
std::vector<Entity*> fillIn;


//Shader Program and Model Matrix
ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, modelMatrix2, projectionMatrix;

GLuint LoadTexture(const char* filePath) {//Loads textures 
	int w, h, n;
	unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint textureID;
	glGenTextures(1, &textureID);

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_image_free(image);
	return textureID;
}

//Draw Text Program 
void DrawText(ShaderProgram *program, GLuint fontTextureID, std::string text, float size, float spacing, glm::vec3 position) {
	//Setting up
	float width = 1.0f / 16.0f;
	float height = 1.0f / 16.0f;
	std::vector<float> vertices;
	std::vector<float> texCoords;
	for (int i = 0; i < text.size(); i++) {//For each letter in the string, we create their own verticies and text coordinates
		int index = (int)text[i];
		float offset = (size + spacing) * i;
		float u = (float)(index % 16) / 16.0f;
		float v = (float)(index / 16) / 16.0f;
		vertices.insert(vertices.end(), { offset + (-0.5f * size), 0.5f * size,
			offset + (-0.5f * size), -0.5f * size,
			offset + (0.5f * size), 0.5f * size,
			offset + (0.5f * size), -0.5f * size,
			offset + (0.5f * size), 0.5f * size,
			offset + (-0.5f * size), -0.5f * size, });
		texCoords.insert(texCoords.end(), { u, v,      
			u, v + height,       
			u + width, v,     
			u + width, v + height,   
			u + width, v,      
			u, v + height, 
			}
		);
	} 

	//Rendering text
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, position);
	program->SetModelMatrix(modelMatrix);
	glUseProgram(program->programID);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	
	glBindTexture(GL_TEXTURE_2D, fontTextureID);
	glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));
	
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

void Initialize() {//Initialize the game with variables and other set up
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	displayWindow = SDL_CreateWindow("Kirby!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 480);
	//load shaders for textures
	program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl"); 

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

	music = Mix_LoadMUS("Green Greens - Kirby's Dream Land.mp3");
	Mix_PlayMusic(music, -1);
	Mix_VolumeMusic(MIX_MAX_VOLUME / 50);

	success = Mix_LoadWAV("smb_1-up.wav");
	fail = Mix_LoadWAV("smb_mariodie.wav");

	viewMatrix = glm::mat4(1.0f);
	modelMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	program.SetColor(1.0f, 0.0f, 0.0f, 1.0f);

	glUseProgram(program.programID);

	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	
	
	fontTextureID = LoadTexture("pixel_font.png");


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Initialize Start Screen and Various enemies

	stage = START;

	//set up Player unit
	state.player = new Entity();
	state.player->position = glm::vec3(-4.5f, -2.5f, 0);
	state.player->acceleration = glm::vec3(0, -9.81f, 0);
	state.player->movement = glm::vec3(0);
	state.player->speed = 2.5f;
	state.player->textureID = LoadTexture("Kirby SS.png");

	state.player->entityType = PLAYER;
	
	//different frames for moving
	state.player->animRight = new int[2]{ 4, 6 };
	state.player->animLeft = new int[2]{ 5, 7 };
	state.player->animRightFly = new int[2]{ 0, 2 };
	state.player->animLeftFly = new int[2]{ 1, 3 };
	state.player->animIdle = new int[2]{ 8, 9 };
	state.player->flyheight = 4.0f;
	state.player->height = 0.75f;
	state.player->width = 0.7f;

	state.player->animFrames = 2;
	state.player->animIndices = state.player->animIdle;
	state.player->animCols = 4;
	state.player->animRows = 3;

	//set up for Background
	state.background = new Entity();
	state.background->textureID = LoadTexture("Sky.png");
	state.background->entityType = BACKGROUND;
	state.background->Update(0, state.player, fillIn, 0);
	
	collisionChecksEnemy.push_back(state.player);

	//set up for regular and goal blocks
	state.platforms = new Entity[PLATFORM_COUNT];
	

	GLuint platformTextureID = LoadTexture("Grass Block.png");
	GLuint successTextureID = LoadTexture("Grass Block Success.png");

	//positions of the blocks
	
	// set up for regular blocks and pushing into entity vector for collision checking later
	int temp = 0;
	for (int k = 0; k < PLATFORM_COUNT; k++) {
		if (k % 2 == 0) {
			state.platforms[k].textureID = platformTextureID;
		}
		else {
			state.platforms[k].textureID = successTextureID;
		}
		

		if (k <10) {
			state.platforms[k].position = glm::vec3(-4.5f + k, -3.5f, 0);
		}
		else if (k < 14) {
			state.platforms[k].position = glm::vec3(-4.5f + temp, -1.5f, 0);
			temp++;
		}
		else if (k < 18) {
			state.platforms[k].position = glm::vec3(5.5f - temp, 0, 0);
			temp--;
		}
		else {
			state.platforms[k].position = glm::vec3(-4.5f + temp, 1.5f, 0);
			temp++;
		}
		state.platforms[k].height = 0.8f;
		state.platforms[k].entityType = PLATFORM;
		state.platforms[k].Update(0, state.player, fillIn, 0); //update once
		collisionChecksPlayer.push_back(&state.platforms[k]);
		collisionChecksEnemy.push_back(&state.platforms[k]);
	}

	state.enemies = new Entity[3];
	//set up Enemy unit
	for(int m = 0; m < ENEMY_COUNT; m++){
		state.enemies[m].acceleration = glm::vec3(0, -9.81f, 0);
		state.enemies[m].movement = glm::vec3(0);
		state.enemies[m].entityType = ENEMY;
		state.enemies[m].height = 0.85f;
		state.enemies[m].width = 0.8f;
		state.enemies[m].animTimer = 0.75f;

		//different frames for moving
		state.enemies[m].animIndices = new int[2]{ 0, 1 };
		state.enemies[m].animFrames = 2;
		state.enemies[m].animCols = 2;
		state.enemies[m].animRows = 1;
		collisionChecksPlayer.push_back(&state.enemies[m]);
	}
	state.enemies[0].position = glm::vec3(4.0f, -2.5f, 0);
	state.enemies[0].textureID = LoadTexture("Pixel Snorlax.png");
	state.enemies[0].enemyType = SLEEPER;

	state.enemies[1].position = glm::vec3(-4.5f, 0.0f, 0);
	state.enemies[1].textureID = LoadTexture("Link.png"); 
	state.enemies[1].acceleration = glm::vec3(0, -7.0f, 0);
	state.enemies[1].speed = 1.75f;
	state.enemies[1].flyheight = 4.0f;
	state.enemies[1].enemyType = JUMPER;

	state.enemies[2].position = glm::vec3(4.0f, 2.5f, 0);
	state.enemies[2].textureID = LoadTexture("Pixel Mimikyu.png");
	state.enemies[2].speed = 2.0f;
	state.enemies[2].enemyType = STALKER;
	
	
	//PROJECTILE SET UP
	state.projectiles = new Entity[3];
	for (int m = 0; m < projectile_countmax; m++) {
		state.projectiles[m].position = glm::vec3(-4.5f, -2.5f, 0);
		state.projectiles[m].textureID = LoadTexture("Apple Pixel.png");

		state.projectiles[m].isActive = false;

		state.projectiles[m].movement = glm::vec3(0);
		state.projectiles[m].speed = 5.0f;
		state.projectiles[m].entityType = PROJECTILE;
		state.projectiles[m].height = 0.75f;
		state.projectiles[m].width = 0.8f;

		//different frames for moving

		collisionChecksEnemy.push_back(&state.projectiles[m]);
	}
}

void ProcessInput() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {

		state.player->movement = glm::vec3(0);

		switch (event.type) {
		case SDL_QUIT:
		case SDL_WINDOWEVENT_CLOSE:
			gameIsRunning = false;
			break;
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_UP:
				if (stage != START) {
					if (state.player->jumpcount <= state.player->maxjumpcount) {
						state.player->fly = true;
						state.player->jumpcount++;
						state.player->airborne = true;
						//state.enemies[1].fly = true;
						//state.enemies[1].jumpcount++;
						//state.enemies[1].airborne = true;
					}
				}
				break;
			case SDLK_SPACE:
				if (stage == START) {
					stage = STAGE_ONE;
				}
				else {
					//shoot an apple
					for (int i = 0; i < projectile_countmax; i++) {
						if (state.projectiles[i].isActive == false ) {
							state.projectiles[i].Shoot(state.player);
							break;
						}
					}
				}
				break;
			break;
			}
			break;
		}

	}


	state.player->movement = glm::vec3(0, 0, 0);
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (stage != START && !state.player->success && !state.player->fail) {
		if (keys[SDL_SCANCODE_LEFT]) {
			state.player->movement.x = -1.0f;
			if (!state.player->airborne) {
				state.player->animIndices = state.player->animLeft;
			}
			state.player->goLeft = true;
		}
		else if (keys[SDL_SCANCODE_RIGHT]) {
			state.player->movement.x = 1.0f;
			if (!state.player->airborne) {
				state.player->animIndices = state.player->animRight;
			}
			state.player->goLeft = false;
		}
		if (keys[SDL_SCANCODE_UP]) {
			switch (state.player->goLeft) {

			case (false):
				state.player->animIndices = state.player->animRightFly;
				break;

			case (true):
				state.player->animIndices = state.player->animLeftFly;
				break;
			}
		}
		/*else if (keys[SDL_SCANCODE_DOWN]) {
			state.player->movement.y = -1.0f;
		}
		*/
		if (glm::length(state.player->movement) > 1.0f) {
			state.player->movement = glm::normalize(state.player->movement);
		}
	}
}




void Update() {//Updates game logic
	int knockout = 0;
	for (int k = 0; k < ENEMY_COUNT; k++) {
		if (!state.enemies[k].isActive) {
			knockout++;
		}
	}
	if ( knockout>= ENEMY_COUNT) {
		state.player -> success =true;
		for (int k = 0; k < ENEMY_COUNT; k++) {
			state.enemies[k].isActive = true;
				
		}
		return;
	}
	//Set up Delta Time
	float tick = (float)SDL_GetTicks() / 1000.f;
	float deltaTime = tick - LastTicks;
	LastTicks = tick;

	//Fixed Time step used to regulate the distance our character moves 
	deltaTime += accumulator;
	if (deltaTime < FIXED_TIMESTEP) {
		accumulator = deltaTime;
		return;
	}
	while (deltaTime >= FIXED_TIMESTEP) {
		state.player->Update(FIXED_TIMESTEP, state.player, collisionChecksPlayer, collisionChecksPlayer.size());
		state.enemies[1].counter += FIXED_TIMESTEP;
		for (int j = 0; j < ENEMY_COUNT; j++) {
			state.enemies[j].Update(FIXED_TIMESTEP, state.player, collisionChecksEnemy, collisionChecksEnemy.size());
			if (state.enemies[j].position.x > edgeRight - 0.5f) {
				state.enemies[j].movement.x *= -1.0f;
				state.enemies[j].position.x = edgeRight - 0.5f;
			}
			else if (state.enemies[j].position.x < edgeLeft + 0.5f) {
				state.enemies[j].movement.x *= -1.0f;
				state.enemies[j].position.x = edgeLeft + 0.5f;
			}
		}
		if (state.player->position.x > edgeRight || state.player->position.x < edgeLeft) {
			state.player->fail = true;
		}
		for (int i = 0; i < projectile_countmax; i++) {
			if (state.projectiles[i].isActive) {
				state.projectiles[i].Update(FIXED_TIMESTEP, state.player, collisionChecksPlayer, collisionChecksPlayer.size());
			}
			if (state.projectiles[i].position.x > edgeRight || state.projectiles[i].position.x < edgeLeft) {
				state.projectiles[i].isActive= false;
			}
		}
		deltaTime -= FIXED_TIMESTEP;
	}
	accumulator = deltaTime;

	//This is the reset button pretty much for when the game is over
	if (state.player->fail || state.player->success) {
		Mix_HaltMusic();
		if (state.player->success & resetTimer ==0) {
			Mix_PlayChannel(-1, success, 0);
		}
		if (state.player->fail & resetTimer == 0) {
			Mix_PlayChannel(-1, fail, 0);
		}
		resetTimer += FIXED_TIMESTEP; //why not
		//We let the game sit to let the player read if they succeed or fail & reset the game after
		if (resetTimer >= RESET) {
			Mix_PlayMusic(music, -1);
			resetTimer = 0;
			stage = START;
			
			//clean player variables that need cleaning
			state.player->fail = false;
			state.player->isActive = true;
			state.player->success = false;
			state.player->collidedBottom = false;
			state.player->collidedTop = false;
			state.player->collidedLeft = false;
			state.player->collidedRight = false;

			state.player->position = glm::vec3(-4.5f, -2.5f, 0);
			state.player->movement = glm::vec3(0);
			state.player->acceleration = glm::vec3(0, -9.81, 0);
			state.player->velocity = glm::vec3(0, 0, 0);
			state.player->animIndex = 2;
			//clean platform variables
			for (int k = 0; k < collisionChecksPlayer.size(); k++) {
				collisionChecksPlayer[k]->collidedBottom = false;
				collisionChecksPlayer[k]->collidedTop = false;
				collisionChecksPlayer[k]->collidedLeft = false;
				collisionChecksPlayer[k]->collidedRight = false;
				collisionChecksPlayer[k]->isActive = true;
			}
			state.enemies[0].position = glm::vec3(4.0f, -2.5f, 0);
			state.enemies[0].hp = 2;

			state.enemies[1].position = glm::vec3(-4.5f, 0.0f, 0);

			state.enemies[2].position = glm::vec3(4.0f, 2.5f, 0);
			state.enemies[2].chase = false;
		}
	}
	
}

void Render() { //renders all the parts of our game
	glClear(GL_COLOR_BUFFER_BIT);
	
	state.background->Render(&program);
	
	if (stage != START) {
		//Render Regular Blocks
		for (int j = 0; j < PLATFORM_COUNT; j++) {
			state.platforms[j].Render(&program);
		}
		for (int i = 0; i < ENEMY_COUNT; i++) {
			state.enemies[i].Render(&program);
		}
		
		for (int k = 0; k < projectile_countmax; k++) {
			state.projectiles[k].Render(&program);
		}
		//Render Player after determining index
		state.player->Render(&program);
	}
	//Different game states need different texts
	if (stage == START) {
		DrawText(&program, fontTextureID, "Level: 1-2", .2f, 0.1f, fontPos1);
		DrawText(&program, fontTextureID, "Kirby's Pokemon Picnic", .3f, 0.1f, fontPos3);
		DrawText(&program, fontTextureID, "Press Space To Start!", .2f, 0.1f, fontPos2);
	}
	else if (state.player->success) {
		DrawText(&program, fontTextureID, "You Win!", .3f, 0.1f, fontPos1);
	}
	else if (state.player->fail) {
		DrawText(&program, fontTextureID, "Game Over!", .3f, 0.1f, fontPos1);
	}


	SDL_GL_SwapWindow(displayWindow);


}



void Shutdown() {
	SDL_Quit();
}

int main(int argc, char* argv[]) {
	Initialize();

	while (gameIsRunning) {
		ProcessInput();
		Update();
		Render();
	}

	Shutdown();
	return 0;
}