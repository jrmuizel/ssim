/*
 * Copyright 2002-2008 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "ssim.h"

#define PNG_DEBUG 3
#include <png.h>

void abort_(const char * s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}

int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;

png_bytep *read_png_file(char* file_name)
{
	png_bytep * row_pointers;
	char header[8];	// 8 is the maximum size that can be checked

	/* open file and test for it being a png */
	FILE *fp = fopen(file_name, "rb");
	if (!fp)
		abort_("[read_png_file] File %s could not be opened for reading", file_name);
	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8))
		abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
		abort_("[read_png_file] png_create_read_struct failed");

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		abort_("[read_png_file] png_create_info_struct failed");

	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during init_io");

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	width = info_ptr->width;
	height = info_ptr->height;
	color_type = info_ptr->color_type;
	bit_depth = info_ptr->bit_depth;

	number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);


	/* read file */
	if (setjmp(png_jmpbuf(png_ptr)))
		abort_("[read_png_file] Error during read_image");

	row_pointers = malloc(sizeof(png_bytep) * height);
	for (y=0; y<height; y++)
		row_pointers[y] = (png_byte*) malloc(info_ptr->rowbytes);

	png_read_image(png_ptr, row_pointers);

        fclose(fp);
	return row_pointers;
}


int clamp(int a)
{
	if (a > 255)
		return 255;
	if (a < 0)
		return 0;
	return a;
}

int yc(int r, int g, int b)
{
	return clamp(16 + 65.738*r/256 + 129.057*g/256 + 25.064*b/256);
}

int uc(int r, int g, int b)
{
	return clamp(128 + -37.945*r/256 - 74.494*g/256 + 112.439*b/256);
}
int vc(int r, int g, int b)
{
	return clamp(128 + 112.439*r/256 - 94.154*g/256 - 18.285*b/256);
}

void process_file(png_bytep * row_pointers, png_bytep *row_pointers2)
{
	if (info_ptr->color_type != PNG_COLOR_TYPE_RGB)
		abort_("[process_file] color_type of input file must be PNG_COLOR_TYPE_RGBA (is %d)", info_ptr->color_type);

	int bytes_per_component = 3;

	YV12_BUFFER_CONFIG src;
	src.y_height = height;
	src.y_width = width;
	src.y_stride = width;
	src.uv_height = height;
	src.uv_width = width;
	src.uv_stride = width;
	src.y_buffer = malloc(height*width);
	src.y_buffer = malloc(height*width);
	src.u_buffer = malloc(height*width);
	src.v_buffer = malloc(height*width);

	YV12_BUFFER_CONFIG dest;
	dest.y_buffer = malloc(height*width);
	dest.u_buffer = malloc(height*width);
	dest.v_buffer = malloc(height*width);
	dest.y_height = height;
	dest.y_width = width;
	dest.y_stride = width;
	dest.uv_height = height;
	dest.uv_width = width;
	dest.uv_stride = width;

	for (y=0; y<height; y++) {
		png_byte* row1 = row_pointers[y];
		png_byte* row2 = row_pointers2[y];
		for (x=0; x<width; x++) {
			int src_r = row1[x*3 + 0];
			int src_g = row1[x*3 + 1];
			int src_b = row1[x*3 + 2];
			//int src_a = row1[x*3 + 3];

			src.y_buffer[y*width + x] = yc(src_r, src_g, src_b);
			src.u_buffer[y*width + x] = uc(src_r, src_g, src_b);
			src.v_buffer[y*width + x] = vc(src_r, src_g, src_b);

			int dst_r = row2[x*3 + 0];
			int dst_g = row2[x*3 + 1];
			int dst_b = row2[x*3 + 2];
			//int dst_a = row2[x*4 + 3];

			dest.y_buffer[y*width + x] = yc(dst_r, dst_g, dst_b);
			dest.u_buffer[y*width + x] = uc(dst_r, dst_g, dst_b);
			dest.v_buffer[y*width + x] = vc(dst_r, dst_g, dst_b);


		}
	}
	double ssim_y;
	double ssim_u;
	double ssim_v;
	double ssim = vp8_calc_ssimg
		(
		 &src,
		 &dest,
		 &ssim_y,
		 &ssim_u,
		 &ssim_v
		);

	printf("ssimg: %f\n\ty: %f\n\tu: %f\n\tv: %f\n", 1/(1-ssim), 1/(1-ssim_y), 1/(1-ssim_u), 1/(1-ssim_v));
	double weight;
	double p = vp8_calc_ssim
		(
		 &src, &dest,
		 1, &weight
		);
	printf("ssim: %f\n", 1/(1-p));


}


int main(int argc, char **argv)
{
	if (argc != 3)
		abort_("Usage: program_name <file1> <file2>");

	png_bytep * row_pointers;
	png_bytep * row_pointers2;
	row_pointers = read_png_file(argv[1]);
	row_pointers2 = read_png_file(argv[2]);
	process_file(row_pointers, row_pointers2);

        return 0;
}
