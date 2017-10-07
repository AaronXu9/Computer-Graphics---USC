/*
CSCI 420 Computer Graphics, USC
Assignment 1: Height Fields
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

GLuint numVertice;
GLfloat *position;
GLfloat *color;
float zStudent = 3 + 7619063345 / 1000000000;

static const float S_value = 0.5;
int rounds = 0;

GLuint uiVBOData;
OpenGLMatrix *matrix;
BasicPipelineProgram* pipelineProgram;
GLuint vao;

int screenshots_count = 0; // to set screenshot file's name.

						   // write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
	unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
	glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

	ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

	if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
		cout << "File " << filename << " saved successfully." << endl;
	else cout << "Failed to save file " << filename << '.' << endl;

	delete[] screenshotData;
}

void bindProgram() {
	pipelineProgram->Bind();

	GLuint program = pipelineProgram->GetProgramHandle();
	glBindBuffer(GL_ARRAY_BUFFER, uiVBOData);
	GLuint loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc);
	const void* offset = (const void*)0;
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);

	GLuint loc2 = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(loc2);
	const void* offset2 = (const void*)(sizeof(GLfloat) * 3 * numVertice);
	glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, offset2);

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
	matrix->LookAt(0.0, 0.0, zStudent, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0);
	matrix->Translate(landTranslate[0], landTranslate[1], landTranslate[2]);  //translate
	matrix->Rotate(landRotate[0], 1.0, 0.0, 0.0); // rotate on x-axis
	matrix->Rotate(landRotate[1], 0.0, 1.0, 0.0); // rotate on y-axis
	matrix->Rotate(landRotate[2], 0.0, 0.0, 1.0); // rotate on z-axis
	matrix->Scale(landScale[0], landScale[1], landScale[2]);  //scale
	matrix->GetMatrix(m);
	glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);

}

void renderObj() {
	GLint first = 0;
	GLsizei count = numVertice;
	glShadeModel(GL_SMOOTH);

	glDrawArrays(GL_LINE_STRIP, first, count);  // points
}

void displayFunc()
{
	// render some stuff...
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	bindProgram();
	renderObj();
	glBindVertexArray(0);
	glutSwapBuffers();
}

void idleFunc()
{
	// do some stuff... 
	// for example, here, you can save the screenshots to disk (to make the animation)
	// make the screen update
	/*if (screenshots_count <= 300) { // store 200 .jpeg files under the ScreenShots folder.
	int i, j, k;
	string file_name = "./ScreenShots/";
	i = screenshots_count / 100;
	j = (screenshots_count - i * 100) / 10;
	k = screenshots_count % 10;
	file_name = file_name + to_string(i) + to_string(j) + to_string(k) + ".jpeg";
	saveScreenshot(file_name.c_str());
	}
	++screenshots_count;*/
	saveScreenshot("hw2_milestone.jpg");
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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

void createSplines() {

	float U_basis[4] = { 0.0,0.0,0.0,0.0 };
	int u_number = (int)(1.0 / 0.02 + 1);
	numVertice = u_number * rounds;

	position = (GLfloat*)malloc(sizeof(GLfloat)*numVertice * 3);
	color = (GLfloat*)malloc(sizeof(GLfloat)*numVertice * 4);

	int num = 0;

	for (int i = 0; i < numSplines; ++i) {
		for (int j = 1; j < splines[i].numControlPoints - 2; ++j) {

			Point p0 = splines[i].points[j - 1];
			Point p1 = splines[i].points[j];
			Point p2 = splines[i].points[j + 1];
			Point p3 = splines[i].points[j + 2];

			for (float u = 0.0f; u <= 1.0f; u = u + 0.02f) {
				U_basis[0] = (-S_value)*u*u*u + (2 * S_value)*u*u + (-S_value)*u;
				U_basis[1] = (2 - S_value)*u*u*u + (S_value - 3)*u*u + 1;
				U_basis[2] = (S_value - 2)*u*u*u + (3 - 2 * S_value)*u*u + (S_value)*u;
				U_basis[3] = (S_value)*u*u*u + (-S_value)*u*u;

				position[num * 3 + 0] = U_basis[0] * p0.x + U_basis[1] * p1.x + U_basis[2] * p2.x + U_basis[3] * p3.x;
				position[num * 3 + 1] = U_basis[0] * p0.y + U_basis[1] * p1.y + U_basis[2] * p2.y + U_basis[3] * p3.y;
				position[num * 3 + 2] = U_basis[0] * p0.z + U_basis[1] * p1.z + U_basis[2] * p2.z + U_basis[3] * p3.z;

				color[num * 4 + 0] = 0.0;
				color[num * 4 + 1] = 1.0;
				color[num * 4 + 2] = 0.0;
				color[num * 4 + 3] = 1.0;
				++num;

			}
		}
	}
}

void initVBO() {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &uiVBOData);

	glBindBuffer(GL_ARRAY_BUFFER, uiVBOData);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * numVertice + sizeof(GLfloat) * 4 * numVertice, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * 3 * numVertice, position);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * numVertice, sizeof(GLfloat) * 4 * numVertice, color);

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
		rounds += splines[i].numControlPoints - 3;
	}
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	// do additional initialization here...
	glEnable(GL_DEPTH_TEST);
	createSplines();
	matrix = new OpenGLMatrix();
	initVBO();  // initialize VBO and VAO
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

