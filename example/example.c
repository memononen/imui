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

static float minf(float a, float b) { return a < b ? a : b; }
static float maxf(float a, float b) { return a > b ? a : b; }
static float clampf(float a, float mn, float mx) { return a < mn ? mn : (a > mx ? mx : a); }
static float absf(float a) { return a < 0.0f ? -a : a; }

static void drawBox(struct NVGcontext* vg, float x, float y, float w, float h, float* cr, float is)
{
	float d;
	x -= is;
	y -= is;
	w = maxf(0, w + is*2);
	h = maxf(0, h + is*2);
	d = minf(w, h);
	nvgMoveTo(vg, x, y+h*0.5f);
	nvgArcTo(vg, x,y, x+w,y, minf(cr[0]+is, d/2));
	nvgArcTo(vg, x+w,y, x+w,y+h, minf(cr[1]+is, d/2));
	nvgArcTo(vg, x+w,y+h, x,y+h, minf(cr[2]+is, d/2));
	nvgArcTo(vg, x,y+h, x,y, minf(cr[3]+is, d/2));
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

static void drawBorderBox(struct NVGcontext* vg, float x, float y, float w, float h, float* cr, float sw, unsigned int* bcol)
{
	if ((bcol[0] | bcol[1] | bcol[2] | bcol[3]) == 0) {
		// All transparent, do not draw
		return;
	} else if (bcol[0] == bcol[1] && bcol[0] == bcol[2] && bcol[0] == bcol[3]) {
		// All same color, draw as closed stroke.
		nvgBeginPath(vg);
		drawBox(vg, x,y, w,h, cr, sw*0.5f);
		nvgStrokeColor(vg, bcol[0]);
		nvgStrokeWidth(vg, sw);
		nvgStroke(vg);
	} else {
		// Mixed appearance
		int first, i;
		float corners[4*2];
		float rad[4], is = -sw*0.5f, d;
		x -= is;
		y -= is;
		w = maxf(0, w + is*2);
		h = maxf(0, h + is*2);
		d = minf(w, h);
		rad[0] = clampf(cr[0]+is, 0, d/2);
		corners[0] = x+rad[0];
		corners[1] = y+rad[0];
		rad[1] = clampf(cr[1]+is, 0, d/2);
		corners[2] = x+w-rad[1];
		corners[3] = y+rad[1];
		rad[2] = clampf(cr[2]+is, 0, d/2);
		corners[4] = x+w-rad[2];
		corners[5] = y+h-rad[2];
		rad[3] = clampf(cr[3]+is, 0, d/2);
		corners[6] = x+rad[3];
		corners[7] = y+h-rad[3];

		// Find first transition
		first = 0;
		for (i = 0; i < 4; i++) {
			int idx = i % 4;
			int pidx = (idx+3) % 4;
			if (bcol[pidx] != bcol[i]) {
				first = i;
				break;
			}
		}

		nvgBeginPath(vg);
		for (i = 0; i <= 4; i++) {
			int idx = first % 4;
			int pidx = (idx+3) % 4;
			float a0 = NVG_PI + i*NVG_PI*0.5f;
			float a1 = a0 + NVG_PI*0.5f;
			if (bcol[pidx] == bcol[idx]) {
				if (bcol[idx] != 0) {
					nvgArc(vg, corners[idx*2], corners[idx*2+1], rad[idx], a0, a1, NVG_CW);
				}
			} else {
				if (bcol[pidx] != 0) {
					nvgArc(vg, corners[idx*2], corners[idx*2+1], rad[idx], a0, (a0+a1)*0.5f, NVG_CW);
					nvgStrokeColor(vg, bcol[pidx]);
					nvgStrokeWidth(vg, sw);
					nvgStroke(vg);
				}
				if (bcol[idx] != 0) {
					nvgBeginPath(vg);
					nvgArc(vg, corners[idx*2], corners[idx*2+1], rad[idx], (a0+a1)*0.5f, a1, NVG_CW);
				}
			}
			first++;
		}
		// Clear any path generated.
		nvgBeginPath(vg);
	}
}

static void calcLinearGradientPoints(float* pts, float x, float y, float w, float h, float angle)
{
	int i;
	float corners[4*2] = {x,y, x+w,y, x+w,y+h, x,y+h };
	float cx = x+w*0.5f, cy = y+h*0.5f;
	float dx = -sinf(angle), dy = cosf(angle);
	float dmax = 0, d;
	for (i = 0; i < 4; i++) {
		d = dx*(corners[i*2] - cx) + dy*(corners[i*2+1] - cy);
		if (d > dmax) dmax = d;
	}
	pts[0] = cx + dx*dmax;
	pts[1] = cy + dy*dmax;
	pts[2] = cx - dx*dmax;
	pts[3] = cy - dy*dmax;
}

void drawLayout(struct NVGcontext* vg, float ox, float oy, struct IMUIbox* box)
{
	struct IMUIbox* it;
	struct IMUIcomputedStyle* style = &box->computedStyle;
	float bx = ox + box->computedPosition.x, by = oy + box->computedPosition.y; 
	float bw = box->computedSize.x, bh = box->computedSize.y;
	const char* text = box->text;
	if (style->content != NULL)
		text = style->content;

	if (style->boxShadow.color != 0) {
		struct NVGpaint paint;
		float x,y,w,h,r,r2,blur,spread,sx,sy;
		sx = style->boxShadow.offset.x;
		sy = style->boxShadow.offset.y;
		blur = style->boxShadow.blur;
		spread = style->boxShadow.spread;
		x = bx;
		y = by;
		w = bw;
		h = bh;
		r = blur*0.5f + spread + 1;
		if (sx < 0) x += sx;
		if (sy < 0) y += sy;
		w += absf(sx);
		h += absf(sy);
		x -= r;
		y -= r;
		w += r*2;
		h += r*2;

		r2 = (style->radius[0] + style->radius[1] + style->radius[2] + style->radius[3]) / 4.0f;
		paint = nvgBoxGradient(vg, sx+bx-spread, sy+by-spread,
								bw+spread*2, bh+spread*2, r2+spread,
								blur, style->boxShadow.color, nvgTransRGBA(style->boxShadow.color, 0));

		nvgBeginPath(vg);
		nvgRect(vg, x,y,w,h);
		drawBox(vg, bx, by, bw, bh, style->radius, 0.0f);
		nvgPathWinding(vg, NVG_CW);
		nvgFillPaint(vg, paint);
		nvgFill(vg);
	}

	if (style->background.startColor != 0) {
		float pts[4];
		nvgBeginPath(vg);
		drawBox(vg, bx, by, bw, bh, style->radius, 0.0f);
		if (style->background.startColor == style->background.endColor) {
			nvgFillColor(vg, style->background.startColor);
		} else {
			struct NVGpaint grad;
			calcLinearGradientPoints(pts, bx, by, bw, bh, style->background.angle);
			grad = nvgLinearGradient(vg, pts[0],pts[1],pts[2],pts[3], style->background.startColor, style->background.endColor);
			nvgFillPaint(vg, grad);
		}
		nvgFill(vg);

/*		nvgBeginPath(vg);
		nvgMoveTo(vg, pts[0],pts[1]);
		nvgLineTo(vg, pts[2],pts[3]);
		nvgCircle(vg, pts[2],pts[3],3.0f);
		nvgStrokeWidth(vg, 1.0f);
		nvgStrokeColor(vg, nvgRGBA(255,255,255,255));
		nvgStroke(vg);*/
	}

	if (style->outlineColor != 0) {
		float offset = style->outlineWidth*0.5f + style->outlineOffset;
		nvgBeginPath(vg);
		drawBox(vg, bx, by, bw, bh, style->radius, offset);
		nvgStrokeColor(vg, style->outlineColor);
		nvgStrokeWidth(vg, style->outlineWidth);
		nvgStroke(vg);
	}

	drawBorderBox(vg, bx, by, bw, bh, style->radius, style->borderWidth, style->borderColor);

	if (text != NULL) {
		if (style->fontId != -1) {
			nvgFontFaceId(vg, style->fontId);
			nvgFontSize(vg, style->fontSize);
			nvgLetterSpacing(vg, style->letterSpacing);
			nvgTextAlign(vg, NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
			if (style->textShadow.color != 0) {
				nvgFontBlur(vg, style->textShadow.blur);
				nvgFillColor(vg, style->textShadow.color);
				nvgText(vg, bx+bw*0.5+style->textShadow.offset.x, by+bh*0.5+style->textShadow.offset.y, text, NULL);
			}
			nvgFontBlur(vg, 0);
			nvgFillColor(vg, style->contentColor);
			nvgText(vg, bx+bw*0.5, by+bh*0.5, text, NULL);
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
