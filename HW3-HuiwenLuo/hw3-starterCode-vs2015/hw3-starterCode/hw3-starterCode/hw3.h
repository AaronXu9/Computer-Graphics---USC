#pragma once
#ifdef WIN32
#include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
#include <GL/gl.h>
#include <GL/glut.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#define strcasecmp _stricmp
#endif

#include <imageIO.h>
#include <cmath>
#include <string>
#include <ctime>
#include <iostream>
using namespace std;

/*************macros**********************/
//window setting
#define WIDTH 640
#define HEIGHT 480

//the field of view of the camera
#define fov 60.0

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

#define PI 3.14159265

#define INFINITY 1e8

//limitations
#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

//max threads used in the program
#define MAXTHREADNUM 20

/*********************struct*********************************/
struct Vertex
{
	double position[3];
	double color_diffuse[3];
	double color_specular[3];
	double normal[3];
	double shininess;
};

struct Triangle
{
	Vertex v[3];
};

struct Sphere
{
	double position[3];
	double color_diffuse[3];
	double color_specular[3];
	double shininess;
	double radius;
};

struct Light
{
	double position[3];
	double color[3];
};

struct Color {
	double r;
	double g;
	double b;

	Color() : r(0), g(0), b(0) {}
	Color(double r, double g, double b) : r(r), g(g), b(b) {}

	Color& operator+= (const Color& other) {
		r += other.r;
		if (r > 1.0f) r = 1.0f;
		else if (r < 0.0f) r = 0.0f;
		g += other.g;
		if (g > 1.0f) g = 1.0f;
		else if (g < 0.0f) g = 0.0f;
		b += other.b;
		if (b > 1.0f) b = 1.0f;
		else if (b < 0.0f) b = 0.0f;
		return *this;
	}
	Color operator* (double scalar) const { return Color(scalar * r, scalar * g, scalar * b); }
};

struct Vector {
	double x;
	double y;
	double z;

	Vector() : x(0), y(0), z(0) {}
	Vector(double x, double y, double z) : x(x), y(y), z(z) {}

	// operators reload
	Vector operator+ (const Vector& vec) const { return Vector(x + vec.x, y + vec.y, z + vec.z); }
	Vector operator- (const Vector& vec) const { return Vector(x - vec.x, y - vec.y, z - vec.z); }
	Vector operator* (double scalar) const { return Vector(scalar * x, scalar * y, scalar * z); }

	double dot(const Vector& vec) { return x*vec.x + y*vec.y + z*vec.z; }
	double magnitude() { return sqrt(x*x + y*y + z*z); }
	Vector& cross(const Vector& vec) {
		// using cross product to calculate normal: a x b= (a2b3-a3b2)i+(a3b1-a1b3)j+(a1b2-a2b1)k
		double vx = y * vec.z - z * vec.y;
		double vy = z * vec.x - x * vec.z;
		double vz = x * vec.y - y * vec.x;
		return Vector(vx, vy, vz);
	}
	Vector& normalize() {
		double distance = sqrt(x*x + y*y + z*z);
		if (distance != 0) {
			x = x / distance;
			y = y / distance;
			z = z / distance;
		}
		return *this;
	}
};

struct Ray {
	Vector origin;
	Vector direction;

	Ray() {}
	Ray(const Vector& origin, const Vector& direction) : origin(origin), direction(direction) {}

	bool intersectSphere(const Sphere& sphere, Vector& intersection, double& distance);
	bool intersectTriangle(const Triangle& triangle, Vector& intersection, double& distance);
};

/*****************variables********************************/
//store the file name
char * filename = NULL;

//pixel buffer to store rgb
unsigned char buffer[HEIGHT][WIDTH][3];

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

bool anti_alias = false; // flag to show whether the program uses antialiasing
int reflect_time = 0; // default is 0;

Vector sphere_intersection(0, 0, INFINITY);
int index_sphere = -1; // record the index of sphere, which is intersected with ray
Vector triangle_intersection(0, 0, INFINITY);
int index_triangle = -1; // record the index of triangle, which is intersected with ray

int mode = MODE_DISPLAY;

/*****************functions******************************/
// load scene
int loadScene(char *argv);
void parse_doubles(FILE* file, const char *check, double p[3]);
void parse_rad(FILE *file, double *r);
void parse_shi(FILE *file, double *shi);
void parse_check(const char *expected, char *found);

//plot pixels
void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);

//sphere part
Vector normalAtSphere(Sphere sphere, Vector intersection);
Color createSphereShading(Sphere& sphere, Light& light, Vector& intersection);//Sphere Phong Shading																			  
Color detectSphereIntersection(Ray& ray, Color& color, double& closest_dist);

//triangle part
Vector normalAtTriangle(Triangle triangle, Vector intersection);
Color createTriangleShading(Triangle& triangle, Light& light, Vector& intersection);//Triangle Phong Shading
Color detectTriangleIntersection(Ray& ray, Color& color, double& closest_dist);

//create ray and color
Color createColor(Ray& ray, int count_reflect);
Ray createRay(double x, double y);

//draw scene
void draw_scene();

//save image
void save_jpg();
