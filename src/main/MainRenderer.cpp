#include "MainRenderer.h"
#include "MainConfig.h"

MainRenderer::MainRenderer()
{
    leftDocking.layerIndex = -1;
    rightDocking.layerIndex = -1;
    bottomDocking.layerIndex = -1;
    topDocking.layerIndex = -1;
}

MainRenderer::~MainRenderer()
{
}

GLFWwindow* MainRenderer::getWindow()
{
    return window;
}

bool MainRenderer::init(int width, int height, const char* title)
{
    if (!glfwInit())
        return false;
    
    MainConfig cfg;
    
    // Use 32-bit color (in sRGB) and 24-bit for depth buffer
    if (cfg.getValue<bool>(MainConfig::USE_SRGB))
        glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    if (cfg.getValue<bool>(MainConfig::MULTISAMPLING))
        glfwWindowHint(GLFW_SAMPLES, cfg.getValue<unsigned>(MainConfig::SAMPLES));
    
    // Use OpenGL 3.2 core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Create a window
    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!window) {
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    
    // Set vertical retrace rate (0 == run as fast as possible)
    glfwSwapInterval(1);
    
    // Initialize GLEW, which provides access to OpenGL functions / extensions
	glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        glfwTerminate();
        return false;
    }
    
    if (cfg.getValue<bool>(MainConfig::USE_SRGB))
        glEnable(GL_FRAMEBUFFER_SRGB);
    
    this->width = width;
    this->height = height;
    
    return true;
}

void MainRenderer::draw()
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // draw from stack
    int layer = 0;
    for (Renderer* r : activeLayers) {
        updateViewport(r, layer);
        r->draw();
        layer++;
    }
    
    glfwSwapBuffers(window);
}

void MainRenderer::updateViewport(Renderer* renderer, int layer)
{
    int x = 0;
    int y = 0;
    int w = width;
    int h = height;
    
    if (layer == bottomDocking.layerIndex) {
		h = static_cast<int>(h * bottomDocking.percent);
    } else if (layer == topDocking.layerIndex) {
        y += static_cast<int>(height * (1.0 - topDocking.percent));
		h = static_cast<int>(h * topDocking.percent);
    } else if (layer == leftDocking.layerIndex) {
		w = static_cast<int>(w * leftDocking.percent);
    } else if (layer == rightDocking.layerIndex) {
		x += static_cast<int>(width * (1.0 - rightDocking.percent));
		w = static_cast<int>(w * rightDocking.percent);
    }
    
    if (layer < bottomDocking.layerIndex) {
		y += static_cast<int>(height * bottomDocking.percent);
		h = static_cast<int>(h * (1.0 - bottomDocking.percent));
    }
    
    if (layer < topDocking.layerIndex) {
		h = static_cast<int>(h * (1.0 - topDocking.percent));
    }
    
    if (layer < leftDocking.layerIndex) {
		x += static_cast<int>(width * leftDocking.percent);
		w = static_cast<int>(w * (1.0 - leftDocking.percent));
    }
    
    if (layer < rightDocking.layerIndex) {
		w = static_cast<int>(w * (1.0 - leftDocking.percent));
    }
    
    renderer->setViewport(x, y, w, h);
    renderer->getViewport().apply();
}

void MainRenderer::pushLayer(Renderer* layer)
{
    activeLayers.push_back(layer);
}

void MainRenderer::dockLeft(Renderer* layer, double percent)
{
    leftDocking.percent = percent;
    leftDocking.layerIndex = static_cast<int>(activeLayers.size());
    pushLayer(layer);
}

void MainRenderer::dockRight(Renderer* layer, double percent)
{
    rightDocking.percent = percent;
    rightDocking.layerIndex = static_cast<int>(activeLayers.size());
    pushLayer(layer);
}

void MainRenderer::dockBottom(Renderer* layer, double percent)
{
    bottomDocking.percent = percent;
	bottomDocking.layerIndex = static_cast<int>(activeLayers.size());
    pushLayer(layer);
}

void MainRenderer::dockTop(Renderer* layer, double percent)
{
    topDocking.percent = percent;
	topDocking.layerIndex = static_cast<int>(activeLayers.size());
    pushLayer(layer);
}

Renderer* MainRenderer::popLayer()
{
    if (activeLayers.empty())
        return NULL;
    
    Renderer* popped = activeLayers.back();
    activeLayers.pop_back();
    
    // check if this was a docked layer and remove if so
    if (leftDocking.layerIndex == activeLayers.size())
        leftDocking.layerIndex = -1;
    if (rightDocking.layerIndex == activeLayers.size())
        rightDocking.layerIndex = -1;
    if (bottomDocking.layerIndex == activeLayers.size())
        bottomDocking.layerIndex = -1;
    if (topDocking.layerIndex == activeLayers.size())
        topDocking.layerIndex = -1;
    
    return popped;
}

void MainRenderer::clearLayers()
{
    activeLayers.clear();
    leftDocking.layerIndex = -1;
    rightDocking.layerIndex = -1;
    bottomDocking.layerIndex = -1;
    topDocking.layerIndex = -1;
}

int MainRenderer::getWidth()
{
    return width;
}

int MainRenderer::getHeight()
{
    return height;
}

void MainRenderer::resize(int width, int height)
{
    this->width = width;
    this->height = height;
}
