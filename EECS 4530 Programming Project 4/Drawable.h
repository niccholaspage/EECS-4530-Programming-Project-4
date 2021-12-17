#ifndef __DRAWABLE_H__
#define __DRAWABLE_H__
#include "vmath.h"
#include "linmath.h"
#include <iostream>
#include <string>
using namespace std;
class Drawable {
private:
protected:
public:
	virtual void draw() = 0;
#ifdef __VMATH_H__
	virtual void draw(vmath::mat4 transform) = 0;
#else // Assume linmath.h
	virtual void draw(mat4x4 transform) = 0;
#endif
};
#endif