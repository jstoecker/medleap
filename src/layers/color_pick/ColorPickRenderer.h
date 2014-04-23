#ifndef __medleap__ColorPickRenderer__
#define __medleap__ColorPickRenderer__

#include "layers/Renderer.h"
#include "gl/geom/Rectangle.h"
#include "gl/Buffer.h"
#include "gl/Program.h"
#include "gl/math/Math.h"
#include "util/Color.h"
#include "gl/util/Draw.h"
#include "util/TextRenderer.h"

class ColorPickRenderer : public Renderer
{
public:
	ColorPickRenderer();
	void draw() override;
	void resize(int width, int height) override;

	void leapCursor(const gl::Vec2& pos) { m_leap_cursor = pos; }
	void choose(const ColorHSV& color) { mColor = color; }
	void tracking(bool tracking) { mTracking = tracking; }
	const gl::Rectangle<float>& circleRect() const { return mCircleRect; }
	const gl::Rectangle<float>& alphaRect() const { return mAlphaRect; }
	const gl::Rectangle<float>& valueRect() const { return mValueRect; }
	const gl::Rectangle<float>& previewRect() const { return mPreviewRect; }

private:
	gl::Buffer geomVBO;
	gl::Program circleShader;
	gl::Program selectShader;
	gl::Program gradientShader;
	gl::Rectangle<float> mCircleRect;
	gl::Rectangle<float> mAlphaRect;
	gl::Rectangle<float> mValueRect;
	gl::Rectangle<float> mPreviewRect;
	gl::Draw mCursor;
	ColorHSV mColor;
	gl::Mat4 mProjection;
	gl::Vec2 m_leap_cursor;
	TextRenderer text;
	bool mTracking;

	void quad(gl::Program prog, const gl::Rectangle<float>& rect);
};

#endif // __medleap__ColorPickRenderer__