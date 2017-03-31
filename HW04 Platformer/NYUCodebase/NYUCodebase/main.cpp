#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <vector>

#include "ShaderProgram.h"
#include "Matrix.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#include <iostream>
using namespace std;
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

SDL_Window* displayWindow;

//Game state
int state = 0;
enum GameState { TITLE_SCREEN, GAME };
enum EntityType {ENTITY_PLAYER, ENTITY_BLOCK, ENTITY_GEM};

GLuint LoadTexture(const char *filePath){
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL){
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(image); 
	return retTexture;
}

void DrawText(ShaderProgram* program, int fontTexture, std::string text, float size, float spacing){
	
	float texture_size = 1.0 / 16.0f;
	
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (size_t i = 0; i < text.size(); i++){

		int spriteIndex = (int)text[i];

		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}

	glUseProgram(program->programID);
	glBindTexture(GL_TEXTURE_2D, fontTexture);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

class SheetSprite {
public:
	SheetSprite(){}
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size): textureID(textureID), u(u), v(v), width(width), height(height), size(size) {}
	void Draw(ShaderProgram* program){
		glBindTexture(GL_TEXTURE_2D, textureID);

		GLfloat texCoords[] = {
			u, v + height,
			u + width, v,
			u, v,
			u + width, v,
			u, v + height,
			u + width, v + height
		};

		float aspect = width / height;
		float vertices[] = {
			-0.5f * size * aspect, -0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, 0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, -0.5f * size,
			0.5f * size * aspect, -0.5f * size
		};

		glUseProgram(program->programID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);

		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};

class Vector3 {
public:
	Vector3() {}
	Vector3(float x1, float y1, float z1) : x(x1), y(y1), z(z1) {}

	float x;
	float y;
	float z;
};

class Entity{
public:
	Entity(){}
	Entity(SheetSprite sprite, EntityType type, bool isstatic, float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2)
		: sprite(sprite), entityType(type), isStatic(isstatic), position(x0, y0, z0), velocity(x1, y1, z1), acceleration(x2, y2, z2) {}

	Vector3 position;
	Vector3 velocity;
	Vector3 acceleration;

	float top;
	float bottom;
	float left;
	float right;

	SheetSprite sprite;
	EntityType entityType;

	bool isStatic;
	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
};

GLuint fontSheet;
Entity player;
std::vector<Entity> tiles;
std::vector<Entity> gems;
float gravity = 1.0f;
float playerVelocity = 2.0f;
 
void RenderMainMenu(ShaderProgram* program, Matrix& modelMatrix){
	modelMatrix.identity();
	modelMatrix.Scale(2.5f, 2.5f, 0.0f);
	modelMatrix.Translate(-1.0f, 0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "PLATFORMER GAME", 0.15f, 0.0f);

	modelMatrix.identity();
	modelMatrix.Scale(2.0f, 2.0f, 0.0f); 
	modelMatrix.Translate(-1.5f, 0.2f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "USE LEFT AND RIGHT KEYS TO MOVE", 0.1f, 0.0f);

	modelMatrix.identity();
	modelMatrix.Scale(2.0f, 2.0f, 0.0f);
	modelMatrix.Translate(-0.9f, 0.1f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "AND SPACE TO JUMP", 0.1f, 0.0f);

	modelMatrix.identity();
	modelMatrix.Scale(2.0f, 2.0f, 0.0f);
	modelMatrix.Translate(-1.25f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "PRESS SPACE TO ENTER GAME", 0.1f, 0.0f);
}

void setSize(Entity& entity){
	entity.top = entity.position.y + entity.sprite.width;
	entity.bottom = entity.position.y - entity.sprite.width;
	entity.left = entity.position.x - entity.sprite.height;
	entity.right = entity.position.x + entity.sprite.height;
}

void RenderGame(ShaderProgram* program, Matrix& modelMatrix, Matrix& viewMatrix){
	for (size_t i = 0; i < tiles.size(); i++){
		modelMatrix.identity();
		modelMatrix.Translate(tiles[i].position.x, tiles[i].position.y, tiles[i].position.z);
		program->setModelMatrix(modelMatrix);
		tiles[i].sprite.Draw(program);
	}

	for (size_t i = 0; i < gems.size(); i++){
		modelMatrix.identity();
		modelMatrix.Translate(gems[i].position.x, gems[i].position.y, gems[i].position.z);
		program->setModelMatrix(modelMatrix);
		gems[i].sprite.Draw(program);
	}

	modelMatrix.identity();
	modelMatrix.Translate(player.position.x, player.position.y, player.position.z);
	program->setModelMatrix(modelMatrix);
	player.sprite.Draw(program);
	viewMatrix.identity();
	viewMatrix.Translate(-player.position.x, -player.position.y, 0.0f); //so it moves in the opposite direction
	program->setViewMatrix(modelMatrix);
}

void UpdateGame(float elapsed){
	const Uint8* keys = SDL_GetKeyboardState(NULL);
	float penetration;

	player.collidedBottom = false;
	player.collidedLeft = false;
	player.collidedRight = false;
	player.collidedTop = false;

	setSize(player);
	for (size_t i = 0; i < tiles.size(); i++){
		setSize(tiles[i]);
	}
	for (size_t i = 0; i < gems.size(); i++){
		setSize(gems[i]);
	}

	//Box/box collisions
	for (size_t i = 0; i < tiles.size(); i++){
		if (player.top > tiles[i].bottom && player.bottom < tiles[i].top && player.left < tiles[i].right && player.right > tiles[i].left){
			
			float y_distance = fabs(player.position.y - tiles[i].position.y);
			float playerHalfHeight = player.sprite.size * 2;
			float tileHalfHeight = tiles[i].sprite.size * 2;
			penetration = fabs(y_distance - playerHalfHeight - tileHalfHeight);

			if (player.position.y < tiles[i].position.y){
				player.position.y += penetration;
				player.top += penetration;
				player.bottom += penetration;
				player.collidedTop = true;
			}
			else if (player.position.y > tiles[i].position.y){
				player.position.y -= penetration;
				player.top -= penetration;
				player.bottom -= penetration;
				player.collidedBottom = true; 
			}
			break;
		}
	}

	for (size_t i = 0; i < tiles.size(); i++){
		if (player.top > tiles[i].bottom && player.bottom < tiles[i].top && player.left < tiles[i].right && player.right > tiles[i].left){
			
			float x_distance = fabs(player.position.x - tiles[i].position.x);
			float playerHalfWidth = player.sprite.size * 2;
			float tileHalfWidth = tiles[i].sprite.size * 2;
			penetration = fabs(x_distance - playerHalfWidth - tileHalfWidth);

			if (player.position.x > tiles[i].position.x){
				player.position.x += penetration;
				player.left += penetration;
				player.right += penetration;
				player.collidedRight = true;
			}
			else if (player.position.x < tiles[i].position.x){
				player.position.x -= penetration;
				player.left -= penetration;
				player.right -= penetration;
				player.collidedLeft = true;

			}
			break;
		}
	}
	
	//Erase gems once caught
	for (size_t i = 0; i < gems.size(); i++){
		if (player.top < gems[i].bottom && player.bottom > gems[i].top && player.left < gems[i].right &&  player.right > gems[i].left){
			std::swap(tiles[i], tiles[tiles.size() - 1]);
			tiles.pop_back();
			player.velocity.x = playerVelocity;
		}
	}

	//Movement (left and right)
	if (keys[SDL_SCANCODE_LEFT]){
		player.position.x -= playerVelocity * elapsed;
	}
	else if (keys[SDL_SCANCODE_RIGHT]){
		player.position.x += playerVelocity * elapsed;
	}
	//Jumping, doesn't work :(
	if (keys[SDL_SCANCODE_SPACE] && player.collidedBottom == true){
		player.velocity.y = playerVelocity;
	}

	if (player.velocity.y > 0.0f && player.collidedBottom){
		player.velocity.y = 0.0f;
	}
	else if (player.velocity.y < 0.0f && player.collidedTop > 0.0f){
		player.velocity.y = 0.0f;
	}
	if (player.velocity.y > 0.0f && !player.collidedBottom){
		player.position.y -= player.velocity.y * elapsed;
	}
}

void setup(){
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif
}

int main(int argc, char *argv[]){
	setup();
	//Window initialization
	glViewport(0, 0, 640, 360);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	Matrix modelMatrix;
	Matrix viewMatrix;
	Matrix projectionMatrix;
	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	//Loading Texture
	fontSheet = LoadTexture(RESOURCE_FOLDER"font1.png"); 
	GLuint sheetTexture = LoadTexture(RESOURCE_FOLDER"spriteSheet.png");;

	SheetSprite tileSprite(sheetTexture, 36.0f / 1024.0f, 140.0f / 1024.0f, 32.0f / 1024.0f, 30.0f /1024.0f, 0.4f);
	for (int i = 0; i < 17; i++){
		tiles.push_back(Entity(tileSprite, ENTITY_BLOCK, true, -3.2f + (i * 0.4f), -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	}
	for (int i = 0; i < 5; i++){
		tiles.push_back(Entity(tileSprite, ENTITY_BLOCK, true, -2.3f + (i * 0.4f), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	}
	for (int i = 0; i < 5; i++){
		tiles.push_back(Entity(tileSprite, ENTITY_BLOCK, true, 0.3f + (i * 0.4f), 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	}

	SheetSprite gemSprite(sheetTexture, 620.0f / 1024.0f, 414.0f / 1024.0f, 30.0f / 1024.0f, 24.0f / 1024.0f, 0.2f);
	gems.push_back(Entity(gemSprite, ENTITY_GEM, false, 1.1f, 0.85f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	gems.push_back(Entity(gemSprite, ENTITY_GEM, false, -1.4f, 0.35f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	gems.push_back(Entity(gemSprite, ENTITY_GEM, false, 2.1f, -0.65f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
	gems.push_back(Entity(gemSprite, ENTITY_GEM, false, -2.1f, -0.65f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));

	SheetSprite playerSprite(sheetTexture, 650.0f / 1024.0f, 0.0f / 1024.0f, 33.0f / 1024.0f, 35.0f / 1024.0f, 0.5f);
	player = Entity(playerSprite, ENTITY_PLAYER, false, 0.0f, -0.55f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f);

	glUseProgram(program.programID);
	float lastFrameTicks = 0.0f;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		const Uint8* keys = SDL_GetKeyboardState(NULL);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		program.setModelMatrix(modelMatrix);
		program.setViewMatrix(viewMatrix);
		program.setProjectionMatrix(projectionMatrix);

		float fixedElapsed = elapsed;
		if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS){
			fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
		}
		while (fixedElapsed >= FIXED_TIMESTEP){
			fixedElapsed -= FIXED_TIMESTEP;
			UpdateGame(FIXED_TIMESTEP);
		}
		UpdateGame(fixedElapsed);

		if (keys[SDL_SCANCODE_SPACE] && state == 0){
			state = 1;
		}

		switch (state){
		case TITLE_SCREEN:
			RenderMainMenu(&program, modelMatrix);
			break;
		case GAME:
			RenderGame(&program, modelMatrix, viewMatrix);
			break;
		}

		//Clean up
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
