#ifndef __CGL_PROGRAM_H_
#define __CGL_PROGRAM_H_

#include "gl/glew.h"
#include "gl/Shader.h"
#include <memory>
#include <vector>

namespace gl
{
    /** A compiled and linked set of vertex and fragment shaders. */
    class Program
    {
    public:
        Program();

		~Program();
        
		/** Returns the handle to the OpenGL resource, or 0 if none. */
		GLuint id() const;

		/** Clears this pointer. Detaches all shaders. */
		void release();
        
		/** Creates a new OpenGL resource. This object will point to it. */
		void generate();

		/** Attached a shader to this program */
		void attach(const Shader& shader);

		/** Attempts to link attached shaders. Returns true if successful */
		bool link();

        GLint getUniform(const GLchar* name) const;
        
        GLint getAttribute(const GLchar* name) const;
        
        void enable();
        
        void disable();
        
        static Program create(const char* vsrc, const char* fsrc);
        static Program createFromSrc(const char* vsrc, const char* fsrc);
        static Program create(const Shader& vShader, const Shader& fShader);

    private:
		std::shared_ptr<GLuint> handle;
		std::vector<gl::Shader> attached;
    };
}

#endif // __CGL_PROGRAM_H_
