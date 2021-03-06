#include "MainController.h"
#include "data/VolumeLoader.h"
#include <chrono>
#include "gl/math/Math.h"

using namespace gl;
using namespace std;

void keyboardCB(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    MainController::getInstance().keyboardInput(window, key, action, mods);
}

void resizeCB(GLFWwindow* window, int width, int height)
{
    MainController::getInstance().resize(width, height);
}

void mouseCB(GLFWwindow* window, int button, int action, int mods)
{
    MainController::getInstance().mouseButton(window, button, action, mods);
}

void cursorCB(GLFWwindow* window, double x, double y)
{
    MainController::getInstance().mouseMotion(window, x, y);
}

void scrollCB(GLFWwindow* window, double dx, double dy)
{
    MainController::getInstance().scroll(window, dx, dy);
}

MainController& MainController::getInstance()
{
    static MainController controller;
    return controller;
}

MainController::MainController() :
    volume(NULL)
{
    mode = MODE_3D;
    showHistogram = false;
}

MainController::~MainController()
{
}

void MainController::init(GLFWwindow* window)
{
	this->window = window;

    glfwSetFramebufferSizeCallback(window, resizeCB);
	glfwSetKeyCallback(window, keyboardCB);
	glfwSetMouseButtonCallback(window, mouseCB);
	glfwSetCursorPosCallback(window, cursorCB);
	glfwSetScrollCallback(window, scrollCB);
       
	volumeInfoController.setVolumeRenderer(&volumeController_);
	volumeInfoController.setSliceRenderer(&sliceController_);
	histogramController.setVolumeRenderer(&volumeController_);
	histogramController.setSliceRenderer(&sliceController_);
	orientationController.camera(&volumeController_.getCamera());

	glfwGetWindowSize(window, &width, &height);

	leapController.config().setFloat("Gesture.Circle.MinRadius", 50.0f);
	leapController.config().save();


	setMode(MODE_3D);
}

void MainController::setMode(MainController::Mode mode)
{
    this->mode = mode;
    switch (mode) {
        case MODE_2D:
            renderer.clearLayers();
            activeControllers.clear();
			pushController(&sliceController_);
            pushController(&volumeInfoController);
            if (showHistogram)
                pushController(&histogramController, Docking(Docking::BOTTOM, 0.14));
			pushController(&orientationController);
			pushController(&load_controller_);
			pushController(&leap_state_controller_, Docking(Docking::LEFT, .07, 96));
			pushController(&menuController_);
			break;
        case MODE_3D:
            renderer.clearLayers();
            activeControllers.clear();
			pushController(&volumeController_);
            pushController(&volumeInfoController);
            if (showHistogram)
                pushController(&histogramController, Docking(Docking::BOTTOM, 0.14));
			pushController(&orientationController);
			pushController(&clip_controller_);
			pushController(&focus_controller_);
			pushController(&mask_controller_);
			pushController(&load_controller_);
			pushController(&leap_state_controller_, Docking(Docking::LEFT, .07, 96));
			pushController(&menuController_);
            break;
    }
}

MainRenderer& MainController::getRenderer()
{
	return renderer;
}

MainController::Mode MainController::getMode()
{
    return mode;
}

void MainController::setVolume(VolumeData* volume)
{
    if (this->volume == volume)
        return;
    
    if (this->volume != NULL)
        delete this->volume;        
    
    this->volume = volume;
	sliceController_.setVolume(volume);
	volumeController_.setVolume(volume);
    volumeInfoController.setVolume(volume);
    histogramController.setVolume(volume);
	orientationController.volume(volume);

	if (focus_stack_.empty()) {
		focusLayer(&volumeController_);
	}
}

void MainController::startLoop()
{
    while (!glfwWindowShouldClose(window)) {
		update();
		renderer.draw(width, height);
		glfwSwapBuffers(window);
    }
    glfwTerminate();
}

Controller* MainController::focusLayer()
{
	if (focus_stack_.empty()) {
		return nullptr;
	}
	return focus_stack_.top();
}

void MainController::focusLayer(Controller* controller)
{
	if (!focus_stack_.empty()) {
		focus_stack_.top()->loseFocus();
		focus_stack_.pop();
	}
	focus_stack_.push(controller);
	controller->gainFocus();
}

void MainController::pushFocus(Controller* controller)
{
	if (!focus_stack_.empty()) {
		focus_stack_.top()->loseFocus();
	}
	focus_stack_.push(controller);
	controller->gainFocus();
}

void MainController::popFocus()
{
	if (!focus_stack_.empty()) {
		focus_stack_.top()->loseFocus();
		focus_stack_.pop();
		if (!focus_stack_.empty()) {
			focus_stack_.top()->gainFocus();
		}
	}
}

void MainController::pickColor(const Color& initialColor, std::function<void(const Color&)> callback)
{
	colorPickController.color(initialColor);
	colorPickController.clearCallbacks();
	colorPickController.addCallback(callback);
	colorPickController.addCallback([&](const Color&){popController(); popFocus(); });
	pushController(&colorPickController);
	pushFocus(&colorPickController);
}

void MainController::update()
{
    static auto prevTime = chrono::high_resolution_clock::now();
	
	auto curTime = chrono::high_resolution_clock::now();
	chrono::milliseconds elapsed = chrono::duration_cast<chrono::milliseconds>(curTime - prevTime);
	prevTime = curTime;

	glfwPollEvents();

	for (int i = 0; i < activeControllers.size(); i++) {
		activeControllers[i]->update(elapsed);
	}

	// leap input
	if (leapController.isConnected()) {
		Controller* focus = focusLayer();

		bool menuPassThrough = true;
		if (!focus || !focus->modal()) {
			menuPassThrough = menuController_.leapInput(leapController, leapController.frame());
		}

		if (focus && menuPassThrough) {
			focus->leapInput(leapController, leapController.frame());
		}
	}
}

void MainController::showTransfer1D(bool show)
{
	showHistogram = show;
	setMode(mode);
}

void MainController::toggleHistogram()
{
    showHistogram = !showHistogram;
    setMode(mode);
}

void MainController::keyboardInput(GLFWwindow *window, int key, int action, int mods)
{
	if (key == GLFW_KEY_M && action == GLFW_PRESS) {
		setMode((mode == MODE_2D) ? MODE_3D : MODE_2D);
	}

	if (key == GLFW_KEY_G && action == GLFW_PRESS) {
		renderer.setBackgroundColor(Vec3(1.0f) - renderer.getBackgroundColor());
		volumeController_.markDirty();
	}

	if (key == GLFW_KEY_H && action == GLFW_PRESS)
		toggleHistogram();
	
    
	for (Controller* c : activeControllers) {
		bool passThrough = c->keyboardInput(window, key, action, mods);
		if (!passThrough) {
			break;
		}
	}
}

void MainController::resize(int width, int height)
{
	this->width = width;
	this->height = height;
}

void MainController::mouseButton(GLFWwindow *window, int button, int action, int mods)
{
	// pushing color picker while iterating causes foreach to break
	for (Controller* c : activeControllers) {
		bool passThrough = c->mouseButton(window, button, action, mods, mMouseX, mMouseY);
		if (!passThrough)
			break;
	}
}

void MainController::mouseMotion(GLFWwindow *window, double x, double y)
{
    // convert y to bottom up
    y = height - y - 1;
	mMouseX = x;
	mMouseY = y;
    for (Controller* c : activeControllers) {
        bool passThrough = c->mouseMotion(window, x, y);
		if (!passThrough)
			break;
    }
}

void MainController::scroll(GLFWwindow *window, double dx, double dy)
{
	for (Controller* c : activeControllers) {
		bool passThrough = c->scroll(window, dx, dy);
		if (!passThrough)
			break;
	}
}

void MainController::popController()
{
	Controller* c = activeControllers.front();
	renderer.popLayer();
	activeControllers.erase(activeControllers.begin());

	chooseTrackedGestures();
}

void MainController::pushController(Controller* controller)
{
    Docking docking(MainController::Docking::NONE, 0);
    pushController(controller, docking);
}

void MainController::pushController(Controller* controller, MainController::Docking docking)
{
    activeControllers.insert(activeControllers.begin(), controller);
        
    switch (docking.position)
    {
        case MainController::Docking::LEFT:
            renderer.dockLeft(controller, docking.percent, docking.pixels);
            break;
        case MainController::Docking::RIGHT:
			renderer.dockRight(controller, docking.percent, docking.pixels);
            break;
        case MainController::Docking::BOTTOM:
			renderer.dockBottom(controller, docking.percent, docking.pixels);
            break;
        case MainController::Docking::TOP:
			renderer.dockTop(controller, docking.percent, docking.pixels);
            break;
        default:
            renderer.pushLayer(controller);
    }
    
	chooseTrackedGestures();
}

void MainController::chooseTrackedGestures()
{
	// MainController uses the CIRCLE gesutre to toggle the menu
	leapController.enableGesture(Leap::Gesture::TYPE_CIRCLE, true);

	leapController.enableGesture(Leap::Gesture::TYPE_KEY_TAP, false);
	leapController.enableGesture(Leap::Gesture::TYPE_SCREEN_TAP, false);
	leapController.enableGesture(Leap::Gesture::TYPE_SWIPE, false);

	// enable any requested gestures from all controllers
	set<Leap::Gesture::Type> allRequired;
	for (Controller* c : activeControllers) {
		for (const Leap::Gesture::Type t : c->requiredGestures())
			leapController.enableGesture(t, true);
	}
}