#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "HierarchicalObject.h"
using namespace std;

void HierarchicalObject::add(HierarchicalObject* obj) {
	subobjects.push_back(obj);
}
void HierarchicalObject::remove(HierarchicalObject* obj) {
	subobjects.remove(obj);
}
void HierarchicalObject::translate(float dx, float dy, float dz) {
#ifdef __VMATH_H__
	currentTransforms.push_back(vmath::translate(vmath::vec3(dx, dy, dz)));
#else
	mat4x4 tr;
	mat4x4_translate(tr, dx, dy, dz);
	currentTransforms.push_back(tr);
#endif
}

void HierarchicalObject::scale(float sx, float sy, float sz) {
#ifdef __VMATH_H__
	currentTransforms.push_back(vmath::scale(sx, sy, sz));
#else
	vec4 *scale = new vec4[4]; 
	mat4x4_identity(scale);
	mat4x4_scale_aniso(scale, scale, sx, sy, sz);
	currentTransforms.push_back(scale);
#endif
}

void HierarchicalObject::rotate(float angle, float vx, float vy, float vz) {
#ifdef __VMATH_H__
	currentTransforms.push_back(vmath::rotate(angle, vmath::vec3(vx, vy, vz)));
#else
	mat4x4 currentTransform;
	mat4x4_identity(currentTransform);
	mat4x4_rotate(currentTransform, currentTransform, vx, vy, vz, angle * 4.0 * atan(1.0f) / 180.0f);
	currentTransforms.push_back(currentTransform);
#endif
}


#ifndef __VMATH_H__
void HierarchicalObject::display(mat4x4 projectionMatrix, mat4x4 viewMatrix, mat4x4 modelingMatrix) {
	mat4x4 newTransform, newmvpTransform;
	mat4x4 savedMVMatrix, savedMVPMatrix, tempTransform;
	GLint  savedProgram;

	glGetIntegerv(GL_CURRENT_PROGRAM, &savedProgram);
	glUseProgram(programID);
	/*
	 *  This should be fed into the object rather than going out and getting it each time 
	 *  it will also help with the naming -- wouldn't need to be named MVMatrix.  Do I
	 *  need separate Model view and projection matrices?
	 */
	GLuint modelView_loc = glGetUniformLocation(programID, "MVMatrix");
	GLuint mvp_loc = glGetUniformLocation(programID, "MVPMatrix");
	glGetUniformfv(programID, modelView_loc, (GLfloat *)&savedMVMatrix);
	glGetUniformfv(programID, mvp_loc, (GLfloat*)&savedMVPMatrix);

	mat4x4_mul(tempTransform, savedMVMatrix, currentTransform);
	mat4x4_mul(newTransform, tempTransform, localTransform);
	mat4x4_mul(tempTransform, savedMVPMatrix, currentTransform);
	mat4x4_mul(newmvpTransform, tempTransform, localTransform);

	glUniformMatrix4fv(modelView_loc, 4, GL_FALSE, (GLfloat*) newTransform);
	glUniformMatrix4fv(mvp_loc, 4, GL_FALSE, (GLfloat*)newmvpTransform);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, bao);
	glDrawArrays(GL_TRIANGLES, 0, nbrOfVertices);
	// Draw Subobjects…
	for (HierarchicalObject * current : subobjects) {
		current->display(projectionMatrix, viewMatrix, modelingMatrix);
	}
	// Restore state
	glUniformMatrix4fv(modelView_loc, 4, GL_FALSE, (GLfloat *) savedMVMatrix);
	glUniformMatrix4fv(mvp_loc, 4, GL_FALSE, (GLfloat*)savedMVPMatrix);
	glUseProgram(savedProgram);
}
#endif
#ifdef __VMATH_H__
void HierarchicalObject::display(vmath::mat4 projectionMatrix, vmath::mat4 viewingMatrix, vmath::mat4 modelingMatrix)
{
	vmath::mat4 newTransform, newmvpTransform, newModelingTransform;
	vmath::mat4 savedMVMatrix, savedMVPMatrix;
	GLint  savedProgram;

	glGetIntegerv(GL_CURRENT_PROGRAM, &savedProgram);
	glUseProgram(programID);
	/*
	 *  This should be fed into the object rather than going out and getting it each time
	 *  it will also help with the naming -- wouldn't need to be named MVMatrix.  Do I
	 *  need separate Model view and projection matrices?
	 */
	GLuint modelView_loc = glGetUniformLocation(programID, "MVMatrix");
	GLuint mvp_loc = glGetUniformLocation(programID, "MVPMatrix");
	glGetUniformfv(programID, modelView_loc, (GLfloat*)&savedMVMatrix);
	glGetUniformfv(programID, mvp_loc, (GLfloat*)&savedMVPMatrix);
	newTransform = savedMVMatrix * generateCompositeTransform(currentTransforms) * generateCompositeTransform(localTransforms);
	newmvpTransform = savedMVPMatrix *  generateCompositeTransform(currentTransforms) * generateCompositeTransform(localTransforms);
	glUniformMatrix4fv(modelView_loc, 1, GL_FALSE, (GLfloat*)newTransform);
	glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, (GLfloat*)newmvpTransform);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, bao);
	glDrawArrays(GL_TRIANGLES, 0, nbrOfVertices);
	// Draw Subobjects…
	newTransform = savedMVMatrix * generateCompositeTransform(currentTransforms);
	glUniformMatrix4fv(modelView_loc, 1, GL_FALSE, (GLfloat*)newTransform);
	for (HierarchicalObject* current : subobjects) {
		current->display(projectionMatrix, viewingMatrix, modelingMatrix );
	}
	// Restore state
	glUniformMatrix4fv(modelView_loc, 1, GL_FALSE, (GLfloat*)savedMVMatrix);
	glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, (GLfloat*)savedMVPMatrix);
	glUseProgram(savedProgram);
}

void HierarchicalObject::setLocalTransform(vmath::mat4 localTransform)
{
	localTransforms.clear();
	localTransforms.push_front(localTransform);
}
void HierarchicalObject::addLocalTransform(vmath::mat4 addedTransform)
{
	localTransforms.push_front(addedTransform);
}
void HierarchicalObject::clearCurrentTransform()
{
	currentTransforms.clear();
	currentTransforms.push_back(vmath::scale(1.0f));
}
vmath::mat4 HierarchicalObject::generateCompositeTransform(list<vmath::mat4> transforms)
{
	vmath::mat4 total;
	total = vmath::scale(1.0f);
	for (auto tr : transforms) {
		total = total * tr;
	}
	return total;
}
#else
void HierarchicalObject::setLocalTransform(mat4x4 localTransform)
{
	localTransforms.clear();
	localTransforms.push_front(localTransform);
}
void HierarchicalObject::addLocalTransform(mat4x4 addedTransform)
{
	localTransforms.push_front(addedTransform);
}

HierarchicalObject::generateCompositeTransform(mat4x4 & composite, list<mat4x4> transforms)
{
	mat4x4 result;
	mat4x4_identity(result);
	for (auto tr : transforms) {
		mat4x4_mul(result, result, tr);
	}
	mat4x4_dup(composite, result);
	return;
}

#endif

