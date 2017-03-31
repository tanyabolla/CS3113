#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <vector>
#include <ctime>
#include <windows.h>

#include "ShaderProgram.h"
#include "Matrix.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

//Game state
int state = 0;
enum GameState { TITLE_SCREEN, GAME };

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

	for (int i = 0; i < text.size(); i++){
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
	unsigned int textureID;
	float size = 1.0f;
	float u;
	float v;
	float width;
	float height;

	SheetSprite(unsigned int textureID, float size, float u, float v, float width, float height)
		: textureID(textureID), size(size), u(u), v(v), width(width), height(height) {}

	void Draw(ShaderProgram* program) {
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
};

class Entity{
public:
	float posX;
	float posY;
	float speedX = 0.0f;
	float speedY = 0.0f;
	SheetSprite sprite;
	float size = sprite.size;

	Entity(float x, float y, SheetSprite sprite) : posX(x), posY(y), sprite(sprite) {}
};

void renderMainMenu(Matrix& modelMatrix, ShaderProgram* program, GLuint fontSheet){
	modelMatrix.identity();
	modelMatrix.Scale(2.0f, 2.0f, 0.0f);
	modelMatrix.Translate(-1.0f, 0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "SPACE INVADERS", 0.15f, 0.0f);

	modelMatrix.identity();
	modelMatrix.Scale(2.0f, 2.0f, 0.0f);
	modelMatrix.Translate(-1.5f, 0.2f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "USE LEFT AND RIGHT KEYS TO MOVE", 0.1f, 0.0f);

	modelMatrix.identity();
	modelMatrix.Scale(2.0f, 2.0f, 0.0f);
	modelMatrix.Translate(-0.9f, 0.1f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "AND SPACE TO SHOOT", 0.1f, 0.0f);

	modelMatrix.identity();
	modelMatrix.Scale(2.0f, 2.0f, 0.0f);
	modelMatrix.Translate(-1.25f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "PRESS SPACE TO ENTER GAME", 0.1f, 0.0f);
}

std::vector<Entity> enemies;
std::vector<Entity> enemyBullets;
std::vector<Entity> playerBullets;
float playerFire = 0.0f;
float enemyFire = 0.0f;
int score = 0;

void renderGame(Matrix& modelMatrix, ShaderProgram* program, GLuint fontSheet, int score, SheetSprite enemySprite, Entity player){
	modelMatrix.identity();
	modelMatrix.Scale(1.0f, 1.0f, 0.0f);
	modelMatrix.Translate(-1.75f, 1.8f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, "SCORE", 0.3f, 0.0f);

	modelMatrix.identity();
	modelMatrix.Scale(1.0f, 1.0f, 0.0f);
	modelMatrix.Translate(1.0f, 1.8f, 0.0);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontSheet, std::to_string(score), 0.3f, 0.0f);

	modelMatrix.identity();
	modelMatrix.Translate(player.posX, -1.4f, 0.0f);
	program->setModelMatrix(modelMatrix);
	player.sprite.Draw(program);

	for (int i = 0; i < 28; i++){
		Entity enemy(-1.5f + (i % 7) * 0.5, 1.2 - (i / 7 * 0.5), enemySprite);
		enemy.speedX = 0.5f;
		modelMatrix.identity();
		modelMatrix.Translate(enemy.posX, enemy.posY, 0.0f);
		program->setModelMatrix(modelMatrix);
		enemies.push_back(enemy);
		enemies[i].sprite.Draw(program);
	}

	for (int i = 0; i < playerBullets.size(); i++){
		modelMatrix.identity();
		modelMatrix.Translate(playerBullets[i].posX, playerBullets[i].posY, 0.0f);
		program->setModelMatrix(modelMatrix);
		playerBullets[i].sprite.Draw(program);
	}
	
	for (int i = 0; i < enemyBullets.size(); i++){
		modelMatrix.identity();
		modelMatrix.Translate(enemyBullets[i].posX, enemyBullets[i].posY, 0.0f);
		program->setModelMatrix(modelMatrix);
		enemyBullets[i].sprite.Draw(program);
	}
}

void updateGame(Matrix& modelMatrix, ShaderProgram* program, Entity& player, float elapsed, SheetSprite playerBulletSprite, SheetSprite enemyBulletSprite){
	const Uint8* keys = SDL_GetKeyboardState(NULL);
	//Movement
	if (keys[SDL_SCANCODE_LEFT]){
		if (player.posX >= -3.0){
			player.posX -= elapsed * player.speedX;
		}
	}
	else if (keys[SDL_SCANCODE_RIGHT]){
		if (player.posX <= 3.0){
			player.posX += elapsed * player.speedX;
		}
	}
	//Player Firing bullets
	if (keys[SDL_SCANCODE_SPACE]){
		if (playerFire > 0.4f){
			playerFire = 0.0f;
			playerBullets.push_back(Entity(player.posX, player.posY, playerBulletSprite));
		}
	}

	for (int i = 0; i < playerBullets.size(); i++){
		playerBullets[i].speedY = 1.0f;
		playerBullets[i].posY += elapsed * playerBullets[i].speedY;
	}

	//When the enemies get close to the wall, this makes them move to the other side
	for (int i = 0; i < enemies.size(); i++){
		if (enemies[i].posX > 3.4 || enemies[i].posX < -3.4){
			for (int j = 0; j < enemies.size(); j++){
				enemies[i].speedX = (-1.0) * enemies[i].speedX;
			}
		}
	}

	//Collisions, doesn't work
	for (int i = 0; i < playerBullets.size(); i++){
		for (int j = 0; j < enemies.size(); j++){
			if (playerBullets[i].posX + playerBullets[i].sprite.width < enemies[i].posX + enemies[i].sprite.width &&
				playerBullets[i].posY + playerBullets[i].sprite.height < enemies[i].posX + enemies[i].sprite.height){
				score += 10;
				std::swap(playerBullets[i], playerBullets[playerBullets.size() - 1]);
				std::swap(enemies[i], enemies[enemies.size() - 1]);
				playerBullets.pop_back();
				enemies.pop_back();
			}
		}
	}

	//Enemy Firing Bullets
	if (enemyFire > 0.4f){
		enemyFire = 0.0f;
		//int randEnemy = rand() % enemies.size();
		//enemyBullets.push_back(Entity(enemies[randEnemy].posX, enemies[randEnemy].posY, enemyBulletSprite)); //not working
	}
	for (size_t i = 0; i < enemyBullets.size(); i++) {
		enemyBullets[i].speedY = 1.0f;
		enemyBullets[i].posY -= enemyBullets[i].speedY * elapsed;
	}
	//Did not implement winning or losing, ran out of time :(
}

void setup(){
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("SPACE INVADERS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif
}

int main(int argc, char *argv[]){
	setup();
	//Window intialization
	glViewport(0, 0, 640, 360);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	Matrix modelMatrix;
	Matrix viewMatrix;
	Matrix projectionMatrix;
	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);

	//Loading texture
	GLuint fontSheet = LoadTexture(RESOURCE_FOLDER"font1.png");
	GLuint spriteSheet = LoadTexture(RESOURCE_FOLDER"sheet.png");

	SheetSprite playerSprite(spriteSheet, 0.5f, 224.0f / 1024.0f, 832.0f / 1024.0f, 99.0f / 1024.0f, 76.5f / 1024.0f);
	Entity player(0.0f, -1.4f, playerSprite);
	player.speedX = 1.0f; //Not initialized in the constructor because enemies don't have speed

	SheetSprite enemySprite(spriteSheet, 0.3f, 425.0f / 1024.0f, 468.0f / 1024.0f, 93.0f / 1024.0f, 84.0f / 1024.0f);
	SheetSprite playerBulletSprite(spriteSheet, 0.1f, 844.0f / 1024.0f, 846.0f / 1024.0f, 11.0 / 1024.0f, 57.0 / 1024.0f);
	SheetSprite enemyBulletSprite(spriteSheet, 0.1f, 836.0f / 1024.0f, 565.0f / 1024.0f, 11.0 / 1024.0f, 36.0 / 1024.0f);

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

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		playerFire += elapsed;
		enemyFire += elapsed;

		program.setViewMatrix(viewMatrix);
		program.setProjectionMatrix(projectionMatrix);

		//Switching states
		const Uint8* keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_SPACE] && state == 0){
			state = 1;
		}


		switch (state){
		case TITLE_SCREEN:
			//Nothing to update on title screen
			break;
		case GAME:
			updateGame(modelMatrix, &program, player, elapsed, playerBulletSprite, enemyBulletSprite);
			break;
		}

		switch (state){
		case TITLE_SCREEN:
			renderMainMenu(modelMatrix, &program, fontSheet);
			break;
		case GAME:
			renderGame(modelMatrix, &program, fontSheet, score, enemySprite, player);
			break;
		}

		//Cleanup
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
