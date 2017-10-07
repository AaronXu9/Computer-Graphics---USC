/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields
  C++ starter code

  Student username: <type your USC username here>
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
char windowTitle[512] = "CSCI 420 homework I";

// different rendering mode for users to switch
typedef enum { POINT_MODE, LINE_MODE, TRIANGLE_MODE } RENDER_MODE;
RENDER_MODE renderMode = TRIANGLE_MODE;

typedef enum {RED = 0, GREEN, BLUE}; 
// color file
ImageIO * colorImage;
GLuint color_inc;
GLuint color_row;
GLuint color_col;

//heightmap file
ImageIO * heightmapImage;
GLuint row, col; 
GLuint bytesPerPixel;
GLuint numVertice;

GLfloat *position;
GLfloat *color;
GLuint *indices;
int restartIndex;
float zStudent = 3 + 7619063345 / 1000000000;

GLuint uiVBOData, uiVBOIndices;
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

  delete [] screenshotData;
}

void bindProgram() {
	pipelineProgram->Bind();
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiVBOIndices);
	glEnable(GL_PRIMITIVE_RESTART); // enable it to draw the triangle_strips by indices.
	glPrimitiveRestartIndex(restartIndex);

	GLuint program = pipelineProgram->GetProgramHandle();
	glBindBuffer(GL_ARRAY_BUFFER, uiVBOData);
	GLuint loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc);
	const void* offset = (const void*)0;
	glVertexAttribPointer(loc,3,GL_FLOAT,GL_FALSE,0,offset);

	GLuint loc2 = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(loc2);
	const void* offset2 = (const void*)(sizeof(GLfloat) * 3 * numVertice);
	glVertexAttribPointer(loc2,4,GL_FLOAT,GL_FALSE,0,offset2);

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
	matrix->LookAt(0.0,0.0,zStudent,0.0,0.0,-1.0,0.0,1.0,0.0);
	matrix->Translate(landTranslate[0],landTranslate[1],landTranslate[2]);  //translate
	matrix->Rotate(landRotate[0], 1.0, 0.0, 0.0); // rotate on x-axis
	matrix->Rotate(landRotate[1], 0.0, 1.0, 0.0); // rotate on y-axis
	matrix->Rotate(landRotate[2], 0.0, 0.0, 1.0); // rotate on z-axis
	matrix->Scale(landScale[0],landScale[1],landScale[2]);  //scale
	matrix->GetMatrix(m);
	glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);

}

void renderObj() {
	GLint first = 0;
	GLsizei count = numVertice;
	glShadeModel(GL_SMOOTH);

	if (renderMode == POINT_MODE) {
		glDrawArrays(GL_POINTS, first, count);  // points
	}
	else if (renderMode == LINE_MODE) {
		glDrawElements(GL_TRIANGLE_STRIP, (row - 1)*col * 2 + row - 2, GL_UNSIGNED_INT, 0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // wireframe
		//glDrawElements(GL_LINE_STRIP, (row - 1)*col * 2 + row - 2, GL_UNSIGNED_INT, 0);
	}
	else if (renderMode == TRIANGLE_MODE) {
		glDrawElements(GL_TRIANGLE_STRIP, (row - 1)*col * 2 + row - 2, GL_UNSIGNED_INT, 0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // solid triangle
	}
}

void displayFunc()
{
  // render some stuff...
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
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
	if (screenshots_count <= 300) { // store 200 .jpeg files under the ScreenShots folder.
		int i, j, k;
		string file_name = "./ScreenShots/";
		i = screenshots_count / 100;
		j = (screenshots_count - i *100) / 10;
		k = screenshots_count % 10;
		file_name = file_name + to_string(i) + to_string(j) + to_string(k) + ".jpeg";
		saveScreenshot(file_name.c_str());
	}
	++screenshots_count;
    glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  // setup perspective matrix...
  GLfloat aspect = (GLfloat) w / h;
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

	case 'p' :  
		renderMode = POINT_MODE;
		cout << "change to point mode." << endl;
	break;

	case 'l': 
		renderMode = LINE_MODE;
		cout << "change to wireframe mode." << endl;
	break;

	case 't':  
		renderMode = TRIANGLE_MODE;
		cout << "change to solid triangle mode." << endl;
	break;
  }
}

bool loadColorImage() {
	// load another image to color the heightmap, although I use the same image
	// but it can work on different image with same size.
	
	colorImage = new ImageIO();
	const char* color_file = "./heightmap/Heightmap-color.jpg";
	if (colorImage->loadJPEG(color_file) != ImageIO::OK)  // check whether the color file exists
	{
		cout << "Error reading image " << color_file << ", using grayscale by default." << endl;
		return false;
	}
	else {
		color_inc = colorImage->getBytesPerPixel();
		color_row = colorImage->getHeight();
		color_col = colorImage->getWidth();	
		if (color_row != row || color_col != col) {
			cout << "color image should be " << row << "*" << col << " , using grayscale right now.";
			return false;
		}
		cout << "color the heightmap by using " << color_file << "." << endl;
		return true;
	}
}

void initHeightMap() {
	//Load Image to get every pixel's x value, y value and grayscale.
	//Converted it to 3D coordinates(x, y, z), i.e.its new position.

	float height;
	int number = 0;
	float sizeX = 4.0f; 
	float sizeY = 4.0f;

	row = heightmapImage->getHeight();
	col = heightmapImage->getWidth();
	bytesPerPixel = heightmapImage->getBytesPerPixel();
	numVertice = row * col; 
	position = (GLfloat*)malloc(sizeof(GLfloat)* 3 * numVertice); //(x,y,z) for each vertice
	color = (GLfloat*)malloc(sizeof(GLfloat) * 4 * numVertice); // RGBA for each vertice

	int channel = 0;
	if (bytesPerPixel == 3) {
		channel = GREEN;
	}

	bool color_flag = loadColorImage(); // color_flag to check whether the color file is loaded

	for (int i = 0; i < row; ++i) {
		for (int j = 0; j < col; ++j) { 
			height = (float)heightmapImage->getPixel(i, j, channel) / 255.0f;
			//center the heightmap at (0,0), range(-2,2) on X axis and Y axis
			position[number * 3 + 0] = -sizeX / 2.0f + (float)sizeX * i / (row - 1);
			position[number * 3 + 1] = -sizeY / 2.0f + (float)sizeY * j / (col - 1);
			position[number * 3 + 2] = height;
			if (color_flag == false) {
				// grayscale
				color[number * 4 + 0] = height;
				color[number * 4 + 1] = height;
				color[number * 4 + 2] = height;  
			}
			else {  // using the color from the loaded color file.
				color[number * 4 + 0] = (float)colorImage->getPixel(i, j, RED) / 255.0f;
				color[number * 4 + 1] = (float)colorImage->getPixel(i, j, GREEN) / 255.0f;
				color[number * 4 + 2] = (float)colorImage->getPixel(i, j, BLUE) / 255.0f;
			}
			color[number * 4 + 3] = 1.0;  // alpha
			++number;
		}
	}
}

void initIndex() {
	//provide index to every vertex, to tell the computer where it stores,
	//save space, like re - ordering all vertices to make it easier to draw triangle_strips.

	int number = 0;
	restartIndex = row * col; // index is from 0 to row*col-1, so restartIndex should not be one of them.
	indices = (GLuint*)malloc(sizeof(GLuint)*((row-1)*col*2 + row-2));
	/* 
	for example, 4*4 heightmap
	int indices[] =
	{
	0, 4, 1, 5, 2, 6, 3, 7, 16, // First row, then restart
	4, 8, 5, 9, 6, 10, 7, 11, 16, // Second row, then restart
	8, 12, 9, 13, 10, 14, 11, 15 // Third row, no restart
	};
	*/
	for (int i = 0; i < row - 1; ++i) {
		for (int j = 0; j < col; ++j) {
			for (int k = 0; k < 2; ++k) {
				int iRow = i + k;
				int index = iRow * col + j;
				indices[number] = index;
				++number;
			}
		}
		if (i != row - 2) { // the last row does not need restartIndex to restart.
			indices[number] = restartIndex;
			++number;
		}
	}
}

void initVBO() {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &uiVBOData);
	glGenBuffers(1, &uiVBOIndices);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, uiVBOData);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * numVertice + sizeof(GLfloat) * 4 * numVertice, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * 3 * numVertice, position);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * numVertice, sizeof(GLfloat) * 4 * numVertice, color);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uiVBOIndices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*((row - 1)*col * 2 + row - 2), indices, GL_STATIC_DRAW);

	glBindVertexArray(0);
}

void initPipelineProgram() {
	pipelineProgram = new BasicPipelineProgram();
	pipelineProgram->Init("../openGLHelper-starterCode");
}

void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  // do additional initialization here...
  glEnable(GL_DEPTH_TEST);
  initHeightMap();
  initIndex();
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
  glutInit(&argc,argv);

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

