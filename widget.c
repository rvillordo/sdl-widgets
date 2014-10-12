/*
 * small SDL user interface api
 * developed in a rush for usage with rtl-sdr tools
 *
 * it was written all at once, so its a good idea to do a good clean up
 * at the code.
 *
 * this was extracted from the original code
 *
 * copyright (c) 2014, rafael villordo
 */

#define WID_VISIBLE 0
#define WID_ENABLE 20

#define BORDER_DARK_COLOR 0, 0, 0
#define BORDER_LIGHT_COLOR 240, 240, 240

#define INPUTTEXT_BACKGROUND_COLOR 205, 205, 205
#define INPUTTEXT_FOCUS_COLOR 255, 255, 255

#include <stdio.h>

#include <SDL2/SDL.h>

#include "rtl-sdr.h"
#include "image.h"
#include "widget.h"
#include "chargen.h"
#include "var.h"

double zFactor=23;
double z3d=10;
double y3d=400;

static int windowID = 0;

//extern struct output_state output;
//extern struct demod_state demod;
//extern struct controller_state controller;
//extern struct dongle_state dongle;

static char *arrowUp[] = 
{ 
	"000000000000000",
	"000000010000000",
	"000000111000000",
	"000001111100000",
	"000011111110000",
	"000111111111000",
	"000111111111000",
	"000000000000000",
	NULL
};

static char *arrowDown[] = 
{ 
	"000000000000000",
	"000111111111000",
	"000111111111000",
	"000011111110000",
	"000001111100000",
	"000000111000000",
	"000000010000000",
	"000000000000000",
	NULL
};

static char *arrowRight[] = 
{ 
	"000000000000000",
	"001111000000000",
	"001111111000000",
	"001111111111000",
	"001111111111000",
	"001111111000000",
	"001111000000000",
	"000000000000000",
	NULL
};

/* supported widgets */
enum {
	SUI_BUTTON	= 0,
	SUI_GRAPH,
	SUI_CHECKBUTTON,
	SUI_PROGRESSBAR,
	SUI_TOOLBUTTON,
	SUI_SLIDERBAR,
	SUI_INPUTTEXT,
	SUI_LABEL,
	SUI_FRAME,
	SUI_LIST,
	SUI_BOX
};

enum {
#if SDL_BYTEORDER == SDL_MSB_FIRST
	rmask = 0xff000000,
	gmask = 0x00ff0000,
	bmask = 0x0000ff00,
	amask = 0x00000000,
#else
	rmask = 0x000000ff,
	gmask = 0x0000ff00,
	bmask = 0x00ff0000,
	amask = 0x00000000,
#endif
};

window_t	*windowList[16];
int windowCnt = 1;

sdl_widget_t *widgetList[128];

sdl_widget_t *graph;

int widCnt = 0;
window_t *configwin;

enum {
	ALIGN_LEFT,
	ALIGN_RIGHT,
	ALIGN_TOP,
	ALIGN_BOTTOM
};

void sdl_widget_align(sdl_widget_t *w, sdl_widget_t *w1, int type, int space)
{
	switch(type)
	{
		case ALIGN_LEFT:
			w1->r.x = (w->r.x + w->r.w + space);
			w1->r.y = w->r.y;
			break;
		case ALIGN_RIGHT:
			w1->r.x = (w->r.x - w->r.w - space);
			w1->r.y = w->r.y;
			break;
		case ALIGN_TOP:
		case ALIGN_BOTTOM:
			w1->r.x = w->r.x;
			w1->r.y = (w->r.y + w->r.h + space);
			break;
	}
}

SDL_Rect sdl_rect(int x, int y, int w, int h)
{
	SDL_Rect r = { x, y, w, h };
	return (r);
}
#define FONT_6x10 0
#define FONT_6x12 1
#define FONT_7x13 2
#define FONT_7x14 3
sdl_font_t sdl_font(int font, int color, int bold, int underline)
{
	sdl_font_t f = { font, color, bold, underline };
	return f;
}

sdl_widget_t *sdl_widget_get_withid(int id)
{
	return wlist[id];
}

sdl_widget_t *sdl_widget(void *parent, char *name, int type, int x, int y, int w, int h, uint32_t flags)
{
	int i;
	static int ids = 0;

	sdl_widget_t *wid = malloc(sizeof(sdl_widget_t));
	if(wid==NULL) {
		perror("sdl_widget: malloc");
		return NULL;
	}
	wid->id = ids++;
	wlist[wid->id] = wid;

	if(name)
		snprintf(wid->name, sizeof(wid->name), "%s", name);
	else snprintf(wid->name, sizeof(wid->name), "widget%d",wid->id);

	wid->type = type;
	wid->r = sdl_rect(x, y, w, h);
	wid->flags = flags;

	wid->border = BORDER_OUT;

	wid->font = sdl_font(FONT_6x10, 32, 0, 0);

	wid->parent = parent;

	wid->dataptr = NULL;
	wid->text = NULL;
	wid->datasize = 0;

	wid->pixels = malloc(wid->r.w * wid->r.h);
	memset(wid->pixels, 0xFF, wid->r.w * wid->r.h);

	for(i = 0; i < MAX_EVENTS; i++)
		wid->events[i] = NULL;

	wid->widget = NULL;
	wid->widgets = 0;

	wid->dirty = 1;
	wid->visible = 1;
	wid->enable = 1;
	wid->check = 0;

	wid->flags |= WID_VISIBLE | WID_ENABLE;

	return (wid);
}

void sdl_widget_set_font(sdl_widget_t *wid, int font)
{
	wid->font.id = font;
}

void sdl_widget_set_fontColor(sdl_widget_t *wid, int fontColor)
{
	wid->font.color = fontColor;
}

void sdl_widget_set_fontBold(sdl_widget_t *wid, int bold)
{
	wid->font.bold = bold;
}

void sdl_widget_set_fontUnderline(sdl_widget_t *wid, int underline)
{
	wid->font.underline = underline;
}

void sdl_widget_set_dataptr(sdl_widget_t *wid, void *dataptr, int datasize)
{
	wid->dataptr = dataptr;
	wid->datasize = datasize;
}

void sdl_widget_set_size(sdl_widget_t *wid, int w, int h)
{
	wid->r.w = w;
	wid->r.h = h;
}

void sdl_widget_set_position(sdl_widget_t *wid, int x, int y)
{
	wid->r.x = x;
	wid->r.y = y;
}

void sdl_widget_set_parent(sdl_widget_t *wid, void *parent)
{
	wid->parent = parent;
}

void sdl_widget_set_callback(sdl_widget_t *wid, int ev, void (*cb)(void *))
{
	wid->events[ev] = cb;
}

int getidx(chargen_t *chargen, unsigned char c)
{
	int i;
	for(i = 0; chargen->data[i].sym != 0xFF; i++)
		if(chargen->data[i].sym == c)
			return i;
	return (-1);
}

void sdl_putpixel(sdl_widget_t *sdl, int x, int y, unsigned color)
{
	// remove this for extra speed, and if you are sure x and y are always valid
	//if ((x<0) || (x>(sdl->width-1)) || (y<0) || (y>(sdl->height - 1))) return;
	Uint8 *page_draw = (Uint8 *)sdl->pixels;
	page_draw[(sdl->r.w * y) + x + 0] = color;
}

void sdl_print_chargen(sdl_widget_t *widget, int x, int y, char *msg, int mlen, int opa, int bold);

void sdl_printf(sdl_widget_t *wid, int x, int y, int bold, const char *fmt, ...)
{
	va_list ap;
	char buf[8192];
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	sdl_print_chargen(wid, x, y, buf, strlen(buf), 0, bold);
	va_end(ap);
}

void sdl_print_chargen(sdl_widget_t *widget, int x, int y, char *msg, int mlen, int opa, int bold)
{
	int i, k;
	int sx = 0, sy = y;
	int idx;
	int yy,xx,l;
	int xsy;
	chargen_t *chargen = &fonts[widget->font.id];
	int lx=0;
	int scale_x = chargen->sx;
	int scale_y = chargen->sy;

	xx = x;
	xsy = (scale_y * chargen->h);

	for(i = 0; i < mlen; i++) {
		if((idx = getidx(chargen, msg[i])) == -1) {
			printf("bug\n");
			continue;
		}
		sy = y;
		for(yy = 1; yy < xsy; yy++, sy++) {
			sx = xx;
			for(k = (chargen->w + 1); k >= 0; k--, sx+=scale_x) {
				if((yy/scale_y) >= 15) {
					printf("PORRA CARALHO\n");
				}
				if((chargen->data[idx].byte[yy/scale_y] >> k) & 1) {
					sdl_putpixel(widget, sx, sy, widget->font.color);
					lx=1;
					if(bold) {
						sdl_putpixel(widget, sx+1, sy, widget->font.color);
					}
					if(scale_x > 1) {
						for(l = 1; l < scale_x; l++)
							sdl_putpixel(widget, sx+l, sy, widget->font.color);
					}
				} else if(opa){
					if(lx) {
						sdl_putpixel(widget, sx, sy, widget->font.color);

						lx=0;
					}
				} else {
				}
			} 
		}
		xx = sx;
	}
}

window_t *sdl_window_new(char *name, int x, int y, int w, int h, int flags)
{
	window_t *screen = malloc(sizeof(window_t));
	if(screen == NULL) {
		perror("sdl_window_new: malloc");
		return (NULL);
	}

	sprintf(screen->name, "%s", name);

	screen->x = x;
	screen->y = y;
	screen->w = w;
	screen->h = h;

	screen->dirty = 1;

	screen->window = SDL_CreateWindow(name, x, y, w, h, flags);
	screen->render = SDL_CreateRenderer(screen->window, -1, SDL_RENDERER_PRESENTVSYNC);

	screen->widget  = NULL;
	screen->widgets = 0;
	screen->focused = NULL;
	
	return (screen);
}

void sdl_window_add_widget(window_t *window, sdl_widget_t *widget)
{
	int id = window->widgets;
	window->widget = realloc(window->widget, (sizeof(sdl_widget_t *) * ++window->widgets));
	if(window->widget == NULL) {
		perror("sdl_window_add_widget: realloc");
		return;
	}
	window->widget[id] = widget;
	widget->window = window;
}

void sdl_widget_add_child(sdl_widget_t *window, sdl_widget_t *widget)
{
	int id = window->widgets;
	window->widget = realloc(window->widget, (sizeof(sdl_widget_t *) * ++window->widgets));
	if(window->widget == NULL) {
		perror("sdl_window_add_widget: realloc");
		return;
	}
	window->widget[id] = widget;
	widget->window = window->window;
	widget->parent = window;
}

void sdl_draw_border(SDL_Renderer *render, int x, int y, int w, int h, int type)
{
	switch(type) {
		case BORDER_FLAT:
		case BORDER_OUT:
			SDL_SetRenderDrawColor(render, BORDER_DARK_COLOR, 0xFF);
			break;
		case BORDER_IN:
			SDL_SetRenderDrawColor(render, BORDER_LIGHT_COLOR, 0xFF);
	}

	SDL_RenderDrawLine(render, x+w, y, x+w, y+h);
	SDL_RenderDrawLine(render, x, y+h, x+w, y+h);

	switch(type) {
		case BORDER_FLAT:
		case BORDER_IN:
			SDL_SetRenderDrawColor(render, BORDER_DARK_COLOR, 0xFF);
			break;
		case BORDER_OUT:
			SDL_SetRenderDrawColor(render, BORDER_LIGHT_COLOR, 0xFF);
	}
	SDL_RenderDrawLine(render, x, y, x, y+h);
	SDL_RenderDrawLine(render, x, y, x+w, y);
}

void sdl_draw_toolbutton(sdl_widget_t *widget, SDL_Renderer *render)
{
	int x, y, w, h;
	sdl_widget_t *p = widget->parent;
	SDL_Rect r;

	x = widget->r.x;// + p->r.x;
	y = widget->r.y;// + p->r.y;
	w = widget->r.w;
	h = widget->r.h;

	SDL_SetRenderDrawColor(render, BORDER_DARK_COLOR, 0xFF);
	sdl_draw(render, x-1, y+1, (unsigned char**)widget->dataptr);
	sdl_draw_border(render, x, y, w, h, widget->border);
}

void sdl_draw_button(sdl_widget_t *widget, SDL_Renderer *render)
{
	int x, y, w, h;
	sdl_widget_t *p = widget->parent;

	SDL_Rect r;

	x = widget->r.x;// + p->r.x;
	y = widget->r.y;// + p->r.y;
	w = widget->r.w;
	h = widget->r.h;

	r.x = x + 4;
	r.y = y + 4;
	r.h = 10;
	r.w = 5 * 6 + 2;

	int c=32;
	int rr, g, b;
	int xx, yy;
	
	int ww = strlen(widget->text) * fonts[widget->font.id].w;
	r.x = x + ((w/2) - (ww/2)) - 1; // + fonts[widget->font.id].w;
	memset(widget->pixels, 0xFF, w * h);
	sdl_draw_border(render, x, y, w, h, widget->border);
	sdl_print_chargen(widget, 0, 0, widget->text, strlen(widget->text), 0, 1);

	for(yy = 0, y = r.y; y < (r.y+h); y++, yy++) {
		for(xx=0,x = r.x;x < (r.x + w); x++,xx++) {
			SDL_SetRenderDrawColor(render,0, 0, 0, 0xFF);
			if(widget->pixels[(yy * w) + xx]  != 0xFF) {
				SDL_RenderDrawPoint(render, x, y);
			}
		}
	}
}

void sdl_draw_checkbutton(sdl_widget_t *widget, SDL_Renderer *render)
{
	int x, y, w, h;
	sdl_widget_t *p = widget->parent;

	SDL_Rect r;

	x = widget->r.x;
	y = widget->r.y;
	w = widget->r.w;
	h = widget->r.h;

	r.x = x + w;
	r.y = y;
	r.h = 10;
	r.w = 5 * 6 + 2;

	int c=32;
	int rr, g, b;
	int xx, yy;
	//get_color(32, &rr, &g, &b);
	
	int ww = strlen(widget->text) * 5;
	r.x = x + 12;
	sdl_draw_border(render, x, y, 8, 8, BORDER_IN);

	memset(widget->pixels, 0xFF, w * h);
	sdl_print_chargen(widget, 0, 0, widget->text, strlen(widget->text), 0, 1);
	SDL_SetRenderDrawColor(render,0,0,0,0xFF);
	for(yy = 0, y = r.y; y < (r.y + h); y++, yy++) {
		for(xx=0,x = r.x;x < (r.x + w); x++,xx++) {
			if(widget->pixels[(yy * w) + xx]  != 0xFF) {
				SDL_RenderDrawPoint(render, x, y);
			}
		}
	}
	if(widget->check) {
		r.x = widget->r.x+2;
		r.y = widget->r.y+2;
		r.w=r.h=5;
		SDL_RenderFillRect(render, &r);
	}
}

void sdl_draw_label(sdl_widget_t *widget, SDL_Renderer *render)
{
	int x, y, w, h;
	int xx, yy;
	SDL_Rect r;

	sdl_widget_t *p = widget->parent;

	r.x = x = widget->r.x;
	r.y = y = widget->r.y;
	w = widget->r.w;
	h = widget->r.h;

	memset(widget->pixels, 0xFF, w*h);
	SDL_SetRenderDrawColor(render,0,0,0,0xFF);
	sdl_printf(widget, 0, 0, widget->font.bold, widget->text);

	for(yy = 0, y = r.y; y < (r.y + h); y++, yy++) {
		for(xx=0,x = r.x;x < (r.x + w); x++,xx++) {
			if(widget->pixels[(yy * w) + xx]  != 0xFF) {
				SDL_RenderDrawPoint(render, x, y);
			}
		}
	}
	widget->dirty=0;
}

void sdl_draw_inputtext(sdl_widget_t *widget, SDL_Renderer *render)
{
	int x, y, w, h;
	int xx, yy;
	SDL_Rect r;

	sdl_widget_t *p = widget->parent;

	r.x = x = widget->r.x;
	r.y = y = widget->r.y;
	w = widget->r.w;
	h = widget->r.h;

	r.x = x = widget->r.x;
	r.y = y = widget->r.y;
	r.w = w = widget->r.w;
	r.h = h = widget->r.h;

	if(windowList[windowID]->focused == widget) {
		SDL_SetRenderDrawColor(render, INPUTTEXT_FOCUS_COLOR, 0xFF);
	} else
	SDL_SetRenderDrawColor(render, INPUTTEXT_BACKGROUND_COLOR, 0xFF);
	SDL_RenderFillRect(render, &r);

	sdl_draw_border(render, x, y, w, h, BORDER_IN);

	if(widget->text) {
		memset(widget->pixels, 0xFF, w*h);
		SDL_SetRenderDrawColor(render,0,0,0,0xFF);
		sdl_print_chargen(widget, 0, 0, widget->text, strlen(widget->text), 0, 1);
		for(yy = 0, y = r.y+4; y < (r.y + h); y++, yy++) {
			for(xx=0,x = r.x;x < (r.x + w); x++,xx++) {
				if(widget->pixels[(yy * w) + xx]  != 0xFF) {
					SDL_RenderDrawPoint(render, x, y);
				}
			}
		}
	}
}

void sdl_draw_progressbar(sdl_widget_t *widget, SDL_Renderer *render)
{
	int x, y, w, h;
	sdl_widget_t *p = widget->parent;

	SDL_Rect r;

	r.x = x = widget->r.x;
	r.y = y = widget->r.y;
	r.w = w = widget->r.w;
	r.h = h = widget->r.h;

	r.w = ((widget->cur - widget->r.x) * 100) / (widget->max);
	sdl_draw_border(render, x, y, w, h, BORDER_IN);

	r.x += 2;
	r.y += 2;
	r.w -= 2;
	r.h -= 3;
	SDL_SetRenderDrawColor(render, 55, 55, 155, 0xFF);
	SDL_RenderFillRect(render, &r);
}

void sdl_progressbar_set_limits(sdl_widget_t *widget, int min, int max)
{
	widget->min = min;
	widget->max = max;
	widget->cur = 0;
}

void sdl_progressbar_set_value(sdl_widget_t *widget, int val)
{
	widget->cur = val;
	widget->dirty = 1;
}

void sdl_draw_frame(sdl_widget_t *widget, SDL_Renderer *render)
{
	int rr, g, b;
	SDL_Rect r;
	window_t *p = widget->parent;
	rr = g = b = 198;
	SDL_SetRenderDrawColor(render, rr, g, b, 0xFF);

	r.x = widget->r.x;
	r.y = widget->r.y;
	r.w = widget->r.w;
	r.h = widget->r.h;
	SDL_RenderFillRect(render, &r);
}

void sdl_draw_box(sdl_widget_t *widget, SDL_Renderer *render)
{
	int rr, g, b;
	SDL_Rect r;
	sdl_widget_t *p = widget->parent;
	rr = g = b = 108;
	r.x = widget->r.x + p->r.x;
	r.y = widget->r.y + p->r.y;
	r.w = widget->r.w;
	r.h = widget->r.h;

	SDL_SetRenderDrawColor(render, rr, g, b, 0xFF);
	SDL_RenderFillRect(render, &r);
	sdl_draw_border(render, r.x, r.y, widget->r.w, widget->r.h, BORDER_IN);
}

void sdl_draw_list(sdl_widget_t *wid, SDL_Renderer *render);

void sdl_widget_draw( sdl_widget_t *widget, SDL_Renderer *render) 
{
	if( !widget ) return;

	switch(widget->type) {
		case SUI_LIST:
			sdl_draw_list(widget, render);
			break;
		case SUI_PROGRESSBAR:
			sdl_draw_progressbar(widget, render);
			break;
		case SUI_SLIDERBAR:
			sdl_draw_sliderbar(widget, render);
			break;
		case SUI_CHECKBUTTON:
			sdl_draw_checkbutton(widget, render);
			break;
		case SUI_TOOLBUTTON:
			sdl_draw_toolbutton(widget, render);
			break;
		case SUI_BUTTON:
			sdl_draw_button(widget, render);
			break;
		case SUI_GRAPH:
			sdl_draw_graph(widget, render);
			break;
		case SUI_FRAME:
			sdl_draw_frame(widget, render);
			break;
		case SUI_BOX:
			sdl_draw_box(widget, render);
			break;
		case SUI_LABEL:
			sdl_draw_label(widget, render);
			break;
		case SUI_INPUTTEXT:
			sdl_draw_inputtext(widget, render);
			break;
	}
}

void sdl_window_redraw(window_t *window)
{
	int i = 0;
	int j = 0;

	SDL_SetRenderDrawColor(window->render, 128, 128, 128, 0xFF);
	SDL_RenderClear(window->render);

	for( i = 0; i < window->widgets; i++ ) {
		sdl_widget_draw(window->widget[i], window->render);
		for(j = 0; j < window->widget[i]->widgets; j++)
			sdl_widget_draw(window->widget[i]->widget[j], window->render);
	}
	if(window->focused != NULL) {
		sdl_widget_draw(window->focused, window->render);
	}
	SDL_RenderPresent(window->render);
	window->dirty=0;
}

void sdl_window_update(window_t *window)
{
	int i = 0;
	int j = 0;

	if(window->dirty) {
		SDL_SetRenderDrawColor(window->render, 128, 128, 128, 0xFF);
		SDL_RenderClear(window->render);
	}
	for( i = 0; i < window->widgets; i++ ) {
		if(window->widget[i]->dirty) {
			sdl_widget_draw(window->widget[i], window->render);
			window->widget[i]->dirty = 0;
		}
		for(j = 0; j < window->widget[i]->widgets; j++)
			if(window->widget[i]->widget[j]->dirty) {
				sdl_widget_draw(window->widget[i]->widget[j], window->render);
				window->widget[i]->widget[j]->dirty = 0;
			}
	}
	if(window->focused != NULL) {
		sdl_widget_draw(window->focused, window->render);
	}
	if(window->dirty) {
		window->dirty = 0;
		SDL_RenderPresent(window->render);
	}
}

sdl_widget_t *sdl_graph(sdl_widget_t *parent, char *name, int points[], int total, int x, int y, int w, int h)
{
	sdl_widget_t *wid = sdl_widget(parent, name, SUI_GRAPH, x, y, w, h, 0xFFFFFFFF);
	wid->dataptr=points;
	wid->datasize=total;
	sdl_widget_add_child(parent, wid);
	return (wid);
}

sdl_widget_t *sdl_button(sdl_widget_t *parent, char *text, char *name, int x, int y, int w, int h)
{
	sdl_widget_t *wid = sdl_widget(parent, name, SUI_BUTTON, x, y, w, h, 0xFFFFFFFF);
	wid->font.id = FONT_6x10;
	if(text)
		wid->text = strdup(text);
	sdl_widget_add_child(parent, wid);
	return (wid);
}

sdl_widget_t *sdl_toolbutton(sdl_widget_t *parent, char *text, char *name, int x, int y, int w, int h)
{
	sdl_widget_t *wid = sdl_widget(parent, name, SUI_TOOLBUTTON, x, y, w, h, 0xFFFFFFFF);
	wid->font.id = FONT_6x10;
	if(text)
		wid->text = strdup(text);
	wid->border = BORDER_OUT;
	sdl_widget_add_child(parent, wid);
	return (wid);
}

sdl_widget_t *sdl_checkbutton(sdl_widget_t *parent, char *text, char *name, int x, int y, int w, int h)
{
	if(w < (strlen(text) * fonts[0].w))
		w += (strlen(text) * fonts[0].w) - w;
	sdl_widget_t *wid = sdl_widget(parent, name, SUI_CHECKBUTTON, x, y, w, h, 0xFFFFFFFF);
	wid->font.id = FONT_6x10;
	if(text)
		wid->text = strdup(text);
	wid->border = BORDER_IN;
	sdl_widget_add_child(parent, wid);
	return (wid);
}

sdl_widget_t *sdl_label(sdl_widget_t *parent, char *text, char *name, int x, int y, int w, int h)
{
	if(w < (strlen(text) * fonts[parent->font.id].w))
		w = (strlen(text) * fonts[parent->font.id].w) + (strlen(text) * 2);
	sdl_widget_t *wid = sdl_widget(parent, name, SUI_LABEL, x, y, w, h, 0xFFFFFFFF);
	if(text)
		wid->text = strdup(text);
	sdl_widget_add_child(parent, wid);
	wid->dirty = 1;
	return (wid);
}

sdl_widget_t *sdl_inputtext(sdl_widget_t *parent, char *text, char *name, int x, int y, int w, int h)
{
	if(w < (strlen(text) * fonts[0].w))
		w = (strlen(text) * fonts[0].w) + (strlen(text) * 2);
	sdl_widget_t *wid = sdl_widget(parent, name, SUI_INPUTTEXT, x, y, w, h, 0xFFFFFFFF);
	if(text)
		wid->text = strdup(text);
	sdl_widget_add_child(parent, wid);
	wid->dirty = 1;
	return (wid);
}

sdl_widget_t *sdl_box(sdl_widget_t *parent, char *text, char *name, int x, int y, int w, int h)
{
	sdl_widget_t *wid = sdl_widget(parent, name, SUI_BOX, x, y, w, h, 0xFFFFFFFF);
	if(text)
		wid->text = strdup(text);
	sdl_widget_add_child(parent, wid);
	wid->dirty = 1;
	return (wid);
}

sdl_widget_t *sdl_list(sdl_widget_t *parent, char *text, char *name, int x, int y, int w, int h)
{
	sdl_widget_t *wid = sdl_widget(parent, name, SUI_LIST, x, y, w, h, 0xFFFFFFFF);
	if(text)
		wid->text = strdup(text);
	sdl_widget_add_child(parent, wid);
	wid->enable = wid->index = 0;
	wid->dirty = 1;
	return (wid);
}

void sdl_list_adddata(sdl_widget_t *wid, char *list[], int count)
{
	wid->dataptr = list;
	wid->datasize = count;
}

sdl_widget_t *sdl_progressbar(sdl_widget_t *parent, char *text, char *name, int x, int y, int w, int h)
{
	sdl_widget_t *wid = sdl_widget(parent, name, SUI_PROGRESSBAR, x, y, w, h, 0xFFFFFFFF);
	if(text)
		wid->text = strdup(text);
	sdl_widget_add_child(parent, wid);
	wid->dirty = 1;
	return (wid);
}

void sdl_widget_sliderbar_change(void *ptr)
{
	sdl_widget_t *wid = (sdl_widget_t *)ptr;
	int x, y;
	SDL_GetMouseState(&x, &y);
	wid->cur = x-1;
 	//printf("val: %d\n", (((wid->cur - wid->r.x) * wid->max) / (wid->r.w)) + 1);
	wid->dirty=1;
}

sdl_widget_t *sdl_sliderbar(sdl_widget_t *parent, char *text, char *name, int x, int y, int w, int h)
{
	sdl_widget_t *wid = sdl_widget(parent, name, SUI_SLIDERBAR, x, y, w, h, 0xFFFFFFFF);
	if(text)
		wid->text = strdup(text);
	sdl_widget_add_child(parent, wid);


	wid->dirty = 1;
	return (wid);
}

void sdl_sliderbar_set(sdl_widget_t *wid, double min, double max, double cur)
{
	wid->min = min;
	wid->max = max;
	wid->cur = cur;
}

void sdl_sliderbar_setvalue(sdl_widget_t *wid, double cur)
{
	wid->cur = (cur>wid->max)?wid->max:(cur<wid->min)?wid->min:cur;
}

void sdl_draw_sliderbar(sdl_widget_t *wid, SDL_Renderer *render)
{
	int x, y, w, h;
	int xx,yy;
	SDL_Rect r;
	sdl_widget_t *sdl = (sdl_widget_t *)wid->parent;

	r.x = x = wid->r.x;
	r.y = y = wid->r.y;
	r.w = w = wid->r.w;
	r.h = h = wid->r.h;
	r.y-=10;
	r.h=30;
	r.w+=20;
	SDL_SetRenderDrawColor(render, 0, 0, 0, 0xFF);
	sdl_draw_border(render, x, y+4, w+5, 3, BORDER_IN);
	r.x = (wid->cur / wid->max) * (wid->r.w);
	r.x += wid->r.x;
	r.y = y-3;
	r.w = 5;
	r.h = 15;
	SDL_SetRenderDrawColor(render, 120, 130, 140, 0xFF);
	r.x = (wid->cur * 100) / wid->max;
	SDL_RenderFillRect(render, &r);
	sdl_draw_border(render, r.x, r.y, 5, r.h, BORDER_OUT);

	if(wid->text != NULL) {
		r.x = wid->r.x+wid->r.w;
		r.y = y-2;
		r.w = (strlen(wid->text) * fonts[wid->font.id].w) + 2;
		r.h = fonts[wid->font.id].h;

		memset(wid->pixels, 0xFF, wid->r.w * wid->r.h);
		sdl_print_chargen(wid, 0, 0, wid->text, strlen(wid->text), 0,1);
		for(yy = 0, y = wid->r.y; y < (wid->r.y+h); y++, yy++) {
			for(xx=0,x = wid->r.x+wid->r.w+4; xx < (w); x++,xx++) {
				SDL_SetRenderDrawColor(render,0, 0, 0, 0xFF);
				if(wid->pixels[(yy * w) + xx]  != 0xFF) {
					SDL_RenderDrawPoint(render, x, y);
				}
			}
		}
	}
	wid->dirty = 0;
}

int check_wid_xy(sdl_widget_t *wid, int x, int y)
{
	int i; int j;
	sdl_widget_t *p = wid->parent;
	for( i = 0; i < wid->widgets; i++ ) {
		if((x > wid->widget[i]->r.x) && (x < (wid->widget[i]->r.x + wid->widget[i]->r.w)) && 
				((y > wid->widget[i]->r.y) && (y < (wid->widget[i]->r.y + wid->widget[i]->r.h)))) {
			if(wid->widget[i]->type == SUI_BOX || wid->widget[i]->type == SUI_FRAME)
				continue;
			if(wid->widget[i]->type == SUI_LIST) {
				int k = y - (wid->widget[i]->r.y);
				if(wid->widget[i]->enable) {
					wid->widget[i]->index = (k/(fonts[wid->font.id].h+2));
					if(wid->widget[i]->index < 0)
						wid->widget[i]->index = 0;
				}
				wid->widget[i]->dirty = 1;
			}
			return wid->widget[i]->id;
		}
	}
	if((x > wid->r.x) && (x < (wid->r.x + wid->r.w)) && 
			((y > wid->r.y) && (y < (wid->r.y + wid->r.h)))) {
		return wid->id;
	}
	return -1;
}

int check_xy(window_t *window, int x, int y)
{
	int i; int j;
	for( i = 0; i < window->widgets; i++ ) {
		if(check_wid_xy(window->widget[i], x, y))
			return 1;
	}
	return 0;
}

int check_xy_now(window_t *window)
{
	int i; int j;
	int x, y;
	int m;
	SDL_GetMouseState(&x, &y);
	for( i = 0; i < window->widgets; i++ ) {
		if((m = check_wid_xy(window->widget[i], x, y)) != -1)
			return m;
	}
	return -1;
}

int _mouse_stuff(window_t *window)
{
	static double prev_x = -100;
	static double velo = 0;
	double deaccel = 10;
	int x, y, buttons;
	buttons = SDL_GetMouseState(&x, &y);
	if (buttons & SDL_BUTTON_LMASK)
	{
		if (prev_x < 0)
		{
			prev_x = x;
		}
		velo = x - prev_x;
		prev_x = x;
	} else {
		prev_x = -100;
		if (velo > deaccel)
		{velo -= deaccel;}
		if (velo < -deaccel)
		{velo += deaccel;}
		if (velo >= -deaccel && velo <= deaccel)
		{velo *= 0.5;}
	}
	return (int)round(velo);
}

int get_longest(char **items, int len)
{
	int i;
	int max = 0;
	for(i = 0; i< len; i++)
		if(strlen(items[i]) > max)
			max = strlen(items[i]);
	return (max+1);

}
void sdl_draw(SDL_Renderer *render, int x, int y, unsigned char **buf)
{
	int i,j,k;
	i=0;
	SDL_Point p;
	while(buf[i]) {
		k=strlen((char*)buf[i]);
		for(j=0;j<k;j++) {
			if(buf[i][j]==0x31) {
				p.x = x+j;
				p.y = y+i;
				SDL_RenderDrawPoint(render, p.x, p.y);
			}
		}
		i++;
	}
}

void sdl_draw_list(sdl_widget_t *wid, SDL_Renderer *render)
{
	SDL_Rect r;
	SDL_Rect cell;
	int x, y, w, h;
	int i;
	char **items,*item;
	int l;
	int datasize = wid->datasize;
	items = wid->dataptr;

	r.x = x = wid->r.x;
	r.y = y = wid->r.y;
	l = get_longest(items, wid->datasize);
	r.w=w=wid->r.w = (l * fonts[wid->font.id].w) + (l * 2) + 8;
	r.h=h = wid->r.h = fonts[wid->font.id].h * datasize + (datasize * 2) + 2;
	r.y+=1;
	datasize = wid->enable ? wid->datasize:1;
	r.x = x = wid->r.x;
	r.y = y = wid->r.y;
	l = get_longest(items, wid->datasize);
	r.w=w =  wid->r.w = (l * fonts[wid->font.id].w) + (l * 2)+2;
	r.h=h = wid->r.h = fonts[wid->font.id].h * datasize + (datasize * 2);
	SDL_SetRenderDrawColor(render, 0, 0, 0, 0xFF);
	sdl_draw_border(render, x, y, w, h, BORDER_FLAT);
	cell.x=x+1;	cell.y=y+1;
	cell.w=w-1;
	cell.h=h-1;
	SDL_SetRenderDrawColor(render, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_RenderFillRect(render, &cell);
	cell.w = w-2;
	cell.h = fonts[wid->font.id].h + 2;
	int px = w + 1;
	int py = y + 2;
	int xx, yy;
	SDL_SetRenderDrawColor(render, 0xFF, 0xFF, 0xFF, 0xFF);
	memset(wid->pixels, 0xFF, wid->r.w * wid->r.h);
	wid->font.id=0;
	y = 0;
	for(i=(wid->enable?0:(wid->index));i<wid->datasize;i++) {
		item = items[i];
		if(i==wid->index&&wid->enable) {
			cell.x=wid->r.x+2;
			cell.y=(wid->r.y) + (wid->index * fonts[wid->font.id].h) + (2 * wid->index)+1;
			SDL_SetRenderDrawColor(render, 0xd0, 0xf0, 0xFF, 0xFF);
			SDL_RenderFillRect(render, &cell);
			sdl_print_chargen(wid, 4, y+2, item, strlen(item), 0,1);
		}else {
			sdl_print_chargen(wid, 4, y+2, item, strlen(item), 0,1);
		}
		y+=fonts[wid->font.id].h+2;
		if(!wid->enable)
			break;
	}
	SDL_SetRenderDrawColor(render, 0x00, 0x0, 0x00, 0xFF);

	sdl_draw(render, x + px + 1, py - 1, (unsigned char**)arrowDown);
	sdl_draw_border(render, x+px, py-2, 16, 12, BORDER_FLAT);

	for(yy = 0, y = wid->r.y; y < (wid->r.y+h); y++, yy++) {
		for(xx=0,x = wid->r.x;x < (wid->r.x + w); x++,xx++) {
			SDL_SetRenderDrawColor(render,0, 0, 0, 0xFF);
			if(wid->pixels[(yy * w) + xx]  != 0xFF) {
				SDL_RenderDrawPoint(render, x, y);
			}
		}
	}
	wid->r.w += 18;
}

int sdl_loop(window_t *sdl)
{
	int i;
	int configWindowID =-1;
	static int mousedown = 0;
	int m;
	int val = 0;
	int do_quit = 0;

	windowID = SDL_GetWindowID(sdl->window);
	SDL_Event event;
	sdl_widget_t *wid = NULL;
	if(SDL_PollEvent(&event)) 
	{
		switch(event.type) {
			case SDL_WINDOWEVENT:  
				switch (event.window.event)  {
					case SDL_WINDOWEVENT_SHOWN:
						sdl_window_redraw(windowList[event.window.windowID]);
						break;
					case SDL_WINDOWEVENT_ENTER:
						windowID = event.window.windowID;
						break;
					case SDL_WINDOWEVENT_SIZE_CHANGED: 
						//width = event.window.data1;
						//height = event.window.data2;
						break;
					case SDL_WINDOWEVENT_CLOSE:  
						if(event.window.windowID == 1) {
							event.type = SDL_QUIT;
							SDL_PushEvent(&event);
						} else if(event.window.windowID == configWindowID) {
							SDL_HideWindow(configwin->window);
						}
						break;
				}
			case SDL_MOUSEMOTION:
				if(SDL_BUTTON(event.button.button) == 1) {
					if((m = check_xy_now(windowList[windowID])) != -1) {
						wid = sdl_widget_get_withid(m);
						if(wid->type == SUI_BUTTON||wid->type==SUI_TOOLBUTTON) {
							if(wid->border == BORDER_OUT)
								wid->border = BORDER_IN;
						}
						windowList[windowID]->dirty = 1;
						wid->dirty = 1;
					}
					if(wid->events[ON_MOUSE_MOVE] != NULL)
						wid->events[ON_MOUSE_MOVE](wid);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if(!mousedown) {
					if(SDL_BUTTON(event.button.button) == 1) {
						if((m = check_xy_now(windowList[windowID])) != -1) {
							wid = sdl_widget_get_withid(m);
							printf("WID: %s\n", wid->name);
							if(wid->type == SUI_BUTTON||wid->type==SUI_TOOLBUTTON) {
								if(wid->border == BORDER_OUT) {
									wid->border = BORDER_IN;
								}
							}
							windowList[windowID]->dirty = 1;
							wid->dirty = 1;
							switch(wid->type) {
								case SUI_LIST:
								case SUI_INPUTTEXT:
								case SUI_SLIDERBAR:
									sdl->focused = wid;
							}
							//if(wid->type != SUI_FRAME && wid->type != SUI_BOX)
							//	sdl->focused = wid;
						}
						mousedown = 1;
					}
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if(mousedown) {
					if((m = check_xy_now(windowList[windowID])) != -1) {
						wid = sdl_widget_get_withid(m);
						if(wid->type==SUI_CHECKBUTTON){ 
							wid->check = !wid->check;
						} 
						if(wid->type==SUI_LIST) {
							wid->enable = !wid->enable;
						} else
							if(sdl->focused && sdl->focused->enable) {
								sdl->focused->enable = !sdl->focused->enable;
							}
						if(wid->border == BORDER_IN)
							wid->border = BORDER_OUT;
						if(wid->events[ON_CLICK] != NULL)
							wid->events[ON_CLICK](wid);
						wid->dirty = 1;
					}
					windowList[windowID]->dirty = 1;
					//m = mouse_stuff(sdl);
					mousedown = 0;
				}
				break;
			case SDL_KEYDOWN:
				if(sdl->focused) {
					sdl->focused->key = event.key.keysym.sym;
					if(sdl->focused->events[ON_KEYPRESS] != NULL)
						sdl->focused->events[ON_KEYPRESS](sdl->focused);
				}
					sdl->key=event.key.keysym.sym;
				switch(event.key.keysym.sym) {
					case SDLK_ESCAPE:
						do_quit=1;
						break;
					case SDLK_p:
						//sdl_progressbar_set_value(pbar, val++);
						//if(val >= pbar->max)
						//	val = 0;
						//sdl->dirty = 1;
						break;
				}
				break;
			case SDL_QUIT:
				do_quit = 1;
				break;
		}
	}
	//sdl_window_redraw(windowList[0]);
	for(i = 0; i < windowCnt; i++) {
	  if(windowList[i] == NULL)
	  continue;
	  if(windowList[i]->dirty) {
	  	sdl_window_redraw(windowList[i]);
	  	windowList[i]->dirty = 0;
	  	}
	  }

	return do_quit;
}


int sdl_widget_name(sdl_widget_t *w, char *name)
{
	return (!strcmp(w->name, name));
}

/* start */
void on_click(void *ptr)
{
	sdl_widget_t *wid = ptr;
	char **items;
	if(sdl_widget_name(wid, "inputSampleRate")) {
		items = wid->dataptr;
		if(wid->index) {
			//demod.rate_in = atof(items[wid->index]);
			//dongle.rate= atoi(items[wid->index])/1;
			//printf("RATE: %d\n", dongle.rate);
			//safe_cond_signal(&controller.hop , &controller.hop_m);
		}
		printf("INDEX: %d/%s\n", wid->index, items[wid->index]);
	}else
		printf("CLICK %s!\n", wid->name);
}

void on_freq_keypress(void *ptr)
{
	sdl_widget_t *wid = ptr;
	sdl_widget_t *p = wid->parent;

	if(wid->key == SDLK_UP) {
		wid->cur+=1000;
	} else if(wid->key == SDLK_DOWN) {
		wid->cur-=1000;
	} else if(wid->key == SDLK_LEFT) {
		wid->cur+=100000;
	} else if(wid->key == SDLK_RIGHT) {
		wid->cur-=100000;
	}

	int fm = (wid->cur / 1000000);
	int fh = (wid->cur - (fm * 1000000));
	fh /= 10000;
	int fz = (wid->cur - (fm * 1000000)) - (fh * 10000);
	fz /= 100;
	sprintf((char*)wid->text, " %03d.%03d.%03d MHZ", fm, fh, fz);
	//controller.freq_now = wid->cur;
	//safe_cond_signal(&controller.hop , &controller.hop_m);

	wid->dirty=1;
	p->dirty=1;
	sdl_window_update(p->window);
}

void on_ppm_keypress(void *ptr)
{
	sdl_widget_t *wid = ptr;
	sdl_widget_t *p = wid->parent;

	if(wid->key == SDLK_UP) {
		wid->cur+=1;
	} else if(wid->key == SDLK_DOWN) {
		wid->cur-=1;
	}
	sprintf((char*)wid->text, " %d ppm", wid->cur);
	wid->dirty=1;
	sdl_window_update(p->window);
}

void on_check(void *ptr)
{
	sdl_widget_t *wid = ptr;
	if(!strcmp(wid->name, "deemp")) {
		//demod.deemph=wid->check;
	} else if(!strcmp(wid->name, "edge")) {
		//controller.edge = wid->check;
	} else if(!strcmp(wid->name, "graphlines")) {
		printf("check: %d\n", graph->check=wid->check);
		//wid->check=!wid->check;
	}
	wid->dirty = 1;
}

void btCloseConfig_click(void *ptr)
{
	sdl_widget_t *wid = ptr;
	printf("NAME: %s\n", wid->name);
	//SDL_HideWindow(windowList[1]->window);
}

void sliderbar_event(void *ptr)
{
	int x, y;
	sdl_widget_t *wid = ptr;
	SDL_GetMouseState(&x, &y);
	wid->cur = ((x-wid->r.x) + wid->r.x);
	if(wid->cur<0)
		wid->cur=0;
	if(wid->text == NULL) {
		wid->text = malloc(128);
		memset(wid->text, 0, 128);
	} 
	if(!strcmp(wid->name, "volume")) {
		//output.volume = wid->cur;
	} else {
		//dongle.gain = wid->cur;
		//dongle.gain = nearest_gain(dongle.dev, dongle.gain);
		//verbose_gain_set(dongle.dev, dongle.gain);
	}

	sprintf(wid->text, " %d", wid->cur);
	wid->dirty=1;
}

static char *samplesR[] = { " -- RATE -- ", "3200000", "2700000", "2400000", "2200000", "1700000", "1400000", "1000000", "900000" };

/* 
 * not thread safe at all */
void progress_loop(void *ptr)
{
	static int inc = 1;
	int a, b, val;
	sdl_widget_t *pbar = ptr;
	a = b = 0;

	while(1) {
		SDL_Delay(25);
		pbar->cur+=inc;
 		val = (((pbar->cur - pbar->r.x) * pbar->max) / pbar->r.w);
		if(val>=(pbar->max)) {
			inc=-1;
		}
		else if(val<=pbar->min) {
			inc=1;
		}
		pbar->cur+=inc;
		pbar->dirty=1;
		windowList[windowID]->dirty=1;
	}
}

void sdl_draw_graph(sdl_widget_t *wid, SDL_Renderer *render)
{
	int x, y, yy, i = 0, j  = 0;
	SDL_Rect r;
	r.x = wid->r.x;
	r.y = wid->r.y;
	r.w = wid->r.w;
	r.h = wid->r.h;

	SDL_SetRenderDrawColor(render, 4, 24, 24, 0xFF);
	SDL_RenderFillRect(render, &r);

	SDL_SetRenderDrawColor(render, 204, 204, 0, 0x0F);
	int *logp = (int *)wid->dataptr;

	if(logp==NULL)
		return;

	j=0;
	int x2d, y2d,xx;
	int lastX=0,lastY=0;
	for(x = wid->r.x; x < (wid->r.x+wid->r.w); x++) {
		x2d = (int)((x * zFactor) / z3d);
		y2d = (int)((logp[j++] * zFactor)/zFactor) - wid->r.h;
		if(!graph->check) {
			SDL_RenderDrawPoint(render, x2d, y2d);
		} else {
			/*if(!lastX){
				lastX=x2d-(wid->r.w/2);
				lastY=y2d-wid->r.y;
				continue;
			}*/
			/*if(lastY < 0 || y2d < 0) {
				printf("porra! %d %d\n", lastY, wid->r.y);
				if(y2d < 0 && lastY > 0) {
					y2d = lastY;
				} else if(lastY < 0 && y2d > 0) {
					lastY = y2d + wid->r.y;
				} else if(lastY < 0 && y2d < 0) {
					y2d = lastY = wid->r.y;
				}
				//lastY=wid->r.y;
				//y2d=wid->r.h/2;
			}
			if((y2d-wid->r.y)<0)
				y2d=(wid->r.h-wid->r.y);
			*/
			//SDL_RenderDrawLine(render, lastX, lastY, x2d - (wid->r.w/2), y2d-wid->r.y);
			SDL_RenderDrawLine(render, lastX, lastY, x2d, y2d);
			lastX=x2d;
			lastY=y2d;
			//lastX=x2d-(wid->r.w/2);
			//lastY=y2d-wid->r.y;
		}
	}
	SDL_RenderDrawLine(render, (wid->r.x + (wid->r.w/2)), wid->r.y, (wid->r.x + (wid->r.w/2)), wid->r.y+wid->r.h);
}

int main(int argc, char **argv)
{
	int i;
	int do_quit = 0;
	static char *devName[32];
	SDL_Event event;
	sdl_widget_t *frame, *button, *box, *frame1, *check, *input,*label;
	var_t *var;
	var_list_t *inputList;
	var_list_t *filterList;

	var_list_t *list = NULL;

	//parse_json_file(argv[1], &list);
	//inputList = var_list(var_get(list, "input"));
	//filterList = var_list(var_get(list, "filters"));

	window_t *sdl = sdl_window_new("title", 10, 10, 640, 240, SDL_WINDOWPOS_CENTERED);
	windowList[SDL_GetWindowID(sdl->window)] = sdl;
	windowCnt = 2;

	frame = sdl_widget(sdl->window, "frame", SUI_FRAME, 0, 0, 1280, 640, 0xFFFFFFFF);

	button = sdl_button(frame, "button ", "button", 10, 10, 62, 15);
	sdl_widget_set_callback(button, ON_CLICK, btCloseConfig_click);
	/*sdl_widget_t *label1 = sdl_label(frame, "carlos rafael villordo", "carlo", 82, 60, 100, 20);
	sdl_widget_align(button, label1, ALIGN_LEFT, 2);
	sdl_widget_draw(label1, sdl->render);*/

	//check = sdl_checkbutton(frame, "checkbox ", " checkbox ", 10, 45, 140, 16);
	label = sdl_label(frame, "testing", "testing", 10, 60, 80, 20);
	label->font.id = FONT_6x10;
	label->dirty=1;

	/*image_t *in, *out, *oo;
	in = image_alloc(300, 20, 8, 0);
	memset(in->data, 0xFF, 300*20);
	print_chargen(in, 0, 0, "carlos rafael villordo", strlen("carlos rafael villordo")/2, 0, 0);
	out = image_alloc(300, 20, 8, 0);
	oo = image_alloc(600, 40, 8, 0);
	memset(oo->data, 0xFF, 600*40);
	image_scale_up(in, oo);
	image_scale_down(in, out);
	memset(label->pixels, 0xFF, label->r.w*label->r.h);
	memcpy(label->pixels, out->data, 300*10);
	label->dirty=1;*/
	
	sdl_widget_t *sbar = sdl_sliderbar(frame, NULL, "sliderbar", 10, 85, 240, 14);
	sdl_widget_set_callback(sbar, ON_CLICK, sdl_widget_sliderbar_change);
	sdl_widget_set_callback(sbar, ON_MOUSE_MOVE, sdl_widget_sliderbar_change);

	char tmp[1024];

	sdl_progressbar_set_limits(sbar, 0, 100);
	sdl_progressbar_set_value(sbar, 32);

	sdl_widget_t *pbar = sdl_progressbar(frame, NULL, "progressbar", 10, 105, 240, 14);
	sdl_progressbar_set_limits(pbar, 0, 100);
	sdl_progressbar_set_value(pbar, 82);
	box = sdl_box(frame, "box", "box", 280, 10, 150, 150);

	devName[0] = " -- COMBOBOX --";
	sdl_widget_t *inputDevice = sdl_list(box, NULL, "inputDevice", 10, 14, 180, 24);
	sdl_list_adddata(inputDevice, devName, 1);
	sdl_widget_align(label, inputDevice, ALIGN_LEFT, 2);

	sdl_widget_t *flabel = sdl_inputtext(frame, tmp, "freq_value", 10, 165, 222, 16);
	sdl_widget_set_callback(flabel, ON_KEYPRESS, on_freq_keypress);

	/*check = sdl_checkbutton(frame, "offset tunning", "offset_tunning", 10, 62, 142, 16);
	check->check = var_int(var_get(filterList, "offset"));
	sdl_widget_set_callback(check, ON_CLICK, on_check);

	check = sdl_checkbutton(frame, "edge tunning", "edge_tunning", 10, 78, 142, 16);
	check->check = var_int(var_get(filterList, "edge"));
	sdl_widget_set_callback(check, ON_CLICK, on_check);

	input = sdl_inputtext(frame, " -33 ppm", "ppm_value", 185, 65, 72, 16);
	sdl_widget_set_callback(input, ON_KEYPRESS, on_ppm_keypress);
	*/

	button = sdl_toolbutton(frame, NULL, "btUp", 200, 65, 12, 10);
	sdl_widget_set_dataptr(button, arrowUp, sizeof(arrowUp) / sizeof(arrowUp[0]));
	sdl_widget_set_callback(button, ON_CLICK, btCloseConfig_click);

	/*sdl_widget_align(input, button, ALIGN_LEFT, 2);
	box = sdl_toolbutton(frame, NULL, "btDown", 50, 80, 12,10);
	sdl_widget_set_dataptr(box, arrowDown, sizeof(arrowDown) / sizeof(arrowDown[0]));
	sdl_widget_set_callback(box, ON_CLICK, btCloseConfig_click);
	sdl_widget_align(button, box, ALIGN_LEFT, 2);

	graph = sdl_graph(frame, "grapho", NULL, 0, 400, 30, 1024, 240);
	sdl_widget_align(check, graph, ALIGN_BOTTOM, 2);
	sdl_widget_set_callback(graph, ON_KEYPRESS, on_freq_keypress);
	*/

	sdl_window_add_widget(sdl, frame);

	SDL_CreateThread(progress_loop, "progress_loop", pbar);

	for(;do_quit == 0;) 
	{
		do_quit = sdl_loop(sdl);
		if(sdl->dirty)
			sdl_window_redraw(sdl);
		SDL_Delay(1);
	}

	exit(0);

	}

