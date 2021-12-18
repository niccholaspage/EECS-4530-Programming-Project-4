#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include "LoadShaders.h"
#include "linmath.h"
//#include "vgl.h"
#include <map>
#include <vector>
#include "HierarchicalObject.h"
using namespace std;

#define BUFFER_OFFSET(x)  ((const void*) (x))

/*
* @author Nicholas Nassar
* Date: 12/17/2021
* Modified the sample to include movement of the right leg, and
* moved object files to objs/ directory for better project organization.
* This should be at the same level as the source code directory.
*/

GLuint programID;
/*
* Arrays to store the indices/names of the Vertex Array Objects and
* Buffers.  Rather than using the books enum approach I've just
* gone out and made a bunch of them and will use them as needed.
*
* Not the best choice I'm sure.
*/

GLuint vertexBuffers[20], arrayBuffers[20], elementBuffers[20];
/*
* Global variables
*   The location for the transformation and the current rotation
*   angle are set up as globals since multiple methods need to
*   access them.
*/
float rotationAngle;
bool elements;
int nbrTriangles[20], materialToUse = 0;
int startTriangle = 0, endTriangle = 12;
bool rotationOn = false;
bool motionOn = false;
float t; // parameter for motion...

#ifdef __VMATH_H__
vmath::mat4 rotation, viewMatrix, projectionMatrix;
#else
mat4x4 rotation, viewMatrix, projectionMatrix;
#endif
map<string, GLuint> locationMap;
HierarchicalObject* BaseObject, * lowerArmObject, * upperArmObject, * lampObject;

// Prototypes
GLuint buildProgram(string vertexShaderName, string fragmentShaderName);
GLFWwindow* glfwStartUp(int& argCount, char* argValues[],
	string windowTitle = "No Title", int width = 500, int height = 500);
void setAttributes(float lineWidth = 1.0, GLenum face = GL_FRONT_AND_BACK,
	GLenum fill = GL_FILL);
void buildObjects();
void connectSkeleton();
void BuildPart(int nbrTriangles, int nbrVertices, GLfloat  vertices[], int nbrNormals, GLfloat  normals[], GLfloat color[]);
void getLocations();
void init(string vertexShader, string fragmentShader);
void updateJointPositions();
float* readOBJFile(string filename, int& nbrTriangles, float*& normalArray);
/*
 * Error callback routine for glfw -- uses cstdio
 */
static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

/*
 * keypress callback for glfw -- Escape exits...
 */
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
#ifdef __VMATH_H__
		rotation = vmath::rotate(10.0f, vmath::vec3(0.0f, 1.0f, 0.0f)) * rotation;
#else
		mat4x4_rotate_Y(rotation, rotation, 0.31419f);
#endif
	}
	else if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
#ifdef __VMATH_H__
		rotation = vmath::rotate(-10.0f, vmath::vec3(0.0f, 1.0f, 0.0f)) * rotation;
#else
		mat4x4_rotate_Y(rotation, rotation, -0.31419f);
#endif
	}
	else if (key == GLFW_KEY_X && action == GLFW_PRESS) {
#ifdef __VMATH_H__
		viewMatrix = vmath::lookat(vmath::vec3(1.0f, 0.0f, 0.0f),
			vmath::vec3(0.0f, 0.0f, 0.0f),
			vmath::vec3(0.0f, 1.0f, 0.0f));
#else
		mat4x4_look_at(viewMatrix, vec3{ 1.0f, 0.0f, 0.0f }, vec3{ 0.0f, 0.0f, 0.0f }, vec3{ 0.0f, 1.0f, 0.0f });
#endif
	}
	else if (key == GLFW_KEY_Y && action == GLFW_PRESS) {
#ifdef __VMATH_H__
		viewMatrix = vmath::lookat(vmath::vec3(0.0f, 1.0f, 0.0f),
			vmath::vec3(0.0f, 0.0f, 0.0f),
			vmath::vec3(0.0f, 0.0f, 1.0f));
#else
		mat4x4_look_at(viewMatrix, vec3{ 0.0f, 1.0f, 0.0f }, vec3{ 0.0f, 0.0f, 0.0f }, vec3{ 0.0f, 0.0f, 1.0f });
#endif
	}

	else if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
#ifdef __VMATH_H__
		viewMatrix = vmath::lookat(vmath::vec3(0.0f, 0.0f, 1.0f),
			vmath::vec3(0.0f, 0.0f, 0.0f),
			vmath::vec3(0.0f, 1.0f, 0.0f));
#else
		mat4x4_look_at(viewMatrix, vec3{ 0.0f, 0.0f, 1.0f }, vec3{ 0.0f, 0.0f, 0.0f }, vec3{ 0.0f, 1.0f, 0.0f });
#endif
	}
	else if (key == GLFW_KEY_M && action == GLFW_PRESS) {
		motionOn = !motionOn;
	}
}

/*
 * Routine to encapsulate some of the startup routines for GLFW.  It returns the window ID of the
 * single window that is created.
 */
GLFWwindow* glfwStartUp(int& argCount, char* argValues[], string title, int width, int height) {
	GLFWwindow* window;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);

	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSwapInterval(1);

	return window;
}


/*
 * Use the author's routines to build the program and return the program ID.
 */
GLuint buildProgram(string vertexShaderName, string fragmentShaderName) {

	/*
	*  Use the Books code to load in the shaders.
	*/
	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, vertexShaderName.c_str() },
		{ GL_FRAGMENT_SHADER, fragmentShaderName.c_str() },
		{ GL_NONE, NULL }
	};
	GLuint program = LoadShaders(shaders);
	if (program == 0) {
		cerr << "GLSL Program didn't load.  Error \n" << endl
			<< "Vertex Shader = " << vertexShaderName << endl
			<< "Fragment Shader = " << fragmentShaderName << endl;
	}
	glUseProgram(program);
	return program;
}

/*
 * Set up the clear color, lineWidth, and the fill type for the display.
 */
void setAttributes(float lineWidth, GLenum face, GLenum fill) {
	/*
	* I'm using wide lines so that they are easier to see on the screen.
	* In addition, this version fills in the polygons rather than leaving it
	* as lines.
	*/
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glLineWidth(lineWidth);
	glPolygonMode(face, fill);
	glEnable(GL_DEPTH_TEST);

}

/*
 * Global pointers for the objects and the routine to read them in and to
 * set them up in vertex array objects and buffer array objects.
 *
 * Needs the upper body yet.
 */
HierarchicalObject* pelvis, * upperLeftLeg, * upperRightLeg, * lowerLeftLeg, * lowerRightLeg, * leftFoot, * rightFoot, * upperBody;
void buildSkeleton() {
	string inputFiles[] = { "objs/pelvis.obj", "objs/limb20.obj", "objs/limb20.obj", "objs/limb20.obj", "objs/limb20.obj", "objs/foot.obj", "objs/foot.obj", "objs/upperBody.obj" };


	GLfloat* verticesBase;
	GLfloat* normalsBase;
	int nbrOfParts = sizeof(inputFiles) / sizeof(string);
	/*
	*/
	glGenVertexArrays(nbrOfParts, vertexBuffers);

	for (int currentPart = 0; currentPart < nbrOfParts; currentPart++) {

		glBindVertexArray(vertexBuffers[currentPart]);


		/*
		 * Read object in from obj file.
		 */

		glGenBuffers(1, &(arrayBuffers[currentPart]));
		glBindBuffer(GL_ARRAY_BUFFER, arrayBuffers[currentPart]);
		GLfloat baseColor[] = { 0.7f, 0.7f, 0.7f, 1.0f };
		verticesBase = readOBJFile(inputFiles[currentPart], nbrTriangles[currentPart], normalsBase);
		BuildPart(nbrTriangles[currentPart], nbrTriangles[currentPart] * 3 * 4, verticesBase, nbrTriangles[currentPart] * 3 * 3, normalsBase, baseColor);
	}
	pelvis = new HierarchicalObject(programID, vertexBuffers[0], arrayBuffers[0], nbrTriangles[0] * 3);
	upperLeftLeg = new HierarchicalObject(programID, vertexBuffers[1], arrayBuffers[1], nbrTriangles[1] * 3);
	upperRightLeg = new HierarchicalObject(programID, vertexBuffers[2], arrayBuffers[2], nbrTriangles[2] * 3);
	lowerLeftLeg = new HierarchicalObject(programID, vertexBuffers[3], arrayBuffers[3], nbrTriangles[3] * 3);
	lowerRightLeg = new HierarchicalObject(programID, vertexBuffers[4], arrayBuffers[4], nbrTriangles[4] * 3);
	leftFoot = new HierarchicalObject(programID, vertexBuffers[5], arrayBuffers[5], nbrTriangles[5] * 3);
	rightFoot = new HierarchicalObject(programID, vertexBuffers[6], arrayBuffers[6], nbrTriangles[6] * 3);
	upperBody = new HierarchicalObject(programID, vertexBuffers[7], arrayBuffers[7], nbrTriangles[7] * 3);
	connectSkeleton();
}

/*
 *  Connect the skeleton together.  The upper body will need to
 * be added into here once I upload it.
 */
void connectSkeleton()
{
	pelvis->add(upperLeftLeg);
	pelvis->add(upperRightLeg);
	pelvis->add(upperBody);
	upperLeftLeg->add(lowerLeftLeg);
	upperRightLeg->add(lowerRightLeg);
	lowerLeftLeg->add(leftFoot);
	lowerRightLeg->add(rightFoot);

	upperRightLeg->translate(1.0f, 0.0f, 0.0f);
	upperLeftLeg->translate(-1.0f, 0.0f, 0.0f);
	lowerRightLeg->translate(0.0f, -2.0f, 0.0f);
	lowerLeftLeg->translate(0.0f, -2.0f, 0.0f);
	leftFoot->translate(0.0f, -2.0f, 0.0f);
	rightFoot->translate(0.0f, -2.0f, 0.0f);
}
/*
 * current routine to determine pelvis translations and motion.  I know it
 * says hip but it is the pelvis.
 */
void hipMotion(float t, float& translationX, float& translationY, float& translationZ, float& rotationX, float& rotationY, float& rotationZ) {
	static float tilt[] = { -12.5, -12.0, -11.0, -13.0, -14.0, -13.0, -11.0, -11.0, -12.5, -14.0, -12.5 };
	static float rotation[] = { 7.0, 5.0, 3.5, 3.5, -1.0, -7.0, -5.0, -4.0, -4.0, 1.0, 7.0 };
	static float obliquity[] = { 1.0, 5.0, 6.0, 1.0, 0.0, -2.0, -5.0, -6.0, -2.0, 0.0, 1.0 };
	int index = t * 10.0f;
	float value = t * 10.0f - index;
	rotationX = tilt[index] + value * (tilt[index + 1] - tilt[index]);

	rotationY = rotation[index] + value * (rotation[index + 1] - rotation[index]);
	rotationZ = obliquity[index] + value * (obliquity[index + 1] - obliquity[index]);
	translationX = translationY = translationZ = 0.0f;
	return;
}
/*
 * current routine to determine upper leg translations and motion.
 */
void leftUpperLegMotion(float t, float& translationX, float& translationY, float& translationZ,
	float& rotationX, float& rotationY, float& rotationZ) {
	static float flexion[] = { 36.0, 35.0, 22.0, 10.0, -3.0, -10.0, -5.0, 10.0, 30.0, 38.0, 36.0 };
	static float adduction[] = { -3.0, 2.0, 5.5, 3.0, 2.0, -2.0, -8.0, -7.0, -3.0, -1.0, -3.0 };
	static float hipint[] = { -1.0, 1.0, -1.0, -4.0, -5.0, -6.0, -7.0, -5.0, -3.0, -1.0, -1.0 };
	int index = t * 10.0f;
	float value = t * 10.0f - index;
	auto interpolate = [](int index, float value, auto table) { return table[index] + value * (table[index + 1] - table[index]); };
	rotationX = interpolate(index, value, flexion);
	rotationY = interpolate(index, value, hipint);
	rotationZ = interpolate(index, value, adduction);
	translationX = translationY = translationZ = 0.0f;
	return;
}

/*
 * current routine to determine upper leg translations and motion.
 */
void rightUpperLegMotion(float t, float& translationX, float& translationY, float& translationZ,
	float& rotationX, float& rotationY, float& rotationZ) {
	static float flexion[] = { 36.0, 35.0, 22.0, 10.0, -3.0, -10.0, -5.0, 10.0, 30.0, 38.0, 36.0 };
	static float adduction[] = { -3.0, 2.0, 5.5, 3.0, 2.0, -2.0, -8.0, -7.0, -3.0, -1.0, -3.0 };
	static float hipint[] = { -1.0, 1.0, -1.0, -4.0, -5.0, -6.0, -7.0, -5.0, -3.0, -1.0, -1.0 };
	t = t + 0.5f;
	if (t >= 1.0f) {
		t = t - 1.0f;
	}
	int index = t * 10.0f;
	float value = t * 10.0f - index;
	auto interpolate = [](int index, float value, auto table) { return table[index] + value * (table[index + 1] - table[index]); };
	rotationX = interpolate(index, value, flexion);
	rotationY = interpolate(index, value, hipint);
	rotationZ = interpolate(index, value, adduction);
	translationX = translationY = translationZ = 0.0f;
	return;
}

void rightKneeMotion(float t, float& translationX, float& translationY, float& translationZ, float& rotationX, float& rotationY, float& rotationZ) {
	static float flexion[] = { 0.0f, -20.0f, -20.0f, -10.0f, 0.0f, -10.0f, -35.0f, -60.0f, -55.0f, -20.0f,0.0f };
	static float adduction[] = { 2.0f, 5.0f, 2.5f, 2.0f, 2.0f, 0.0f, -2.0f, -5.0f, 0.0f, 2.5f, 2.0f };
	static float intext[] = { -23.0f, -20.0f, -17.0f, -12.50f, -10.0f, -7.0f, -16.0f, -16.0f, -20.0f, -24.0,-23.0f };
	t = t + 0.5f;
	if (t >= 1.0f) {
		t = t - 1.0f;
	}
	int index = t * 10.0f;
	float value = t * 10.0f - index;
	auto interpolate = [](int index, float value, auto table) { return table[index] + value * (table[index + 1] - table[index]); };
	rotationX = interpolate(index, value, flexion);
	rotationY = interpolate(index, value, intext);
	rotationZ = interpolate(index, value, adduction);
	translationX = translationY = translationZ = 0.0f;
}

void leftKneeMotion(float t, float& translationX, float& translationY, float& translationZ, float& rotationX, float& rotationY, float& rotationZ) {
	static float flexion[] = { 0.0f, -20.0f, -20.0f, -10.0f, 0.0f, -10.0f, -35.0f, -60.0f, -55.0f, -20.0f,0.0f };
	static float adduction[] = { 2.0f, 5.0f, 2.5f, 2.0f, 2.0f, 0.0f, -2.0f, -5.0f, 0.0f, 2.5f, 2.0f };
	static float intext[] = { -23.0f, -20.0f, -17.0f, -12.50f, -10.0f, -7.0f, -16.0f, -16.0f, -20.0f, -24.0,-23.0f };

	int index = t * 10.0f;
	float value = t * 10.0f - index;
	auto interpolate = [](int index, float value, auto table) { return table[index] + value * (table[index + 1] - table[index]); };
	rotationX = interpolate(index, value, flexion);
	rotationY = interpolate(index, value, intext);
	rotationZ = interpolate(index, value, adduction);
	translationX = translationY = translationZ = 0.0f;
}
void rightAnkleMotion(float t, float& translationX, float& translationY, float& translationZ, float& rotationX, float& rotationY, float& rotationZ) {
	static float flexion[] = { 3.0f, 0.0f, 7.0f, 8.5f, 10.0f, 5.0f, -12.0f, -4.0f, 5.0f, 4.0f, 3.0f };
	static float adduction[] = { 7.0f, 2.0f, 0.0f, 2.0f, 4.0f, 8.0f, 12.0f, 5.0f, 4.0f, 3.0f, 7.0f };
	static float intext[] = { 10.0f, 4.0f, 6.0f,6.5f, 7.0f, 10.0f, 20.0f, 10.0f, 12.0f, 9.0f, 10.0f };
	t = t + 0.5f;
	if (t >= 1.0f) {
		t = t - 1.0f;
	}
	int index = t * 10.0f;
	float value = t * 10.0f - index;
	auto interpolate = [](int index, float value, auto table) { return table[index] + value * (table[index + 1] - table[index]); };
	rotationX = interpolate(index, value, flexion);
	rotationY = interpolate(index, value, intext);
	rotationZ = interpolate(index, value, adduction);
	translationX = translationY = translationZ = 0.0f;
}
void leftAnkleMotion(float t, float& translationX, float& translationY, float& translationZ, float& rotationX, float& rotationY, float& rotationZ) {
	static float flexion[] = { 3.0f, 0.0f, 7.0f, 8.5f, 10.0f, 5.0f, -12.0f, -4.0f, 5.0f, 4.0f, 3.0f };
	static float adduction[] = { 7.0f, 2.0f, 0.0f, 2.0f, 4.0f, 8.0f, 12.0f, 5.0f, 4.0f, 3.0f, 7.0f };
	static float intext[] = { 10.0f, 4.0f, 6.0f,6.5f, 7.0f, 10.0f, 20.0f, 10.0f, 12.0f, 9.0f, 10.0f };

	int index = t * 10.0f;
	float value = t * 10.0f - index;
	auto interpolate = [](int index, float value, auto table) { return table[index] + value * (table[index + 1] - table[index]); };
	rotationX = interpolate(index, value, flexion);
	rotationY = interpolate(index, value, intext);
	rotationZ = interpolate(index, value, adduction);
	translationX = translationY = translationZ = 0.0f;
}
/*
 * This is code shared in common to build vertex array objects
 * and buffer array objects for the program.
 */
void BuildPart(int nbrTriangles, int nbrVertices, GLfloat  vertices[],
	int nbrNormals, GLfloat  normals[], GLfloat color[])
{
	long verticesSize = nbrVertices * sizeof(GLfloat);
	long normalsSize = nbrNormals * sizeof(GLfloat);
	// The base has no colors associated with the vertices.  I'm going to make it
	// gray for right now.
	GLfloat* colors = new GLfloat[nbrVertices * 4];
	for (int i = 0; i < nbrVertices; i = i + 4) {
		colors[i] = color[0];
		colors[i + 1] = color[1];
		colors[i + 2] = color[2];
		colors[i + 3] = color[3];
	}

	long colorsSize = nbrVertices * sizeof(GLfloat);
	glBufferData(GL_ARRAY_BUFFER,
		verticesSize + colorsSize + normalsSize,
		NULL, GL_STATIC_DRAW);
	//                               offset in bytes   size in bytes     ptr to data    

	glBufferSubData(GL_ARRAY_BUFFER, 0, verticesSize, vertices);
	glBufferSubData(GL_ARRAY_BUFFER, verticesSize, colorsSize, colors);
	glBufferSubData(GL_ARRAY_BUFFER, verticesSize + colorsSize, normalsSize, normals);
	/*
	* Set up variables into the shader programs (Note:  We need the
	* shaders loaded and built into a program before we do this)
	*/
	GLuint vPosition = glGetAttribLocation(programID, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	GLuint vColors = glGetAttribLocation(programID, "vColor");
	glEnableVertexAttribArray(vColors);
	glVertexAttribPointer(vColors, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(verticesSize));
	GLuint vNormal = glGetAttribLocation(programID, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(verticesSize + colorsSize));
}

/*
 * This fills in the locations of most of the uniform variables for the program.
 * there are better ways of handling this but this is good in going directly from
 * what we had.
 *
 * Revised to get the locations and names of the uniforms from OpenGL.  These
 * are then stored in a map so that we can look up a uniform by name when we
 * need to use it.  The map is still global but it is a little neater than the
 * version that used all the locations.  The locations are still there right now
 * in case that is more useful for you.
 *
 */

void getLocations() {
	/*
	 * Find out how many uniforms there are and go out there and get them from the
	 * shader program.  The locations for each uniform are stored in a global -- locationMap --
	 * for later retrieval.
	 */
	GLint numberBlocks;
	char uniformName[1024];
	int nameLength;
	GLint size;
	GLenum type;
	glGetProgramiv(programID, GL_ACTIVE_UNIFORMS, &numberBlocks);
	for (int blockIndex = 0; blockIndex < numberBlocks; blockIndex++) {
		glGetActiveUniform(programID, blockIndex, 1024, &nameLength, &size, &type, uniformName);
		cout << uniformName << endl;
		locationMap[string(uniformName)] = blockIndex;
	}
}

/*
 *  Initialization Code -- This sets up attributes, builds the program,
 * sets up initial projection, viewing, and modeling matrices, builds the
 * objects (vertex array objects and buffer array objects) and
 * gathers up uniform locations.
 */
void init(string vertexShader, string fragmentShader) {

	setAttributes(1.0f, GL_FRONT_AND_BACK, GL_FILL);

	programID = buildProgram(vertexShader, fragmentShader);
#ifdef __VMATH_H__
	rotation = vmath::scale(1.0f);
	viewMatrix = vmath::lookat(vmath::vec3(0.0f, 0.0f, 10.0f), vmath::vec3(0.0f, 0.0f, 0.0f), vmath::vec3(0.0f, 1.0f, 0.0f));
	projectionMatrix = vmath::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -100.0f, 100.0f);
#else
	mat4x4_identity(rotation);
	mat4x4_identity(viewMatrix);
	mat4x4_look_at(viewMatrix, vec3{ 0.0f, 0.0f, 10.0f }, vec3{ 0.0, 0.0, 0.0 }, vec3{ 0.0, 1.0, 0.0 });
	mat4x4_ortho(projectionMatrix, -5.0f, 5.0f, -5.0f, 5.0f, -100.0f, 100.0f);
#endif
	buildSkeleton();
	getLocations();

}


/*
 * The display routine -- Has been changed to allow motion and to have
 * an hierarchical object in it...
 */
void displayDirectional() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// needed
	glUseProgram(programID);
	glBindVertexArray(vertexBuffers[0]);
	glBindBuffer(GL_ARRAY_BUFFER, arrayBuffers[0]);
	GLuint modelMatrixLocation = glGetUniformLocation(programID, "modelingMatrix");
	glUniformMatrix4fv(modelMatrixLocation, 1, false, (const GLfloat*)rotation);
	GLuint viewMatrixLocation = glGetUniformLocation(programID, "viewingMatrix");
	glUniformMatrix4fv(viewMatrixLocation, 1, false, (const GLfloat*)viewMatrix);
	GLuint projectionMatrixLocation = glGetUniformLocation(programID, "projectionMatrix");
	glUniformMatrix4fv(projectionMatrixLocation, 1, false, (const GLfloat*)projectionMatrix);
#ifdef __VMATH_H__
	vmath::mat4 MVMatrix, MVPMatrix, NormalMatrix;
	MVMatrix = viewMatrix * rotation;
	MVPMatrix = projectionMatrix * MVMatrix;
#else
	mat4x4 MVMatrix, MVPMatrix;
	mat4x4_mul(MVMatrix, viewMatrix, rotation);
	mat4x4_mul(MVPMatrix, projectionMatrix, MVMatrix);
#endif
	glUniformMatrix4fv(locationMap["MVMatrix"], 1, false, (const GLfloat*)MVMatrix);
	glUniformMatrix4fv(locationMap["MVPMatrix"], 1, false, (const GLfloat*)MVPMatrix);
	GLfloat ambientLight[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	GLuint ambientLightLocation = glGetUniformLocation(programID, "ambientLight");
	glUniform4fv(ambientLightLocation, 1, ambientLight);
	GLuint directionalLightDirectionLoc = glGetUniformLocation(programID, "directionalLightDirection");
	GLuint directionalLightColorLoc = glGetUniformLocation(programID, "directionalLightColor");
	GLuint halfVectorLocation = glGetUniformLocation(programID, "halfVector");
	GLuint shininessLoc = glGetUniformLocation(programID, "shininess");
	GLuint strengthLoc = glGetUniformLocation(programID, "strength");
	GLfloat directionalLightDirection[] = { 0.0f, 0.7071f, 0.7071f };
	GLfloat directionalLightColor[] = { 1.0f, 1.0f, 1.0f };
	GLfloat shininess = 25.0f;
	GLfloat strength = 1.0f;
	GLfloat halfVector[] = { 0.0f, 0.45514f, 0.924f };
	glUniform1f(shininessLoc, shininess);
	glUniform1f(strengthLoc, strength);
	glUniform3fv(directionalLightDirectionLoc, 1, directionalLightDirection);
	glUniform3fv(directionalLightColorLoc, 1, directionalLightColor);
	glUniform3fv(halfVectorLocation, 1, halfVector);

	if (motionOn) {
		updateJointPositions();
	}
	pelvis->display(projectionMatrix, viewMatrix, rotation);
	t = t + 0.01;
	if (t >= 1.0f) {
		t = t - 1.0f;
	}
}

/*
 * This routine localized the code to update the skeleton joint
 * positions -- I have left in the pelvis motion for now but removed
 * the remaining code.
 *
 */
void updateJointPositions()
{
	float tx, ty, tz, rx, ry, rz;
	auto updateTransforms = [](HierarchicalObject* p, float tx, float ty, float tz, float rx, float ry, float rz) {
		p->translate(tx, ty, tz);
		p->rotate(rx, 1.0f, 0.0f, 0.0f);
		p->rotate(ry, 0.0f, 1.0f, 0.0f);
		p->rotate(rz, 0.0f, 0.0f, 1.0f);
	};

	hipMotion(t, tx, ty, tz, rx, ry, rz);
	pelvis->clearCurrentTransform();
	pelvis->translate(tx, ty, tz);
	pelvis->rotate(rx, 1.0f, 0.0f, 0.0f);
	pelvis->rotate(ry, 0.0f, 1.0f, 0.0f);
	pelvis->rotate(rz, 0.0f, 0.0f, 1.0f);
	upperLeftLeg->clearCurrentTransform();
	upperLeftLeg->translate(-1.0f, 0.0f, 0.0f);
	leftUpperLegMotion(t, tx, ty, tz, rx, ry, rz);
	updateTransforms(upperLeftLeg, tx, ty, tz, rx, ry, rz);
	// added for project
	upperRightLeg->clearCurrentTransform();
	upperRightLeg->translate(1.0f, 0.0f, 0.0f);
	rightUpperLegMotion(t, tx, ty, tz, rx, ry, rz);
	updateTransforms(upperRightLeg, tx, ty, tz, rx, ry, rz);
}

/*
* Handle window resizes -- adjust size of the viewport -- more on this later
*/

void reshapeWindow(GLFWwindow* window, int width, int height)
{
	float ratio;
	ratio = width / (float)height;

	glViewport(0, 0, width, height);

}
/*
* Main program with calls for many of the helper routines.
*/
int main(int argCount, char* argValues[]) {
	GLFWwindow* window = nullptr;
	window = glfwStartUp(argCount, argValues, "Programming Project 4");
	init("pointsource.vert", "directional.frag");
	//	init("pointsource.vert", "pointsource.frag");
	glfwSetWindowSizeCallback(window, reshapeWindow);

	while (!glfwWindowShouldClose(window))
	{
		displayDirectional();
		glfwSwapBuffers(window);
		glfwPollEvents();
	};

	glfwDestroyWindow(window);

	glfwTerminate();
	exit(EXIT_SUCCESS);
}
