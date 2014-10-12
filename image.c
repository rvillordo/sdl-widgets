#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <png.h>
#include <assert.h>
#include <math.h>

#include "widget.h"
#include "image.h"
#include "rgb.c"

extern sdl_font_t fonts[];
int *rgb_map;
int colour_map[256*256];
short quick_rgb_map[0xFFF] = {0,};
short num_colours = 256;

/*
int get_color(int i, int *r, int *g, int *b) {
	int closest_d = 99999999;
	int closest_n = -1;
	int c = rgbMap[i].rgb;
	int c_b = c&0xFF;
	c>>=8;
	int c_g = c&0xFF;
	c>>=8;
	int c_r = c&0xFF;
	*r = c_r;
	*g = c_g;
	*b = c_b;
	return 1;
}

short find_color(int r, int g, int b) {
	int closest_d = 99999999;
	int closest_n = -1;
	int i;
	for (i=0; i<256; ++i) {
		int c = rgbMap[i].rgb;
		int c_b = c&0xFF;
		c>>=8;
		int c_g = c&0xFF;
		c>>=8;
		int c_r = c&0xFF;
		int dist = abs(c_r-r)+abs(c_g-g)+abs(c_b-b);
		if (dist<closest_d) {
			closest_d = dist;
			closest_n = i;
		}
	}
	return closest_n;
}

void compute_xterm256_cmap(int num_c)
{
	int i, c, c_b,c_g,c_r;
	int s, s_r, s_g, s_b;
	num_colours = num_c;
	for (i=0; i<num_colours; i++) {
		c = rgbMap[i].rgb;
		c_b = c&0xFF;
		c>>=8;
		c_g = c&0xFF;
		c>>=8;
		c_r = c&0xFF;
		colour_map[i<<8] = i;
		for (s=1; s<256; ++s) {
			s_r = ((255-s)*c_r)/256;
			s_g = ((255-s)*c_g)/256;
			s_b = ((255-s)*c_b)/256;
			colour_map[s+(i<<8)] = find_color(s_r,s_g,s_b);
		}
	}
}*/

image_t *image_load_png( char *filename )
{
   	image_t *t = NULL;

#ifdef WPNG
	FILE *fp;
   	unsigned char buf[5];
   	png_structp png_ptr;
   	png_infop info_ptr;
   	png_uint_32 width, height, scanline;
   	int bit_depth, color_type, interlace_type, row;
    if((fp = fopen( filename, "rb" ))==NULL)
		return (NULL);
    if(fread( buf, 1, 4, fp ) != 4 ){
		return (NULL);
	}
   if (png_sig_cmp( buf, (png_size_t)0, 4 ) ) { 
	   printf("not a png?");
	   return (NULL);
   }
   if((png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL )) == NULL)
   {
      fclose(fp);
      return NULL;
   }
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
      return NULL;
   }

   if (setjmp(png_ptr->jmpbuf))
   {
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
      fclose(fp);
      return NULL;
   }
   t = malloc(sizeof(image_t));
   png_init_io(png_ptr, fp);
   png_set_sig_bytes(png_ptr, 4 );
   png_read_info(png_ptr, info_ptr);
   png_get_IHDR(png_ptr, info_ptr, &width, &height, 
		   &bit_depth, &color_type, &interlace_type, 
		   NULL, NULL);
   t->w = width;
   t->h = height;
   t->bpp = bit_depth;
   t->scanline = png_get_rowbytes( png_ptr, info_ptr );
   t->data = (unsigned char*)malloc(t->scanline * height);
   for (row = 0; row < height; row++)
   {
      png_read_row( png_ptr, (unsigned char *)(t->data+(row*t->scanline)), NULL );
   }
   png_read_end(png_ptr, info_ptr);
   t->palsize = 0;
   if (info_ptr->palette)
   {
       t->palette = (unsigned char *)malloc(769);
	   t->palsize = 768;
       memcpy( t->palette, info_ptr->palette, 768 );
   }  else t->palette = NULL;
   png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
   fclose(fp);
#endif

   return (t);
}

void image_build_palette(image_t *image)
{
	int color = 255;
	int mcolor = 0;
	int x, y, i, j, d, c=0;
	uint8_t *p = image->data;

	int pal[256];
	memset(pal, 0, sizeof(int) * 256);
	int palsize = 256;
	//image->palette  = malloc(sizeof(uint8_t) * palsize) : NULL;
	for(y = 0; y < image->h - 8; y += 8) {
		for(x = 0; x < image->w - 8; x+= 8) {
			color = 0;
			for(i = 0; i < 8; i++) {
				d = ((i + y) * image->w) + x;
				for(j = 0; j < 8; j++) {
					pal[p[d+i]]++;
					if(p[d+i] < color)
						color = p[d+i];
					if(p[d+i] > mcolor)
						mcolor = p[d+i];
					//color = p[d+i];
					//image->palette[d+i] = color;
				}
			}
		}
	}
}

image_t *image_load_ppm( char *filename )
{
	FILE *fp;
   	char buf[5];
	char tmp[1024];
   	image_t *t = malloc(sizeof(image_t));
   	int bit_depth, color_type, row;
    if((fp = fopen( filename, "rb" ))==NULL)
		return (NULL);
	bit_depth = 0;
	if(fgets(tmp, 1024, fp) != NULL) {
		if(*tmp=='P') {
			if(*(tmp+1) == '5'){
				bit_depth = 8; // grayscale
			} else bit_depth = 24;  // rgb
		}
	}
	if(bit_depth==0) {
		return NULL;
	}
	if(fgets(tmp, 1024, fp) != NULL) {
		sscanf(tmp, "%i %i", &t->w, &t->h); 
	}
	if(t->w<1||t->h<1)
		return (NULL);
	t->bpp=bit_depth;
	fgets(tmp, 1024, fp);
    t->scanline = t->w * (bit_depth / 8);
    t->data = (unsigned char*)malloc(t->scanline * t->h);
    if(fread(t->data, 1, (t->scanline * t->h), fp ) != (t->scanline*t->h)){
		perror("fread");
		return (NULL);
	}
	fclose(fp);
	return (t);
}

image_t *image_alloc(int w, int h, int bpp, int palsize)
{
	image_t *t = malloc(sizeof(image_t));
	assert(t != NULL);
	t->w = w;
	t->h = h;
	t->bpp = bpp;
	t->scanline = (w * (bpp / 8));
	t->data = malloc(sizeof(uint8_t) * (t->scanline * h));
	t->palette  = palsize ? malloc(sizeof(uint8_t) * palsize) : NULL;
	t->palsize = palsize;
	return (t);
}

void *image_alloc_palette(int palsize)
{
	return malloc(palsize);
}

void image_scale_up(image_t *in, image_t *out)
{
	int x, y;
	for(y = 0; y < out->h; y++) {
		for(x = 0; x < out->w; x++) {
			out->data[(y * out->w) + x] = in->data[((y/2) * in->w) + (x / 2)];
		}
	}
}

void image_scale_down(image_t *in, image_t *out)
{
	int x, y;
	for(y = 0; y < out->h; y++) {
		for(x = 0; x < out->w; x++) {
			out->data[(y*out->w) + x] = in->data[((y*1) * in->w) + (x * 2)];
		}
	}
}

void image_flip(image_t *in)
{
	int i,c;
	int k=(in->w*in->h)/2;
	unsigned char *tmp = malloc(in->w);
	for(i = 0; i < in->h/2; i++) {
		memcpy(tmp, in->data + (in->w * i), in->w);
		memcpy(in->data + (in->w * i), in->data + (in->w * (in->h - i)), in->w);
		memcpy(in->data + (in->w * (in->h - i)), tmp, in->w);
	}
}

image_t *image_crop(image_t *in, int xx, int yy, int w, int h)
{
	int x, y;
	int x1, y1;
	image_t *out = image_alloc(w, h, in->bpp, 0);
	for(y1 = 0, y = yy; y < (yy + h); y++, y1++) {
		for(x = xx, x1 = 0; x < (xx + w); x++, x1++) {
			out->data[(y1 * out->w) + x1] = in->data[(y * in->w) + x];
		}
	}
	return (out);
}

void image_free(image_t *t)
{
	if(t->data)
		free(t->data);
	if(t->palette)
		free(t->palette);
	free(t);
}

uint8_t bilinear_interpolate(uint8_t *the_image, double x, double y, long rows, long cols)
{
	double fraction_x, fraction_y,
		   one_minus_x, one_minus_y,
		   tmp_double;
	int    ceil_x, ceil_y, floor_x, floor_y;
	short  p1, p2, p3, result = 128;

	if(x < 0.0               ||
			x >= (double)(cols)   ||
			y < 0.0               ||
			y >= (double)(rows)) {
		printf("FILL! %.2f %.2f\n",x,y);
		return(result);
	}

	tmp_double = floor(x);
	floor_x    = tmp_double;
	tmp_double = floor(y);
	floor_y    = tmp_double;
	tmp_double = ceil(x);
	ceil_x     = tmp_double;
	tmp_double = ceil(y);
	ceil_y     = tmp_double;

	fraction_x = x - floor(x);
	fraction_y = y - floor(y);

	one_minus_x = 1.0 - fraction_x;
	one_minus_y = 1.0 - fraction_y;

	tmp_double = one_minus_x * (double)(
			the_image[(floor_y * rows) + floor_x]) +
		fraction_x * 
		(double)(the_image[(floor_y * rows) + ceil_x]);
	p1 = tmp_double;

	tmp_double = one_minus_x * 
		(double)(the_image[(ceil_y * rows) + floor_x]) +
		fraction_x * 
		(double)(the_image[(ceil_y * rows) + ceil_x]);
	p2 = tmp_double;

	tmp_double = one_minus_y * (double)(p1) +
		fraction_y * (double)(p2);
	p3 = tmp_double;
	return(p3);
} 

//geometry(in, out, width, 0, 0, 0, 0, 0, 0, 0, 1, rows, cols);
void geometry(uint8_t *the_image, uint8_t *out_image, int w,
        double x_angle,
        double x_stretch, double y_stretch,
        uint8_t x_displace, uint8_t y_displace,
        double x_cross, double y_cross,
        int bilinear,
        long rows,
        long cols)
{
   double cosa, sina, radian_angle, tmpx, tmpy;
   float  fi, fj, x_div, y_div, x_num, y_num;
   int    i, j, new_i, new_j;


      /******************************
      *
      *   Load the terms array with
      *   the correct parameters.
      *
      *******************************/

      /* the following magic number is from
         180 degrees divided by pi */
   radian_angle = x_angle/57.29577951;
   cosa  = cos(radian_angle);
   sina  = sin(radian_angle);

      /************************************
      *
      *   NOTE: You divide by the
      *   stretching factors. Therefore, if
      *   they are zero, you divide by 1.
      *   You do this with the x_div y_div
      *   variables. You also need a
      *   numerator term to create a zero
      *   product.  You do this with the
      *   x_num and y_num variables.
      *
      *************************************/

   if(x_stretch < 0.00001){
      x_div = 1.0;
      x_num = 0.0;
   }
   else{
      x_div = x_stretch;
      x_num = 1.0;
   }

   if(y_stretch < 0.00001){
      y_div = 1.0;
      y_num = 0.0;
   }
   else{
      y_div = y_stretch;
      y_num = 1.0;
   }

      /**************************
      *
      *   Loop over image array
      *
      **************************/

   for(i=0; i<rows; i++){
      for(j=0; j<cols; j++){
         fi = i;
         fj = j;
         tmpx = (double)(j)*cosa         +
                (double)(i)*sina         +
                (double)(x_displace)     +
                (double)(x_num*fj/x_div) +
                (double)(x_cross*i*j);
         tmpy = (double)(i)*cosa         -
                (double)(j)*sina         +
                (double)(y_displace)     +
                (double)(y_num*fi/y_div) +
                (double)(y_cross*i*j);
         if(x_stretch != 0.0)
            tmpx = tmpx - (double)(fj*cosa + fi*sina);
         if(y_stretch != 0.0)
            tmpy = tmpy - (double)(fi*cosa - fj*sina);

         new_j = tmpx;
         new_i = tmpy;

         if(bilinear == 0){
            if(new_j < 0       ||
               new_j >= cols   ||
               new_i < 0       ||
               new_i >= rows)
               out_image[(j * w) + i] = 255;
            else
               out_image[(j * w) + i] = the_image[(new_j * w) + new_i];
         }  /* ends if bilinear */
         else{
            out_image[(j * w) + i] = 
               bilinear_interpolate(the_image,
                                    tmpx, tmpy,
                                    cols, rows*2);
         }  /* ends bilinear if */

      }  /* ends loop over j */
   }  /* ends loop over i */

}  /* ends geometry */

pixel_t image_get_pixel(image_t *image, int x, int y)
{
	pixel_t pix;
	int r, g, b;
	if((image->bpp / 8) == 1) {
		pix.color = image->data[(y * image->scanline) + x];
	} else {
		r = image->data[(y * image->scanline) + x];// image->palette[image->data[(y*(image->scanline))+x]];
		g = image->data[(y * image->scanline) + x + 1];//= image->palette[image->data[(y*(image->scanline))+x+1]];
		b = image->data[(y*(image->scanline))+x+2];
		//pix.color = find_color(r, g, b);
	}
	return (pix);
}

void putpix(image_t *im, int x, int y, int color)
{
	im->data[(y * im->scanline) + x] = color;
}

void print_chargen(image_t *widget, int x, int y, char *msg, int mlen, int opa, int bold)
{
	int i, k;
	int sx = 0, sy = y;
	int idx;
	int yy,xx,l;
	int xsy;
	chargen_t *chargen = &fonts[0];
	sdl_font_t font;
	font.color=0;
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
		for(yy = 0; yy < xsy; yy++, sy++) {
			sx = xx;
			for(k = (chargen->w + 1); k >= 0; k--, sx+=scale_x) {
				if((chargen->data[idx].byte[yy/scale_y] >> k) & 1) {
					putpix(widget, sx, sy, font.color);
					lx=1;
					if(bold) {
						putpix(widget, sx+1, sy, font.color);
					}
					if(scale_x > 1) {
						for(l = 1; l < scale_x; l++)
							putpix(widget, sx+l, sy, font.color);
					}
				} else if(opa){
					if(lx) {
						putpix(widget, sx, sy, font.color);

						lx=0;
					}
				} else {
				}
			} 
		}
		xx = sx;
	}
}
