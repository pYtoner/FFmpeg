#ifndef SUBPIXEL_SCALE_H
#define SUBPIXEL_SCALE_H

#include <stdint.h>
#include <libavutil/pixfmt.h>

int sws_scale_fp_planar(const uint8_t *src[4], const int src_stride[4],
                        int src_width, int src_height,
                        float crop_x, float crop_y, float crop_w, float crop_h,
                        uint8_t *dst[4], const int dst_stride[4],
                        int dst_width, int dst_height,
                        enum AVPixelFormat pix_fmt);

#endif
