#ifndef INCLUDE_JPG_H
#define INCLUDE_JPG_H
void yuv420sp_to_yuv420p(unsigned char* yuv420sp, unsigned char* yuv420p, int width, int height);
int yuv2Jpg(unsigned char *imgBuf, int width, int height, std::vector<unsigned char> &outBuf);
#endif
