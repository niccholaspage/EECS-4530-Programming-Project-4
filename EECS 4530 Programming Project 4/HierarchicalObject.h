#ifndef __HIERARCHICALOBJECT_H__
#define __HIERARCHICALOBJECT_H__
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "vmath.h"
#include "linmath.h"
#include <list>
#include <string>
using namespace std;

class HierarchicalObject
{
private:
protected:
	GLuint bao, vao, programID;
	list <HierarchicalObject*> subobjects;
	int nbrOfVertices = 0;
	string mvpName = "MVPMatrix";
	string mvName = "MVMatrix";
	string normalName = "NormalMatrix";
#ifdef __VMATH_H__
	list<vmath::mat4> currentTransforms;
	list<vmath::mat4> localTransforms;
#else
	list<mat4x4> currentTransforms;
	list<mat4x4> localTransforms;
#endif

public:
	HierarchicalObject(int progID = 0, GLuint vao = 0, GLuint bao = 0, int nbrOfVertices = 0) {
		programID = progID;
		this->vao = vao;
		this->bao = bao;
		this->nbrOfVertices = nbrOfVertices;
#ifdef __VMATH_H__
		vmath::mat4 identity;
		identity = vmath::mat4(vmath::vec4(1.0f, 0.0f, 0.0f, 0.0f),
			vmath::vec4(0.0f, 1.0f, 0.0f, 0.0f),
			vmath::vec4(0.0f, 0.0f, 1.0f, 0.0f),
			vmath::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		localTransforms.push_front(identity);
		currentTransforms.push_front(identity);
#else 
		mat4x4 identity;
		mat4x4_identity(identity);
		localTransforms.push_front(identity);
		currentTransforms.push_front(identity);
#endif
	}
	string getMVPname() { return mvpName; }
	string getMVname() { return mvName; }
	string getNormalName() { return normalName; }
	void setMVPname(string newName) { mvpName = newName; }
	void setMVname(string newName) { mvName = newName; }
	void setNormalName(string newName) { normalName = newName; }
	void translate(float dx, float dy, float dz);
	void scale(float sx, float sy, float sz);
	void rotate(float angle, float vx, float vy, float vz);
	void add(HierarchicalObject* obj);
	void remove(HierarchicalObject* obj);
	int nbrSubobjects() { return subobjects.size(); }
	void setNbrOfVertices(int n) { nbrOfVertices = n; }
	int getNbrOfVertices() { return nbrOfVertices; }
#ifdef __VMATH_H__
	void display(vmath::mat4 projectionMatrix, vmath::mat4 viewingMatrix, vmath::mat4 modelingMatrix);
	void setLocalTransform(vmath::mat4 localTransform);
	void addLocalTransform(vmath::mat4 addedTransform);
	void clearCurrentTransform();
	vmath::mat4 generateCompositeTransform(list<vmath::mat4> transforms);
#else
	void display(mat4x4 projectionMatrix, mat4x4 viewingMatrix, mat4x4 modelingMatrix);
	void setLocalTransform(mat4x4 localTransform);
	void addLocalTransform(mat4x4 addedTransform);
	void generateCompositeTransform(list<mat4x4> transforms);
#endif
};

#endif
