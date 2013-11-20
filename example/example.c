//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include <stdio.h>
#include <string.h>
#include <iconv.h>
#include <math.h>
#include <GLFW/glfw3.h>
#define IMUI_IMPLEMENTATION
#include "imui.h"
#include "nanovg.h"
#define GLNANOVG_IMPLEMENTATION
#include "glnanovg.h"

/*
#define ICON_SEARCH 0x1F50D
#define ICON_CIRCLED_CROSS 0x2716

static char* cpToUTF8(int cp, char* str)
{
	int n = 0;
	if (cp < 0x80) n = 1;
	else if (cp < 0x800) n = 2;
	else if (cp < 0x10000) n = 3;
	else if (cp < 0x200000) n = 4;
	else if (cp < 0x4000000) n = 5;
	else if (cp <= 0x7fffffff) n = 6;
	str[n] = '\0';
	switch (n) {
	case 6: str[5] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x4000000;
	case 5: str[4] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x200000;
	case 4: str[3] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x10000;
	case 3: str[2] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0x800;
	case 2: str[1] = 0x80 | (cp & 0x3f); cp = cp >> 6; cp |= 0xc0;
	case 1: str[0] = cp;
	}
	return str;
}
*/

void drawLayout(struct NVGcontext* vg, struct IMUIbox* box)
{
	struct IMUIbox* it;
	float ox = 500.0f, oy = 20.0f;

	nvgFillColor(vg, nvgRGBA(0,0,0,128));
	nvgText(vg, ox+2+box->computedPosition.x, oy+2+box->computedPosition.y+12, box->type, NULL);

	nvgStrokeColor(vg, nvgRGBA(0,0,0,32));
	nvgBeginPath(vg);
	nvgRect(vg, ox+box->computedPosition.x, oy+box->computedPosition.y, box->computedSize.x, box->computedSize.y);
	nvgStroke(vg);

	for (it = box->items; it != NULL; it = it->next)
		drawLayout(vg, it);
}

static int measureItem(void* uptr, struct IMUIbox* box, float* itemw, float* itemh)
{
	struct NVGcontext* vg = (struct NVGcontext*)uptr;
	float textw, lineh;
	const char* text = box->computedStyle.content != NULL ? box->computedStyle.content : box->text;

	if (text != NULL) {
		if (box->computedStyle.fontId != -1) {
//				printf("measure '%s' font:%s %fpt\n", text, box->computedStyle.fontFamily, box->computedStyle.fontSize);
//				printf("%s needs size\n", box->type);
			nvgFontFaceId(vg, box->computedStyle.fontId);
			nvgFontSize(vg, box->computedStyle.fontSize);
			nvgLetterSpacing(vg, 0);
			nvgFontBlur(vg, 0);
			nvgTextAlign(vg, NVG_ALIGN_LEFT|NVG_ALIGN_BASELINE);

			textw = nvgTextBounds(vg, text, NULL, NULL);
			nvgVertMetrics(vg, NULL, NULL, &lineh);

//				printf("%f %f\n", textw, lineh);
			*itemw = textw;
			*itemh = lineh;
		}
	}

//	printf("measured: %f x %f\n", width, height);

	return 1;
}

static int loadLayoutFonts(struct IMUIlayout* layout, struct NVGcontext* vg)
{
	struct IMUIfontFace* font;
	char name[128];
	for (font = layout->fonts; font; font = font->next) {
		snprintf(name, 128, "%s-%d%s", font->family, font->weight, font->style == IMUI_ITALIC ? "-italic" : "");
		// Check to see if the font exists
		font->id = nvgFindFont(vg, name);
		if (font->id != -1)
			continue;
		// Load font
		font->id = nvgCreateFont(vg, name, font->filename);
		printf("loading font %s (%d): %s\n", name, font->id, font->filename);
		if (font->id == -1) {
			printf("Failed to load font '%s' as '%s'\n", font->filename, name);
			return 0;
		}
	}
	return 1;
}


void errorcb(int error, const char* desc)
{
	printf("GLFW error: %s\n", desc);
}

int main()
{
	GLFWwindow* window;
	const GLFWvidmode* mode;
	struct IMUIlayout* layout = NULL;
	struct IMUItemplate* matWin = NULL;
	struct NVGcontext* vg = NULL;
	struct NVGpaint paint;
	char icon[8];

	if (!glfwInit()) {
		printf("Failed to init GLFW.");
		return -1;
	}

	glfwSetErrorCallback(errorcb);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    window = glfwCreateWindow(mode->width - 40, mode->height - 80, "Font Stash", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	vg = glnvgCreate(512,512);
	if (vg == NULL) {
		printf("Could not init nanovg.\n");
		return -1;
	}

	if (nvgCreateFont(vg, "icon", "../example/entypo.ttf") == -1) {
		printf("Could not add font icons.\n");
		return -1;
	}
	if (nvgCreateFont(vg, "sans", "../example/Roboto-Regular.ttf") == -1) {
		printf("Could not add font regulsr.\n");
		return -1;
	}
	if (nvgCreateFont(vg, "sans", "../example/Roboto-Bold.ttf") == -1) {
		printf("Could not add font bold.\n");
		return -1;
	}

	layout = imuiParseLayoutFromFile("../example/templates.xml");
	if (layout == NULL) {
		printf("Could not load layout.\n");
		return -1;
	}
	if (!loadLayoutFonts(layout, vg)) {
		return -1;
	}

	matWin = imuiFindTemplate(layout, "material-window");
	if (matWin == NULL) {
		printf("Could not find template.\n");
		return -1;
	}


	glfwSetTime(0);

	while (!glfwWindowShouldClose(window))
	{
		float sx, sy, dx, dy, lh = 0;
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		// Update and render
		glViewport(0, 0, width, height);
		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glDisable(GL_TEXTURE_2D);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,width,height,0,-1,1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);
		glColor4ub(255,255,255,255);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		float t = glfwGetTime();
		float u = fabsf(fmodf(t, 3.0f) / 1.5f - 1.0f);

		sx = 100; sy = 100;

		float w = 200.0f + u*100.0f;
		float h = 500.0f + u*100.0f;

		imuiComputeLayout(matWin, w, h, measureItem, vg);

		nvgFontSize(vg, 15.0f);
		nvgFontFace(vg, "sans");
		drawLayout(vg, matWin->root);

		glEnable(GL_DEPTH_TEST);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glnvgDelete(vg);
	imuiDeleteLayout(layout);

	glfwTerminate();
	return 0;
}
