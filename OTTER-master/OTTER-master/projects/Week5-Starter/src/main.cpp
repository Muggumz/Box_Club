#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "VertexArrayObject.h"
#include "Shader.h"
#include "Camera.h"

#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "VertexTypes.h"

#include <windows.h>

#include <string>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define LOG_GL_NOTIFICATIONS

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

/*
	Handles debug messages from OpenGL
	https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
	@param source    Which part of OpenGL dispatched the message
	@param type      The type of message (ex: error, performance issues, deprecated behavior)
	@param id        The ID of the error or message (to distinguish between different types of errors, like nullref or index out of range)
	@param severity  The severity of the message (from High to Notification)
	@param length    The length of the message
	@param message   The human readable message from OpenGL
	@param userParam The pointer we set with glDebugMessageCallback (should be the game pointer)
*/
void GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string sourceTxt;
	switch (source) {
		case GL_DEBUG_SOURCE_API: sourceTxt = "DEBUG"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceTxt = "WINDOW"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceTxt = "SHADER"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY: sourceTxt = "THIRD PARTY"; break;
		case GL_DEBUG_SOURCE_APPLICATION: sourceTxt = "APP"; break;
		case GL_DEBUG_SOURCE_OTHER: default: sourceTxt = "OTHER"; break;
	}
	switch (severity) {
		case GL_DEBUG_SEVERITY_LOW:          LOG_INFO("[{}] {}", sourceTxt, message); break;
		case GL_DEBUG_SEVERITY_MEDIUM:       LOG_WARN("[{}] {}", sourceTxt, message); break;
		case GL_DEBUG_SEVERITY_HIGH:         LOG_ERROR("[{}] {}", sourceTxt, message); break;
			#ifdef LOG_GL_NOTIFICATIONS
		case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO("[{}] {}", sourceTxt, message); break;
			#endif
		default: break;
	}
}

// Stores our GLFW window in a global variable for now
GLFWwindow* window;
// The current size of our window in pixels
glm::ivec2 windowSize = glm::ivec2(800, 800);
// The title of our GLFW window
std::string windowTitle = "Midterm Project";

void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	windowSize = glm::ivec2(width, height);
}

/// <summary>
/// Handles intializing GLFW, should be called before initGLAD, but after Logger::Init()
/// Also handles creating the GLFW window
/// </summary>
/// <returns>True if GLFW was initialized, false if otherwise</returns>
bool initGLFW() {
	// Initialize GLFW
	if (glfwInit() == GLFW_FALSE) {
		LOG_ERROR("Failed to initialize GLFW");
		return false;
	}

	//Create a new GLFW window and make it current
	window = glfwCreateWindow(windowSize.x, windowSize.y, windowTitle.c_str(), nullptr, nullptr);
	glfwMakeContextCurrent(window);
	
	// Set our window resized callback
	glfwSetWindowSizeCallback(window, GlfwWindowResizedCallback);

	return true;
}

/// <summary>
/// Handles initializing GLAD and preparing our GLFW window for OpenGL calls
/// </summary>
/// <returns>True if GLAD is loaded, false if there was an error</returns>
bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		LOG_ERROR("Failed to initialize Glad");
		return false;
	}
	return true;
}

int main() {
	Logger::Init(); // We'll borrow the logger from the toolkit, but we need to initialize it

	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(GlDebugMessage, nullptr);
	
	static const GLfloat points[] = {
		//-0.875f, -0.25f, 0.1f,//0  front face
		//0.875f, -0.25f, 0.1f, //3
		//-0.875f, 0.25f, 0.1f, //1
		//0.875f, -0.25f, 0.1f, //3
		//0.875f, 0.25f, 0.1f, //2
		//-0.875f, 0.25f, 0.1f, //1

		///// box[0]
		3.125f, -5.25f, 0.1f,
		4.875f, -5.25f, 0.1f,
		3.125f, -4.75f, 0.1f,
		4.875f, -5.25f, 0.1f,
		4.875f, -4.75f, 0.1f,
		3.125f, -4.75f, 0.1f,

		///// box[2]

		-0.875f, -5.25f, 0.1f,
		0.875f, -5.25f, 0.1f,
		-0.875f, -4.75f, 0.1f,
		0.875f, -5.25f, 0.1f,
		0.875f, -4.75f, 0.1f,
		-0.875f, -4.75f, 0.1f,

		///// box[4]
		-4.875f, -5.25f, 0.1f,
		-3.125f, -5.25f, 0.1f,
		-4.875f, -4.75f, 0.1f,
		-3.125f, -5.25f, 0.1f,
		-3.125f, -4.75f, 0.1f,
		-4.875f, -4.75f, 0.1f,

		///// box[10]

		1.125f, -3.25f, 0.1f,
		2.875f, -3.25f, 0.1f,
		1.125f, -2.75f, 0.1f,
		2.875f, -3.25f, 0.1f,
		2.875f, -2.75f, 0.1f,
		1.125f, -2.75f, 0.1f,

		///// box[12]

		-2.875f, -3.25f, 0.1f,
		-1.125f, -3.25f, 0.1f,
		-2.875f, -2.75f, 0.1f,
		-1.125f, -3.25f, 0.1f,
		-1.125f, -2.75f, 0.1f,
		-2.875f, -2.75f, 0.1f,

		///// box [14]

		0.125f, -2.25f, 0.1f,
		1.875f, -2.25f, 0.1f,
		0.125f, -1.75f, 0.1f,
		1.875f, -2.25f, 0.1f,
		1.875f, -1.75f, 0.1f,
		0.125f, -1.75f, 0.1f,

		////// box [15]

		-1.875f, -2.25f, 0.1f,
		-0.125f, -2.25f, 0.1f,
		-1.875f, -1.75f, 0.1f,
		-0.125f, -2.25f, 0.1f,
		-0.125f, -1.75f, 0.1f,
		-1.875f, -1.75f, 0.1f,

		//left wall

		6.75f, -7.0f, 0.6f,
		7.25f, -7.0f, 0.6f,
		6.75f, 7.0f, 0.6f,
		7.25f, -7.0f, 0.6f,
		7.25f, 7.0f, 0.6f,
		6.75f, 7.0f, 0.6f

	};

	static const GLfloat colors[] = {
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,

		///
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		//
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		//
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,

		//
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		//
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,

		//
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		//
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,

		//
		 0.435294f, 0.258824f, 0.258824f,
		 0.435294f, 0.258824f, 0.258824f,
		 0.435294f, 0.258824f, 0.258824f,
		 0.435294f, 0.258824f, 0.258824f,
		 0.435294f, 0.258824f, 0.258824f,
		 0.435294f, 0.258824f, 0.258824f
	};

	static const uint16_t indices2[] = {
		3, 0 ,1,
		3, 1, 2
	};
	IndexBuffer::Sptr points_ibo = IndexBuffer::Create();
	points_ibo->LoadData(indices2, 3 * 2);

	//VBO - Vertex buffer object
	VertexBuffer::Sptr posVbo = VertexBuffer::Create();
	posVbo->LoadData(points, 180);

	VertexBuffer::Sptr color_vbo = VertexBuffer::Create();
	color_vbo->LoadData(colors, 180);

	VertexArrayObject::Sptr vao = VertexArrayObject::Create();
	vao->AddVertexBuffer(posVbo, {
		BufferAttribute(0, 3, AttributeType::Float, 0, NULL, AttribUsage::Position)
	});
	vao->AddVertexBuffer(color_vbo, {
		{ 1, 3, AttributeType::Float, 0, NULL, AttribUsage::Color }
	});
	vao->SetIndexBuffer(points_ibo);
	
	
	////////////////////////// DISPLAYING SCORE //////////////////////////////
	

	GLuint textureHandle[2];
	

	loadImage("metalBox.jpg");

	glGenTextures(2, textureHandle);
	glBindTexture(GL_TEXTURE_2D, textureHandle[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

	//Texture Parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//free image space
	stbi_image_free(image);
	/*
	// Load our shaders
	if (!loadShaders())
		return 1;
	*/
	////////// LECTURE 04 //////////////

	// Projection - FoV, aspect ratio, near, far
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	glm::mat4 Projection = glm::perspective(glm::radians(45.0f),
		(float)width / (float)height, 0.001f, 100.0f);

	// View matrix - Camera


	// Model matrix
	glm::mat4 Model = glm::mat4(1.0f);//Identity matrix - resets your matrix

	glm::mat4 mvp;// = Projection * View * Model;

	// Handle for our mvp
	//GLuint matrixMVP = glGetUniformLocation(shader_program, "MVP");
	
	////// LEC 05 - uniform variables
	//GLuint matrixModel = glGetUniformLocation(shader_program, "Model");
	//GLuint lightPosID = glGetUniformLocation(shader_program, "lightPos");
	//GLuint cameraPosID = glGetUniformLocation(shader_program, "cameraPos");
	//////////////////////////////// 

	// GL states
	glEnable(GL_DEPTH_TEST);
	// LEC 05
	glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CCW);
	//glCullFace(GL_FRONT); //GL_BACK, GL_FRONT_AND_BACK

	
	/////////////////////////////////////////////////////////////////////////////



	///////////////Odd Definitions

	// ie. int a = 5 or something

	int lives = 6; // for ball count down the road
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

	/////////////////////////// Camera and Shaders //////////////////////////////////
	
	static const float interleaved[] = {
		// X      Y    Z       R     G     B
		 0.875f, -0.25f, 0.1f,   1.0f, 0.0f, 0.0f,
		 0.875f,  0.25f, 0.1f,   1.0f, 0.0f, 0.0f,
		-0.875f,  0.25f, 0.1f,   1.0f, 0.0f, 0.0f,
		-0.875f, -0.25f, 0.1f,   1.0f, 0.0f, 0.0f
	};
	VertexBuffer::Sptr interleaved_vbo = VertexBuffer::Create();
	interleaved_vbo->LoadData(interleaved, 6 * 4);

	static const uint16_t indices[] = {
		3, 0, 1,
		3, 1, 2
	};
	IndexBuffer::Sptr interleaved_ibo = IndexBuffer::Create();
	interleaved_ibo->LoadData(indices, 3 * 2);

	size_t stride = sizeof(float) * 6;
	VertexArrayObject::Sptr vao2 = VertexArrayObject::Create();
	vao2->AddVertexBuffer(interleaved_vbo, {
		BufferAttribute(0, 3, AttributeType::Float, stride, 0, AttribUsage::Position),
		BufferAttribute(1, 3, AttributeType::Float, stride, sizeof(float) * 3, AttribUsage::Color),
	});
	vao2->SetIndexBuffer(interleaved_ibo);
	

	//Texture 0
	
	// Load our shaders
	Shader* shader = new Shader();
	shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", ShaderPartType::Vertex);
	shader->LoadShaderPartFromFile("shaders/frag_shader.glsl", ShaderPartType::Fragment);
	shader->Link();
	/*
	// GL states, we'll enable depth testing and backface fulling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	*/
	// Get uniform location for the model view projection
	Camera::Sptr camera = Camera::Create();
	camera->SetPosition(glm::vec3(0, 0, 5));
	camera->LookAt(glm::vec3(0.0f));
	
	//////////////////////////////////////////////////////The Place we Make objects/////////////////////////////////////
	

	// Create a mat4 to store our mvp (for now)
	//glm::mat4 transform = glm::mat4(1.0f);
	glm::mat4 paddle = glm::mat4(1.0f);
	//glm::mat4 transform3 = glm::mat4(1.0f);
	glm::mat4 ball = glm::mat4(1.0f);
	glm::mat4 test = glm::mat4(1.0f);


	glm::mat4 life[3];

	for (int counter = 0; counter < 3; counter++) {
		life[counter] = glm::mat4(1.0f);
	}

	///////////////////////BOXES//////////////////
	glm::mat4 boxes[16];

	for (int counter = 0; counter < 16; counter++) {
		boxes[counter] = glm::mat4(1.0f);
	}

	glm::mat4 boxText[16];

	for (int counter = 0; counter < 16; counter++) {
		boxText[counter] = glm::mat4(1.0f);
	}
	/////////////////////////////////////////////

	glm::mat4 Walls = glm::mat4(1.0f);

	////////////////// NUMBERS ////////////////////
	glm::mat4 one[2] = { glm::mat4(1.0f), glm::mat4(1.0f) };

	glm::mat4 two = glm::mat4(1.0f);
	glm::mat4 three = glm::mat4(1.0f);
	glm::mat4 four = glm::mat4(1.0f);
	glm::mat4 five = glm::mat4(1.0f);
	glm::mat4 six = glm::mat4(1.0f);
	glm::mat4 seven = glm::mat4(1.0f);
	glm::mat4 eight = glm::mat4(1.0f);
	glm::mat4 nine = glm::mat4(1.0f);
	glm::mat4 zero = glm::mat4(1.0f);
	//////////////////////////////////////////////

	// Our high-precision timer
	double lastFrame = glfwGetTime();

	LOG_INFO("Starting mesh build");

	MeshBuilder<VertexPosCol> paddleMesh;
	MeshFactory::AddCube(paddleMesh, glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(3.0f,0.5f, 0.5f));
	VertexArrayObject::Sptr paddleVAO = paddleMesh.Bake();
	
	MeshBuilder<VertexPosCol> ballMesh;
	MeshFactory::AddCube(ballMesh, glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f));
	VertexArrayObject::Sptr ballVAO = ballMesh.Bake();
	

	//////////////////////////////// BOXES ///////////////////////////////
	/* Box Map  (bracket is how many hits to destroy)
	
			[box0(2)]		[box1(1)]		[box2(2)]		[box3(1)]		[box4(2)]

					[box5(1)]		[box6(1)]		[box7(1)]    [box8(1)]

			[box9(1)]		[box10(2)]		[box11(1)]		[box12(2)]		[box13(1)]

									[box14(2)]		[box15(2)]
	
	
	*/

	MeshBuilder<VertexPosCol> boxMesh[16];

	glm::vec3 boxCoords[16];

	boxCoords[0] = glm::vec3(4.0f, -5.0f, 0.0f);
	boxCoords[1] = glm::vec3(2.0f, -5.0f, 0.0f);
	boxCoords[2] = glm::vec3(0.0f, -5.0f, 0.0f);
	boxCoords[3] = glm::vec3(-2.0f, -5.0f, 0.0f);
	boxCoords[4] = glm::vec3(-4.0f, -5.0f, 0.0f);

	boxCoords[5] = glm::vec3(3.0f, -4.0f, 0.0f);
	boxCoords[6] = glm::vec3(1.0f, -4.0f, 0.0f);
	boxCoords[7] = glm::vec3(-1.0f, -4.0f, 0.0f);
	boxCoords[8] = glm::vec3(-3.0f, -4.0f, 0.0f);
	
	boxCoords[9] = glm::vec3(4.0f, -3.0f, 0.0f);
	boxCoords[10] = glm::vec3(2.0f, -3.0f, 0.0f);
	boxCoords[11] = glm::vec3(0.0f, -3.0f, 0.0f);
	boxCoords[12]= glm::vec3(-2.0f, -3.0f, 0.0f);
	boxCoords[13] = glm::vec3(-4.0f, -3.0f, 0.0f);

	boxCoords[14] = glm::vec3(1.0f, -2.0f, 0.0f);
	boxCoords[15] = glm::vec3(-1.0f, -2.0f, 0.0f);

	VertexArrayObject::Sptr boxVAO[16];

	for (int counter = 0; counter < 16; counter++) {
		MeshFactory::AddCube(boxMesh[counter], boxCoords[counter], glm::vec3(1.75f, 0.5f, 0.1f));

		boxVAO[counter] = boxMesh[counter].Bake();
	}
	/////////////////////////////// WALLS ////////////////////////////////

	MeshBuilder<VertexPosCol> leftWallMesh;
	MeshFactory::AddCube(leftWallMesh, glm::vec3(7.0f, 0.0f, 0.0f), glm::vec3(0.5f, 15.0f, 0.5f));
	VertexArrayObject::Sptr leftWallVAO = leftWallMesh.Bake();

	MeshBuilder<VertexPosCol> rightWallMesh;
	MeshFactory::AddCube(rightWallMesh, glm::vec3(-7.0f, 0.0f, 0.0f), glm::vec3(0.5f, 15.0f, 0.5f));
	VertexArrayObject::Sptr rightWallVAO = rightWallMesh.Bake();

	MeshBuilder<VertexPosCol> ceilingMesh;
	MeshFactory::AddCube(ceilingMesh, glm::vec3(0.0f, -7.0f, 0.0f), glm::vec3(15.0f, 0.5f, 0.5f));
	VertexArrayObject::Sptr ceilingVAO = ceilingMesh.Bake();

	
	//////////////////////////// Life Tokens //////////////////////////////
	MeshBuilder<VertexPosCol> lifeMesh[3];

	glm::vec3 lifeCoords[3];

	lifeCoords[0] = glm::vec3(5.5f, 6.0f, 0.0f);
	lifeCoords[1] = glm::vec3(4.75f, 6.0f, 0.0f);
	lifeCoords[2] = glm::vec3(4.0f, 6.0f, 0.0f);

	VertexArrayObject::Sptr lifeVAO[3];

	for (int counter = 0; counter < 3; counter++) {
		MeshFactory::AddCube(lifeMesh[counter], lifeCoords[counter], glm::vec3(0.5f, 0.5f, 0.1f));

		lifeVAO[counter] = lifeMesh[counter].Bake();
	}
	/////////////////////////////////// NUMBERS ///////////////////////////////////////////
	////////// 1/10-16 //////////
	MeshBuilder<VertexPosCol> oneMesh[2];

	glm::vec3 oneCoords[2];

	oneCoords[0] = glm::vec3(-5.8f, 6.1f, 0.0f);
	oneCoords[1] = glm::vec3(-5.f, 6.1f, 0.0f);

	VertexArrayObject::Sptr oneVAO[2];
	for (int counter = 0; counter < 2; counter++) {
		MeshFactory::AddCube(oneMesh[counter], oneCoords[counter], glm::vec3(0.17f, 1.f, 0.1f));
		oneVAO[counter] = oneMesh[counter].Bake();
	}
	////////// 2 //////////
	MeshBuilder<VertexPosCol> twoMesh[5];
	glm::vec3 twoCoords[5];
	VertexArrayObject::Sptr twoVAO[5];

	MeshFactory::AddCube(twoMesh[0], glm::vec3(-5.8f, 5.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(twoMesh[1], glm::vec3(-6.05f, 5.92f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(twoMesh[2], glm::vec3(-5.8f, 6.1f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(twoMesh[3], glm::vec3(-5.64f, 6.44f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(twoMesh[4], glm::vec3(-5.8f, 6.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));

	for (int counter = 0; counter < 5; counter++) {
		twoVAO[counter] = twoMesh[counter].Bake();
	}
	////////// 3 //////////
	MeshBuilder<VertexPosCol> threeMesh[5];
	glm::vec3 threeCoords[5];
	VertexArrayObject::Sptr threeVAO[5];

	MeshFactory::AddCube(threeMesh[0], glm::vec3(-5.8f, 5.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(threeMesh[1], glm::vec3(-6.05f, 5.92f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(threeMesh[2], glm::vec3(-5.8f, 6.1f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(threeMesh[3], glm::vec3(-5.8f, 6.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(threeMesh[4], glm::vec3(-6.05f, 6.44f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));

	for (int counter = 0; counter < 5; counter++) {
		threeVAO[counter] = threeMesh[counter].Bake();
	}
	////////// 4 //////////
	MeshBuilder<VertexPosCol> fourMesh[4];
	glm::vec3 fourCoords[4];
	VertexArrayObject::Sptr fourVAO[4];

	MeshFactory::AddCube(fourMesh[0], glm::vec3(-5.64f, 5.92f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(fourMesh[1], glm::vec3(-6.05f, 5.92f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(fourMesh[2], glm::vec3(-5.8f, 6.1f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(fourMesh[3], glm::vec3(-6.05f, 6.44f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));

	for (int counter = 0; counter < 4; counter++) {
		fourVAO[counter] = fourMesh[counter].Bake();
	}
	////////// 5 //////////
	MeshBuilder<VertexPosCol> fiveMesh[5];
	glm::vec3 fiveCoords[5];
	VertexArrayObject::Sptr fiveVAO[5];

	MeshFactory::AddCube(fiveMesh[0], glm::vec3(-5.64f, 5.92f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(fiveMesh[1], glm::vec3(-5.8f, 5.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(fiveMesh[2], glm::vec3(-5.8f, 6.1f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(fiveMesh[3], glm::vec3(-5.8f, 6.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(fiveMesh[4], glm::vec3(-6.05f, 6.44f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));

	for (int counter = 0; counter < 5; counter++) {
		fiveVAO[counter] = fiveMesh[counter].Bake();
	}
	////////// 6 //////////
	MeshBuilder<VertexPosCol> sixMesh[6];
	glm::vec3 sixCoords[6];
	VertexArrayObject::Sptr sixVAO[6];

	MeshFactory::AddCube(sixMesh[0], glm::vec3(-5.64f, 5.92f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(sixMesh[1], glm::vec3(-5.8f, 5.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(sixMesh[2], glm::vec3(-5.8f, 6.1f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(sixMesh[3], glm::vec3(-5.64f, 6.44f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(sixMesh[4], glm::vec3(-5.8f, 6.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(sixMesh[5], glm::vec3(-6.05f, 6.44f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));

	for (int counter = 0; counter < 6; counter++) {
		sixVAO[counter] = sixMesh[counter].Bake();
	}
	////////// 7 //////////
	MeshBuilder<VertexPosCol> sevenMesh[3];
	glm::vec3 sevenCoords[3];
	VertexArrayObject::Sptr sevenVAO[3];

	MeshFactory::AddCube(sevenMesh[0], glm::vec3(-5.8f, 5.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(sevenMesh[1], glm::vec3(-6.05f, 5.92f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(sevenMesh[2], glm::vec3(-6.05f, 6.44f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));

	for (int counter = 0; counter < 3; counter++) {
		sevenVAO[counter] = sevenMesh[counter].Bake();
	}
	////////// 8 //////////
	MeshBuilder<VertexPosCol> eightMesh[7];
	glm::vec3 eightCoords[7];
	VertexArrayObject::Sptr eightVAO[7];

	MeshFactory::AddCube(eightMesh[0], glm::vec3(-5.64f, 5.92f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(eightMesh[1], glm::vec3(-5.8f, 5.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(eightMesh[2], glm::vec3(-6.05f, 5.92f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(eightMesh[3], glm::vec3(-5.8f, 6.1f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(eightMesh[4], glm::vec3(-5.64f, 6.44f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(eightMesh[5], glm::vec3(-5.8f, 6.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(eightMesh[6], glm::vec3(-6.05f, 6.44f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));

	for (int counter = 0; counter < 7; counter++) {
		eightVAO[counter] = eightMesh[counter].Bake();
	}
	////////// 9 //////////
	MeshBuilder<VertexPosCol> nineMesh[6];
	glm::vec3 nineCoords[6];
	VertexArrayObject::Sptr nineVAO[6];

	MeshFactory::AddCube(nineMesh[0], glm::vec3(-5.64f, 5.92f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(nineMesh[1], glm::vec3(-5.8f, 5.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(nineMesh[2], glm::vec3(-6.05f, 5.92f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(nineMesh[3], glm::vec3(-5.8f, 6.1f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(nineMesh[4], glm::vec3(-5.8f, 6.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(nineMesh[5], glm::vec3(-6.05f, 6.44f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));

	for (int counter = 0; counter < 6; counter++) {
		nineVAO[counter] = nineMesh[counter].Bake();
	}
	////////// 0 //////////
	MeshBuilder<VertexPosCol> zeroMesh[6];
	glm::vec3 zeroCoords[6];
	VertexArrayObject::Sptr zeroVAO[6];

	MeshFactory::AddCube(zeroMesh[0], glm::vec3(-5.64f, 5.92f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(zeroMesh[1], glm::vec3(-5.8f, 5.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(zeroMesh[2], glm::vec3(-6.05f, 5.92f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(zeroMesh[3], glm::vec3(-5.64f, 6.44f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));
	MeshFactory::AddCube(zeroMesh[4], glm::vec3(-5.8f, 6.6f, .0f), glm::vec3(0.5f, 0.17f, 0.1f));
	MeshFactory::AddCube(zeroMesh[5], glm::vec3(-6.05f, 6.44f, .0f), glm::vec3(0.17f, 0.5f, 0.1f));

	for (int counter = 0; counter < 6; counter++) {
		zeroVAO[counter] = zeroMesh[counter].Bake();
	}
	/////////////////////////////////////////////////// Game loop ///////////////////////////////////////////////////////////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// WEEK 5: Input handling
		if (glfwGetKey(window, GLFW_KEY_A)) {
			
			if (!isButtonPressed == true) {
				// This is the action we want to perform on key press
				isMoving = !isMoving;
				paddleX += .1f;
			}
			//isButtonPressed = true;
		}
		else {
			//isButtonPressed = false;
			
		}
		
		if (glfwGetKey(window, GLFW_KEY_D)) {

			if (!isButtonPressed == true) {
				// This is the action we want to perform on key press
				isMoving = !isMoving;
				paddleX -= 0.1f;
			}
			//isButtonPressed = true;
		}
		else {
			//isButtonPressed = false;

		}
		
		if (glfwGetKey(window, GLFW_KEY_SPACE)) {
			if (!isButtonPressed == true && respawn == true) {
				respawn = false;
				ballvelx = 0.0f;
				ballvely = 1.0f;
				ballvelz = 0.0f;
				ballx = paddleX;
				bally = 2.5f;
			}
			//isButtonPressed = true
		}
		else {
			//isButtonPressed = false;
		}

		//***Manually reset ball for testing purposes***
		if (glfwGetKey(window, GLFW_KEY_E)) {

			if (!isButtonPressed == true) {
				// This is the action we want to perform on key press
				ballvelx = 0.0f;
				ballvely = 2.0f;
				ballvelz = 0.0f;
				ballx = paddleX;
				bally = 2.5f;
			}
			//isButtonPressed = true;
		}
		else {
			//isButtonPressed = false;

		}


		// Calculate the time since our last frame (dt)
		double thisFrame = glfwGetTime();
		float dt = static_cast<float>(thisFrame - lastFrame);

		// TODO: Week 5 - toggle code

		// Rotate our models around the z axis
		
		if (isMoving) {
			//transform  = glm::rotate(glm::mat4(1.0f), static_cast<float>(thisFrame), glm::vec3(0, 0, 1));
			paddle = glm::translate(glm::mat4(1.0f), glm::vec3(paddleX, 0.0f, 0.0f));
		}

		//////////////////////// Ball Movement ////////////////////////////////

		if (respawn == true) {
			ball = glm::translate(glm::mat4(1.0f), glm::vec3(paddleX, 1.0f, 0.0f));
		}
		else
		{
			ballx -= (ballvelx / 25)*((score + 4) / 4);
			bally -= (ballvely / 25)*((score + 4) / 4);
			ballz -= ballvelz/25;
			ball = glm::translate(glm::mat4(1.0f), glm::vec3(ballx, bally, ballz));
		}
					
		
		//// respawn condition and code
		if (bally > 8.0f)
		{
			if (lives > 0)
			{
				respawn = true;


				ballvelx = 0;
				ballvely = 0;
				ballvelz = 0;
				ball = glm::translate(glm::mat4(1.0f), glm::vec3(paddleX, 1.0f, 0.0f));
			}
			
		}

		//Keep paddle inside play space
		if (paddleX  >= 6.26 - 1.5)
		{
			paddleX = 6.25 - 1.5;
		}
		else if (paddleX <= -6.26 + 1.5)
		{
			paddleX = -6.25 + 1.5;
		}
		// Clear the color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Bind our shader and upload the uniform
		shader->Bind();

		// Draw MeshFactory Sample

																								
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* paddle);
		paddleVAO->Draw();

		VertexArrayObject::Unbind();

		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* ball);
		ballVAO->Draw();

		VertexArrayObject::Unbind(); 


		///////////// WALLS /////////////////
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* Walls);
		leftWallVAO->Draw();
		VertexArrayObject::Unbind();

		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* Walls);
		rightWallVAO->Draw();
		VertexArrayObject::Unbind();

		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * Walls);
		ceilingVAO->Draw();
		VertexArrayObject::Unbind();
		/////////////////////////////////////

		float ballXLeft = ballx + 0.25;
		float ballXRight = ballx - 0.25;
		float ballYTop = bally + 2.25f;
		float ballYBottom = bally + 1.75f;

		/////////////////////////////////////////////////// Box Physics

		for (int counter = 0; counter < 16; counter++) {
			
			if (counter == 1 || counter == 3 || counter == 5 || counter == 6 || counter == 7 || counter == 8 || counter == 9 || counter == 11 || counter == 13) {
				// Normal box (destroys in one hit)
				if (boxdestroyed[counter] != true)
				{
					if (((ballXLeft > boxCoords[counter].x - 0.875) && (ballXLeft < boxCoords[counter].x + 0.875) && (ballYTop > boxCoords[counter].y - 0.25) && (ballYTop < boxCoords[counter].y + 0.25)) //Top left corner in brick
						|| ((ballXLeft > boxCoords[counter].x - 0.875) && (ballXLeft < boxCoords[counter].x + 0.875) && (ballYBottom > boxCoords[counter].y - 0.25) && (ballYBottom < boxCoords[counter].y + 0.25)) //Bottom left Corner in brick
						|| ((ballXRight > boxCoords[counter].x - 0.875) && (ballXRight < boxCoords[counter].x + 0.875) && (ballYTop > boxCoords[counter].y - 0.25) && (ballYTop < boxCoords[counter].y + 0.25)) //Top right corner in brick
						|| ((ballXRight > boxCoords[counter].x - 0.875) && (ballXRight < boxCoords[counter].x + 0.875) && (ballYBottom > boxCoords[counter].y - 0.25) && (ballYBottom < boxCoords[counter].y + 0.25))) //Bottom right Corner in brick
					{
						boxdestroyed[counter] = true;
						//collision = true;
						score += 1;

						if (ballXLeft > boxCoords[counter].x + 0.875 || ballXRight < boxCoords[counter].x - 0.875)
						{
							ballvelx *= -1.0f;
						}
						else if (ballYTop > boxCoords[counter].y + 0.25 || ballYBottom < boxCoords[counter].y + 0.25)
						{
							ballvely *= -1.0f;
						}

					}
					else
					{
						shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * boxes[counter]);
						boxVAO[counter]->Draw();

						VertexArrayObject::Unbind();

						shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* boxText[counter]);
						vao2->Bind();
						glDrawElements(GL_TRIANGLES, interleaved_ibo->GetElementCount(), (GLenum)interleaved_ibo->GetElementType(), nullptr);

						boxText[counter] = glm::translate(glm::mat4(1.0f), boxCoords[counter]);

						VertexArrayObject::Unbind();
					}

				}
			}
			else if (counter == 0 || counter == 2 || counter == 4 || counter == 10 || counter == 12 || counter == 14 || counter == 15) {
				//Box type two (takes 2 hits to destroy)
				if (boxdamaged[counter] != true)
				{
					if ((ballXLeft > boxCoords[counter].x - 0.875 && ballXLeft < boxCoords[counter].x + 0.875 && ballYTop > boxCoords[counter].y - 0.25 && ballYTop < boxCoords[counter].y + 0.25) //Top left corner in brick
						|| (ballXLeft > boxCoords[counter].x - 0.875 && ballXLeft < boxCoords[counter].x + 0.875 && ballYBottom > boxCoords[counter].y - 0.25 && ballYBottom < boxCoords[counter].y + 0.25) //Bottom left Corner in brick
						|| (ballXRight > boxCoords[counter].x - 0.875 && ballXRight < boxCoords[counter].x + 0.875 && ballYTop > boxCoords[counter].y - 0.25 && ballYTop < boxCoords[counter].y + 0.25) //Top right corner in brick
						|| (ballXRight > boxCoords[counter].x - 0.875 && ballXRight < boxCoords[counter].x + 0.875 && ballYBottom > boxCoords[counter].y - 0.25 && ballYBottom < boxCoords[counter].y + 0.25)) //Bottom right Corner in brick
					{
						boxdamaged[counter] = true;
					
						if (ballXLeft > boxCoords[counter].x + 0.875 || ballXRight < boxCoords[counter].x - 0.875)
						{
							ballvelx *= -1.0f;
						}
						else if (ballYTop > boxCoords[counter].y + 0.25 || ballYBottom < boxCoords[counter].y + 0.25)
						{
							ballvely *= -1.0f;
						}
					}
					else
					{
						//draw undamaged box
						shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * boxes[counter]);
						boxVAO[counter]->Draw();

						if (counter == 0)
						{
							vao->Bind();
							glDrawArrays(GL_TRIANGLES, 0, 6);
							vao->Unbind();
						}
						else if (counter == 2)
						{
							vao->Bind();
							glDrawArrays(GL_TRIANGLES, 6, 6);
							vao->Unbind();
						}
						else if (counter == 4)
						{
							vao->Bind();
							glDrawArrays(GL_TRIANGLES, 12, 6);
							vao->Unbind();
						}
						else if (counter == 10)
						{
							vao->Bind();
							glDrawArrays(GL_TRIANGLES, 18, 6);
							vao->Unbind();
						}
						else if (counter == 12)
						{
							vao->Bind();
							glDrawArrays(GL_TRIANGLES, 24, 6);
							vao->Unbind();

						}
						else if (counter == 14)
						{
							vao->Bind();
							glDrawArrays(GL_TRIANGLES, 30, 6);
							vao->Unbind();
						}
						else if (counter == 15)
						{
							vao->Bind();
							glDrawArrays(GL_TRIANGLES, 36, 6);
							vao->Unbind();
						}
					}

				}
				else if (boxdamaged[counter] == true && boxdestroyed[counter] != true)
				{

					if ((ballXLeft > boxCoords[counter].x - 0.875 && ballXLeft < boxCoords[counter].x + 0.875 && ballYTop > boxCoords[counter].y - 0.25 && ballYTop < boxCoords[counter].y + 0.25) //Top left corner in brick
						|| (ballXLeft > boxCoords[counter].x - 0.875 && ballXLeft < boxCoords[counter].x + 0.875 && ballYBottom > boxCoords[counter].y - 0.25 && ballYBottom < boxCoords[counter].y + 0.25) //Bottom left Corner in brick
						|| (ballXRight > boxCoords[counter].x - 0.875 && ballXRight < boxCoords[counter].x + 0.875 && ballYTop > boxCoords[counter].y - 0.25 && ballYTop < boxCoords[counter].y + 0.25) //Top right corner in brick
						|| (ballXRight > boxCoords[counter].x - 0.875 && ballXRight < boxCoords[counter].x + 0.875 && ballYBottom > boxCoords[counter].y- 0.25 && ballYBottom < boxCoords[counter].y + 0.25)) //Bottom right Corner in brick
					{
						boxdestroyed[counter] = true;
						score += 1;
						//don't draw box as it is destroyed
						//collision = true;

						if (ballXLeft > boxCoords[counter].x + 0.875 || ballXRight < boxCoords[counter].x - 0.875)
						{
							ballvelx *= -1.0f;
						}
						else if (ballYTop > boxCoords[counter].y + 0.25 || ballYBottom < boxCoords[counter].y + 0.25)
						{
							ballvely *= -1.0f;
						}

					}
					else
					{
						//draw damaged box
						shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * boxes[counter]);
						boxVAO[counter]->Draw();

						VertexArrayObject::Unbind();

						shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* boxText[counter]);
						vao2->Bind();
						glDrawElements(GL_TRIANGLES, interleaved_ibo->GetElementCount(), (GLenum)interleaved_ibo->GetElementType(), nullptr);

						boxText[counter] = glm::translate(glm::mat4(1.0f), boxCoords[counter]);

						VertexArrayObject::Unbind();

					}
				}
			}
			
		}
		
		//////////////////////////////        UI        ///////////////////////////////////////////
		if (score % 10 == 0) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* zero);
			for (int counter = 0; counter < 6; counter++)
				zeroVAO[counter]->Draw();
		}
		if (score % 10 == 1) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* one[0]);
			oneVAO[0]->Draw();
		}
		if (score % 10 == 2) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* two);
			for (int counter = 0; counter < 5; counter++)
				twoVAO[counter]->Draw();
		}
		if (score % 10 == 3) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* three);
			for (int counter = 0; counter < 5; counter++)
				threeVAO[counter]->Draw();
		}
		if (score % 10 == 4) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* four);
			for (int counter = 0; counter < 4; counter++)
				fourVAO[counter]->Draw();
		}
		if (score % 10 == 5) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* five);
			for (int counter = 0; counter < 5; counter++)
				fiveVAO[counter]->Draw();
		}
		if (score % 10 == 6) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* six);
			for (int counter = 0; counter < 6; counter++)
				sixVAO[counter]->Draw();
		}
		if (score % 10 == 7) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* seven);
			for (int counter = 0; counter < 3; counter++)
				sevenVAO[counter]->Draw();
		}
		if (score % 10 == 8) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* eight);
			for (int counter = 0; counter < 7; counter++)
				eightVAO[counter]->Draw();
		}
		if (score % 10 == 9) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* nine);
			for (int counter = 0; counter < 6; counter++)
				nineVAO[counter]->Draw();
		}
		
		if (score >= 10) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* one[1]);
			oneVAO[1]->Draw();
		}

		
		///////////////////////////////////////////////////////////////////////////////////////////

		////////////////////////// Paddle Physics

		if (respawn != true)
		{
			if ( (ballx > paddleX - 1.5f) && (ballx < paddleX + 1.5f) && (bally + 0.25f > 3.0f) && (bally - 0.25f < 3.0f) )
			{
				ballvely *= -1.0f;

				if (ballx > paddleX)
				{
					ballvelx = -1 * (ballx - paddleX);
				}
				else if (ballx < paddleX)
				{
					ballvelx = -1 * (ballx - paddleX);
				}

			}
		}

		if (respawn != true)
		{
			if ((ballx > 6.75f) || (ballx < -6.75f))
			{
				ballvelx *= -1.0f;
			}
			else if (bally < -8.0f)
			{
				ballvely *= -1.0f;
			}
		}

		/*
		// Draw OBJ loaded model
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection() * transform3);
		vao4->Draw();
		
		VertexArrayObject::Unbind();
		
		shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* test);
		vao2->Bind();
		glDrawElements(GL_TRIANGLES, interleaved_ibo->GetElementCount(), (GLenum)interleaved_ibo->GetElementType(), nullptr);

		VertexArrayObject::Unbind();
		*/
		///////////////////////////////////   Life Counter   ////////////////////////////////////////////
		if (bally <= 8 && bally >= 7.9) {
			lives -= 1;
		}

		if (lives >= 5) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* life[2]);
			lifeVAO[2]->Draw();
		}
		if (lives >= 3) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* life[1]);
			lifeVAO[1]->Draw();
		}
		if (lives >= 1) {
			shader->SetUniformMatrix("u_ModelViewProjection", camera->GetViewProjection()* life[0]);
			lifeVAO[0]->Draw();
		}
		//////////////////////////////////Debug Testing Zone

		///////////////////////////////////////
		////// Bind texture 1
		glBindTexture(GL_TEXTURE_2D, textureHandle[0]);
		///// draw 

		glfwSwapBuffers(window);
	}



	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}

