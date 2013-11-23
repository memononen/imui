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

#ifndef IMUI_H
#define IMUI_H

#define IMUI_MAX_BOX_STYLES 16

#define IMUI_AUTO -1
#define IMUI_NONE -2

enum IMUIalign {
	IMUI_START = 1,
	IMUI_END = 2,
	IMUI_CENTER = 3,
	IMUI_JUSTIFY = 4,
};

enum IMUIdirection {
	IMUI_ROW = 1,
	IMUI_COL = 2,
};

struct IMUIpoint {
	float x, y;
	unsigned char type;
};

struct IMUIshadow {
	float offset[2];
	float blur;
	float spread;
	unsigned int color;
};

struct IMUIcomputedStyle
{
	int fontId;
	char* fontFamily;
	int fontStyle;
	int fontWeight;
	float fontSize;

	unsigned int contentColor;
	unsigned int outlineColor;
	unsigned int backgroundColor;

	float outlineWidth;

	float width;
	float height;

	float padding[4];
	float margin[4];
	float radius[4];

	char* content;

	struct IMUIshadow textShadow;
	struct IMUIshadow boxShadow;
};

struct IMUIbox {

	char type[8];
	char* id;
	char* text;
	char* src;

	// Input for layout
//	float width, height;
	struct IMUIpoint contentSize;
	struct IMUIpoint elementSize;
	char dir;
	char pack;
	char align;
	char alignSelf;
	char grow;
	char shrink;

	// Styles
	struct IMUIstyle* styles[IMUI_MAX_BOX_STYLES];
	int nstyles;

	struct IMUIcomputedStyle computedStyle;

	// Position and size after layout
	struct IMUIpoint requestedSize;
	struct IMUIpoint computedSize;
	struct IMUIpoint computedPosition;

	// Used during layout
	struct IMUIpoint minimumSize;
	float mainSize;
	float crossSize;

	struct IMUIbox* parent;
	struct IMUIbox* items;
	struct IMUIbox* next;
};

enum IMUIboxState {
	IMUI_STATE_NORMAL,
	IMUI_STATE_HOVER,
	IMUI_STATE_ACTIVE,
	IMUI_STATE_FOCUS,
};

enum IMUIproperties {
	IMUI_PROP_FONT_FAMILY,
	IMUI_PROP_FONT_STYLE,
	IMUI_PROP_FONT_WEIGHT,
	IMUI_PROP_FONT_SIZE,
	IMUI_PROP_CONTENT_COLOR,
	IMUI_PROP_BACKGROUND_COLOR,
	IMUI_PROP_OUTLINE_COLOR,
	IMUI_PROP_OUTLINE_WIDTH,
	IMUI_PROP_WIDTH,
	IMUI_PROP_HEIGHT,
	IMUI_PROP_CONTENT,
	IMUI_PROP_PADDING_TOP,
	IMUI_PROP_PADDING_RIGHT,
	IMUI_PROP_PADDING_BOTTOM,
	IMUI_PROP_PADDING_LEFT,
	IMUI_PROP_MARGIN_TOP,
	IMUI_PROP_MARGIN_RIGHT,
	IMUI_PROP_MARGIN_BOTTOM,
	IMUI_PROP_MARGIN_LEFT,
	IMUI_PROP_RADIUS_TOPLEFT,
	IMUI_PROP_RADIUS_TOPRIGHT,
	IMUI_PROP_RADIUS_BOTRIGHT,
	IMUI_PROP_RADIUS_BOTLEFT,
	IMUI_PROP_TEXTSHADOW_OX,
	IMUI_PROP_TEXTSHADOW_OY,
	IMUI_PROP_TEXTSHADOW_BLUR,
	IMUI_PROP_TEXTSHADOW_COLOR,
	IMUI_PROP_BOXSHADOW_OX,
	IMUI_PROP_BOXSHADOW_OY,
	IMUI_PROP_BOXSHADOW_BLUR,
	IMUI_PROP_BOXSHADOW_SPREAD,
	IMUI_PROP_BOXSHADOW_COLOR,
};

struct IMUIprop {
	struct IMUIprop* next;
	unsigned char cmd;
	unsigned char state;
	union {
		float fval;
		unsigned int uval;
		int ival;
		char str[4];
	};
};

enum IMUIstyles {
	IMUI_STYLE_CLASS,
	IMUI_STYLE_ELEM,
	IMUI_STYLE_ID,
};

struct IMUIstyle
{
	char* name;
	int state;
	int type;
	struct IMUIprop* props;
	struct IMUIstyle* next;
};


enum IMUIfontStyle {
	IMUI_REGULAR,
	IMUI_ITALIC,
};

struct IMUIfontFace {
	int id;
	char* family;
	int weight;
	int style;
	char* filename;
	struct IMUIfontFace* next;
};

struct IMUItemplate {
	char* id;
	struct IMUIbox* root;
	struct IMUItemplate* next;
	struct IMUIlayout* layout;
};

struct IMUIlayout {
	struct IMUItemplate* templates;
	struct IMUIstyle* styles;
	struct IMUIfontFace* fonts;
};

struct IMUIlayout* imuiParseLayout(const char* name, char* input);
struct IMUIlayout* imuiParseLayoutFromFile(const char* file);

struct IMUItemplate* imuiFindTemplate(struct IMUIlayout* layout, const char* id);
void imuiComputeLayout(struct IMUItemplate* tmpl, float w, float h, int (*measureItem)(void* uptr, struct IMUIbox* box, float* width, float* height), void* uptr);

void imuiDeleteLayout(struct IMUIlayout* layout);


#endif
