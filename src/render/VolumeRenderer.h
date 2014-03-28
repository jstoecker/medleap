#ifndef __medleap__VolumeRenderer__
#define __medleap__VolumeRenderer__

#include <GL/glew.h>
#include "math/Matrix4.h"
#include "gl/Viewport.h"
#include "gl/Program.h"
#include "gl/Texture.h"
#include "volume/VolumeData.h"
#include "util/Camera.h"
#include "render/Renderer.h"

class VolumeRenderer : public Renderer
{
public:
    VolumeRenderer();
    ~VolumeRenderer();
    void init();
    void resize(int width, int height);
    void draw();
    
    void setVolume(VolumeData* volume);
    
    cgl::Camera& getCamera();
    
    void markDirty();
    void setMoving(bool moving);
    
private:
    bool dirty;
    bool moving;
    cgl::Texture* volumeTexture;
    cgl::Texture* gradientTexture;
    VolumeData* volume;
    cgl::Camera camera;
    cgl::Viewport viewport;
    cgl::Mat4 model;
    
    // grid
    Program* lineShader;
    GLuint vao;
    GLuint vbo;
    int numGridVerts;
    
    // box
    Program* boxShader;
    int numSliceIndices;
    GLuint boxVBO;
    GLuint boxIBO;
    
    void updateSlices();
};

#endif /* defined(__medleap__VolumeRenderer__) */
