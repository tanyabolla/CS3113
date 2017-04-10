/*	
	Chaitanya Bolla
	HW05
	SAT COLLISIONS
*/
#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <vector>
#include <algorithm>
#include <math.h>

#include "ShaderProgram.h"
#include "Matrix.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

SDL_Window* displayWindow;
using namespace std;

//Single intialization
float texCoords[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };

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

class Vector {
public:
	Vector() {}
	Vector(float x1, float y1, float z1) : x(x1), y(y1), z(z1) {}

	float length() const {
		return sqrt(x*x + y*y + z*z);
	}

	void normalize(){
		x = x / length();
		y = y / length();
		z = z / length();
	}

	Vector operator*(const Matrix &w){
		Vector aVec;
		aVec.x = x*w.m[0][0] + y*w.m[1][0] + z*w.m[2][0];
		aVec.y = x*w.m[0][1] + y*w.m[1][1] + z*w.m[2][1];
		aVec.z = 0.0f;
		return aVec;
	}

	float x;
	float y;
	float z;
};

class Entity{
public:
	Matrix matrix;
	GLuint texture;
	Vector position;
	Vector velocity;
	Vector acceleration;
	Vector scale;
	float rotation;
	vector<Vector> world;

	Entity(){}
	Entity(GLuint tex, float x0, float y0, float z0, float x1, float y1, float z1, float rot, float x2, float y2, float z2, float x3, float y3, float z3)
		: texture(tex), position(x0, y0, z0), scale(x1, y1, z1), rotation(rot), velocity(x2, y2, z2), acceleration(x3, y3, z3) {}

	void Update(float elapsed){
		//Colliding with walls
		if (position.x >= 3.55 || position.x <= -3.55){
			velocity.x = -velocity.x;
		}
		if (position.y >= 2.0f || position.y <= -2.0f){
			velocity.y = -velocity.y;
		}

		velocity.x += acceleration.x * elapsed;
		velocity.y += acceleration.y * elapsed;

		position.x += velocity.x * elapsed;
		position.y += velocity.y * elapsed;
	}

	void Render(ShaderProgram* program, float vertices[]){
		matrix.identity();
		matrix.Scale(scale.x, scale.y, scale.z);
		matrix.Translate(position.x, position.y, position.z);
		matrix.Rotate(rotation);
		program->setModelMatrix(matrix);

		glBindTexture(GL_TEXTURE_2D, texture);

		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);

		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}
};

bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Vector> &points1, const std::vector<Vector> &points2, Vector &penetration) {
	float normalX = -edgeY;
	float normalY = edgeX;
	float len = sqrtf(normalX*normalX + normalY*normalY);
	normalX /= len;
	normalY /= len;

	std::vector<float> e1Projected;
	std::vector<float> e2Projected;

	for (size_t i = 0; i < points1.size(); i++) {
		e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
	}
	for (size_t i = 0; i < points2.size(); i++) {
		e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
	}

	std::sort(e1Projected.begin(), e1Projected.end());
	std::sort(e2Projected.begin(), e2Projected.end());

	float e1Min = e1Projected[0];
	float e1Max = e1Projected[e1Projected.size() - 1];
	float e2Min = e2Projected[0];
	float e2Max = e2Projected[e2Projected.size() - 1];

	float e1Width = fabs(e1Max - e1Min);
	float e2Width = fabs(e2Max - e2Min);
	float e1Center = e1Min + (e1Width / 2.0);
	float e2Center = e2Min + (e2Width / 2.0);
	float dist = fabs(e1Center - e2Center);
	float p = dist - ((e1Width + e2Width) / 2.0);

	if (p >= 0) {
		return false;
	}

	float penetrationMin1 = e1Max - e2Min;
	float penetrationMin2 = e2Max - e1Min;

	float penetrationAmount = penetrationMin1;
	if (penetrationMin2 < penetrationAmount) {
		penetrationAmount = penetrationMin2;
	}

	penetration.x = normalX * penetrationAmount;
	penetration.y = normalY * penetrationAmount;

	return true;
}

bool penetrationSort(const Vector &p1, const Vector &p2) {
	return p1.length() < p2.length();
}

bool checkSATCollision(const std::vector<Vector> &e1Points, const std::vector<Vector> &e2Points, Vector &penetration) {
	std::vector<Vector> penetrations;
	for (size_t i = 0; i < e1Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e1Points.size() - 1) {
			edgeX = e1Points[0].x - e1Points[i].x;
			edgeY = e1Points[0].y - e1Points[i].y;
		}
		else {
			edgeX = e1Points[i + 1].x - e1Points[i].x;
			edgeY = e1Points[i + 1].y - e1Points[i].y;
		}
		Vector penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}
	for (size_t i = 0; i < e2Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e2Points.size() - 1) {
			edgeX = e2Points[0].x - e2Points[i].x;
			edgeY = e2Points[0].y - e2Points[i].y;
		}
		else {
			edgeX = e2Points[i + 1].x - e2Points[i].x;
			edgeY = e2Points[i + 1].y - e2Points[i].y;
		}
		Vector penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);

		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}

	std::sort(penetrations.begin(), penetrations.end(), penetrationSort);
	penetration = penetrations[0];

	Vector e1Center;
	for (size_t i = 0; i < e1Points.size(); i++) {
		e1Center.x += e1Points[i].x;
		e1Center.y += e1Points[i].y;
	}
	e1Center.x /= (float)e1Points.size();
	e1Center.y /= (float)e1Points.size();

	Vector e2Center;
	for (size_t i = 0; i < e2Points.size(); i++) {
		e2Center.x += e2Points[i].x;
		e2Center.y += e2Points[i].y;
	}
	e2Center.x /= (float)e2Points.size();
	e2Center.y /= (float)e2Points.size();

	Vector ba;
	ba.x = e1Center.x - e2Center.x;
	ba.y = e1Center.y - e2Center.y;

	if ((penetration.x * ba.x) + (penetration.y * ba.y) < 0.0f) {
		penetration.x *= -1.0f;
		penetration.y *= -1.0f;
	}

	return true;
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
	GLuint r = LoadTexture(RESOURCE_FOLDER"element_red_square_glossy.png");
	GLuint g = LoadTexture(RESOURCE_FOLDER"element_green_square_glossy.png");
	GLuint b = LoadTexture(RESOURCE_FOLDER"element_blue_square_glossy.png");

	//Vertices of each texture
	float rVertices[] = { 0.0f, 1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f };
	float gVertices[] = { 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	float bVertices[] = { -1.0f, -1.0f, -2.0f, -1.0f, -2.0f, -2.0f, -1.0f, -1.0f, -2.0f, -2.0f, -1.0f, -2.0f };
	
	//(texture, position, scale, rotation, velocity, acceleration);
	Entity red(r, 1.5f, 0.3f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.5f, -0.5f, 0.0f);
	//World space of red: vertices from the vertex array
	red.world.push_back(Vector(-1.0f, 1.0f, 0.0f));
	red.world.push_back(Vector(-1.0f, 0.0f, 0.0f));
	red.world.push_back(Vector(0.0f, 0.0f, 0.0f));
	red.world.push_back(Vector(0.0f, 1.0f, 0.0f));

	Entity green(g, 0.5f, 0.3f, 0.0f, 1.5f, 1.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, -0.5f, 0.5f, 0.0f);
	//World space of green
	green.world.push_back(Vector(1.0f, 1.0f, 0.0f));
	green.world.push_back(Vector(0.0f, 1.0f, 0.0f));
	green.world.push_back(Vector(0.0f, 0.0f, 0.0f));
	green.world.push_back(Vector(1.0f, 0.0f, 0.0f));

	Entity blue(b, 0.0f, 0.3f, 0.0f, 1.25f, 1.25f, 0.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.5f, 0.5f, 0.0f);
	//World space of blue
	blue.world.push_back(Vector(-2.0f, -1.0f, 0.0f));
	blue.world.push_back(Vector(-2.0f, -2.0f, 0.0f));
	blue.world.push_back(Vector(-1.0f, -2.0f, 0.0f));
	blue.world.push_back(Vector(-1.0f, -1.0f, 0.0f));

	//Matrix of each multiplied by vector to combine into one world
	vector<Vector> redWorld(4); 
	vector<Vector> greenWorld(4);
	vector<Vector> blueWorld(4);
	
	glUseProgram(program.programID);
	float lastFrameTicks = 0.0f;

	Vector penetration;

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

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
			//Update(FIXED_TIMESTEP);
			red.Update(FIXED_TIMESTEP);
			green.Update(FIXED_TIMESTEP);
			blue.Update(FIXED_TIMESTEP);
		}
		red.rotation = (45 * 3.14159265) / 180.0f;
		red.Update(fixedElapsed);
		green.Update(fixedElapsed);
		blue.Update(fixedElapsed);

		//Combine
		for (size_t i = 0; i < red.world.size(); i++){
			redWorld[i] = red.world[i] * red.matrix; //uses the overloaded * operator to multiply
			greenWorld[i] = green.world[i] * green.matrix;
			blueWorld[i] = blue.world[i] * blue.matrix;
		}
		 
		//Collisions - don't work but when they pass over each other, sometimes they slow/speed up?
		if (checkSATCollision(redWorld, greenWorld, penetration)){
			red.position.x += penetration.x * 1.0f;
			red.position.y += penetration.y * 1.0f;
			green.position.x -= penetration.x * 1.0f;
			green.position.y -= penetration.y * 1.0f;
			red.velocity.x = red.velocity.y = 0.0f; //to go back
			red.acceleration.x = penetration.x * 5.0f;
			red.acceleration.y = penetration.y * 5.0f;
			green.velocity.x = green.velocity.y = 0.0f; //to go back
			green.acceleration.x = -penetration.x * 5.0f; //opposite of red's acceleration
			green.acceleration.y = -penetration.y * 5.0f; //opposite of red's acceleration
		}
		if (checkSATCollision(redWorld, blueWorld, penetration)){
			red.position.x += penetration.x * 1.0f;
			red.position.y += penetration.y * 1.0f;
			blue.position.x -= penetration.x * 1.0f;
			blue.position.y -= penetration.y * 1.0f;
			red.velocity.x = red.velocity.y = 0.0f; //to go back
			red.acceleration.x = penetration.x * 5.0f;
			red.acceleration.y = penetration.y * 5.0f;
			blue.velocity.x = blue.velocity.y = 0.0f; //to go back
			blue.acceleration.x = -penetration.x * 5.0f;
			blue.acceleration.y = -penetration.y * 5.0f;
		}
		if (checkSATCollision(greenWorld, blueWorld, penetration)){
			green.position.x += penetration.x * 1.0f;
			green.position.y += penetration.y * 1.0f;
			blue.position.x -= penetration.x * 1.0f;
			blue.position.y -= penetration.y * 1.0f;
			green.velocity.x = green.velocity.y = 0.0f; //to go back
			green.acceleration.x = penetration.x * 5.0f;
			green.acceleration.y = penetration.y * 5.0f;
			blue.velocity.x = blue.velocity.y = 0.0f; //to go back
			blue.acceleration.x = -penetration.x * 5.0f;
			blue.acceleration.y = -penetration.y * 5.0f;
		}

		red.Render(&program, rVertices);
		green.Render(&program, gVertices);
		blue.Render(&program, bVertices);

		//Clean up
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
