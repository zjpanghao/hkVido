#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <turbojpeg.h>
#include <vector>

void yuv420sp_to_yuv420p(unsigned char* yuv420sp, unsigned char* yuv420p, int width, int height)  {  
	int i, j;      
	int y_size = width * height;        
	unsigned char* y = yuv420sp;      
	unsigned char* uv = yuv420sp + y_size;        
	unsigned char* y_tmp = yuv420p;      
	unsigned char* u_tmp = yuv420p + y_size;      
	unsigned char* v_tmp = yuv420p + y_size * 5 / 4;        // y      
	memcpy(y_tmp, y, y_size);        // u      
	for (j = 0, i = 0; j < y_size/2; j+=2, i++) {    
		v_tmp[i] = uv[j + 1];          
		u_tmp[i] = uv[j];      
	}  

}




int yuv2Jpg(unsigned char *imgBuf, int width, int height, std::vector<unsigned char> &outBuf) {
  tjhandle tjInstance = NULL;
  int rc = 0;
  unsigned char *jpegBuf = NULL;  /* Dynamically allocate the JPEG buffer */
  unsigned long jpegSize = 0;
  int padding = 4;
  int need_size = tjBufSizeYUV2(width, padding, height, TJ_420);  
  if (need_size != width * height *3 /2) {
  	return -1;
  }
  if ((tjInstance = tjInitCompress()) == NULL) {
	return -2;
  }

  if (tjCompressFromYUV(tjInstance, imgBuf, width, padding, height, TJ_420,
                    &jpegBuf, &jpegSize, 95, 0) < 0) {
	rc = -3;
	goto END;
  }
  outBuf.assign(jpegBuf, jpegBuf + jpegSize);	
END:
  if (tjInstance) {
  	tjDestroy(tjInstance);
	tjInstance = NULL;
  }
  if (jpegBuf) {
  	tjFree(jpegBuf);
  }
  return rc;
}
