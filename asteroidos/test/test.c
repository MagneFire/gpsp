#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "gles_video.h"

#define RGB15(r, g, b)  (((r) << (5+6)) | ((g) << 6) | (b))





void showbitmap( uint32_t wd, uint32_t ht, uint32_t f) {
int index;
uint16_t j,k;
uint8_t r,g,b;
uint16_t * bitmap;

    bitmap=malloc(wd*ht*sizeof(uint16_t));

    b=16;
    index=0;
    for (j=0;j<ht;j++) {
	r=(j*31)/(ht-1);
	for (k=0; k<wd ; k++) {
	    g=(k*31)/(wd-1);
	    bitmap[index++]=RGB15(r,g,b);
	}	
    }
bitmap[0]=0;

video_init(wd,ht,f);
video_draw(bitmap);
printf("DRAW VIDEO\r\n");
sleep(1);
video_close();
free(bitmap);
}

int main(void) {

showbitmap( 320, 240,0);
showbitmap( 320, 240,1);
showbitmap( 320, 240,2);
showbitmap( 240, 160,0);
showbitmap( 400, 272,0);

return 0;
}