/* 
	Chaitanya Bolla
	cb3172
	Pong Game with Sound
*/
#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <time.h>
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

//Single intialization since they are they are the same for all
float texCoords[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };

class Paddle {
public:
	Paddle(float x1, float y1, float x2, float y2) : posX1(x1), posY1(y1), posX2(x2), posY2(y2) {}
	float posX1;
	float posY1;
	float posX2;
	float posY2;
};

class Ball {
public:
	Ball(float posX1, float posY1, float posX2, float posY2, float dirX, float dirY, float speed)
		: positionX1(posX1), positionY1(posY1), positionX2(posX2), positionY2(posY2), directionX(dirX), directionY(dirY), speed(speed){}
	float positionX1;
	float positionY1;
	float positionX2;
	float positionY2;
	float directionX;
	float directionY;
	float speed;
};

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
	//Window intialization
	glViewport(0, 0, 640, 360);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	
	//Loading texture
	GLuint whiteTexture = LoadTexture(RESOURCE_FOLDER"whiteTexture.png");

	Matrix pongBall;
	Matrix lefPaddle;
	Matrix righPaddle;
	Matrix viewMatrix;
	Matrix projectionMatrix;
	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	glUseProgram(program.programID);

	float lastFrameTicks = 0.0f;

	//Direction change formula of ball if it hits the paddle or the edge of the screen
	float angle = float((rand() % 4) * 90 + 45);
	float directX = cos((angle * 3.14159265f) / 180);
	float directY = sin((angle * 3.14159265f) / 180);
	//Ball initialization
	Ball ball(-0.1f, 0.1f, 0.1f, -0.1f, directX, directY, 1.0f);
	//Paddle initialization
	Paddle leftPaddle(-3.55f, 0.5f, -3.3f, -0.5f);
	Paddle rightPaddle(3.3f, 0.5f, 3.55f, -0.5f);

	//Initializing the sound and music files
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	Mix_Chunk *start = Mix_LoadWAV("start.wav"); //Plays a sound when the ball initially starts moving
	Mix_Chunk *hitSound = Mix_LoadWAV("hit.wav");
	Mix_Music *music = Mix_LoadMUS("music.mp3");

	//Plays the music
	Mix_PlayMusic(music, 1);
	//Plays a sound when the ball initially starts moving
	Mix_PlayChannel(-1, start, 0); 

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_Event event;
	bool done = false;
	bool playGame = false;
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
		
		program.setViewMatrix(viewMatrix);
		program.setProjectionMatrix(projectionMatrix);

		//Bind texture
		glBindTexture(GL_TEXTURE_2D, whiteTexture);

		//Left Paddle
		lefPaddle.identity();
		//Left paddle controls, using the w (up) and s (down)
		if (keys[SDL_SCANCODE_W] && leftPaddle.posY1 < 2.0f){
			leftPaddle.posY1 += 2.0f * elapsed;
			leftPaddle.posY2 += 2.0f * elapsed;
		}
		else if (keys[SDL_SCANCODE_S] && leftPaddle.posY2 > -2.0f){
			leftPaddle.posY1 -= 2.0f * elapsed;
			leftPaddle.posY2 -= 2.0f * elapsed;
		}
		lefPaddle.Translate(0.0f, leftPaddle.posY2, 0.0f);
		program.setModelMatrix(lefPaddle);

		float leftVertices[] = { -3.3f, 1.0f, -3.55f, 1.0f, -3.55f, 0.0f, -3.55f, 0.0f, -3.3f, 0.0f, -3.3f, 1.0f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, leftVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//Right Paddle
		righPaddle.identity();
		//Right paddle controls, using the up and down keys
		if (keys[SDL_SCANCODE_UP] && rightPaddle.posY1 < 2.0f){
			rightPaddle.posY1 += 2.0f * elapsed;
			rightPaddle.posY2 += 2.0f * elapsed;
		}
		else if (keys[SDL_SCANCODE_DOWN] && rightPaddle.posY2 > -2.0f){
			rightPaddle.posY1 -= 2.0f * elapsed;
			rightPaddle.posY2 -= 2.0f * elapsed;
		}
		righPaddle.Translate(0.0f, rightPaddle.posY2, 0.0f);
		program.setModelMatrix(righPaddle);

		float rightVertices[] = { 3.55f, 1.0f, 3.3f, 1.0f, 3.3f, 0.0f, 3.3f, 0.0f, 3.55f, 0.0f, 3.55f, 1.0f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, rightVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		//glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		//glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		//glDisableVertexAttribArray(program.texCoordAttribute);

		//Ball
		pongBall.identity();
		//Movement of the ball
		//When the ball makes contact with the paddles
		if (ball.positionX1 <= leftPaddle.posX2 && ball.positionY1 <= leftPaddle.posY1 && ball.positionY2 >= leftPaddle.posY2){
			ball.directionX = -ball.directionX;
			Mix_PlayChannel(-1, hitSound, 0);
		}
		else if (ball.positionX2 >= rightPaddle.posX1 && ball.positionY1 <= rightPaddle.posY1 && ball.positionY2 >= leftPaddle.posY2){
			ball.directionX = -ball.directionX;
			Mix_PlayChannel(-1, hitSound, 0);
		}
		//When the ball collides with the top wall or the bottom wall
		else if (ball.positionY1 >= 2.0f && ball.directionY > 0 || ball.positionY2 <= -2.0f && ball.directionY < 0){
			ball.directionY = -ball.directionY;
		}
		ball.positionX1 += (ball.speed * ball.directionX * elapsed);
		ball.positionX2 += (ball.speed * ball.directionX * elapsed);
		ball.positionY1 += (ball.speed * ball.directionY * elapsed);
		ball.positionY2 += (ball.speed * ball.directionY * elapsed);
		pongBall.Translate(ball.positionX1, ball.positionY2, 0.0f);
		program.setModelMatrix(pongBall);

		float ballVertices[] = { 0.2f, 0.2f, 0.0f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.0f, 0.2f, 0.2f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ballVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//If the ball misses the paddles and collides with the wall
		//Can't put it in the if statements because it would just translate
		if (ball.positionX1 < -3.55){
			break; //right player wins
		}
		if (ball.positionX2 > 3.55){
			break; //left player wins
		}
		//Clean up
		SDL_GL_SwapWindow(displayWindow);
	}
	SDL_Quit();
	return 0;
}
