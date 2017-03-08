/*
Chaitanya Bolla
cb3172
HW#1 2D 
*/
#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

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

int main(int argc, char *argv[]){

	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	//Setup
	glViewport(0, 0, 640, 360);
	//Textured shader program
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	//Loading the sprites
	GLuint cat = LoadTexture(RESOURCE_FOLDER"fighting cat.png");
	GLuint dog = LoadTexture(RESOURCE_FOLDER"dead dog.png");
	GLuint cactus = LoadTexture(RESOURCE_FOLDER"cactus.png");
	GLuint background = LoadTexture(RESOURCE_FOLDER"background.png");

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	float positionOfDog = 0.0f;
	//float angleOfDog = 0.0f;
	projectionMatrix.setOrthoProjection(-3.55f, 3.55f, -2.0f, 2.0f, -1.0f, 1.0f);
	float lastFrameTicks = 0.0f;
	glUseProgram(program.programID);

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		positionOfDog += elapsed;
		//angleOfDog += elapsed;

		glClearColor(0.22f, 0.69f, 0.87f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		//Cat
		glBindTexture(GL_TEXTURE_2D, cat);
		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);

		float vertices[] = { -0.75f, -0.75f, 0.75f, -0.75f, 0.75f, 0.75f, -0.75f, -0.75f, 0.75f, 0.75f, -0.75f, 0.75f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//Dog
		glBindTexture(GL_TEXTURE_2D, dog);
		modelMatrix.identity();
		modelMatrix.Translate(positionOfDog, 0.0f, 0.0f);
		//modelMatrix.Rotate(angleOfDog*(3.1415926/180.0));
		program.setModelMatrix(modelMatrix);
		float dVertices[] = { -0.75f, -0.75f, 0.75f, -0.75f, 0.75f, 0.75f, -0.75f, -0.75f, 0.75f, 0.75f, -0.75f, 0.75f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, dVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float dTexCoords[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, dTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//Cactus
		glBindTexture(GL_TEXTURE_2D, cactus);
		modelMatrix.identity();
		modelMatrix.Translate(-2.5f, 0.0f, 0.0f);
		program.setModelMatrix(modelMatrix);
		float cVertices[] = { -0.75f, -0.75f, 0.75f, -0.75f, 0.75f, 0.75f, -0.75f, -0.75f, 0.75f, 0.75f, -0.75f, 0.75f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, cVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float cTexCoords[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, cTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//background
		glBindTexture(GL_TEXTURE_2D, background);
		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);
		float bVertices[] = { 3.55f, -0.55f, -3.55f, -0.55f, -3.55f, -2.0f, -3.55f, -2.0f, 3.55f, -2.0f, 3.55f, -0.55f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, bVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float bTexCoords[] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, bTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		//End
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
