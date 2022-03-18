#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream> //03
#include <string> //03
#include <GLM/glm.hpp> // Lec 04
#include <GLM/gtc/matrix_transform.hpp> //lec 04

#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"

#include "IndexBuffer.h"
#include "VertexArrayObject.h"
#include "VertexBuffer.h"
#include "Shader.h"
#include "VertexTypes.h"

#include <math.h>

// Lecture 07 ///////////////////
// Load textures (images)
// Load UVs into VBO, send to GPU
// Set up texture parameters
// Bind texture to use it, then draw the object

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

unsigned char* image;
int width, height;

void loadImage(const std::string& filename) {
	int channels;
	stbi_set_flip_vertically_on_load(true); //because opengl loads it flipped

	// Load image
	image = stbi_load(filename.c_str(), &width, &height, &channels, 0);

	if (image)
		std::cout << "Image loaded: " << width << " x " << height << std::endl;
	else std::cout << "Failed to load texture!!!!!" << std::endl;

}



GLFWwindow* window;
// The current size of our window in pixels
glm::ivec2 windowSize = glm::ivec2(800, 800);
// The title of our GLFW window
std::string windowTitle = "Midterm Project";

void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	windowSize = glm::ivec2(width, height);
}

bool initGLFW() {
	if (glfwInit() == GLFW_FALSE) {
		std::cout << "Failed to Initialize GLFW" << std::endl;
		return false;
	}

	//Create a new GLFW window
	window = glfwCreateWindow(800, 800, "INFR1350U", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	return true;
}

bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		std::cout << "Failed to initialize Glad" << std::endl;
		return false;
	}
}


GLuint shader_program;

bool loadShaders() {
	// Read Shaders from file
	std::string vert_shader_str;
	std::ifstream vs_stream("vertex_shader.glsl", std::ios::in);
	if (vs_stream.is_open()) {
		std::string Line = "";
		while (getline(vs_stream, Line))
			vert_shader_str += "\n" + Line;
		vs_stream.close();
	}
	else {
		printf("Could not open vertex shader!!\n");
		return false;
	}
	const char* vs_str = vert_shader_str.c_str();

	std::string frag_shader_str;
	std::ifstream fs_stream("frag_shader.glsl", std::ios::in);
	if (fs_stream.is_open()) {
		std::string Line = "";
		while (getline(fs_stream, Line))
			frag_shader_str += "\n" + Line;
		fs_stream.close();
	}
	else {
		printf("Could not open fragment shader!!\n");
		return false;
	}
	const char* fs_str = frag_shader_str.c_str();

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vs_str, NULL);
	glCompileShader(vs);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fs_str, NULL);
	glCompileShader(fs);

	shader_program = glCreateProgram();
	glAttachShader(shader_program, fs);
	glAttachShader(shader_program, vs);
	glLinkProgram(shader_program);

	return true;
}

int main() {

	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	///////////////////////////////////////////////////////////////////////////////////Odd Definitions

	// ie. int a = 5 or something

	int lives = 3; // for ball count down the road
	int score = 0; // for box score down the road
	bool respawn = true; // setup condition for making the player launch the ball at start

	// the balls current pos relative to axis
	float ballx = 0.0f;
	float bally = 0.0f;
	float ballz = 0.0f;

	// the balls velocity relative to axis
	float ballvelx = 0.0f;
	float ballvely = 0.0f;
	float ballvelz = 0.0f;

	bool collision = false;

	bool boxdestroyed[16]; //bool condition for destroying the box
	bool boxdamaged[16];

	for (int counter = 0; counter < 16; counter++) {
		boxdestroyed[counter] = false;
		boxdamaged[counter] = false;
	}

	//debug items
	bool test1 = false;
	bool test2 = true;

	bool isMoving = true;
	bool isButtonPressed = false;

	GLfloat paddleX = 0.0f;
	///////////////////////////////////////////////////////////////////////////////////////

	//// Lecture 3 starts here

	// Cube data
	static const GLfloat points[] = {//front face, 2 triangles
		-0.5f, -0.5f, 0.5f,//0  front face
		0.5f, -0.5f, 0.5f, //3
		-0.5f, 0.5f, 0.5f, //1
		0.5f, -0.5f, 0.5f, //3
		0.5f, 0.5f, 0.5f, //2
		-0.5f, 0.5f, 0.5f, //1
		0.5f, -0.5f, 0.5f, //3 Right face
		0.5f, -0.5f, -0.5f, //7
		0.5f, 0.5f, 0.5f, //2
		0.5f, -0.5f, -0.5f, //7
		0.5f, 0.5f, -0.5f, //6
		0.5f, 0.5f, 0.5f,  //2
		-0.5f, -0.5f, -0.5f, //4 Left face
		-0.5f, -0.5f, 0.5f, //0
		-0.5f, 0.5f, -0.5f, //5
		-0.5f, -0.5f, 0.5f, //0
		-0.5f, 0.5f, 0.5f,  //1
		-0.5f, 0.5f, -0.5f, //5
		0.5f, -0.5f, -0.5f, //7 back face
		-0.5f, -0.5f, -0.5f, //4
		0.5f, 0.5f, -0.5f, //6
		-0.5f, -0.5f, -0.5f, //4
		-0.5f, 0.5f, -0.5f, //5
		0.5f, 0.5f, -0.5f, //6
		-0.5f, 0.5f, 0.5f, //1 top face
		0.5f, 0.5f, 0.5f, //2
		-0.5f, 0.5f, -0.5f, //5
		0.5f, 0.5f, 0.5f, //2
		0.5f, 0.5f, -0.5f, //6
		-0.5f, 0.5f, -0.5f, //5
		-0.5f, -0.5f, -0.5f, //4 bottom face
		0.5f, -0.5f, -0.5f, //7
		-0.5f, -0.5f, 0.5f,//0
		0.5f, -0.5f, -0.5f, //7
		0.5f, -0.5f, 0.5f, //3
		-0.5f, -0.5f, 0.5f, //0

		-2.5f, -0.5f, 0.5f,//0  front face
		-1.5f, -0.5f, 0.5f, //3
		-2.5f, 0.5f, 0.5f, //1
		-1.5f, -0.5f, 0.5f, //3
		-1.5f, 0.5f, 0.5f, //2
		-2.5f, 0.5f, 0.5f, //1
		-1.5f, -0.5f, 0.5f, //3 Right face
		-1.5f, -0.5f, -0.5f, //7
		-1.5f, 0.5f, 0.5f, //2
		-1.5f, -0.5f, -0.5f, //7
		-1.5f, 0.5f, -0.5f, //6
		-1.5f, 0.5f, 0.5f,  //2
		-2.5f, -0.5f, -0.5f, //4 Left face
		-2.5f, -0.5f, 0.5f, //0
		-2.5f, 0.5f, -0.5f, //5
		-2.5f, -0.5f, 0.5f, //0
		-2.5f, 0.5f, 0.5f,  //1
		-2.5f, 0.5f, -0.5f, //5
		-1.5f, -0.5f, -0.5f, //7 back face
		-2.5f, -0.5f, -0.5f, //4
		-1.5f, 0.5f, -0.5f, //6
		-2.5f, -0.5f, -0.5f, //4
		-2.5f, 0.5f, -0.5f, //5
		-1.5f, 0.5f, -0.5f, //6
		-2.5f, 0.5f, 0.5f, //1 top face
		-1.5f, 0.5f, 0.5f, //2
		-2.5f, 0.5f, -0.5f, //5
		-1.5f, 0.5f, 0.5f, //2
		-1.5f, 0.5f, -0.5f, //6
		-2.5f, 0.5f, -0.5f, //5
		-2.5f, -0.5f, -0.5f, //4 bottom face
		-1.5f, -0.5f, -0.5f, //7
		-2.5f, -0.5f, 0.5f,//0
		-1.5f, -0.5f, -0.5f, //7
		-1.5f, -0.5f, 0.5f, //3
		-2.5f, -0.5f, 0.5f //0

	};

	// Color data
	static const GLfloat colors[] = {
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,

		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 1.0f
	};

	/////// LECTURE 05 //////////

	static const GLfloat normals[] = {
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, // front
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, //right
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f, //left
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f, // back
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f, // top
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f, // bottom

		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, // front
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, //right
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f, //left
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f, // back
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f, // top
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f // bottom
	};

	/////// LECTURE 07 /////////
	static const GLfloat uv[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f

	};


	/// LECTURE 05
	GLfloat cameraPos[] = { 0.0f, 0.0f, 3.0f };
	GLfloat lightPos[] = { 0.0f, 0.0f, 3.0f };
	////////////
	
	//////////////////////////////////////////////////////////////// 

	//VBO - Vertex buffer object
	GLuint pos_vbo = 0;
	glGenBuffers(1, &pos_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
	
	GLuint color_vbo = 1;
	glGenBuffers(1, &color_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
	
	glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
	
	//						index, size, type, normalize?, stride, pointer
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	/////////// LECTURE 05 ///////////////
	GLuint normal_vbo = 2;
	glGenBuffers(1, &normal_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, normal_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	///////////// LETURE 07 ///////////////
	GLuint uv_vbo = 3;
	glGenBuffers(1, &uv_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, uv_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, NULL);


	//////////////

	glEnableVertexAttribArray(0);//pos
	glEnableVertexAttribArray(1);//colors
	glEnableVertexAttribArray(2);//normals
	glEnableVertexAttribArray(3); // uv

	////////////
	
	


	/*																					// currently not in use or set up
	loadImage("box.bmp");

	GLuint textureHandle;
	glGenTextures(1, &textureHandle);
	glBindTexture(GL_TEXTURE_2D, textureHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	
	//Texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//free image space
	stbi_image_free(image);
	*/

	//////////////////////// 07
	//// hands on lec 7

	loadImage("metalBox.jpg");

	GLuint texture2Handle;
	glGenTextures(1, &texture2Handle);
	glBindTexture(GL_TEXTURE_2D, texture2Handle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

	//Texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//free image space
	stbi_image_free(image);


	////

	// Load our shaders

	if (!loadShaders())
		return 1;

	////////// LECTURE 04 //////////////

	// Projection - FoV, aspect ratio, near, far
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	glm::mat4 Projection = glm::perspective(glm::radians(45.0f),
		(float)width / (float)height, 0.001f, 100.0f);

	// View matrix - Camera

	glm::mat4 View = glm::lookAt(
		glm::vec3(0, 2, 10), // camera position
		glm::vec3(0, 0, 0), //target
		glm::vec3(0, 1, 0) //up vector
	);

	//////////////////////////////////////////////////////The Place we Make objects/////////////////////////////////////

	// Model matrix
	glm::mat4 Model = glm::mat4(1.0f);//Identity matrix - resets your matrix
	glm::mat4 mvp;// = Projection * View * Model;

	glm::mat4 paddle = glm::mat4(1.0f);
	glm::mat4 ball = glm::mat4(1.0f);
	glm::mat4 Walls = glm::mat4(1.0f);

	///////////////////////BOXES//////////////////
	glm::mat4 boxes[16];

	for (int counter = 0; counter < 16; counter++) {
		boxes[counter] = glm::mat4(1.0f);
	}
	/////////////////////////////////////////////

	// Handle for our mvp
	GLuint matrixMVP = glGetUniformLocation(shader_program, "MVP");
	
	////// LEC 05 - uniform variables
	GLuint matrixModel = glGetUniformLocation(shader_program, "Model");
	GLuint lightPosID = glGetUniformLocation(shader_program, "lightPos");
	GLuint cameraPosID = glGetUniformLocation(shader_program, "cameraPos");
	////////////////////////////////
	
	// Our high-precision timer
	double lastFrame = glfwGetTime();

	//LOG_INFO("Starting mesh build");

	//MeshBuilder<VertexPosCol> paddleMesh;
	//MeshFactory::AddCube(paddleMesh, glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(3.0f, 0.5f, 0.5f));
	//VertexArrayObject::Sptr paddleVAO = paddleMesh.Bake();

	//MeshBuilder<VertexPosCol> ballMesh;
	//MeshFactory::AddCube(ballMesh, glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f));
	//VertexArrayObject::Sptr ballVAO = ballMesh.Bake();

	//////////////////////////////// BOXES ///////////////////////////////
	/* Box Map  (bracket is how many hits to destroy)

			[box0(2)]		[box1(1)]		[box2(2)]		[box3(1)]		[box4(2)]

					[box5(1)]		[box6(1)]		[box7(1)]    [box8(1)]

			[box9(1)]		[box10(2)]		[box11(1)]		[box12(2)]		[box13(1)]

									[box14(2)]		[box15(2)]


	*/

	MeshBuilder<VertexPosCol> boxMesh[16];

	glm::vec3 boxCoords[16];

	boxCoords[0] = glm::vec3(4.0f, 5.0f, 0.0f);
	boxCoords[1] = glm::vec3(2.0f, 5.0f, 0.0f);
	boxCoords[2] = glm::vec3(0.0f, 5.0f, 0.0f);
	boxCoords[3] = glm::vec3(-2.0f, 5.0f, 0.0f);
	boxCoords[4] = glm::vec3(-4.0f, 5.0f, 0.0f);

	boxCoords[5] = glm::vec3(3.0f, 4.0f, 0.0f);
	boxCoords[6] = glm::vec3(1.0f, 4.0f, 0.0f);
	boxCoords[7] = glm::vec3(-1.0f, 4.0f, 0.0f);
	boxCoords[8] = glm::vec3(-3.0f, 4.0f, 0.0f);

	boxCoords[9] = glm::vec3(4.0f, 3.0f, 0.0f);
	boxCoords[10] = glm::vec3(2.0f, 3.0f, 0.0f);
	boxCoords[11] = glm::vec3(0.0f, 3.0f, 0.0f);
	boxCoords[12] = glm::vec3(-2.0f, 3.0f, 0.0f);
	boxCoords[13] = glm::vec3(-4.0f, 3.0f, 0.0f);

	boxCoords[14] = glm::vec3(1.0f, 2.0f, 0.0f);
	boxCoords[15] = glm::vec3(-1.0f, -.0f, 0.0f);

	VertexArrayObject::Sptr boxVAO[16];

	//for (int counter = 0; counter < 16; counter++) {
	//	MeshFactory::AddCube(boxMesh[counter], boxCoords[counter], glm::vec3(1.75f, 0.5f, 0.1f));

	//	boxVAO[counter] = boxMesh[counter].Bake();
	//}
	/////////////////////////////// WALLS ////////////////////////////////

//	MeshBuilder<VertexPosCol> leftWallMesh;
//	MeshFactory::AddCube(leftWallMesh, glm::vec3(7.0f, 0.0f, 0.0f), glm::vec3(0.5f, 15.0f, 0.5f));
//	VertexArrayObject::Sptr leftWallVAO = leftWallMesh.Bake();

//	MeshBuilder<VertexPosCol> rightWallMesh;
//	MeshFactory::AddCube(rightWallMesh, glm::vec3(-7.0f, 0.0f, 0.0f), glm::vec3(0.5f, 15.0f, 0.5f));
//	VertexArrayObject::Sptr rightWallVAO = rightWallMesh.Bake();

//	MeshBuilder<VertexPosCol> ceilingMesh;
//	MeshFactory::AddCube(ceilingMesh, glm::vec3(0.0f, 7.0f, 0.0f), glm::vec3(15.0f, 0.5f, 0.5f));
//	VertexArrayObject::Sptr ceilingVAO = ceilingMesh.Bake();


	/////////////////

	// GL states
	glEnable(GL_DEPTH_TEST);
	// LEC 05
	glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CCW);
	//glCullFace(GL_FRONT); //GL_BACK, GL_FRONT_AND_BACK

	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


	/////////////////////////////////////////////////// Game loop ///////////////////////////////////////////////////////////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shader_program);
		
		////////// Lecture 04								 X,    Y,    Z	
		//Model = glm::mat4(1.0f); // reset Model

		

		//Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, movZ));
		mvp = Projection * View * Model;
		
		// Send mvp to GPU
		glUniformMatrix4fv(matrixMVP, 1, GL_FALSE, &mvp[0][0]);

		///////// LEC 05
		glUniformMatrix4fv(matrixModel, 1, GL_FALSE, &Model[0][0]);
		
		glUniform3fv(lightPosID, 1, &lightPos[0]);
		glUniform3fv(cameraPosID, 1, &cameraPos[0]);

		/////////////////

		////// Bind texture 1
		///// draw 
		///// Bind texture 2
		///// draw

		glDrawArrays(GL_TRIANGLES, 0, 36); //36
		glDrawArrays(GL_TRIANGLES, 36, 36);
		
		

		glfwSwapBuffers(window);
	}
	return 0;

}