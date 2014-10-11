#ifndef IMAGE_H
#define IMAGE_H
typedef struct {
	uint8_t color;
	uint8_t ascii;
}pixel_t;


typedef struct image
{
	int 	 w,h,bpp;
	uint32_t scanline;
	uint16_t palsize;
	uint8_t	 *data;
	uint8_t	 *palette;
	uint8_t  lut[8192];
} image_t;

typedef struct {
	image_t	*image;
	uint8_t *palette;
	uint16_t palsize;
	uint8_t	lut[8192];
} texture_t;

image_t *image_load_png( char *filename);
void image_build_palette(image_t *image);
image_t *image_load_ppm( char *filename );
image_t *image_alloc(int w, int h, int bpp, int palsize);
void *image_alloc_palette(int palsize);
void image_scale_up(image_t *in, image_t *out);
void image_scale_down(image_t *in, image_t *out);
image_t *image_crop(image_t *in, int xx, int yy, int w, int h);
void image_free(image_t *t);
uint8_t bilinear_interpolate(uint8_t *the_image, double x, double y, long rows, long cols);
void geometry(uint8_t *the_image, uint8_t *out_image, 
		int w,
        double x_angle,
        double x_stretch, double y_stretch,
        uint8_t x_displace, uint8_t y_displace,
        double x_cross, double y_cross,
        int bilinear,
        long rows,
        long cols);
 



#endif
