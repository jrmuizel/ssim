typedef struct
{
	int y_width;
	int y_height;
	int y_stride;

	int uv_width;
	int uv_height;
	int uv_stride;

	unsigned char *y_buffer;
	unsigned char *u_buffer;
	unsigned char *v_buffer;
} YV12_BUFFER_CONFIG;

double vp8_calc_ssimg
(
    YV12_BUFFER_CONFIG *source,
    YV12_BUFFER_CONFIG *dest,
    double *ssim_y,
    double *ssim_u,
    double *ssim_v
);
double vp8_calc_ssim
(
    YV12_BUFFER_CONFIG *source,
    YV12_BUFFER_CONFIG *dest,
    int lumamask,
    double *weight
);

