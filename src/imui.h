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

#ifdef IMUI_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>

#define IMUI_MAX_LAYOUT_NESTING 256

struct IMUIvar {
	char* key;
	char* value;
	struct IMUIvar* next;
};

struct IMUIparser {
	// Resulting layout.
	struct IMUIlayout* layout;
	// Current style or template being parsed
	struct IMUItemplate* curTemplate;
	struct IMUIstyle* curStyle;
	int curStyleState;
	struct IMUIfontFace* curFont;
	struct IMUIvar* vars;
	// Current hierarchy being parse
	struct IMUIbox* boxstack[IMUI_MAX_LAYOUT_NESTING];
	int nboxstack;
	const char* elstack[IMUI_MAX_LAYOUT_NESTING];
	int nelstack;
	// Used to error reporting
	char* input;
	const char* name;

	float emSize;
};


static int imui__isspace(char c)
{
	return strchr(" \t\n\v\f\r", c) != 0;
}

static int imui__isnum(char c)
{
	return strchr("0123456789+-.eE", c) != 0;
}

static int imui__mini(int a, int b) { return a < b ? a : b; }
static int imui__maxi(int a, int b) { return a > b ? a : b; }
static float imui__minf(float a, float b) { return a < b ? a : b; }
static float imui__maxf(float a, float b) { return a > b ? a : b; }
static float imui__absf(float a) { return a >= 0.0f ? a : -a; }
static float imui__clampf(float a, float mn, float mx) { return a < mn ? mn : (a > mx ? mx : a); }

static unsigned int imui__RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return (r) | (g << 8) | (b << 16) | (a << 24);
}


// Simple XML parser

#define IMUI_XML_TAG 1
#define IMUI_XML_CONTENT 2
#define IMUI_XML_COMMENT 3
#define IMUI_XML_MAX_ATTRIBS 256

static int imui__parseContentXML(char* s,
								 int (*contentCb)(void* ud, char* s),
								 void* ud)
{
	// Trim start white spaces
	while (*s && imui__isspace(*s)) s++;
	if (!*s) return 1;
	
	if ((*contentCb)(ud, s) == 0)
		return 0;

	return 1;
}

static int imui__parseElementXML(char* s,
								 int (*startelCb)(void* ud, const char* el, const char** attr, int nattr),
								 int (*endelCb)(void* ud, const char* el),
								 void* ud)
{
	const char* attr[IMUI_XML_MAX_ATTRIBS];
	int nattr = 0;
	char* name;
	int start = 0;
	int end = 0;
	
	// Skip white space after the '<'
	while (*s && imui__isspace(*s)) s++;

	// Check if the tag is end tag
	if (*s == '/') {
		s++;
		end = 1;
	} else {
		start = 1;
	}

	// Skip comments, data and preprocessor stuff.
	if (!*s || *s == '?')
		return 1;

	// Get tag name
	name = s;
	while (*s && !imui__isspace(*s)) s++;
	if (*s) { *s++ = '\0'; }

	// Get attribs
	while (!end && *s && nattr < IMUI_XML_MAX_ATTRIBS) {
		// Skip white space before the attrib name
		while (*s && imui__isspace(*s)) s++;
		if (!*s) break;
		if (*s == '/') {
			end = 1;
			break;
		}
		attr[nattr++] = s;
		// Find end of the attrib name.
		while (*s && !imui__isspace(*s) && *s != '=') s++;
		if (*s) { *s++ = '\0'; }
		// Skip until the beginning of the value.
		while (*s && *s != '\"') s++;
		if (!*s) break;
		s++;
		// Store value and find the end of it.
		attr[nattr++] = s;
		while (*s && *s != '\"') s++;
		if (*s) { *s++ = '\0'; }
	}
	
	// Call callbacks.
	if (start) {
		if ((*startelCb)(ud, name, attr, nattr/2) == 0)
			return 0;
	}
	if (end) {
		if ((*endelCb)(ud, name) == 0)
			return 0;
	}

	return 1;
}

int imui__parseXML(char* input,
				   int (*startelCb)(void* ud, const char* el, const char** attr, int nattr),
				   int (*endelCb)(void* ud, const char* el),
				   int (*contentCb)(void* ud, char* s),
				   void* ud)
{
	char* s = input;
	char* mark = s;
	int state = IMUI_XML_CONTENT;

	while (*s) {
		if (state == IMUI_XML_CONTENT && *s == '<') {
			*s++ = '\0';
			if (*s == '!') {
				// Skip comment
				state = IMUI_XML_COMMENT;
			} else {
				// Start of a tag
				if (!imui__parseContentXML(mark, contentCb, ud))
					return 0;
				mark = s;
				state = IMUI_XML_TAG;
			}
		} else if (state == IMUI_XML_TAG && *s == '>') {
			// Start of a content or new tag.
			*s++ = '\0';
			if (!imui__parseElementXML(mark, startelCb, endelCb, ud))
				return 0;
			mark = s;
			state = IMUI_XML_CONTENT;
		} else if (state == IMUI_XML_COMMENT && s[0] == '-' && s[1] == '-' && s[2] == '>') { // Assuming short-circuit will handle null termination.
			s += 3;
			mark = s;
			state = IMUI_XML_CONTENT;
		} else {
			s++;
		}
	}
	
	return 1;
}

#define IMUI_CSS_MAX_VALUES 8

static char* imui__parseCommentCSS(char* s)
{
	// Comment, skip until '*/'
	while (*s) {
		if (s[0] == '*' && s[1] == '/') {
			s += 2;
			break;
		}
		s++;
	}
	return s;
}

static char* imui__parseVarCSS(char* s,
							   int (*variableCb)(void* ud, char* key, char* val),
							   void* ud)
{
	char* key;
	char* val;
	// parse key until = or white space
	key = s;
	while (*s && !imui__isspace(*s) && *s != '=') s++;
	if (*s) *s++ = '\0';
	// Skip '='' or white.
	while (*s && (imui__isspace(*s) || *s == '=')) s++;
	val = s;
	// Until ';'
	while (*s && *s != ';') s++;
	if (*s) *s++ = '\0';
	if (*key && *val) {
		if (variableCb(ud, key, val) == 0)
			return NULL;
	}
	return s;
}

static char* imui__parsePropertyCSS(char* s,
									int (*propertyCb)(void* ud, char* key, char** vals, int nvals),
									void* ud)
{
	char* key;
	char* vals[IMUI_CSS_MAX_VALUES];
	int nvals = 0;
	char st = ' ';
	// parse key until : or white space
	key = s;
	while (*s && !imui__isspace(*s) && *s != ':') s++;
	if (*s) *s++ = '\0';
	// Skip ':'' and white.
	while (*s && (imui__isspace(*s) || *s == ':')) s++;
//	val = s;

	// Until ';'
	while (*s) {
		if (*s == ';') {
			// End of property.
			*s++ = '\0';
			break;
		}
		if (st == ' ') {
			// Advance over white space
			if (!imui__isspace(*s)) {
				if (*s == '\"' || *s == '\'') {
					if (nvals < IMUI_CSS_MAX_VALUES)
						vals[nvals++] = s+1;
					st = *s;
				} else {
					if (nvals < IMUI_CSS_MAX_VALUES)
						vals[nvals++] = s;
					st = 'a';
				}
			}
			s++;
		} else if (st == 'a') {
			// Advance until value ends.
			if (imui__isspace(*s)) {
				*s++ = '\0';
				st = ' ';
			} else {
				s++;
			}
		} else if (st == '\'' || st == '\"') {
			// Advance until string ends.
			if (*s == st) {
				*s++ = '\0';
				st = ' ';
			} else {
				s++;
			}
		} else {
			s++;
		}
	}

/*	while (*s && *s != ';') s++;
	if (*s) *s++ = '\0';*/

	if (*key && nvals) {
		if (propertyCb(ud, key, vals, nvals) == 0)
			return NULL;
	}
	return s;
}

static char* imui__parseBlockCSS(char* s,
								 int (*blockCb)(void* ud, char* block),
								 int (*propertyCb)(void* ud, char* key, char** vals, int nvals),
								 void* ud)
{
	char* prop;
	char* val;
	char* block = s;
	// Skip until white space or '{'.
	while (*s && !imui__isspace(*s) && *s != '{') s++;
	if (*s) *s++ = '\0';
	// Skip until '{'
	while (*s && *s != '{') s++;
	if (*s != '{') return NULL;
	if (blockCb(ud, block) == 0)
		return NULL;
	s++; // Step over '{'
	// Parse properties
	prop = NULL;
	val = NULL;
	while (*s) {
		if (*s == '}') {
			// End of block
			s++;
			break;
		} else if (s[0] == '/' && s[1] == '*') {
			// Comment
			s = imui__parseCommentCSS(s+2);
			if (s == NULL) return NULL;
		} else if (imui__isspace(*s)) {
			// Skip space between properties
			s++;
		} else {
			// Parse property
			s = imui__parsePropertyCSS(s, propertyCb, ud);
			if (s == NULL) return NULL;
		}
	}

	return s;
}

// Simple CSS parser
int imui__parseCSS(char* input,
				   int (*blockCb)(void* ud, char* block),
				   int (*propertyCb)(void* ud, char* key, char** vals, int nvals),
				   int (*variableCb)(void* ud, char* key, char* val),
				   void* ud)
{
	char* s = input;
	while (*s) {
		if (imui__isspace(*s)) {
			s++;
		} else if (s[0] == '/' && s[1] == '*') {
			// Comment
			s = imui__parseCommentCSS(s+2);
			if (s == NULL) return 0;
		} else if (s[0] == '$') {
			// Variable
			s = imui__parseVarCSS(s, variableCb, ud);
			if (s == NULL) return 0;
		} else {
			// Block
			s = imui__parseBlockCSS(s, blockCb, propertyCb, ud);
			if (s == NULL) return 0;
		}
	}

	return 1;
}



// Returns line and column position for given parse location. Slow, but used for error reporting only.
static int imui__getLine(struct IMUIparser* p, const char* str)
{
	const char* src = p->input;
	int pos = (int)(str - p->input);
	int line = 0;
	if (src == NULL || pos < 0 || pos > 10*1024*1024) {
		return -1;
	}
	while (src != str) {
		if (*src == '\n')
			line++;
		src++;
	}
	return line + 1;
}


static struct IMUIbox* imui__allocBox(struct IMUIparser* p, const char* type)
{
	struct IMUIbox* box = (struct IMUIbox*)malloc(sizeof(struct IMUIbox));
	if (box == NULL) return NULL;
	memset(box, 0, sizeof(struct IMUIbox));

	strncpy(box->type, type, sizeof(box->type));
	box->type[sizeof(box->type)-1] = '\0';

	box->elementSize.x = IMUI_AUTO;
	box->elementSize.y = IMUI_AUTO;
	box->dir = IMUI_ROW;
	box->pack = IMUI_START;
	box->align = IMUI_START;
	box->alignSelf = IMUI_AUTO;
	box->grow = 0;
	box->shrink = 1;

	return box;
}

static void imui__deleteBox(struct IMUIbox* box)
{
	struct IMUIbox* child = box->items;
	while (child != NULL) {
		struct IMUIbox* next = child->next;
		imui__deleteBox(child);
		child = next;
	}
	if (box->id) free(box->id);
	if (box->text) free(box->text);
	if (box->src) free(box->src);
	free(box);
}

static struct IMUItemplate* imui__allocTemplate(struct IMUIparser* p)
{
	struct IMUItemplate* tmpl = (struct IMUItemplate*)malloc(sizeof(struct IMUItemplate));
	if (tmpl == NULL) return NULL;
	memset(tmpl, 0, sizeof(struct IMUItemplate));
	return tmpl;
}

static void imui__deleteTemplate(struct IMUItemplate* tmpl)
{
	if (tmpl->root != NULL)
		imui__deleteBox(tmpl->root);
	free(tmpl);
}

static struct IMUIstyle* imui__allocStyle(struct IMUIparser* p)
{
	struct IMUIstyle* style = (struct IMUIstyle*)malloc(sizeof(struct IMUIstyle));
	if (style == NULL) return NULL;
	memset(style, 0, sizeof(struct IMUIstyle));

	return style;
}


static void imui__deleteStyle(struct IMUIstyle* style)
{
	struct IMUIprop* prop = style->props;
	while (prop) {
		struct IMUIprop* next = prop->next;
		free(prop);
		prop = next;
	}
	free(style);
}

static struct IMUIvar* imui__allocVar(struct IMUIparser* p)
{
	struct IMUIvar* v = (struct IMUIvar*)malloc(sizeof(struct IMUIvar));
	if (v == NULL) return NULL;
	memset(v, 0, sizeof(struct IMUIvar));

	return v;
}

static void imui__deleteVar(struct IMUIvar* v)
{
	if (v->key) free(v->key);
	if (v->value) free(v->value);
	free(v);
}

static struct IMUIfontFace* imui__allocFont(struct IMUIparser* p)
{
	struct IMUIfontFace* f = (struct IMUIfontFace*)malloc(sizeof(struct IMUIfontFace));
	if (f == NULL) return NULL;
	memset(f, 0, sizeof(struct IMUIfontFace));

	f->style = IMUI_REGULAR;
	f->weight = 400;

	return f;
}

static void imui__deleteFont(struct IMUIfontFace* f)
{
	if (f->family) free(f->family);
	if (f->filename) free(f->filename);
	free(f);
}

// Trim trailing spaces, and duplicate string.
static char* imui__strtrimdup(const char* s)
{
	int len = strlen(s);
	while (len > 0 && imui__isspace(s[len-1]))
		len--;
	if (len <= 0)
		return NULL;
	char* str = (char*)malloc(len+1);
	if (str == NULL) return NULL;
	memcpy(str, s, len);
	str[len] = '\0';
	return str;
}

static void imui__pushBox(struct IMUIparser* p, struct IMUIbox* box)
{
	if (p->nboxstack < IMUI_MAX_LAYOUT_NESTING) {
		p->boxstack[p->nboxstack] = box;
		p->nboxstack++;
	}
}

static struct IMUIbox* imui__popBox(struct IMUIparser* p)
{
	struct IMUIbox* box = NULL;
	if (p->nboxstack > 0) {
		box = p->boxstack[p->nboxstack-1];
		p->nboxstack--;
	}
	return box;
}

static struct IMUIbox* imui__topBox(struct IMUIparser* p)
{
	if (p->nboxstack > 0)
		return p->boxstack[p->nboxstack-1];
	return NULL;
}

static int imui__parseTemplate(struct IMUIparser* p, const char** attr, int nattr)
{
	struct IMUItemplate* tmpl = imui__allocTemplate(p);
	if (tmpl == NULL) return 0;
	int i;
	for (i = 0; i < nattr*2; i += 2) {
		if (strcmp(attr[i], "id") == 0)
			tmpl->id = imui__strtrimdup(attr[i + 1]);
	}
	tmpl->next = p->layout->templates;
	p->layout->templates = tmpl;
	tmpl->layout = p->layout;

	p->curTemplate = tmpl;

	return 1;
}

static float imui__parseSize(struct IMUIparser* p, const char* s)
{
	if (strcmp(s, "auto") == 0) {
		return IMUI_AUTO;
	} else if (strcmp(s, "none") == 0) {
		return IMUI_NONE;
	}
	char units[32];
	float size = 0.0f;
	sscanf(s, "%f%s", &size, units);
	printf("units='%s'\n", units);
	if (units[0] == '%') {
		// percents, store as [0-0.1] to make it easier to distinguish 100% and 1px.  
		return imui__clampf(size / 1000.0f, 0.0f, 0.1f);
	} else if (strcmp(units, "em") == 0) {
		// ems
		return imui__maxf(1.0f, size * p->emSize);
	} else {
		// pixels
		return imui__maxf(1.0f, size);
	}
	return size;
}

static unsigned char imui__parsePackAlign(const char* s)
{
	if (strncmp(s, "sta", 3) == 0)
		return IMUI_START;
	else if (strncmp(s, "end", 3) == 0)
		return IMUI_END;
	else if (strncmp(s, "cen", 3) == 0)
		return IMUI_CENTER;
	else if (strncmp(s, "jus", 3) == 0)
		return IMUI_JUSTIFY;
	return IMUI_START;
}

static struct IMUIstyle* imui__findStyle(struct IMUIlayout* layout, int type, const char* name)
{
	struct IMUIstyle* style;
	for (style = layout->styles; style; style = style->next) {
		if (style->type == type && strcmp(style->name, name) == 0)
			return style;
	}
	return NULL;
}

static void imui__parseBoxClasses(struct IMUIparser* p, struct IMUIbox* box, const char* s)
{
	struct IMUIstyle* style;
	char styleName[64];
	int n;
	while (*s) {
		// Skip white spaces
		while (*s && imui__isspace(*s)) s++;
		// Store string
		n = 0;
		while (*s && !imui__isspace(*s) && n < 64)
			styleName[n++] = *s++;
		if (n == 0) break;
		styleName[n++] = '\0';
		// Process style
		style = imui__findStyle(p->layout, IMUI_STYLE_CLASS, styleName);
		if (style != NULL) {
			if (box->nstyles < IMUI_MAX_BOX_STYLES)
				box->styles[box->nstyles++] = style;
			else
				printf("%s:%d: warning: <%s> Too many styles per box %d (max %d)\n", p->name, imui__getLine(p, s), box->type, box->nstyles, IMUI_MAX_BOX_STYLES);
		}
	}
}

static int imui__parseBox(struct IMUIparser* p, const char* el, const char** attr, int nattr, unsigned char dir)
{
	struct IMUIbox* parent;
	struct IMUIbox* box;
	struct IMUIstyle* style;

	if (p->curTemplate == NULL) {
		printf("%s:%d: error: <%s> Elements must be placed in templates.\n", p->name, imui__getLine(p, el), el);
		return 0;
	}

	box = imui__allocBox(p, el);
	if (box == NULL) {
		printf("%s:%d: error: <%s> Failed to allocate box.\n", p->name, imui__getLine(p, el), el);
		return 0;
	}

	box->dir = dir;

	// Add default element style
	style = imui__findStyle(p->layout, IMUI_STYLE_ELEM, box->type);
	if (style != NULL)
		box->styles[box->nstyles++] = style;

	// Parse common attributes.
	int i, n;
	for (i = 0; i < nattr*2; i += 2) {
		if (strcmp(attr[i], "id") == 0) {
			box->id = imui__strtrimdup(attr[i + 1]);
		} else if (strcmp(attr[i], "width") == 0) {
			box->elementSize.x = imui__parseSize(p, attr[i+1]);
		} else if (strcmp(attr[i], "height") == 0) {
			box->elementSize.y = imui__parseSize(p, attr[i+1]);
		} else if (strcmp(attr[i], "pack") == 0) {
			box->pack = imui__parsePackAlign(attr[i+1]);
		} else if (strcmp(attr[i], "align") == 0) {
			box->align = imui__parsePackAlign(attr[i+1]);
		} else if (strcmp(attr[i], "alignSelf") == 0) {
			box->alignSelf = imui__parsePackAlign(attr[i+1]);
		} else if (strcmp(attr[i], "grow") == 0) {
			sscanf(attr[i+1], "%d", &n);
			box->grow = (unsigned char)n;
		} else if (strcmp(attr[i], "shrink") == 0) {
			sscanf(attr[i+1], "%d", &n);
			box->shrink = (unsigned char)n;
		} else if (strcmp(attr[i], "src") == 0) {
			box->src = imui__strtrimdup(attr[i+1]);
		} else if (strcmp(attr[i], "text") == 0) {
			if (box->text) free(box->text);
			box->text = imui__strtrimdup(attr[i+1]);
		} else if (strcmp(attr[i], "class") == 0) {
			imui__parseBoxClasses(p, box, attr[i+1]);
		} else {
			printf("%s:%d: warning: <%s> Invalid attribute: %s.\n", p->name, imui__getLine(p, el), el, attr[i]);
		}
	}

	// Add named style last
	if (box->id != NULL) {
		style = imui__findStyle(p->layout, IMUI_STYLE_ID, box->id);
		if (style != NULL)
			box->styles[box->nstyles++] = style;
	}

	parent = imui__topBox(p);
	if (parent == NULL) {
		if (p->curTemplate == NULL) {
			printf("%s:%d: error: <%s> Element started outside template.\n", p->name, imui__getLine(p, el), el);
			return 0;
		}
		p->curTemplate->root = box;
	} else {
		// Add to end of the items list.
		if (parent->items == NULL) {
			parent->items = box;
		} else {
			struct IMUIbox* child;
			for (child = parent->items; child != NULL; child = child->next) {
				if (child->next == NULL) {
					child->next = box;
					break;
				}
			}
		}
		box->parent = parent;
	}

	imui__pushBox(p, box);

	return 1;
}

static int imui__iselem(const char* el) 
{
	if (strcmp(el, "win") == 0) return 1;
	if (strcmp(el, "col") == 0) return 1;
	if (strcmp(el, "row") == 0) return 1;
	if (strcmp(el, "header") == 0) return 1;
	if (strcmp(el, "icon") == 0) return 1;
	if (strcmp(el, "label") == 0) return 1;
	if (strcmp(el, "spacer") == 0) return 1;
	if (strcmp(el, "button") == 0) return 1;
	if (strcmp(el, "input") == 0) return 1;
	if (strcmp(el, "field") == 0) return 1;
	if (strcmp(el, "item") == 0) return 1;
	if (strcmp(el, "img") == 0) return 1;
	if (strcmp(el, "template") == 0) return 1;
	return 0;
}

static int imui__blockCSS(void* ud, char* block)
{
	struct IMUIparser* p = (struct IMUIparser*)ud;
	int type, state;
	char* s;

	p->curFont = NULL;
	p->curStyle = NULL;

	if (block[0] == '@') {
		// Keyword
		if (strcmp(block, "@font-face") == 0) {
			struct IMUIfontFace* font = imui__allocFont(p);
			font->next = p->layout->fonts;
			p->layout->fonts = font;
			p->curFont = font;
		} else {
			printf("%s:%d: error: Keyword <%s> not recognized.\n", p->name, imui__getLine(p, block), block);
			return 0;
		}
	} else {
		// Style for element of class.
		if (block[0] == '.') {
			type = IMUI_STYLE_CLASS;
			block++; // skip .
		} else if (block[0] == '#') {
			type = IMUI_STYLE_ID;
			block++; // skip #
		} else {
			type = IMUI_STYLE_ELEM;
			if (!imui__iselem(block)) {
				printf("%s:%d: error: Element <%s> not recognized, if you mean class, start the name with period (.) .\n", p->name, imui__getLine(p, block), block);
				return 0;
			}
		}

		// Check for states
		state = IMUI_STATE_NORMAL;
		for (s = block; *s; s++) {
			if (*s == ':') {
				*s = '\0';
				s++;
				if (strcmp(s, "hover") == 0)
					state = IMUI_STATE_HOVER;
				else if (strcmp(s, "active") == 0)
					state = IMUI_STATE_ACTIVE;
				else if (strcmp(s, "focus") == 0)
					state = IMUI_STATE_FOCUS;
				break;
			}
		}

		// Use existing style if possible.
		struct IMUIstyle* style = NULL;
		style = imui__findStyle(p->layout, type, block);
		if (style == NULL) {
			// Create new style.
			style = imui__allocStyle(p);
			if (style == NULL) return 0;
			style->type = type;
			style->name = imui__strtrimdup(block);
			style->next = p->layout->styles;
			p->layout->styles = style;
		}
		p->curStyle = style;
		p->curStyleState = state;
	}

	return 1;
}

static char* imui__getVar(struct IMUIparser* p, const char* key)
{
	struct IMUIvar* v;
	for (v = p->vars; v != NULL; v = v->next) {
		if (strcmp(v->key, key) == 0)
			return v->value;
	}
	return NULL;
}

static int imui__parseFontStyle(struct IMUIparser* p, const char* val)
{
	if (strcmp(val, "normal") == 0) {
		return IMUI_REGULAR;
	} else if (strcmp(val, "italic") == 0) {
		return IMUI_ITALIC;
	} else if (strcmp(val, "oblique") == 0) {
		return IMUI_ITALIC; // sorry!
	}
	printf("%s:%d: warning: Invalid font style '%s'.\n", p->name, imui__getLine(p, val), val);
	return IMUI_REGULAR;
}

static int imui__parseFontWeight(struct IMUIparser* p, const char* val)
{
	int w = -1;
	if (strcmp(val, "bold") == 0)
		w = 700;
	else if (strcmp(val, "normal") == 0)
		w = 400;
	else
		sscanf(val, "%d", &w);
	if (w != -1)
		return w;
	printf("%s:%d: warning: Invalid font weight '%s'.\n", p->name, imui__getLine(p, val), val);
	return 0;
}

static float imui__parseSizePx(struct IMUIparser* p, const char* val)
{
	char units[32];
	float size = 0.0f;
	sscanf(val, "%f%s", &size, units);
	if (strcmp(units, "px") == 0) {
		return size;
	}
	printf("%s:%d: warning: Invalid size '%s' expect units in pixels.\n", p->name, imui__getLine(p, val), val);
	return 0.0f;
}

static void imui__cpToUTF8(unsigned int cp, char* str)
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
}

static char* imui__parseContent(struct IMUIparser* p, const char* val)
{
	// For now, expect the content to be a hex code point
	if (val[0] == '\\') {
		unsigned int cp = 0;
		sscanf(val+1, "u%x", &cp);
		if (cp != 0xffffffff) {
			char* ret = (char*)malloc(7);
			imui__cpToUTF8(cp, ret);
			return ret;
		}
	}
	return NULL;
}

static void imui__parseMargin(struct IMUIparser* p, char** vals, int nvals, float* margin)
{
	if (nvals == 1) {
		margin[0] = margin[1] = margin[2] = margin[3] = imui__parseSizePx(p, vals[0]);
	} else if (nvals == 2) {
		margin[0] = margin[2] = imui__parseSizePx(p, vals[0]);
		margin[1] = margin[3] = imui__parseSizePx(p, vals[1]);
	} else if (nvals == 3) {
		margin[0] = imui__parseSizePx(p, vals[0]);
		margin[1] = margin[3] = imui__parseSizePx(p, vals[1]);
		margin[2] = imui__parseSizePx(p, vals[2]);
	} else if (nvals == 4) {
		margin[0] = imui__parseSizePx(p, vals[0]);
		margin[1] = imui__parseSizePx(p, vals[1]);
		margin[2] = imui__parseSizePx(p, vals[2]);
		margin[3] = imui__parseSizePx(p, vals[3]);
	}
}

static unsigned int imui__parseColor(struct IMUIparser* p, const char* val)
{
	if (val[0] == '#') {
		// Handle #rrggbb and # rgb
		unsigned int c;
		int n = strlen(val+1);
		if (n == 6) {
			sscanf(val+1, "%x", &c);
		} else if (n == 3) {
			unsigned int c = 0;
			sscanf(val+1, "%x", &c);
			c = (c&0xf) | ((c&0xf0) << 4) | ((c&0xf00) << 8);
			c |= c<<4;
		}
		return c;
	} else if (val[0] == 'r' && val[1] == 'g' && val[2] == 'b' && val[3] == 'a' && val[4] == '(') {
		// Handle rgba(r,g,b,a)
		int r = -1, g = -1, b = -1;
		float a;
		char s1[32]="", s2[32]="", s3[32]="";
		sscanf(val+5, "%d%[%%, \t]%d%[%%, \t]%d%[%%, \t]%f", &r, s1, &g, s2, &b, s3, &a);
		if (strchr(s1, '%')) {
			return imui__RGBA((r*255)/100, (g*255)/100, (b*255)/100, (int)(a*255.0f));
		} else {
			return imui__RGBA(r, g, b, (int)(a*255.0f));
		}
	} else if (val[0] == 'r' && val[1] == 'g' && val[2] == 'b' && val[3] == '(') {
		// Handle rgb(r,g,b)
		int r = -1, g = -1, b = -1;
		char s1[32]="", s2[32]="";
		sscanf(val+4, "%d%[%%, \t]%d%[%%, \t]%d", &r, s1, &g, s2, &b);
		if (strchr(s1, '%')) {
			return imui__RGBA((r*255)/100, (g*255)/100, (b*255)/100, 255);
		} else {
			return imui__RGBA(r, g, b, 255);
		}
	}
	return 0;
}

static void imui__addProp(struct IMUIstyle* style, struct IMUIprop* prop)
{
	struct IMUIprop* p = style->props;
	// Append at end
	while (p != NULL) {
		if (p->next == NULL) {
			p->next = prop;
			return;
		}
		p = p->next;
	}
	// First item
	style->props = prop;
}

static void imui__addPropInt(struct IMUIstyle* style, unsigned char state, unsigned char cmd, int val)
{
	struct IMUIprop* prop = (struct IMUIprop*)malloc(sizeof(struct IMUIprop));
	if (prop == NULL) return;
	prop->cmd = cmd;
	prop->state = state;
	prop->ival = val;
	prop->next = NULL;
	imui__addProp(style, prop);
}

static void imui__addPropUint(struct IMUIstyle* style, unsigned char state, unsigned char cmd, unsigned int val)
{
	struct IMUIprop* prop = (struct IMUIprop*)malloc(sizeof(struct IMUIprop));
	if (prop == NULL) return;
	prop->cmd = cmd;
	prop->state = state;
	prop->uval = val;
	prop->next = NULL;
	imui__addProp(style, prop);
}

static void imui__addPropFloat(struct IMUIstyle* style, unsigned char state, unsigned char cmd, float val)
{
	struct IMUIprop* prop = (struct IMUIprop*)malloc(sizeof(struct IMUIprop));
	if (prop == NULL) return;
	prop->cmd = cmd;
	prop->state = state;
	prop->fval = val;
	prop->next = NULL;
	imui__addProp(style, prop);
}

static void imui__addPropString(struct IMUIstyle* style, unsigned char state, unsigned char cmd, const char* str)
{
	int len, pad;
	struct IMUIprop* prop;
	if (str == NULL) return;
	len = strlen(str);
	pad = imui__maxi(0, (len+1)-4);
	prop = (struct IMUIprop*)malloc(sizeof(struct IMUIprop)+pad);
	if (prop == NULL) return;
	prop->cmd = cmd;
	prop->state = state;
	strcpy(prop->str, str);
	prop->next = NULL;
	imui__addProp(style, prop);
}


static void imui__resetComputedStyle(struct IMUIcomputedStyle* style)
{
	style->fontId = -1;
	style->fontFamily = NULL;
	style->fontStyle = IMUI_REGULAR;
	style->fontWeight = 400;
	style->fontSize = 16.0f;

	style->contentColor = imui__RGBA(0,0,0,255);
	style->backgroundColor = imui__RGBA(0,0,0,0);
	style->outlineColor = imui__RGBA(0,0,0,255);
	style->outlineWidth = 0.0f;

	style->width = IMUI_NONE;
	style->height = IMUI_NONE;

	style->padding[0] = 0;
	style->padding[1] = 0;
	style->padding[2] = 0;
	style->padding[3] = 0;

	style->margin[0] = 0;
	style->margin[1] = 0;
	style->margin[2] = 0;
	style->margin[3] = 0;
	
	style->radius[0] = 0;
	style->radius[1] = 0;
	style->radius[2] = 0;
	style->radius[3] = 0;

	style->content = NULL;

	style->textShadow.offset[0] = 0;
	style->textShadow.offset[1] = 0;
	style->textShadow.blur = 0;
	style->textShadow.spread = 0;
	style->textShadow.color = imui__RGBA(0,0,0,128);

	style->boxShadow.offset[0] = 0;
	style->boxShadow.offset[1] = 0;
	style->boxShadow.blur = 0;
	style->boxShadow.spread = 0;
	style->boxShadow.color = imui__RGBA(0,0,0,128);
}

static void imui__inheritComputedStyle(struct IMUIcomputedStyle* style, struct IMUIcomputedStyle* parent)
{
	style->fontId = parent->fontId;
	style->fontFamily = parent->fontFamily;
	style->fontStyle = parent->fontStyle;
	style->fontWeight = parent->fontWeight;
	style->fontSize = parent->fontSize;
	style->contentColor = parent->contentColor;
}

static void imui__applyProperties(struct IMUIcomputedStyle* style, struct IMUIprop* props)
{
	unsigned char states[] = { IMUI_STATE_NORMAL, IMUI_STATE_HOVER, IMUI_STATE_ACTIVE, IMUI_STATE_FOCUS };
	struct IMUIprop* prop;
	int i;
	for (i = 0; i < 4; i++) {
		for (prop = props; prop != NULL; prop = prop->next) {
			if (prop->state != states[i])
				continue;
			switch(prop->cmd) {
			case IMUI_PROP_FONT_FAMILY:		style->fontFamily = prop->str; break;
			case IMUI_PROP_FONT_STYLE:		style->fontStyle = prop->ival; break;
			case IMUI_PROP_FONT_WEIGHT:		style->fontWeight = prop->ival; break;
			case IMUI_PROP_FONT_SIZE:		style->fontSize = prop->fval; break;
			case IMUI_PROP_CONTENT_COLOR:	style->contentColor = prop->uval; break;
			case IMUI_PROP_BACKGROUND_COLOR:style->backgroundColor = prop->uval; break;
			case IMUI_PROP_OUTLINE_COLOR:	style->outlineColor = prop->uval; break;
			case IMUI_PROP_OUTLINE_WIDTH:	style->outlineWidth = prop->fval; break;
			case IMUI_PROP_WIDTH:			style->width = prop->fval; break;
			case IMUI_PROP_HEIGHT:			style->height = prop->fval; break;
			case IMUI_PROP_CONTENT:			style->content = prop->str; break;
			case IMUI_PROP_PADDING_TOP:		style->padding[0] = prop->fval; break;
			case IMUI_PROP_PADDING_RIGHT:	style->padding[1] = prop->fval; break;
			case IMUI_PROP_PADDING_BOTTOM:	style->padding[2] = prop->fval; break;
			case IMUI_PROP_PADDING_LEFT:	style->padding[3] = prop->fval; break;
			case IMUI_PROP_MARGIN_TOP:		style->margin[0] = prop->fval; break;
			case IMUI_PROP_MARGIN_RIGHT:	style->margin[1] = prop->fval; break;
			case IMUI_PROP_MARGIN_BOTTOM:	style->margin[2] = prop->fval; break;
			case IMUI_PROP_MARGIN_LEFT:		style->margin[3] = prop->fval; break;
			case IMUI_PROP_RADIUS_TOPLEFT:	style->radius[0] = prop->fval; break;
			case IMUI_PROP_RADIUS_TOPRIGHT:	style->radius[1] = prop->fval; break;
			case IMUI_PROP_RADIUS_BOTRIGHT:	style->radius[2] = prop->fval; break;
			case IMUI_PROP_RADIUS_BOTLEFT:	style->radius[3] = prop->fval; break;
			case IMUI_PROP_TEXTSHADOW_OX:	style->textShadow.offset[0] = prop->fval; break;
			case IMUI_PROP_TEXTSHADOW_OY:	style->textShadow.offset[1] = prop->fval; break;
			case IMUI_PROP_TEXTSHADOW_BLUR:	style->textShadow.blur = prop->fval; break;
			case IMUI_PROP_TEXTSHADOW_COLOR:style->textShadow.color = prop->uval; break;
			case IMUI_PROP_BOXSHADOW_OX:	style->boxShadow.offset[0] = prop->fval; break;
			case IMUI_PROP_BOXSHADOW_OY:	style->boxShadow.offset[1] = prop->fval; break;
			case IMUI_PROP_BOXSHADOW_BLUR:	style->boxShadow.blur = prop->fval; break;
			case IMUI_PROP_BOXSHADOW_SPREAD:style->boxShadow.spread = prop->fval; break;
			case IMUI_PROP_BOXSHADOW_COLOR:	style->boxShadow.color = prop->uval; break;
			}
		}
	}
}


static int imui__propertyCSS(void* ud, char* key, char** vals, int nvals)
{
	struct IMUIparser* p = (struct IMUIparser*)ud;
	struct IMUIstyle* style = p->curStyle;
	int state = p->curStyleState;
	int i;

	// Substitute values with variables as needed.
	for (i = 0; i < nvals; i++) {
		if (vals[i][0] == '$') {
			char* vv = imui__getVar(p, vals[i]);
			if (vv == NULL) {
				printf("%s:%d: error: Variable '%s' is undefined.\n", p->name, imui__getLine(p, vals[i]), vals[i]);
				return 0;
			} 
			vals[i] = vv;
		}
	}

	if (style != NULL) {

/*
		printf("key=%s vals=", key);
		for (i = 0; i < nvals; i++)
			printf("%s ", vals[i]);
		printf("\n");
*/

		if (strcmp(key, "font") == 0) {
			// font: 15px Family
			// font: italic bold 15px Family
			if (nvals == 2) {
				imui__addPropFloat(style, state, IMUI_PROP_FONT_SIZE, imui__parseSizePx(p, vals[0]));
				imui__addPropString(style, state, IMUI_PROP_FONT_FAMILY, vals[1]);
			} else if (nvals == 3) {
				imui__addPropInt(style, state, IMUI_PROP_FONT_WEIGHT, imui__parseFontWeight(p, vals[0]));
				imui__addPropFloat(style, state, IMUI_PROP_FONT_SIZE, imui__parseSizePx(p, vals[1]));
				imui__addPropString(style, state, IMUI_PROP_FONT_FAMILY, vals[2]);
			} else if (nvals == 4) {
				imui__addPropInt(style, state, IMUI_PROP_FONT_STYLE, imui__parseFontWeight(p, vals[0]));
				imui__addPropInt(style, state, IMUI_PROP_FONT_WEIGHT, imui__parseFontWeight(p, vals[1]));
				imui__addPropFloat(style, state, IMUI_PROP_FONT_SIZE, imui__parseSizePx(p, vals[2]));
				imui__addPropString(style, state, IMUI_PROP_FONT_FAMILY, vals[3]);
			} else {
				printf("%s:%d: error: Property %s assumes 1, 2, 3, or 4 values, got %d.\n", p->name, imui__getLine(p, key), key, nvals);
				return 0;
			}
		} else if (strcmp(key, "font-family") == 0) {
			if (nvals == 1)
				imui__addPropString(style, state, IMUI_PROP_FONT_FAMILY, vals[0]);
		} else if (strcmp(key, "font-weight") == 0) {
			if (nvals == 1)
				imui__addPropInt(style, state, IMUI_PROP_FONT_WEIGHT, imui__parseFontWeight(p, vals[0]));
		} else if (strcmp(key, "font-size") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_FONT_SIZE, imui__parseSizePx(p, vals[0]));
		} else if (strcmp(key, "font-style") == 0) {
			if (nvals == 1)
				imui__addPropInt(style, state, IMUI_PROP_FONT_STYLE, imui__parseFontWeight(p, vals[0]));
		} else if (strcmp(key, "color") == 0) {
			if (nvals == 1)
				imui__addPropUint(style, state, IMUI_PROP_CONTENT_COLOR, imui__parseColor(p, vals[0]));
		} else if (strcmp(key, "outline") == 0) {
			if (nvals == 1) {
				if (strcmp(vals[0], "none") == 0) { // expect none
					imui__addPropFloat(style, state, IMUI_PROP_OUTLINE_WIDTH, 0);
					imui__addPropUint(style, state, IMUI_PROP_OUTLINE_COLOR, 0);
				}
			} else if (nvals == 2) {
				imui__addPropFloat(style, state, IMUI_PROP_OUTLINE_WIDTH, imui__parseSizePx(p, vals[0]));
				imui__addPropUint(style, state, IMUI_PROP_OUTLINE_COLOR, imui__parseColor(p, vals[1]));
			}
		} else if (strcmp(key, "outline-color") == 0) {
			if (nvals == 1)
				imui__addPropUint(style, state, IMUI_PROP_OUTLINE_COLOR, imui__parseColor(p, vals[0]));
		} else if (strcmp(key, "outline-width") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_OUTLINE_WIDTH, imui__parseSizePx(p, vals[0]));
		} else if (strcmp(key, "background") == 0) {
			if (nvals == 1)
				imui__addPropUint(style, state, IMUI_PROP_BACKGROUND_COLOR, imui__parseColor(p, vals[0]));
		} else if (strcmp(key, "width") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_WIDTH, imui__parseSize(p, vals[0]));
		} else if (strcmp(key, "height") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_HEIGHT, imui__parseSize(p, vals[0]));
		} else if (strcmp(key, "padding") == 0) {
			float padding[4] = {0,0,0,0};
			imui__parseMargin(p, vals, nvals, padding);
			imui__addPropFloat(style, state, IMUI_PROP_PADDING_TOP, padding[0]);
			imui__addPropFloat(style, state, IMUI_PROP_PADDING_RIGHT, padding[1]);
			imui__addPropFloat(style, state, IMUI_PROP_PADDING_BOTTOM, padding[2]);
			imui__addPropFloat(style, state, IMUI_PROP_PADDING_RIGHT, padding[3]);
		} else if (strcmp(key, "padding-top") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_PADDING_TOP, imui__parseSizePx(p, vals[0]));
		} else if (strcmp(key, "padding-right") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_PADDING_RIGHT, imui__parseSizePx(p, vals[0]));
		} else if (strcmp(key, "padding-bottom") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_PADDING_BOTTOM, imui__parseSizePx(p, vals[0]));
		} else if (strcmp(key, "padding-left") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_PADDING_LEFT, imui__parseSizePx(p, vals[0]));
		} else if (strcmp(key, "margin") == 0) {
			float margin[4] = {0,0,0,0};
			imui__parseMargin(p, vals, nvals, margin);
			imui__addPropFloat(style, state, IMUI_PROP_MARGIN_TOP, margin[0]);
			imui__addPropFloat(style, state, IMUI_PROP_MARGIN_RIGHT, margin[1]);
			imui__addPropFloat(style, state, IMUI_PROP_MARGIN_BOTTOM, margin[2]);
			imui__addPropFloat(style, state, IMUI_PROP_MARGIN_RIGHT, margin[3]);
		} else if (strcmp(key, "margin-top") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_MARGIN_TOP, imui__parseSizePx(p, vals[0]));
		} else if (strcmp(key, "margin-right") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_MARGIN_RIGHT, imui__parseSizePx(p, vals[0]));
		} else if (strcmp(key, "margin-bottom") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_MARGIN_BOTTOM, imui__parseSizePx(p, vals[0]));
		} else if (strcmp(key, "margin-left") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_MARGIN_LEFT, imui__parseSizePx(p, vals[0]));
		} else if (strcmp(key, "content") == 0) {
			if (nvals == 1) {
				char* content = imui__parseContent(p, vals[0]);
				imui__addPropString(style, state, IMUI_PROP_CONTENT, content);
				free(content);
			}
		} else if (strcmp(key, "text-shadow") == 0) {
			// text-shadow: h-shadow v-shadow blur color;
			if (nvals == 1) {
				if (strcmp(vals[0], "none") == 0) { // expect none
					imui__addPropFloat(style, state, IMUI_PROP_TEXTSHADOW_OX, 0);
					imui__addPropFloat(style, state, IMUI_PROP_TEXTSHADOW_OY, 0);
					imui__addPropFloat(style, state, IMUI_PROP_TEXTSHADOW_BLUR, 0);
					imui__addPropUint(style, state, IMUI_PROP_TEXTSHADOW_COLOR, 0);
				}
			} else if (nvals == 4) {
				imui__addPropFloat(style, state, IMUI_PROP_TEXTSHADOW_OX, imui__parseSizePx(p, vals[0]));
				imui__addPropFloat(style, state, IMUI_PROP_TEXTSHADOW_OY, imui__parseSizePx(p, vals[1]));
				imui__addPropFloat(style, state, IMUI_PROP_TEXTSHADOW_BLUR, imui__parseSizePx(p, vals[2]));
				imui__addPropUint(style, state, IMUI_PROP_TEXTSHADOW_COLOR, imui__parseColor(p, vals[3]));
			}
		} else if (strcmp(key, "box-shadow") == 0) {
			if (nvals == 1) {
				if (strcmp(vals[0], "none") == 0) { // expect none
					imui__addPropFloat(style, state, IMUI_PROP_BOXSHADOW_OX, 0);
					imui__addPropFloat(style, state, IMUI_PROP_BOXSHADOW_OY, 0);
					imui__addPropFloat(style, state, IMUI_PROP_BOXSHADOW_BLUR, 0);
					imui__addPropFloat(style, state, IMUI_PROP_BOXSHADOW_SPREAD, 0);
					imui__addPropUint(style, state, IMUI_PROP_BOXSHADOW_COLOR, 0);
				}
			} else if (nvals == 5) {
				imui__addPropFloat(style, state, IMUI_PROP_BOXSHADOW_OX, imui__parseSizePx(p, vals[0]));
				imui__addPropFloat(style, state, IMUI_PROP_BOXSHADOW_OY, imui__parseSizePx(p, vals[1]));
				imui__addPropFloat(style, state, IMUI_PROP_BOXSHADOW_BLUR, imui__parseSizePx(p, vals[2]));
				imui__addPropFloat(style, state, IMUI_PROP_BOXSHADOW_SPREAD, imui__parseSizePx(p, vals[2]));
				imui__addPropUint(style, state, IMUI_PROP_BOXSHADOW_COLOR, imui__parseColor(p, vals[3]));
			}
		} else if (strcmp(key, "border-radius") == 0) {
			float radius[4] = {0,0,0,0};
			imui__parseMargin(p, vals, nvals, radius); // same logic as margins
			imui__addPropFloat(style, state, IMUI_PROP_RADIUS_TOPLEFT, radius[0]);
			imui__addPropFloat(style, state, IMUI_PROP_RADIUS_TOPRIGHT, radius[1]);
			imui__addPropFloat(style, state, IMUI_PROP_RADIUS_BOTRIGHT, radius[2]);
			imui__addPropFloat(style, state, IMUI_PROP_RADIUS_BOTLEFT, radius[3]);
		} else if (strcmp(key, "border-top-left-radius") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_RADIUS_TOPLEFT, imui__parseSizePx(p, vals[0]));
		} else if (strcmp(key, "border-top-right-radius") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_RADIUS_TOPRIGHT, imui__parseSizePx(p, vals[0]));
		} else if (strcmp(key, "border-bottom-left-radius") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_RADIUS_BOTRIGHT, imui__parseSizePx(p, vals[0]));
		} else if (strcmp(key, "border-bottom-right-radius") == 0) {
			if (nvals == 1)
				imui__addPropFloat(style, state, IMUI_PROP_RADIUS_BOTLEFT, imui__parseSizePx(p, vals[0]));
		}

	} else if (p->curFont != NULL) {

		if (nvals != 1) {
			printf("%s:%d: error: Property %s assumes 1 value, got %d.\n", p->name, imui__getLine(p, key), key, nvals);
			return 0;
		}

		if (strcmp(key, "font-family") == 0) {
			p->curFont->family = imui__strtrimdup(vals[0]);
		} else if (strcmp(key, "font-weight") == 0) {
			p->curFont->weight = imui__parseFontWeight(p, vals[0]);
		} else if (strcmp(key, "font-style") == 0) {
			p->curFont->style = imui__parseFontStyle(p, vals[0]);
		} else if (strcmp(key, "src") == 0) {
			p->curFont->filename = imui__strtrimdup(vals[0]);
		} else {
			printf("%s:%d: error: Property %s not recognized.\n", p->name, imui__getLine(p, key), key);
			return 0;
		}
	} else {
		return 0;
	}

	return 1;
}

int imui__varCSS(void* ud, char* key, char* val)
{
	struct IMUIparser* p = (struct IMUIparser*)ud;

	struct IMUIvar* v = imui__allocVar(p);
	if (v == NULL) return 0;

	v->next = p->vars;
	p->vars = v;

	v->key = imui__strtrimdup(key);
	v->value = imui__strtrimdup(val);

	return 1;
}

static int imui__parseStyleBlock(struct IMUIparser* p, char* str)
{
	return imui__parseCSS(str, imui__blockCSS, imui__propertyCSS, imui__varCSS, (void*)p);
}

static int imui__startElement(void* ud, const char* el, const char** attr, int nattr)
{
	struct IMUIparser* p = (struct IMUIparser*)ud;
	int ret = 1;
//	printf("%*s<%s>", "", p->nboxstack*2, el);
//	printf("<%s>\n", el);

	if (p->nelstack < IMUI_MAX_LAYOUT_NESTING)
		p->elstack[p->nelstack++] = el;

	if (strcmp(el, "template") == 0) {
		if (p->curTemplate != NULL) {
			printf("%s:%d: error: <%s> Nested templates are not allowed.\n", p->name, imui__getLine(p, el), el);
			return 0;
		}
		ret = imui__parseTemplate(p, attr, nattr);
	} else if (strcmp(el, "style") == 0) {
/*		if (p->curStyle != NULL) {
			int line, col;
			imui_getLineCol(p, el, &line, &col);
			printf("%s:%d: error: <%s> Nested styles are not allowed.\n", p->name, line+1, el);
			return 0;
		}
		ret = imui__initStyleBlock(p, attr);*/
	} else if (strcmp(el, "win") == 0) {
		ret = imui__parseBox(p, el, attr, nattr, IMUI_COL);
	} else if (strcmp(el, "col") == 0) {
		ret = imui__parseBox(p, el, attr, nattr, IMUI_COL);
	} else if (strcmp(el, "row") == 0) {
		ret = imui__parseBox(p, el, attr, nattr, IMUI_ROW);
	} else if (strcmp(el, "header") == 0) {
		ret = imui__parseBox(p, el, attr, nattr, IMUI_ROW);
	} else if (strcmp(el, "icon") == 0) {
		ret = imui__parseBox(p, el, attr, nattr, IMUI_ROW);
	} else if (strcmp(el, "label") == 0) {
		ret = imui__parseBox(p, el, attr, nattr, IMUI_ROW);
	} else if (strcmp(el, "spacer") == 0) {
		ret = imui__parseBox(p, el, attr, nattr, IMUI_ROW);
	} else if (strcmp(el, "button") == 0) {
		ret = imui__parseBox(p, el, attr, nattr, IMUI_ROW);
	} else if (strcmp(el, "input") == 0) {
		ret = imui__parseBox(p, el, attr, nattr, IMUI_ROW);
	} else if (strcmp(el, "field") == 0) {
		ret = imui__parseBox(p, el, attr, nattr, IMUI_ROW);
	} else if (strcmp(el, "item") == 0) {
		ret = imui__parseBox(p, el, attr, nattr, IMUI_ROW);
	} else if (strcmp(el, "img") == 0) {
		ret = imui__parseBox(p, el, attr, nattr, IMUI_ROW);
	} else {
		printf("%s:%d: error: Unrecognized tag: %s.\n", p->name, imui__getLine(p, el), el);
		return 0;
	}
	return ret;
}

static int imui__endElement(void* ud, const char* el)
{
	struct IMUIparser* p = (struct IMUIparser*)ud;

//	printf("%*s</%s>", "", (p->nboxstack-1)*2, el);
//	printf("</%s>\n", el);

	if (p->nelstack > 0)
		p->nelstack--;

	if (strcmp(el, "template") == 0) {
		p->curTemplate = NULL;
	} else if (strcmp(el, "style") == 0) {
//		p->curStyle = NULL;
	} else {
		struct IMUIbox* top = imui__topBox(p);
		if (top == NULL) {
			printf("%s:%d: </%s> error: Tag end mismatch, no matching start tag.\n", p->name, imui__getLine(p, el), el);
			return 0;
		}
		if (strcmp(top->type, el) != 0) {
			printf("%s:%d: error: Ending tag <%s>, expecting </%s> instead.\n", p->name, imui__getLine(p, el), el, top->type);
			return 0;
		}
	
		if (strcmp(el, "win") == 0) {
			imui__popBox(p);
		} else if (strcmp(el, "col") == 0) {
			imui__popBox(p);
		} else if (strcmp(el, "row") == 0) {
			imui__popBox(p);
		} else if (strcmp(el, "header") == 0) {
			imui__popBox(p);
		} else if (strcmp(el, "icon") == 0) {
			imui__popBox(p);
		} else if (strcmp(el, "label") == 0) {
			imui__popBox(p);
		} else if (strcmp(el, "spacer") == 0) {
			imui__popBox(p);
		} else if (strcmp(el, "button") == 0) {
			imui__popBox(p);
		} else if (strcmp(el, "input") == 0) {
			imui__popBox(p);
		} else if (strcmp(el, "field") == 0) {
			imui__popBox(p);
		} else if (strcmp(el, "item") == 0) {
			imui__popBox(p);
		} else if (strcmp(el, "img") == 0) {
			imui__popBox(p);
		} else {
			printf("%s:%d: error: Unrecognized tag: %s.\n", p->name, imui__getLine(p, el), el);
			return 0;
		}
	}

	return 1;
}

static int imui__content(void* ud, char* s)
{
	struct IMUIparser* p = (struct IMUIparser*)ud;
	int ret = 1;
	const char* el = "";

	if (p->nelstack > 0)
		el = p->elstack[p->nelstack-1];

	if (strcmp(el, "template") == 0) {
		// skip
	} else if (strcmp(el, "style") == 0) {
		ret = imui__parseStyleBlock(p, s);
	} else {
		struct IMUIbox* box = imui__topBox(p);
		if (box != NULL)
			box->text = imui__strtrimdup(s);
	}

	return ret;
}

static void imui__deleteParser(struct IMUIparser* p)
{
	struct IMUIvar* v;
	if (p->layout != NULL)
		imuiDeleteLayout(p->layout);
	
	v = p->vars;
	while (v) {
		struct IMUIvar* next = v->next;
		imui__deleteVar(v);
		v = next;
	}

	free(p);
}

static struct IMUIparser* imui__createParser()
{
	struct IMUIparser* p = NULL;
	p = (struct IMUIparser*)malloc(sizeof(struct IMUIparser));
	if (p == NULL) goto error;
	memset(p, 0, sizeof(struct IMUIparser));

	p->layout = (struct IMUIlayout*)malloc(sizeof(struct IMUIlayout));
	if (p->layout == NULL) goto error;
	memset(p->layout, 0, sizeof(struct IMUIlayout));

	p->emSize = 28.f;	// TODO: parse?

	return p;
error:
	if (p != NULL)
		imui__deleteParser(p);
	return NULL;
}

struct IMUIlayout* imuiParseLayout(const char* name, char* input)
{
	struct IMUIparser* p;
	struct IMUIlayout* ret = NULL;
	
	p = imui__createParser();
	if (p == NULL) goto error;
	p->input = input;
	p->name = name;

	if (imui__parseXML(input, imui__startElement, imui__endElement, imui__content, p) == 0)
		goto error;

	ret = p->layout;
	p->layout = NULL;
	
	imui__deleteParser(p);

	return ret;

error:
	if (p != NULL) imui__deleteParser(p);
	return NULL;
}

struct IMUIlayout* imuiParseLayoutFromFile(const char* filename)
{
	FILE* fp = NULL;
	int size;
	char* data = NULL;
	struct IMUIlayout* layout = NULL;

	fp = fopen(filename, "rb");
	if (!fp) goto error;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	data = (char*)malloc(size+1);
	if (data == NULL) goto error;
	fread(data, size, 1, fp);
	data[size] = '\0';	// Must be null terminated.
	fclose(fp);
	layout = imuiParseLayout(filename, data);
	free(data);

	return layout;

error:
	if (fp) fclose(fp);
	if (data) free(data);
	if (layout) imuiDeleteLayout(layout);
	return NULL;
}

void imuiDeleteLayout(struct IMUIlayout* layout)
{
	struct IMUItemplate* tmpl;
	struct IMUIfontFace* font;
	struct IMUIstyle* style;

	tmpl = layout->templates;
	while (tmpl != NULL) {
		struct IMUItemplate* next = tmpl->next;
		imui__deleteTemplate(tmpl);
		tmpl = next;
	}

	style = layout->styles;
	while (style != NULL) {
		struct IMUIstyle* next = style->next;
		imui__deleteStyle(style);
		style = next;
	}

	font = layout->fonts;
	while (font != NULL) {
		struct IMUIfontFace* next = font->next;
		imui__deleteFont(font);
		font = next;
	}

	free(layout);
}

struct IMUItemplate* imuiFindTemplate(struct IMUIlayout* layout, const char* id)
{
	struct IMUItemplate* tmpl;
	for (tmpl = layout->templates; tmpl != NULL; tmpl = tmpl->next) {
		if (strcmp(tmpl->id, id) == 0)
			return tmpl;
	}
	return NULL;
}

static int imui__childCount(struct IMUIbox* box)
{
	struct IMUIbox* it;
	int n = 0;
	for (it = box->items; it != NULL; it = it->next)
		n++;
	return n;
}

static void imui__updateMetrics(struct IMUIbox* box, char dir)
{
	if (dir == IMUI_COL) {
		box->mainSize = box->computedSize.y;
		box->crossSize = box->computedSize.x;
	} else {
		box->mainSize = box->computedSize.x;
		box->crossSize = box->computedSize.y;
	}
}

static float imui__calcPadding(struct IMUIbox* box, char dir)
{
	if (dir == IMUI_COL)
		return box->computedStyle.padding[1] + box->computedStyle.padding[3];
	else
		return box->computedStyle.padding[0] + box->computedStyle.padding[2];
}

static char imui__crossDir(char dir)
{
	return dir == IMUI_COL ? IMUI_ROW : IMUI_COL;
}

static void imui__calculateSizes(struct IMUIbox* box)
{
	struct IMUIbox* it;
	float ms, cs, percent;

	box->computedSize = box->requestedSize;

	// None matches parent
	if (box->parent != NULL) {
		if ((int)box->requestedSize.x == IMUI_NONE) {
	//		printf("%s none X -> (%s) %f\n", box->type, box->parent->type, box->parent->computedSize.x);
			box->computedSize.x = box->parent->computedSize.x;
		}
		if ((int)box->requestedSize.y == IMUI_NONE) {
	//		printf("%s none Y -> (%s) %f\n", box->type, box->parent->type, box->parent->computedSize.y);
			box->computedSize.y = box->parent->computedSize.y;
		}

		// Percent matches parent
		if (box->requestedSize.x >= 0.0f && box->requestedSize.x < 0.5f) {
			percent = box->requestedSize.x * 10.0f;
			printf("%s(%s) percent(%f) X -> %f\n", box->type, box->id, percent, box->parent->computedSize.x);
			box->computedSize.x = imui__maxf(1.0f, box->parent->computedSize.x * percent);
		}
		if (box->requestedSize.y >= 0.0f && box->requestedSize.y < 0.5f) {
			percent = box->requestedSize.y * 10.0f;
			printf("%s(%s) percent(%f) Y -> %f\n", box->type, box->id, percent, box->parent->computedSize.y);
			box->computedSize.y = imui__maxf(1.0f, box->parent->computedSize.y * percent);
		}
	}

	imui__updateMetrics(box, box->dir);
		
//	ms = cs = 0;
	if (box->dir == IMUI_COL) {
		cs = box->contentSize.x;
		ms = box->contentSize.y;
	} else {
		ms = box->contentSize.x;
		cs = box->contentSize.y;
	}

	for (it = box->items; it != NULL; it = it->next) {
		imui__calculateSizes(it);
		imui__updateMetrics(it, box->dir);
		ms += it->mainSize;
		cs = imui__maxf(cs, it->crossSize);
	}

	cs += imui__calcPadding(box, imui__crossDir(box->dir));
	ms += imui__calcPadding(box, box->dir);

	// Keep track of minimum content size.
	if (box->dir == IMUI_COL) {
		box->minimumSize.x = cs;
		box->minimumSize.y = ms;
	} else{
		box->minimumSize.x = ms;
		box->minimumSize.y = cs;
	}
	
	// Fit box to content.
	ms = imui__maxf(ms, box->mainSize);
	cs = imui__maxf(cs, box->crossSize);

	// Auto matches content
	if ((int)box->requestedSize.x == IMUI_AUTO) {
//		printf("%s auto X (%f %f)\n", box->type, cs, ms);
		if (box->dir == IMUI_COL)
			box->computedSize.x = cs; // + imui__calcPadding(box, imui__crossDir(box->dir));
		else
			box->computedSize.x = ms; // + imui__calcPadding(box, box->dir);
	}
	if ((int)box->requestedSize.y == IMUI_AUTO) {
//		printf("%s auto Y (%f %f)\n", box->type, cs, ms);

/*		for (it = box->items; it != NULL; it = it->next)
			printf("%f ",it->crossSize);
		printf("\n");*/

		if (box->dir == IMUI_COL)
			box->computedSize.y = ms + imui__calcPadding(box, box->dir);
		else
			box->computedSize.y = cs + imui__calcPadding(box, imui__crossDir(box->dir));
	}

	// Handle "stretch"
	cs = imui__maxf(1.0f, cs - imui__calcPadding(box, box->dir));
	for (it = box->items; it != NULL; it = it->next) {
		char align = box->align;
		if (it->alignSelf != IMUI_AUTO)
			align = it->alignSelf;
		// check for self alignment
		if (align == IMUI_JUSTIFY) {
			printf("<%s> justify %f\n", it->type, cs);
			if (box->dir == IMUI_COL)
				it->computedSize.x = cs;
			else
				it->computedSize.y = cs;
		}
	}
}

static void imui__growItems(struct IMUIbox* box)
{
	struct IMUIbox* it;
	float totalGrow = 0, totalShrink = 0, ms = 0, mainSize;

	imui__updateMetrics(box, box->dir);

	for (it = box->items; it != NULL; it = it->next) {
		imui__growItems(it);
		imui__updateMetrics(it, box->dir);
		totalGrow += it->grow;
		totalShrink += it->shrink;
		ms += it->mainSize;
	}
	
	mainSize = imui__maxf(1.0f, box->mainSize - imui__calcPadding(box, box->dir));

//	printf("%s grow tot=%f  ms=%f  mainSize=%f\n", box->type, totalGrow, ms, mainSize);

	// Grow
//	printf("GROW: <%s> tot=%f  (ms=%f  mainSize=%f)\n", box->type, totalGrow, ms, mainSize);

	if (totalGrow > 0.0f && ms < mainSize) {
		float space = mainSize - ms;
		for (it = box->items; it != NULL; it = it->next) {
			float itemMainSize = imui__maxf(1.0f, it->mainSize + (space * it->grow / totalGrow));
			if (box->dir == IMUI_COL) {
				it->computedSize.x = it->crossSize;
				it->computedSize.y = itemMainSize;
			} else {
				it->computedSize.x = itemMainSize;
				it->computedSize.y = it->crossSize;
			}
			it->requestedSize = it->computedSize;
			imui__calculateSizes(it);
			imui__growItems(it);
		}
		if (box->dir == IMUI_COL)
			box->minimumSize.y += space;
		else
			box->minimumSize.x += space;
	}

//	printf("%s shrink tot=%f  ms=%f  mainSize=%f\n", box->type, totalShrink, ms, mainSize);

	// Shrink
	if (totalShrink > 0.0f && ms > mainSize) {
		float shrinkSum = 0, space;
		for (it = box->items; it != NULL; it = it->next)
			shrinkSum += it->mainSize * it->shrink;
		space = ms - mainSize;
		for (it = box->items; it != NULL; it = it->next) {
			float itemMainSize;
			if (it->shrink == 0) continue;
			itemMainSize = imui__maxf(1.0f, it->mainSize - space * ((it->mainSize * it->shrink) / shrinkSum));

			if (box->dir == IMUI_COL) {
				it->computedSize.x = it->crossSize;
				it->computedSize.y = itemMainSize;
			} else {
				it->computedSize.x = itemMainSize;
				it->computedSize.y = it->crossSize;
			}
			it->requestedSize = it->computedSize;
			imui__calculateSizes(it);
			imui__growItems(it);
		}
		if (box->dir == IMUI_COL)
			box->minimumSize.y += space;
		else
			box->minimumSize.x += space;
	}
}

static void imui__packItems(struct IMUIbox* box)
{
	struct IMUIbox* it;
	int totalItems  = imui__childCount(box);
	float mainSize, ms = 0, cs = 0, spacing = 0;
	float availableMainSpace = 0;

	imui__updateMetrics(box, box->dir);

	mainSize = imui__maxf(1.0f, box->mainSize - imui__calcPadding(box, box->dir));
	
	if (box->dir == IMUI_COL)
		availableMainSpace = mainSize - box->minimumSize.y;
	else
		availableMainSpace = mainSize - box->minimumSize.x;
	
	switch (box->pack) {
		case IMUI_START:
			break;
		case IMUI_END:
			ms += availableMainSpace;
			break;
		case IMUI_CENTER:
			ms += availableMainSpace/2;
			break;
		case IMUI_JUSTIFY:
			if (totalItems > 1)
				spacing = availableMainSpace / (totalItems - 1);
			else
				ms = availableMainSpace / 2;
			break;
		default:
			break;
	}
	
//		ms += imui__calcPadding(box, box->dir);
//		cs += imui__calcPadding(box, imui__crossDir(box->dir));

	if (box->dir == IMUI_COL) {
		ms += box->computedStyle.padding[3];
		cs += box->computedStyle.padding[0];
	} else {
		ms += box->computedStyle.padding[0];
		cs += box->computedStyle.padding[3];
	}

	for (it = box->items; it != NULL; it = it->next) {
		imui__updateMetrics(it, box->dir);
		if (box->dir == IMUI_COL) {
			it->computedPosition.x = cs;
			it->computedPosition.y = ms;
		} else {
			it->computedPosition.x = ms;
			it->computedPosition.y = cs;
		}
		ms += it->mainSize;
		ms += spacing;
		imui__packItems(it);
	}
}

static void imui__alignItems(struct IMUIbox* box)
{
	struct IMUIbox* it;
	float crossSize;

	imui__updateMetrics(box, box->dir);	
	crossSize = imui__maxf(1.0f, box->crossSize - imui__calcPadding(box, imui__crossDir(box->dir)));

	for (it = box->items; it != NULL; it = it->next) {
		float availableCrossSpace;
		char align;
		imui__updateMetrics(it, box->dir);
		availableCrossSpace = imui__maxf(0.0f, crossSize - it->crossSize);
		align = box->align;
		// check for self alignment
		if (it->alignSelf != IMUI_AUTO)
			align = it->alignSelf;
		switch (align) {
			case IMUI_JUSTIFY:
				// Handled in calc size
				break;
			case IMUI_START:
				break;
			case IMUI_END:
				if (box->dir == IMUI_COL)
					it->computedPosition.x += availableCrossSpace;
				else
					it->computedPosition.y += availableCrossSpace;
				break;
			case IMUI_CENTER:
				if (box->dir == IMUI_COL)
					it->computedPosition.x += availableCrossSpace/2;
				else
					it->computedPosition.y += availableCrossSpace/2;
				break;
			default:
				break;
		}

		if (box->dir == IMUI_COL)
			it->computedPosition.x += box->computedStyle.padding[3];
		else
			it->computedPosition.y += box->computedStyle.padding[0];

		imui__alignItems(it);
	}
}

static void imui__calculatePositions(struct IMUIbox* box, struct IMUIpoint* pt)
{
	struct IMUIbox* it;
	box->computedPosition.x += pt->x;
	box->computedPosition.y += pt->y;
	for (it = box->items; it != NULL; it = it->next)
		imui__calculatePositions(it, &box->computedPosition);
}

static struct IMUIfontFace* imui__findFont(struct IMUIlayout* layout, const char* family, int style, int weight)
{
	struct IMUIfontFace* font;
	if (family == NULL) 
		return NULL;
	for (font = layout->fonts; font; font = font->next) {
		if (font->style == style && font->weight == weight && strcmp(font->family, family) == 0)
			return font;
	}
	return NULL;
}

static void imui__initSizes(struct IMUItemplate* tmpl, struct IMUIbox* box, int (*measureItem)(void* uptr, struct IMUIbox* box, float* width, float* height), void* uptr, int depth)
{
	struct IMUIbox* it;
	struct IMUIstyle* style;
	struct IMUIfontFace* font;
	int i;

	if (box->parent == NULL) {
		// Root inherits all from template
		imui__resetComputedStyle(&box->computedStyle);
		style = imui__findStyle(tmpl->layout, IMUI_STYLE_ELEM, "template");
		if (style != NULL)
			imui__applyProperties(&box->computedStyle, style->props);
	} else {
		imui__resetComputedStyle(&box->computedStyle);
		imui__inheritComputedStyle(&box->computedStyle, &box->parent->computedStyle);
	}

	box->computedStyle.width = box->elementSize.x;
	box->computedStyle.height = box->elementSize.y;
	for (i = 0; i < box->nstyles; i++)
		imui__applyProperties(&box->computedStyle, box->styles[i]->props);

	box->requestedSize.x = box->computedStyle.width;
	box->requestedSize.y = box->computedStyle.height;

//	printf("find: %s %d %d\n", box->computedStyle.fontFamily, box->computedStyle.fontStyle, box->computedStyle.fontWeight);
	font = imui__findFont(tmpl->layout, box->computedStyle.fontFamily, box->computedStyle.fontStyle, box->computedStyle.fontWeight);
	if (font != NULL)
		box->computedStyle.fontId = font->id;

	box->contentSize.x = 0;
	box->contentSize.y = 0;
	measureItem(uptr, box, &box->contentSize.x, &box->contentSize.y);

/*	printf("  - %s (%s): ", box->type, box->id);
	if (box->width == IMUI_AUTO)
		printf("auto");
	else if (box->width == IMUI_NONE)
		printf("none");
	else
		printf("%f", box->width);
	printf(" x ");
	if (box->height == IMUI_AUTO)
		printf("auto");
	else if (box->height == IMUI_NONE)
		printf("none");
	else
		printf("%f", box->height);
	printf("\n");*/

	printf("%*s- %s: con=(%f x %f) req=(%f x %f) pad=(%f %f %f %f)\n", depth*2, "", box->type, box->contentSize.x,box->contentSize.y, box->requestedSize.x,box->requestedSize.y, box->computedStyle.padding[0],box->computedStyle.padding[1],box->computedStyle.padding[2],box->computedStyle.padding[3]);

	for (it = box->items; it != NULL; it = it->next)
		imui__initSizes(tmpl, it, measureItem, uptr, depth+1);
}


void imuiComputeLayout(struct IMUItemplate* tmpl, float w, float h, int (*measureItem)(void* uptr, struct IMUIbox* box, float* width, float* height), void* uptr)
{
	struct IMUIpoint orig = {0,0};
	if (tmpl->root != NULL) {
		if (w > 0) tmpl->root->elementSize.x = w;
		if (h > 0) tmpl->root->elementSize.y = h;
	}

	printf("Sizes (%f,%f):\n", w, h);
	imui__initSizes(tmpl, tmpl->root, measureItem, uptr, 0);

	imui__calculateSizes(tmpl->root);
	imui__growItems(tmpl->root);
	imui__packItems(tmpl->root);
	imui__alignItems(tmpl->root);
	imui__calculatePositions(tmpl->root, &orig);
}

#endif