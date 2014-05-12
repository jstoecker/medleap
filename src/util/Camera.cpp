#include "camera.h"

using namespace gl;

Camera::Camera() : center(0, 0, -1)
{}

const Vec4& Camera::getEye() const
{
    return eye;
}

const Vec4& Camera::getUp() const
{
    return up;
}

const Vec4& Camera::getRight() const
{
    return right;
}

const Vec4& Camera::getForward() const
{
    return forward;
}

const Mat4& Camera::getView() const
{
    return view;
}

const Mat4& Camera::getProjection() const
{
    return projection;
}

void Camera::rotateX(float radians)
{
	view = view * gl::rotationX(radians);
	viewInverse = gl::rotationX(-radians) * viewInverse;
    update();
}

void Camera::rotateY(float radians)
{
	view = view * gl::rotationY(radians);
	viewInverse = gl::rotationY(-radians) * viewInverse;
    update();
}

void Camera::rotateZ(float radians)
{
	view = view * gl::rotationZ(radians);
	viewInverse = gl::rotationZ(-radians) * viewInverse;
    update();
}

void Camera::rotate(float radians, Vec3 axis)
{
	view = view * gl::rotation(radians, axis);
	viewInverse = gl::rotation(-radians, axis) * viewInverse;
    update();
}

void Camera::translate(float x, float y, float z)
{
	view = view * gl::translation(x, y, z);
	viewInverse = gl::translation(-x, -y, -z) * viewInverse;
    update();
}

void Camera::translate(const Vec3& t)
{
    translate(t.x, t.y, t.z);
}

void Camera::translateForward(float units)
{
    translate(forward * units);
}

void Camera::translateBackward(float units)
{
    translate(forward * -units);
}

void Camera::translateRight(float units)
{
    translate(right * units);
}

void Camera::translateLeft(float units)
{
    translate(right * -units);
}

void Camera::translateUp(float units)
{
    translate(up * units);
}

void Camera::translateDown(float units)
{
    translate(up * -units);
}

void Camera::update()
{
    right = viewInverse.col(0);
    up = viewInverse.col(1);
    forward = viewInverse.col(2) * -1.0f;
    eye = viewInverse.col(3);
	fireEvent();
}

void Camera::setView(const Mat4& view)
{
    this->view = view;
    this->viewInverse = view.inverse();
    update();
}

void Camera::setProjection(const Mat4& projection)
{
    this->projection = projection;
	fireEvent();
}

void Camera::addListener(Listener* l)
{
	listeners.push_back(l);
}

void Camera::removeListener(Listener* l)
{
	auto it = std::find(listeners.begin(), listeners.end(), l);
	if (it != listeners.end()) {
		listeners.erase(it);
	}
}

void Camera::fireEvent()
{
	for (Listener* l : listeners)
		l->cameraUpdated(*this);
}