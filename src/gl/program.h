#ifndef __REACH_GL_PROGRAM__
#define __REACH_GL_PROGRAM__

#include <GL/glew.h>
#include <vector>
#include "shader.h"

class Shader;

/// A compiled and linked set of vertex and fragment shaders.
class Program
{
public:
    Program();

	~Program();
    
	GLuint getID() const;

    GLint getUniform(const GLchar* name) const;

	GLint getAttribute(const GLchar* name) const;
    
	void enable();

	void disable();

	static Program* create(const char* vsrc, const char* fsrc);
    
private:
    GLuint id;
    Shader* vShader;
	Shader* fShader;

	Program(GLuint id, Shader* vShader, Shader* fShader);
};


#endif // __REACH_GL_PROGRAM__
