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

static float clampRad(float r0, float r1, float w, float is)
{
	r0 += is;
	r1 += is;
	if (r0 < 0) r0 = 0;
	if (r1 < 0) r1 = 1;
	if (r0+r1 >= w) {
		float s = w/(r0+r1);
		return r0*s;
	}
	return r0;
}

static void drawBox(struct NVGcontext* vg, float x, float y, float w, float h, float* cr, float is)
{
	float d;
	x -= is;
	y -= is;
	w += is*2;
	h += is*2;
	d = w < h ? w : h;
	nvgMoveTo(vg, x, y+h*0.5f);
	nvgArcTo(vg, x,y, x+w,y, clampRad(cr[0],cr[1],d,is));
	nvgArcTo(vg, x+w,y, x+w,y+h, clampRad(cr[1],cr[2],d,is));
	nvgArcTo(vg, x+w,y+h, x,y+h, clampRad(cr[2],cr[3],d,is));
	nvgArcTo(vg, x,y+h, x,y, clampRad(cr[3],cr[0],d,is));
	nvgClosePath(vg);

/*	float r = 10;
	nvgArc(vg, x+r, y+r, r, NVG_PI, NVG_PI*1.5f, NVG_CW);
	nvgArc(vg, x+w-r, y+r, r, NVG_PI*1.5f, NVG_PI*2.0f, NVG_CW);
	nvgArc(vg, x+w-r, y+h-r, r, 0, NVG_PI*0.5f, NVG_CW);
	nvgArc(vg, x+r, y+h-r, r, NVG_PI*0.5f, NVG_PI, NVG_CW);
*/

/*	nvgMoveTo(vg, x, y+h*0.5f);

	nvgArcTo(vg, x,y, x+w,y, 10);
	nvgArcTo(vg, x+w,y, x+w,y+h, 10);
	nvgArcTo(vg, x+w,y+h, x,y+h, 10);
	nvgArcTo(vg, x,y+h, x,y, 10);

	nvgClosePath(vg);*/

}

void drawLayout(struct NVGcontext* vg, float ox, float oy, struct IMUIbox* box)
{
	struct IMUIbox* it;
	const char* text = box->text;
	if (box->computedStyle.content != NULL)
		text = box->computedStyle.content;

	if (box->computedStyle.boxShadow.color != 0) {
		struct NVGpaint paint;
		float x,y,w,h,r,r2,blur,spread,sx,sy;
		sx = box->computedStyle.boxShadow.offset[0];
		sy = box->computedStyle.boxShadow.offset[1];
		blur = box->computedStyle.boxShadow.blur;
		spread = box->computedStyle.boxShadow.spread;
		x = ox + box->computedPosition.x;
		y = oy + box->computedPosition.y;
		w = box->computedSize.x;
		h = box->computedSize.y;
		r = blur*0.5f + spread + 1;
		if (sx < 0) x += sx;
		if (sy < 0) y += sy;
		w += fabsf(sx);
		h += fabsf(sy);
		x -= r;
		y -= r;
		w += r*2;
		h += r*2;

		r2 = (box->computedStyle.radius[0] + box->computedStyle.radius[1] + box->computedStyle.radius[2] + box->computedStyle.radius[3]) / 4.0f;
		paint = nvgBoxGradient(vg, sx+ox+box->computedPosition.x-spread, sy+oy+box->computedPosition.y-spread,
								box->computedSize.x+spread*2, box->computedSize.y+spread*2, r2+blur*0.5f+spread,
								blur, box->computedStyle.boxShadow.color, nvgTransRGBA(box->computedStyle.boxShadow.color, 0));

		nvgBeginPath(vg);
		nvgRect(vg, x,y,w,h);
		drawBox(vg, ox+box->computedPosition.x, oy+box->computedPosition.y, box->computedSize.x, box->computedSize.y, box->computedStyle.radius, 0.0f);
		nvgPathWinding(vg, NVG_CW);
		nvgFillPaint(vg, paint);
		nvgFill(vg);
	}

	if (box->computedStyle.backgroundColor != 0) {
		nvgBeginPath(vg);
		drawBox(vg, ox+box->computedPosition.x, oy+box->computedPosition.y, box->computedSize.x, box->computedSize.y, box->computedStyle.radius, 0.0f);
		nvgFillColor(vg, box->computedStyle.backgroundColor);
		nvgFill(vg);
	}

	if (box->computedStyle.outlineColor != 0) {
		nvgBeginPath(vg);
		drawBox(vg, ox+box->computedPosition.x, oy+box->computedPosition.y, box->computedSize.x, box->computedSize.y, box->computedStyle.radius, box->computedStyle.outlineWidth*0.5f);
		nvgStrokeColor(vg, box->computedStyle.outlineColor);
		nvgStrokeWidth(vg, box->computedStyle.outlineWidth);
		nvgStroke(vg);
	}

	if (text != NULL) {
		if (box->computedStyle.fontId != -1) {
			nvgFontFaceId(vg, box->computedStyle.fontId);
			nvgFontSize(vg, box->computedStyle.fontSize);
			nvgLetterSpacing(vg, 0);
			nvgFontBlur(vg, 0);
			nvgTextAlign(vg, NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
			nvgFillColor(vg, box->computedStyle.contentColor);
			nvgText(vg, ox+box->computedPosition.x+box->computedSize.x*0.5, oy+box->computedPosition.y+box->computedSize.y*0.5, text, NULL);
		}
	}


/*	nvgFillColor(vg, nvgRGBA(0,0,0,128));
	nvgText(vg, ox+2+box->computedPosition.x, oy+2+box->computedPosition.y+12, box->type, NULL);

	nvgStrokeColor(vg, nvgRGBA(0,0,0,32));
	nvgBeginPath(vg);
	nvgRect(vg, ox+box->computedPosition.x, oy+box->computedPosition.y, box->computedSize.x, box->computedSize.y);
	nvgStroke(vg);
*/

	for (it = box->items; it != NULL; it = it->next)
		drawLayout(vg, ox,oy, it);
}

static int measureItem(void* uptr, struct IMUIbox* box, float* itemw, float* itemh)
{
	struct NVGcontext* vg = (struct NVGcontext*)uptr;
	float textw, lineh;
	const char* text = box->text;

	if (box->computedStyle.content != NULL)
		text = box->computedStyle.content;
	if (strcmp(box->type, "field") == 0)
		text = "XXXXX";

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

		imuiComputeLayout(matWin, w,h, measureItem, vg);

		nvgFontSize(vg, 15.0f);
		nvgFontFace(vg, "sans");
		drawLayout(vg, 500,20, matWin->root);

		glEnable(GL_DEPTH_TEST);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glnvgDelete(vg);
	imuiDeleteLayout(layout);

	glfwTerminate();
	return 0;
}
