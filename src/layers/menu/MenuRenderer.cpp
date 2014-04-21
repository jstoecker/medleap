#include "MenuRenderer.h"
#include "main/MainController.h"
#include "util/Util.h"
#include <vector>

using namespace std;
using namespace gl;

MenuRenderer::MenuRenderer(MenuManager* menuManager) : menuManager(menuManager), highlighted(-1)
{
    indexCount = 0;
    indexType = 0;
    setShaderState = nullptr;

	menuVBO.generateVBO(GL_STATIC_DRAW);
	menuIBO.generateIBO(GL_STATIC_DRAW);
	menuShader = Program::create("shaders/menu.vert", "shaders/menu.frag");
}

MenuRenderer::~MenuRenderer()
{
}

void MenuRenderer::highlight(int menuIndex)
{
	highlighted = menuIndex;
}

void MenuRenderer::draw()
{
	if (menuManager->isEmpty())
		return;

	static int lastKnown = -1;
	if (menuManager->top().getItems().size() != lastKnown) {
		lastKnown = menuManager->top().getItems().size();
		createRingGeometry();
	}

    

    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    menuShader.enable();
    glUniformMatrix4fv(menuShader.getUniform("modelViewProjection"), 1, false, modelViewProjection);
    menuVBO.bind();
    menuIBO.bind();    
    setShaderState();
    
	float alpha = this->menuManager->visibility();

	Vec3 menuC;
	Vec3 hlC;
	Vec3 tc;
	Vec3 tc2;
	if (MainController::getInstance().getRenderer().getBackgroundColor().x > 0.5f) {
		menuC = Vec3(.7f, .7f, .7f);
		hlC = Vec3(.1f, .1f, .1f);
		tc = Vec3(0, 0, 0);
		tc2 = Vec3(1.0f) - tc;
	}
	else {
		menuC = Vec3(.3f, .3f, .3f);
		hlC = Vec3(0.1f, 0.1f, 0.1f);
		tc = Vec3(1, 1, 1);
		tc2 = tc;
	}

    glUniform4f(menuShader.getUniform("color"), menuC.x, menuC.y, menuC.z, alpha * 0.85f);
    glDrawElements(GL_TRIANGLES, indexCount, indexType, 0);

    if (highlighted >= 0) {

		float lp = menuManager->getLeapProgress();
		Vec3 a = hlC;
		Vec3 b(0.1f, 0.75f, 1.0f);
		Vec3 c = a * (1 - lp) + b * lp;
		

		glUniform4f(menuShader.getUniform("color"), c.x, c.y, c.z, alpha);
        void* offset = (void*)(indicesPerMenuItem * highlighted * sizeof(GLushort));
		glDrawElements(GL_TRIANGLES, indicesPerMenuItem, indexType, offset);
    }
    glDisable(GL_BLEND);
    
	drawMenu(menuManager->top(), tc, tc2);
}

void MenuRenderer::drawMenu(Menu& menu, Vec3 textColor1, Vec3 textColor2)
{
	TextRenderer& text = MainController::getInstance().getText();

	text.begin(viewport.width, viewport.height);

	text.setColor(textColor1.x, textColor1.y, textColor1.z);
	text.add(
		menu.getName(),
		viewport.width / 2,
		viewport.height,
		TextRenderer::CENTER,
		TextRenderer::TOP);

	double radius = std::min(viewport.width, viewport.height) * 0.5 * 0.7;
	double angleStep = gl::two_pi / menu.getItems().size();
    double angle = angleStep/2.0;
	for (MenuItem& item : menu.getItems()) {
		int x = static_cast<int>(std::cos(angle) * radius + viewport.width / 2);
		int y = static_cast<int>(std::sin(angle) * radius + viewport.height / 2);
		text.add(item.getName(), x, y, TextRenderer::CENTER, TextRenderer::CENTER);
		angle += angleStep;
	}

	text.end();

	// draw highlighted
	if (highlighted >= 0 && highlighted < menu.getItems().size()) {
		double angle = angleStep * highlighted + angleStep/2.0;
		int x = static_cast<int>(std::cos(angle) * radius + viewport.width / 2);
		int y = static_cast<int>(std::sin(angle) * radius + viewport.height / 2);

		text.setColor(textColor2.x, textColor2.y, textColor2.z);
		text.begin(viewport.width, viewport.height);
		text.add(menu.getItems()[highlighted].getName(), x, y, 
			TextRenderer::CENTER, TextRenderer::CENTER);
		text.end();
	}

}

void MenuRenderer::createRingGeometry()
{
    GLfloat innerRadius = min(viewport.width, viewport.height) * 0.5 * 0.55;
	GLfloat outerRadius = min(viewport.width, viewport.height) * 0.5 * 0.85;// 0.5 * sqrt(viewport.width * viewport.width + viewport.height * viewport.height);
    
    vector<GLfloat> verts;
    vector<GLushort> indices;
    
    std::function<void(float,float)> pushVert = [&](float angle, float radius){
        verts.push_back(cos(angle) * radius);
        verts.push_back(sin(angle) * radius);
    };
    
    std::function<void(GLushort,GLushort,GLushort)> pushTriangle = [&](GLushort i, GLushort j, GLushort k) {
        indices.push_back(i);
        indices.push_back(j);
        indices.push_back(k);
    };
    
	// I want a consistent number of segments for a smooth circle regardless of the number of menu items.
	// However, I also need the segments to align with the boundaries of the menu items.
	unsigned stepsPerItem = std::max(1u, static_cast<unsigned>(128u / menuManager->top().getItems().size()));
	unsigned numSteps = stepsPerItem * menuManager->top().getItems().size();
	this->indicesPerMenuItem = stepsPerItem * 6;

    unsigned jmod = 2 * numSteps;
	float step = two_pi / numSteps;
    float angle = 0.0f;
    for (unsigned i = 0; i < numSteps; i++) {
        pushVert(angle, innerRadius);
        pushVert(angle, outerRadius);
        int j = i * 2;
        pushTriangle(j, j+1, (j+3) % jmod);
        pushTriangle(j, (j+3) % jmod, (j+2) % jmod);
        angle += step;
    }
    
    menuVBO.bind();
    menuVBO.data(&verts[0], verts.size() * sizeof(GLfloat));
    
    menuIBO.bind();
    menuIBO.data(&indices[0], indices.size() * sizeof(GLushort));
    indexCount = indices.size();
    indexType = GL_UNSIGNED_SHORT;
    
    setShaderState = [this] {
        int loc = menuShader.getAttribute("vs_position");
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 2, GL_FLOAT, false, 0, 0);
    };
}

void MenuRenderer::createListGeometry()
{
    
}

void MenuRenderer::resize(int width, int height)
{
    modelViewProjection = ortho2D(0, width, 0, height) * translation(width/2.0f, height/2.0f, 0);
	createRingGeometry();
}
