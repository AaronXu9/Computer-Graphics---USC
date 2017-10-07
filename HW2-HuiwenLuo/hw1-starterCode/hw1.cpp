/*
CSCI 420 Computer Graphics, USC
Assignment 2: Rollar Coaster
C++ starter code

Student username: huiwenlu
*/

#include <iostream>
#include <cstring>
#include "openGLHeader.h"
#include "glutHeader.h"

#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"
#include <vector>

#ifdef WIN32
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#ifdef WIN32
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";

struct Point
{
	double x;
	double y;
	double z;
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline
{
	int numControlPoints;
	Point * points;
};

// the spline array 
Spline * splines;
// total number of splines 
int numSplines;

OpenGLMatrix *matrix;
BasicPipelineProgram* pipelineProgram;
GLuint program;

//variables for spline
GLuint splineDataVBO, anotherSplineVBO, leftNormalVBO, rightNormalVBO;
GLuint splineVAO;
vector<Point> position;
vector<Point> tangent;
vector<Point> normal;
vector<Point> binormal;
vector<Point> leftTrack; // position on left track + corssbar
vector<Point> rightTrack; // position on right track
GLuint track_tex;  // handle for the track texture
vector<GLfloat> trackSTs;
vector<GLfloat> crossSTs;
vector<Point> leftNormal; // normal on left track + corssbar
vector<Point> rightNormal; // normal on right track

Point p0, p1, p2, p3; // 4 control points for spline
int count_track = 0; // flag to add crossbar's vertices.

static const float S_value = 0.5;

//move step, float one to control the speed
int move_index = 0;
float move_float = 0.0f;

//variables for terrain
GLuint terrain_tex;  // handle for the terrain texture
GLuint terrainVAO;
GLuint terrainVBO;
GLfloat *terrainPosition;
GLfloat *terrainSTs;

//variables for sky
GLuint sky_tex[5]; // handle for the sky texture
GLuint skyVAO;
GLuint skyVBO;
GLfloat skyPosition[20][3] = { //24 corners for 6 faces, each corner is a point, and has 3 values-xyz
							   // Front face
	{ 64.0f, 64.0f, 64.0f },{ 64.0f, -32.0f, 64.0f },{ -64.0f, 64.0f, 64.0f },{ -64.0f, -32.0f, 64.0f },
	// Back face
	{ -64.0f, 64.0f, -64.0f },{ -64.0f, -32.0f, -64.0f },{ 64.0f, 64.0f, -64.0f },{ 64.0f, -32.0f, -64.0f },
	// Left face
	{ -64.0f, 64.0f, 64.0f },{ -64.0f, -32.0f, 64.0f },{ -64.0f, 64.0f, -64.0f },{ -64.0f, -32.0f, -64.0f },
	// Right face
	{ 64.0f, 64.0f, -64.0f },{ 64.0f, -32.0f, -64.0f },{ 64.0f, 64.0f, 64.0f },{ 64.0f, -32.0f, 64.0f },
	// Top face
	{ -64.0f, 64.0f, -64.0f },{ 64.0f, 64.0f, -64.0f },{ -64.0f, 64.0f, 64.0f },{ 64.0f, 64.0f, 64.0f }
};
GLfloat skySTs[20][2] = { //24 corners for 6 faces, each corner is a point, and has 2 values-ST
	{ 0.0f,1.0f },{ 0.0f,0.0f },{ 1.0f,1.0f },{ 1.0f,0.0f },
	{ 0.0f,1.0f },{ 0.0f,0.0f },{ 1.0f,1.0f },{ 1.0f,0.0f },
	{ 0.0f,1.0f },{ 0.0f,0.0f },{ 1.0f,1.0f },{ 1.0f,0.0f },
	{ 0.0f,1.0f },{ 0.0f,0.0f },{ 1.0f,1.0f },{ 1.0f,0.0f },
	{ 0.0f,1.0f },{ 0.0f,0.0f },{ 1.0f,1.0f },{ 1.0f,0.0f }
};

int screenshots_count = 0; // to set screenshot file's name.

void saveScreenshot(const char * filename)// write a screenshot to the specified filename
{
	unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
	glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

	ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

	if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
		cout << "File " << filename << " saved successfully." << endl;
	else cout << "Failed to save file " << filename << '.' << endl;

	delete[] screenshotData;
}

void renderLeftTrack() { // render left part and the crossbars.
	glBindVertexArray(splineVAO);
	glBindTexture(GL_TEXTURE_2D, track_tex);
	glBindBuffer(GL_ARRAY_BUFFER, splineDataVBO);
	GLuint loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc);
	const void* offset = (const void*)0;
	glVertexAttribPointer(loc, 3, GL_DOUBLE, GL_FALSE, 0, offset);

	GLuint loc2 = glGetAttribLocation(program, "texCoord");
	glEnableVertexAttribArray(loc2);
	const void* offset2 = (const void*)(sizeof(Point)* leftTrack.size());
	glVertexAttribPointer(loc2, 2, GL_FLOAT, GL_FALSE, 0, offset2);

	glBindBuffer(GL_ARRAY_BUFFER, leftNormalVBO);
	GLuint loc3 = glGetAttribLocation(program, "normal");
	glEnableVertexAttribArray(loc3);
	glVertexAttribPointer(loc3, 3, GL_DOUBLE, GL_FALSE, 0, (const void*)0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, leftTrack.size());

	glDisableVertexAttribArray(loc);
	glDisableVertexAttribArray(loc2);
	glDisableVertexAttribArray(loc3);
	glBindVertexArray(0);
}

void renderRightTrack() { // render right part
	glBindVertexArray(splineVAO);
	glBindTexture(GL_TEXTURE_2D, track_tex);
	glBindBuffer(GL_ARRAY_BUFFER, anotherSplineVBO);
	GLuint loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc);
	const void* offset = (const void*)0;
	glVertexAttribPointer(loc, 3, GL_DOUBLE, GL_FALSE, 0, offset);

	GLuint loc2 = glGetAttribLocation(program, "texCoord");
	glEnableVertexAttribArray(loc2);
	const void* offset2 = (const void*)(sizeof(Point)* rightTrack.size());
	glVertexAttribPointer(loc2, 2, GL_FLOAT, GL_FALSE, 0, offset2);

	glBindBuffer(GL_ARRAY_BUFFER, rightNormalVBO);
	GLuint loc3 = glGetAttribLocation(program, "normal");
	glEnableVertexAttribArray(loc3);
	glVertexAttribPointer(loc3, 3, GL_DOUBLE, GL_FALSE, 0, (const void*)0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, rightTrack.size());

	glDisableVertexAttribArray(loc);
	glDisableVertexAttribArray(loc2);
	glDisableVertexAttribArray(loc3);
	glBindVertexArray(0);
}

void renderTerrain() { // render terrain
	glBindVertexArray(terrainVAO);
	glBindTexture(GL_TEXTURE_2D, terrain_tex);
	glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);

	GLuint loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	GLuint loc2 = glGetAttribLocation(program, "texCoord");
	glEnableVertexAttribArray(loc2);
	glVertexAttribPointer(loc2, 2, GL_FLOAT, GL_FALSE, 0, (const void*)(sizeof(GLfloat) * 12));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(loc);
	glDisableVertexAttribArray(loc2);
	glBindVertexArray(0);
}

void renderSky() { // render skybox
	glBindVertexArray(skyVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
	GLuint loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*)0);

	GLuint loc2 = glGetAttribLocation(program, "texCoord");
	glEnableVertexAttribArray(loc2);
	glVertexAttribPointer(loc2, 2, GL_FLOAT, GL_FALSE, 0, (const void*)sizeof(skyPosition));

	for (int i = 0; i < 5; ++i) {
		glBindTexture(GL_TEXTURE_2D, sky_tex[i]);
		glDrawArrays(GL_TRIANGLE_STRIP, i * 4, 4);
	}

	glDisableVertexAttribArray(loc);
	glDisableVertexAttribArray(loc2);
	glBindVertexArray(0);
}

void setCamera() {  // move camera along the track
	GLint h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix");
	GLboolean isRowMajor = GL_FALSE;
	float p[16];
	matrix->SetMatrixMode(OpenGLMatrix::Projection);
	matrix->GetMatrix(p);
	glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p);

	GLint h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix");
	float m[16];
	matrix->SetMatrixMode(OpenGLMatrix::ModelView);
	matrix->LoadIdentity();
	//eye position is set to be just above the position.
	GLint h_cameraPosition = glGetUniformLocation(program, "cameraPosition");
	float cameraPos[3] = { 
		position[move_index].x + 0.3*normal[move_index].x + 0.1* binormal[move_index].x,
		position[move_index].y + 0.3*normal[move_index].y + 0.1* binormal[move_index].y,
		position[move_index].z + 0.3*normal[move_index].z + 0.1* binormal[move_index].z
	};
	glUniform3fv(h_cameraPosition, 1, cameraPos);

	matrix->LookAt(cameraPos[0], cameraPos[1], cameraPos[2],
		position[move_index].x + tangent[move_index].x + 0.1* binormal[move_index].x,
		position[move_index].y + tangent[move_index].y + 0.1* binormal[move_index].y,
		position[move_index].z + tangent[move_index].z + 0.1* binormal[move_index].z,
		normal[move_index].x, normal[move_index].y, normal[move_index].z);
	matrix->GetMatrix(m);
	glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);
}

void setLight() {
	// pass uniforms to shaders to render light
	GLint h_normalMatrix = glGetUniformLocation(program, "normalMatrix");
	float n[16];
	matrix->SetMatrixMode(OpenGLMatrix::ModelView);
	matrix->GetNormalMatrix(n); // get normal matrix
	glUniformMatrix4fv(h_normalMatrix, 1, GL_FALSE, n);

	GLint h_lightAmbient = glGetUniformLocation(program, "lightAmbient");
	float lightAmbient[4] = { 1.0f, 1.0f, 1.0f, 1.0f};
	glUniform4fv(h_lightAmbient, 1, lightAmbient);

	GLint h_lightDiffuse = glGetUniformLocation(program, "lightDiffuse");
	float lightDiffuse[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
	glUniform4fv(h_lightDiffuse, 1, lightDiffuse);

	GLint h_lightSpecular = glGetUniformLocation(program, "lightSpecular");
	float lightSpecular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glUniform4fv(h_lightSpecular, 1, lightSpecular);

	GLint h_lightPosition = glGetUniformLocation(program, "lightPosition");
	float lightPosition[3] = { -64.0f, 48.0f, -32.0f};
	glUniform3fv(h_lightPosition, 1, lightPosition);

	GLint h_matKa = glGetUniformLocation(program, "matKa");
	float matKa = 1.0f;
	glUniform1f(h_matKa, matKa);

	GLint h_matKd = glGetUniformLocation(program, "matKd");
	float matKd = 1.2f;
	glUniform1f(h_matKd, matKd);

	GLint h_matKs = glGetUniformLocation(program, "matKs");
	float matKs = 1.6f;
	glUniform1f(h_matKs, matKs);

	GLint h_matKsExp = glGetUniformLocation(program, "matKsExp");
	float matKsExp = 3.0f;
	glUniform1f(h_matKsExp, matKsExp);
}

void displayFunc()
{
	// render some stuff...
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	pipelineProgram->Bind();
	program = pipelineProgram->GetProgramHandle();

	renderLeftTrack();
	renderRightTrack();
	renderTerrain();
	renderSky();

	setCamera();
	setLight();

	glutSwapBuffers();
}

void idleFunc()
{
	//screenshots will lower the program, so I just comment it when I don't need screenshots.

	/*if (screenshots_count <= 850) { // store 850 .jpeg files under the ScreenShots folder.
	int i, j, k;
	string file_name = "./Screenshots/";
	i = screenshots_count / 100;
	j = (screenshots_count - i * 100) / 10;
	k = screenshots_count % 10;
	file_name = file_name + to_string(i) + to_string(j) + to_string(k) + ".jpeg";
	saveScreenshot(file_name.c_str());
	}
	++screenshots_count;*/

	if (move_index < position.size() - 3) {
		setCamera();
		move_float = move_float + 0.1f;
		move_index = (int)move_float;
	}
	glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
	glViewport(0, 0, w, h);

	// setup perspective matrix...
	GLfloat aspect = (GLfloat)w / h;
	matrix->SetMatrixMode(OpenGLMatrix::Projection);
	matrix->LoadIdentity();
	matrix->Perspective(45.0, aspect, 0.1, 1000.0);
	matrix->SetMatrixMode(OpenGLMatrix::ModelView);

	windowWidth = w;
	windowHeight = h;
}

void mouseMotionDragFunc(int x, int y)
{
	// mouse has moved and one of the mouse buttons is pressed (dragging)

	// the change in mouse position since the last invocation of this function
	int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

	switch (controlState)
	{
		// translate the landscape
	case TRANSLATE:
		if (leftMouseButton)
		{
			// control x,y translation via the left mouse button
			landTranslate[0] += mousePosDelta[0] * 0.01f;
			landTranslate[1] -= mousePosDelta[1] * 0.01f;
		}
		if (middleMouseButton)
		{
			// control z translation via the middle mouse button
			landTranslate[2] += mousePosDelta[1] * 0.01f;
		}
		break;

		// rotate the landscape
	case ROTATE:
		if (leftMouseButton)
		{
			// control x,y rotation via the left mouse button
			landRotate[0] += mousePosDelta[1];
			landRotate[1] += mousePosDelta[0];
		}
		if (middleMouseButton)
		{
			// control z rotation via the middle mouse button
			landRotate[2] += mousePosDelta[1];
		}
		break;

		// scale the landscape
	case SCALE:
		if (leftMouseButton)
		{
			// control x,y scaling via the left mouse button
			landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
			landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
		}
		if (middleMouseButton)
		{
			// control z scaling via the middle mouse button
			landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
		}
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
	// mouse has moved
	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
	// a mouse button has has been pressed or depressed

	// keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		leftMouseButton = (state == GLUT_DOWN);
		break;

	case GLUT_MIDDLE_BUTTON:
		middleMouseButton = (state == GLUT_DOWN);
		break;

	case GLUT_RIGHT_BUTTON:
		rightMouseButton = (state == GLUT_DOWN);
		break;
	}

	// keep track of whether CTRL and SHIFT keys are pressed
	switch (glutGetModifiers())
	{
	case GLUT_ACTIVE_CTRL:
		controlState = TRANSLATE;
		break;

	case GLUT_ACTIVE_SHIFT:
		controlState = SCALE;
		break;

		// if CTRL and SHIFT are not pressed, we are in rotate mode
	default:
		controlState = ROTATE;
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
	// switch all input keys to lowercase so that we don't have to distinguish uppercase and lowercase.
	key = tolower(key);
	switch (key)
	{
	case 27: // ESC key
		exit(0); // exit the program
		break;

	case ' ':
		cout << "You pressed the spacebar." << endl;
		break;

	case 'x':
		// take a screenshot
		saveScreenshot("screenshot.jpg");
		break;
	}
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
	// read the texture image
	ImageIO img;
	ImageIO::fileFormatType imgFormat;
	ImageIO::errorType err = img.load(imageFilename, &imgFormat);

	if (err != ImageIO::OK)
	{
		printf("Loading texture from %s failed.\n", imageFilename);
		return -1;
	}

	// check that the number of bytes is a multiple of 4
	if (img.getWidth() * img.getBytesPerPixel() % 4)
	{
		printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
		return -1;
	}

	// allocate space for an array of pixels
	int width = img.getWidth();
	int height = img.getHeight();
	unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

																		// fill the pixelsRGBA array with the image pixels
	memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
	for (int h = 0; h < height; h++)
		for (int w = 0; w < width; w++)
		{
			// assign some default byte values (for the case where img.getBytesPerPixel() < 4)
			pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
			pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
			pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
			pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

													   // set the RGBA channels, based on the loaded image
			int numChannels = img.getBytesPerPixel();
			for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
				pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
		}

	// bind the texture
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	// initialize the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

	// generate the mipmaps for this texture
	glGenerateMipmap(GL_TEXTURE_2D);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// query support for anisotropic texture filtering
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	printf("Max available anisotropic samples: %f\n", fLargest);
	// set anisotropic texture filtering
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

	// query for any errors
	GLenum errCode = glGetError();
	if (errCode != 0)
	{
		printf("Texture initialization error. Error code: %d.\n", errCode);
		return -1;
	}

	// de-allocate the pixel array -- it is no longer needed
	delete[] pixelsRGBA;

	return 0;
}

void createTerrain() {
	// set positions and STs for the terrain plane, i.e, 4 corners.
	terrainPosition = (GLfloat*)malloc(sizeof(GLfloat) * 12);  // 4 corners with xyz value;
	terrainSTs = (GLfloat*)malloc(sizeof(GLfloat) * 8);  // 4 corners with st value;
	// top left                     
	terrainPosition[0] = -64.0f;    
	terrainPosition[1] = -32.0f;
	terrainPosition[2] = -64.0f;
	terrainSTs[0] = 0;
	terrainSTs[1] = 0;
	//bottom left
	terrainPosition[3] = -64.0f;
	terrainPosition[4] = -32.0f;
	terrainPosition[5] = 64.0f;
	terrainSTs[2] = 0;
	terrainSTs[3] = 1;
	//top right
	terrainPosition[6] = 64.0f;
	terrainPosition[7] = -32.0f;
	terrainPosition[8] = -64.0f;
	terrainSTs[4] = 1;
	terrainSTs[5] = 0;
	//bottom right
	terrainPosition[9] = 64.0f;
	terrainPosition[10] = -32.0f;
	terrainPosition[11] = 64.0f;
	terrainSTs[6] = 1;
	terrainSTs[7] = 1;

	glGenTextures(1, &terrain_tex);
	initTexture("textures/sky_bottom.jpg", terrain_tex);

	glGenVertexArrays(1, &terrainVAO);
	glBindVertexArray(terrainVAO);

	glGenBuffers(1, &terrainVBO);
	glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12 + sizeof(GLfloat) * 8, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * 12, terrainPosition);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12, sizeof(GLfloat) * 8, terrainSTs);

	glBindVertexArray(0);
}

void createSky() {
	//init skybox's textures, the position, st value have been set before.
	//because the bottom part is the terrain plane, so I only set 5 faces here. 
	glGenTextures(5, sky_tex);
	initTexture("textures/sky_front.jpg", sky_tex[0]);
	initTexture("textures/sky_back.jpg", sky_tex[1]);
	initTexture("textures/sky_left.jpg", sky_tex[2]);
	initTexture("textures/sky_right.jpg", sky_tex[3]);
	initTexture("textures/sky_top.jpg", sky_tex[4]);

	glGenVertexArrays(1, &skyVAO);
	glBindVertexArray(skyVAO);

	glGenBuffers(1, &skyVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyPosition) + sizeof(skySTs), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(skyPosition), skyPosition);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(skyPosition), sizeof(skySTs), skySTs);

	glBindVertexArray(0);
}
//////////////////////////////////////////////////////
//supplementary functions to create rollar coaster
int loadSplines(char * argv)
{
	char * cName = (char *)malloc(128 * sizeof(char));
	FILE * fileList;
	FILE * fileSpline;
	int iType, i = 0, j, iLength;

	// load the track file 
	fileList = fopen(argv, "r");
	if (fileList == NULL)
	{
		printf("can't open file\n");
		exit(1);
	}

	// stores the number of splines in a global variable 
	fscanf(fileList, "%d", &numSplines);

	splines = (Spline*)malloc(numSplines * sizeof(Spline));

	// reads through the spline files 
	for (j = 0; j < numSplines; j++)
	{
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL)
		{
			printf("can't open file\n");
			exit(1);
		}

		// gets length for spline file
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		// allocate memory for all the points
		splines[j].points = (Point *)malloc(iLength * sizeof(Point));
		splines[j].numControlPoints = iLength;

		// saves the data to the struct
		while (fscanf(fileSpline, "%lf %lf %lf",
			&splines[j].points[i].x,
			&splines[j].points[i].y,
			&splines[j].points[i].z) != EOF)
		{
			i++;
		}
	}

	free(cName);

	return 0;
}

Point computeTangent(float u) {
	// using matrix multiplication to calculate tangent.
	// tan(u) = p'(u) = [3u^2 2u 1 0] M C
	float U_derivate_basis[4] = { 0.0,0.0,0.0,0.0 };
	Point tan_temp, unit_tan;

	U_derivate_basis[0] = (-S_value) * 3 * u*u + (2 * S_value) * 2 * u + (-S_value);
	U_derivate_basis[1] = (2 - S_value) * 3 * u*u + (S_value - 3) * 2 * u;
	U_derivate_basis[2] = (S_value - 2) * 3 * u*u + (3 - 2 * S_value) * 2 * u + (S_value);
	U_derivate_basis[3] = (S_value) * 3 * u*u + (-S_value) * 2 * u;

	tan_temp.x = U_derivate_basis[0] * p0.x + U_derivate_basis[1] * p1.x + U_derivate_basis[2] * p2.x + U_derivate_basis[3] * p3.x;
	tan_temp.y = U_derivate_basis[0] * p0.y + U_derivate_basis[1] * p1.y + U_derivate_basis[2] * p2.y + U_derivate_basis[3] * p3.y;
	tan_temp.z = U_derivate_basis[0] * p0.z + U_derivate_basis[1] * p1.z + U_derivate_basis[2] * p2.z + U_derivate_basis[3] * p3.z;

	float normalize = sqrt(tan_temp.x*tan_temp.x + tan_temp.y*tan_temp.y + tan_temp.z*tan_temp.z);
	unit_tan.x = (double)tan_temp.x / normalize;
	unit_tan.y = (double)tan_temp.y / normalize;
	unit_tan.z = (double)tan_temp.z / normalize;

	return unit_tan;
}

Point computeNormal(Point u1, Point u2) {
	// using cross product to calculate normal: a x b= (a2b3-a3b2)i+(a3b1-a1b3)j+(a1b2-a2b1)k
	Point normal_temp, unit_normal;
	normal_temp.x = u1.y * u2.z - u1.z * u2.y;
	normal_temp.y = u1.z * u2.x - u1.x * u2.z;
	normal_temp.z = u1.x * u2.y - u1.y * u2.x;

	float normalize = sqrt(normal_temp.x*normal_temp.x + normal_temp.y*normal_temp.y + normal_temp.z*normal_temp.z);
	unit_normal.x = normal_temp.x / normalize;
	unit_normal.y = normal_temp.y / normalize;
	unit_normal.z = normal_temp.z / normalize;

	return unit_normal;
}

Point negativePoint(Point u) {
	//we need to calculate some negative vectors when we create rail cross-section.
	Point neg;
	neg.x = -u.x;
	neg.y = -u.y;
	neg.z = -u.z;
	return neg;
}

Point calculatePoint(Point p, Point n, Point b) {
	//calculate the points of rail cross-section.
	Point v;
	float alpha = 0.01f;
	v.x = p.x + alpha*(n.x + b.x);
	v.y = p.y + alpha*(n.y + b.y);
	v.z = p.z + alpha*(n.z + b.z);
	return v;
}

Point shiftPoint(Point v, Point b) {
	//shift the point's position to get the right track's vertex
	Point tmp;
	tmp.x = v.x + 0.2 * b.x;
	tmp.y = v.y + 0.2 * b.y;
	tmp.z = v.z + 0.2 * b.z;
	return tmp;
}

Point pointWithU(float u) {
	//using p(u) = [u^3 u^2 u 1] M C to calculate the vertex's position with parameter u.
	float U_basis[4] = { 0.0,0.0,0.0,0.0 };
	U_basis[0] = (-S_value)*u*u*u + (2 * S_value)*u*u + (-S_value)*u;
	U_basis[1] = (2 - S_value)*u*u*u + (S_value - 3)*u*u + 1;
	U_basis[2] = (S_value - 2)*u*u*u + (3 - 2 * S_value)*u*u + (S_value)*u;
	U_basis[3] = (S_value)*u*u*u + (-S_value)*u*u;
	Point pos;
	pos.x = U_basis[0] * p0.x + U_basis[1] * p1.x + U_basis[2] * p2.x + U_basis[3] * p3.x;
	pos.y = U_basis[0] * p0.y + U_basis[1] * p1.y + U_basis[2] * p2.y + U_basis[3] * p3.y;
	pos.z = U_basis[0] * p0.z + U_basis[1] * p1.z + U_basis[2] * p2.z + U_basis[3] * p3.z;

	return pos;
}

void Subdivide(float u0, float u1, float maxLineLength) {
	// using sbudivide way to draw spline curves
	float umid = (u0 + u1) / 2;
	Point x0 = pointWithU(u0);
	Point x1 = pointWithU(u1);

	float length = sqrt((x0.x - x1.x)*(x0.x - x1.x) + (x0.y - x1.y)*(x0.y - x1.y) + (x0.z - x1.z)*(x0.z - x1.z));
	if (length > maxLineLength) {
		Subdivide(u0, umid, maxLineLength);
		Subdivide(umid, u1, maxLineLength);
	}
	else {
		position.push_back(x0);
		position.push_back(x1);
		Point tan0 = computeTangent(u0);
		Point tan1 = computeTangent(u1);
		tangent.push_back(tan0);
		tangent.push_back(tan1);
	}

}
///////////////////////////////////////////////////////////
void createTrack() {
	/* set rail cross-section
	v0 = p0 + alpha(-n0+b0),
	v1 = p0 + alpha(n0+b0),
	v2 = p0 + alpha(n0-b0),
	v3 = p0 + alpha(-n0-b0),
	*/
	// variables for left track
	Point p_0, p_1; //underline to distinguish it with global variables p0, p1
	Point n0, n1, neg_n0, neg_n1;
	Point b0, b1, neg_b0, neg_b1;
	Point t0, t1, neg_t0, neg_t1;
	Point v0, v1, v2, v3, v4, v5, v6, v7;
	// variables for right track
	Point q0, q1;
	Point s0, s1, s2, s3, s4, s5, s6, s7;

	for (int i = 0; i < position.size() - 2; ++i) {
		p_0 = position[i];
		n0 = normal[i];
		neg_n0 = negativePoint(n0);
		b0 = binormal[i];
		neg_b0 = negativePoint(b0);
		t0 = tangent[i];
		neg_t0 = negativePoint(t0);

		p_1 = position[i + 1];
		n1 = normal[i + 1];
		neg_n1 = negativePoint(n1);
		b1 = binormal[i + 1];
		neg_b1 = negativePoint(b1);
		t1 = tangent[i + 1];
		neg_t1 = negativePoint(t1);

		q0 = shiftPoint(p_0, b0);
		q1 = shiftPoint(p_1, b1);

		v0 = calculatePoint(p_0, neg_n0, b0); s0 = calculatePoint(q0, neg_n0, b0);
		v1 = calculatePoint(p_0, n0, b0);  s1 = calculatePoint(q0, n0, b0);
		v2 = calculatePoint(p_0, n0, neg_b0); s2 = calculatePoint(q0, n0, neg_b0);
		v3 = calculatePoint(p_0, neg_n0, neg_b0); s3 = calculatePoint(q0, neg_n0, neg_b0);

		v4 = calculatePoint(p_1, neg_n1, b1); s4 = calculatePoint(q1, neg_n1, b1);
		v5 = calculatePoint(p_1, n1, b1); s5 = calculatePoint(q1, n1, b1);
		v6 = calculatePoint(p_1, n1, neg_b1); s6 = calculatePoint(q1, n1, neg_b1);
		v7 = calculatePoint(p_1, neg_n1, neg_b1); s7 = calculatePoint(q1, neg_n1, neg_b1);
		// left track, face normal just get the approximate value.
		//front face 
		leftTrack.push_back(v2); leftNormal.push_back(t0);
		leftTrack.push_back(v3); leftNormal.push_back(t0);
		leftTrack.push_back(v1); leftNormal.push_back(t0);
		leftTrack.push_back(v0); leftNormal.push_back(t0);
		//back face
		leftTrack.push_back(v6); leftNormal.push_back(t1);
		leftTrack.push_back(v7); leftNormal.push_back(t1);
		leftTrack.push_back(v5); leftNormal.push_back(t1);
		leftTrack.push_back(v4); leftNormal.push_back(t1);
		//left face
		leftTrack.push_back(v2); leftNormal.push_back(b0);
		leftTrack.push_back(v3); leftNormal.push_back(b0);
		leftTrack.push_back(v6); leftNormal.push_back(b0);
		leftTrack.push_back(v7); leftNormal.push_back(b0);
		//right face
		leftTrack.push_back(v1); leftNormal.push_back(b1);
		leftTrack.push_back(v0); leftNormal.push_back(b1);
		leftTrack.push_back(v5); leftNormal.push_back(b1);
		leftTrack.push_back(v4); leftNormal.push_back(b1);
		//top face
		leftTrack.push_back(v2); leftNormal.push_back(n0);
		leftTrack.push_back(v1); leftNormal.push_back(n0);
		leftTrack.push_back(v6); leftNormal.push_back(n0);
		leftTrack.push_back(v5); leftNormal.push_back(n0);
		//bottom face
		leftTrack.push_back(v3); leftNormal.push_back(n1);
		leftTrack.push_back(v0); leftNormal.push_back(n1);
		leftTrack.push_back(v7); leftNormal.push_back(n1);
		leftTrack.push_back(v4); leftNormal.push_back(n1);

		if (count_track % 10 == 0) {  // crossbar part
			//left face
			leftTrack.push_back(v5); leftNormal.push_back(b0);
			leftTrack.push_back(v4); leftNormal.push_back(b0);
			leftTrack.push_back(v1); leftNormal.push_back(b0);
			leftTrack.push_back(v0); leftNormal.push_back(b0);
			//right face
			leftTrack.push_back(s2); leftNormal.push_back(b1);
			leftTrack.push_back(s3); leftNormal.push_back(b1);
			leftTrack.push_back(s6); leftNormal.push_back(b1);
			leftTrack.push_back(s7); leftNormal.push_back(b1);
			//front face
			leftTrack.push_back(v1); leftNormal.push_back(t0);
			leftTrack.push_back(v0); leftNormal.push_back(t0);
			leftTrack.push_back(s2); leftNormal.push_back(t0);
			leftTrack.push_back(s3); leftNormal.push_back(t0);
			//back face
			leftTrack.push_back(s6); leftNormal.push_back(t1);
			leftTrack.push_back(s7); leftNormal.push_back(t1);
			leftTrack.push_back(v5); leftNormal.push_back(t1);
			leftTrack.push_back(v4); leftNormal.push_back(t1);
			//top face
			leftTrack.push_back(v5); leftNormal.push_back(n0);
			leftTrack.push_back(v1); leftNormal.push_back(n0);
			leftTrack.push_back(s6); leftNormal.push_back(n0);
			leftTrack.push_back(s2); leftNormal.push_back(n0);
			//bottom face
			leftTrack.push_back(s7); leftNormal.push_back(n1);
			leftTrack.push_back(s3); leftNormal.push_back(n1);
			leftTrack.push_back(v0); leftNormal.push_back(n1);
			leftTrack.push_back(v4); leftNormal.push_back(n1);
			for (int j = 0; j < 6; ++j) {
				crossSTs.push_back(0.0f);
				crossSTs.push_back(0.0f);
				crossSTs.push_back(0.0f);
				crossSTs.push_back(1.0f);
				crossSTs.push_back(1.0f);
				crossSTs.push_back(0.0f);
				crossSTs.push_back(1.0f);
				crossSTs.push_back(1.0f);
			}
		}
		//right track , face normal just get the approximate value.
		//front face
		rightTrack.push_back(s2); rightNormal.push_back(t0);
		rightTrack.push_back(s3); rightNormal.push_back(t0);
		rightTrack.push_back(s1); rightNormal.push_back(t0);
		rightTrack.push_back(s0); rightNormal.push_back(t0);
		//back face
		rightTrack.push_back(s6); rightNormal.push_back(t1);
		rightTrack.push_back(s7); rightNormal.push_back(t1);
		rightTrack.push_back(s5); rightNormal.push_back(t1);
		rightTrack.push_back(s4); rightNormal.push_back(t1);
		//left face
		rightTrack.push_back(s2); rightNormal.push_back(b0);
		rightTrack.push_back(s3); rightNormal.push_back(b0);
		rightTrack.push_back(s6); rightNormal.push_back(b0);
		rightTrack.push_back(s7); rightNormal.push_back(b0);
		//right face
		rightTrack.push_back(s1); rightNormal.push_back(b1);
		rightTrack.push_back(s0); rightNormal.push_back(b1);
		rightTrack.push_back(s5); rightNormal.push_back(b1);
		rightTrack.push_back(s4); rightNormal.push_back(b1);
		//top face
		rightTrack.push_back(s2); rightNormal.push_back(n0);
		rightTrack.push_back(s1); rightNormal.push_back(n0);
		rightTrack.push_back(s6); rightNormal.push_back(n0);
		rightTrack.push_back(s5); rightNormal.push_back(n0);
		//bottom face
		rightTrack.push_back(s3); rightNormal.push_back(n1);
		rightTrack.push_back(s0); rightNormal.push_back(n1);
		rightTrack.push_back(s7); rightNormal.push_back(n1);
		rightTrack.push_back(s4); rightNormal.push_back(n1);
		for (int j = 0; j < 6; ++j) {
			trackSTs.push_back(0.0f);
			trackSTs.push_back(0.0f);
			trackSTs.push_back(0.0f);
			trackSTs.push_back(1.0f);
			trackSTs.push_back(1.0f);
			trackSTs.push_back(0.0f);
			trackSTs.push_back(1.0f);
			trackSTs.push_back(1.0f);
		}
		++count_track;
	}
}

void createSplines() {
	Point v, n, b;
	int start = 0;
	for (int i = 0; i < numSplines; ++i) {
		for (int j = 1; j < splines[i].numControlPoints - 2; ++j) {
			//using 4 control points to draw spline curves.
			p0 = splines[i].points[j - 1];
			p1 = splines[i].points[j];
			p2 = splines[i].points[j + 1];
			p3 = splines[i].points[j + 2];

			Subdivide(0.0f, 1.0f, 0.1);
			/*
			first: N0 = unit(T0 x V) and B0 = unit(T0 x N0),
			next: N1 = unit(B0 x T1) and B1 = unit(T1 x N1).
			*/
			if (normal.empty()) {
				//arbitrary vector v to calculate the first normal
				v.x = tangent[0].y;
				v.y = tangent[0].x;
				v.z = tangent[0].z;
				n = computeNormal(v, tangent[0]);
				normal.push_back(n);
				b = computeNormal(tangent[0], n);
				binormal.push_back(b);
				start = 1;
			}
			for (int k = start; k < tangent.size(); ++k) {
				v = b;
				n = computeNormal(v, tangent[k]);
				normal.push_back(n);
				b = computeNormal(tangent[k], n);
				binormal.push_back(b);
			}
			start = tangent.size();
		}
	}

	createTrack();
	glGenTextures(1, &track_tex);
	initTexture("textures/track.jpg", track_tex);

	glGenVertexArrays(1, &splineVAO);
	glBindVertexArray(splineVAO);

	glGenBuffers(1, &splineDataVBO);
	glBindBuffer(GL_ARRAY_BUFFER, splineDataVBO);
	glBufferData(GL_ARRAY_BUFFER, leftTrack.size() * sizeof(Point) + trackSTs.size() * sizeof(GLfloat)
		+ crossSTs.size() * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, leftTrack.size() * sizeof(Point), leftTrack.data());
	glBufferSubData(GL_ARRAY_BUFFER, leftTrack.size() * sizeof(Point), trackSTs.size() * sizeof(GLfloat), trackSTs.data());
	glBufferSubData(GL_ARRAY_BUFFER, leftTrack.size() * sizeof(Point) + trackSTs.size() * sizeof(GLfloat),
		crossSTs.size() * sizeof(GLfloat), crossSTs.data());

	glGenBuffers(1, &leftNormalVBO);
	glBindBuffer(GL_ARRAY_BUFFER, leftNormalVBO);
	glBufferData(GL_ARRAY_BUFFER, leftNormal.size() * sizeof(Point), leftNormal.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &anotherSplineVBO);
	glBindBuffer(GL_ARRAY_BUFFER, anotherSplineVBO);
	glBufferData(GL_ARRAY_BUFFER, rightTrack.size() * sizeof(Point) + trackSTs.size() * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, rightTrack.size() * sizeof(Point), rightTrack.data());
	glBufferSubData(GL_ARRAY_BUFFER, rightTrack.size() * sizeof(Point), trackSTs.size() * sizeof(GLfloat), trackSTs.data());

	glGenBuffers(1, &rightNormalVBO);
	glBindBuffer(GL_ARRAY_BUFFER, rightNormalVBO);
	glBufferData(GL_ARRAY_BUFFER, rightNormal.size() * sizeof(Point), rightNormal.data(), GL_STATIC_DRAW);
	glBindVertexArray(0);
}

void initPipelineProgram() {
	pipelineProgram = new BasicPipelineProgram();
	pipelineProgram->Init("../openGLHelper-starterCode");
}

void initScene(int argc, char *argv[])
{
	// load the image from a jpeg disk file to main memory
	loadSplines(argv[1]);

	printf("Loaded %d spline(s).\n", numSplines);
	for (int i = 0; i < numSplines; i++) {
		printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);
	}
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	// do additional initialization here...
	glEnable(GL_DEPTH_TEST);

	createSplines();
	createTerrain();
	createSky();

	matrix = new OpenGLMatrix();
	initPipelineProgram();
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		cout << "The arguments are incorrect." << endl;
		cout << "usage: ./hw1 <heightmap file>" << endl;
		exit(EXIT_FAILURE);
	}

	cout << "Initializing GLUT..." << endl;
	glutInit(&argc, argv);

	cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
#endif

	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(0, 0);
	glutCreateWindow(windowTitle);

	cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
	cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// tells glut to use a particular display function to redraw 
	glutDisplayFunc(displayFunc);
	// perform animation inside idleFunc
	glutIdleFunc(idleFunc);
	// callback for mouse drags
	glutMotionFunc(mouseMotionDragFunc);
	// callback for idle mouse movement
	glutPassiveMotionFunc(mouseMotionFunc);
	// callback for mouse button changes
	glutMouseFunc(mouseButtonFunc);
	// callback for resizing the window
	glutReshapeFunc(reshapeFunc);
	// callback for pressing the keys on the keyboard
	glutKeyboardFunc(keyboardFunc);

	// init glew
#ifdef __APPLE__
	// nothing is needed on Apple
#else
	// Windows, Linux
	GLint result = glewInit();
	if (result != GLEW_OK)
	{
		cout << "error: " << glewGetErrorString(result) << endl;
		exit(EXIT_FAILURE);
	}
#endif

	// do initialization
	initScene(argc, argv);

	// sink forever into the glut loop
	glutMainLoop();
}

