CFLAGS=`pkg-config --cflags libpng` -ggdb3
LDFLAGS=`pkg-config --libs libpng` -ggdb3
ssim: png.o ssim.o
