/*
 * Copyright (C) 2013 rafael villordo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifndef WID_H
#define WID_H

#include <SDL.h>

#define DEFAULT_SAMPLE_RATE		24000
#define DEFAULT_BUF_LENGTH		16384 //((2 * FFT_SIZE) * FFT_STACK) //(1 * 16384)
#define MAXIMUM_OVERSAMPLE		16
#define MAXIMUM_BUF_LENGTH		(MAXIMUM_OVERSAMPLE * DEFAULT_BUF_LENGTH)
#define AUTO_GAIN			-100
//#define BUFFER_DUMP			4096

#define FREQUENCIES_LIMIT		1000


#define FONT_6x10	0
#define FONT_6x12	1
#define FONT_7x13 	2
#define FONT_7x14 	3


#define WIDGET_MAX 128

typedef void (*widget_event_f)(void *);
typedef struct font {
	int id;
	int color;
	int bold;
	int underline;
} sdl_font_t;


enum events {
	ON_ENTER,
	ON_LEAVE,
	ON_CLICK,
	ON_DRAW,
	ON_MOUSE_PRESS,
	ON_MOUSE_MOVE,
	ON_KEYPRESS,
	MAX_EVENTS
};

enum {
	BORDER_IN,
	BORDER_OUT,
	BORDER_FLAT,
	BORDER_NONE
};

enum {
	MOUSE_LEFTB = 1,
	MOUSE_RIGHTB = 4,
	MOUSE_SCROLL_UP = 8,
	MOUSE_SCROLL_DOWN = 16,
};

typedef struct sdl_widget 
{
	int 			id;
	char 			name[32];
	int				type;

	SDL_Rect 		r;
	SDL_Rect		gr;

	uint32_t		flags;

	sdl_font_t 		font;

	void			*parent;

	char			*text;

	void			*dataptr;
	int				datasize;

	int 			border;

	uint8_t			*pixels;


	int 			visible, enable;
	int				check, index;

	int 			min, max, cur;
	widget_event_f	events[MAX_EVENTS];

	SDL_Surface		*surface;
	SDL_Texture		*texture;
	//SDL_Window		*window;
	struct window 	*window;

	struct sdl_widget **widget;
	int				widgets;

	int 			dirty;
	int				key;

} sdl_widget_t;

sdl_widget_t *wlist[1024];

typedef struct window 
{
	char 			name[32];
	SDL_Window		*window;
	SDL_Renderer	*render;
	int 			x, y, w, h;
	int 			dirty;
	int				key;

	int				widgets;
	sdl_widget_t	**widget;

	sdl_widget_t	*focused;

} window_t;

typedef struct chardata {
	uint8_t sym;
	uint8_t byte[15];
} chardata_t;

typedef struct genchar {
	char 	*name;
	uint8_t	w, h, sx, sy;
	chardata_t data[260]; /* 255 characters / max of 14 pixels width */
} chargen_t ;

struct output_state
{
	int      exit_flag;
	pthread_t thread;
	FILE     *file;
	char     *filename;
	int16_t  *result;
	int      result_len;
	int      rate;
	int      wav_format;
	int      padded;
	int volume;
	pthread_rwlock_t rw;
	pthread_cond_t ready;
	pthread_mutex_t ready_m;
};

struct fft_state 
{
	pthread_t	thread;
	int16_t		magbuf[MAXIMUM_BUF_LENGTH];
	int16_t		pwrbuf[MAXIMUM_BUF_LENGTH];

	int16_t		buf16[MAXIMUM_BUF_LENGTH + 1];
	int			buflen;
	int 		mad;
	int			ready_fast;
	pthread_rwlock_t rw;
	pthread_cond_t ready;
	pthread_mutex_t ready_m;
};

struct dongle_state
{
	int      exit_flag;
	pthread_t thread;
	//rtlsdr_dev_t *dev;
	int      dev_index;
	uint32_t freq;
	uint32_t rate;
	int      gain;
	int16_t  *buf16;
	uint32_t buf_len;
	int      ppm_error;
	int      offset_tuning;
	int      direct_sampling;
	int      mute;
	struct demod_state *demod_target;
	struct fft_state *fft_target;
};

struct agc_state
{
	int64_t gain_num;
	int64_t gain_den;
	int64_t gain_max;
	int     gain_int;
	int     peak_target;
	int     attack_step;
	int     decay_step;
};

struct demod_state
{
	int      exit_flag;
	pthread_t thread;
	int16_t  *lowpassed;
	int      lp_len;
	int16_t  lp_i_hist[10][6];
	int16_t  lp_q_hist[10][6];
	int16_t  droop_i_hist[9];
	int16_t  droop_q_hist[9];
	int      rate_in;
	int      rate_out;
	int      rate_out2;
	int      now_r, now_j;
	int      pre_r, pre_j;
	int      prev_index;
	int      downsample;    /* min 1, max 256 */
	int      post_downsample;
	int      output_scale;
	int      squelch_level, conseq_squelch, squelch_hits, terminate_on_squelch;
	int      downsample_passes;
	int      comp_fir_size;
	int      custom_atan;
	int      deemph, deemph_a;
	int tau;
	int      now_lpr;
	int      prev_lpr_index;
	int      dc_block, dc_avg;
	int      agc_enable;
	struct   agc_state *agc;
	int		rms_value;
	void     (*mode_demod)(struct demod_state*);
	pthread_rwlock_t rw;
	pthread_cond_t ready;
	pthread_mutex_t ready_m;
	struct output_state *output_target;
};

struct controller_state
{
	int      exit_flag;
	pthread_t thread;
	uint32_t freqs[FREQUENCIES_LIMIT];
	int      freq_len;
	int      freq_now;
	int      edge;
	int      wb_mode;
	pthread_cond_t hop;
	pthread_mutex_t hop_m;
};

#define safe_cond_signal(n, m) pthread_mutex_lock(m); pthread_cond_signal(n); pthread_mutex_unlock(m)
#define safe_cond_wait(n, m) pthread_mutex_lock(m); pthread_cond_wait(n, m); pthread_mutex_unlock(m)

void sdl_draw_border(SDL_Renderer *render, int x, int y, int w, int h, int type);
void sdl_draw_toolbutton(sdl_widget_t *widget, SDL_Renderer *render);
void sdl_draw_button(sdl_widget_t *widget, SDL_Renderer *render);
void sdl_draw_graph(sdl_widget_t *widget, SDL_Renderer *render);
void sdl_draw_checkbutton(sdl_widget_t *widget, SDL_Renderer *render);
void sdl_draw_label(sdl_widget_t *widget, SDL_Renderer *render);
void sdl_draw_inputtext(sdl_widget_t *widget, SDL_Renderer *render);
void sdl_draw_progressbar(sdl_widget_t *widget, SDL_Renderer *render);
void sdl_draw_frame(sdl_widget_t *widget, SDL_Renderer *render);
void sdl_draw_box(sdl_widget_t *widget, SDL_Renderer *render);
void sdl_draw_list(sdl_widget_t *wid, SDL_Renderer *render);
void sdl_draw_sliderbar(sdl_widget_t *wid, SDL_Renderer *render);
void sdl_draw(SDL_Renderer *render, int x, int y, unsigned char **buf);

void sdl_widget_set_dataptr(sdl_widget_t *, void *, int);
#endif
