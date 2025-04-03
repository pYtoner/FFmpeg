#include "subpixel_scale.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

static inline float get_pixel_bilinear(
    const uint8_t *src, int stride, int width, int height, float x, float y)
{
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float fx = x - x0;
    float fy = y - y0;

    x0 = fmaxf(0, fminf(x0, width - 1));
    x1 = fmaxf(0, fminf(x1, width - 1));
    y0 = fmaxf(0, fminf(y0, height - 1));
    y1 = fmaxf(0, fminf(y1, height - 1));

    float p00 = src[y0 * stride + x0];
    float p10 = src[y0 * stride + x1];
    float p01 = src[y1 * stride + x0];
    float p11 = src[y1 * stride + x1];

    return (1 - fx) * (1 - fy) * p00 +
           fx * (1 - fy) * p10 +
           (1 - fx) * fy * p01 +
           fx * fy * p11;
}

static void scale_plane(
    const uint8_t *src, int src_stride, int src_w, int src_h,
    float crop_x, float crop_y, float crop_w, float crop_h,
    uint8_t *dst, int dst_stride, int dst_w, int dst_h)
{
    for (int dy = 0; dy < dst_h; dy++)
    {
        for (int dx = 0; dx < dst_w; dx++)
        {
            float u = (dst_w > 1) ? (float)dx / (float)(dst_w - 1) : 0.0f;
            float v = (dst_h > 1) ? (float)dy / (float)(dst_h - 1) : 0.0f;

            float src_x = crop_x + u * crop_w;
            float src_y = crop_y + v * crop_h;

            float val = get_pixel_bilinear(src, src_stride, src_w, src_h, src_x, src_y);

            dst[dy * dst_stride + dx] = (uint8_t)fminf(fmaxf(val, 0.0f), 255.0f);
        }
    }
}

int sws_scale_fp_planar(const uint8_t *src[4], const int src_stride[4],
                        int src_width, int src_height,
                        float crop_x, float crop_y, float crop_w, float crop_h,
                        uint8_t *dst[4], const int dst_stride[4],
                        int dst_width, int dst_height,
                        enum AVPixelFormat pix_fmt)
{
    if (src_width <= 0 || src_height <= 0 || dst_width <= 0 || dst_height <= 0)
    {
        fprintf(stderr, "Invalid dimensions.\n");
        return -1;
    }

    switch (pix_fmt)
    {
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUVJ420P:
        // Y plane
        scale_plane(src[0], src_stride[0], src_width, src_height,
                    crop_x, crop_y, crop_w, crop_h,
                    dst[0], dst_stride[0], dst_width, dst_height);

        // U and V (chroma, half-res)
        scale_plane(src[1], src_stride[1], src_width / 2, src_height / 2,
                    crop_x / 2, crop_y / 2, crop_w / 2, crop_h / 2,
                    dst[1], dst_stride[1], dst_width / 2, dst_height / 2);

        scale_plane(src[2], src_stride[2], src_width / 2, src_height / 2,
                    crop_x / 2, crop_y / 2, crop_w / 2, crop_h / 2,
                    dst[2], dst_stride[2], dst_width / 2, dst_height / 2);
        break;

    case AV_PIX_FMT_GBRP:
        // Like YUV420P but with full resolution R/G/B
        for (int c = 0; c < 3; c++)
        {
            scale_plane(src[c], src_stride[c], src_width, src_height,
                        crop_x, crop_y, crop_w, crop_h,
                        dst[c], dst_stride[c], dst_width, dst_height);
        }
        break;

    case AV_PIX_FMT_YUV444P:
        for (int c = 0; c < 3; c++)
        {
            scale_plane(src[c], src_stride[c], src_width, src_height,
                        crop_x, crop_y, crop_w, crop_h,
                        dst[c], dst_stride[c], dst_width, dst_height);
        }
        break;

    case AV_PIX_FMT_RGBA:
        for (int channel = 0; channel < 4; channel++)
        {
            for (int dy = 0; dy < dst_height; dy++)
            {
                for (int dx = 0; dx < dst_width; dx++)
                {
                    float u = (dst_width > 1) ? (float)dx / (float)(dst_width - 1) : 0.0f;
                    float v = (dst_height > 1) ? (float)dy / (float)(dst_height - 1) : 0.0f;

                    float src_x = crop_x + u * crop_w;
                    float src_y = crop_y + v * crop_h;

                    int ix = (int)floorf(src_x);
                    int iy = (int)floorf(src_y);
                    float fx = src_x - ix;
                    float fy = src_y - iy;

                    int ix1 = fminf(ix + 1, src_width - 1);
                    int iy1 = fminf(iy + 1, src_height - 1);
                    ix = fmaxf(ix, 0);
                    iy = fmaxf(iy, 0);

                    const uint8_t *src_buf = src[0];
                    int stride = src_stride[0];

                    float c00 = src_buf[iy * stride + ix * 4 + channel];
                    float c10 = src_buf[iy * stride + ix1 * 4 + channel];
                    float c01 = src_buf[iy1 * stride + ix * 4 + channel];
                    float c11 = src_buf[iy1 * stride + ix1 * 4 + channel];

                    float val = (1 - fx) * (1 - fy) * c00 +
                                    fx * (1 - fy) * c10 +
                                    (1 - fx) * fy * c01 +
                                    fx * fy * c11;

                    dst[0][dy * dst_stride[0] + dx * 4 + channel] = (uint8_t)fminf(fmaxf(val, 0.0f), 255.0f);
                }
            }
        }
        break;

    case AV_PIX_FMT_GRAY8:
        // 1 plane, just like Y
        scale_plane(src[0], src_stride[0], src_width, src_height,
                    crop_x, crop_y, crop_w, crop_h,
                    dst[0], dst_stride[0], dst_width, dst_height);
        break;

    case AV_PIX_FMT_RGB24:
        for (int channel = 0; channel < 3; channel++)
        {
            for (int dy = 0; dy < dst_height; dy++)
            {
                for (int dx = 0; dx < dst_width; dx++)
                {
                    float u = (dst_width > 1) ? (float)dx / (float)(dst_width - 1) : 0.0f;
                    float v = (dst_height > 1) ? (float)dy / (float)(dst_height - 1) : 0.0f;

                    float src_x = crop_x + u * crop_w;
                    float src_y = crop_y + v * crop_h;

                    // Calculate float sampling position in RGB24 buffer
                    int stride = src_stride[0];
                    const uint8_t *src_buf = src[0];
                    int width = src_width;
                    int height = src_height;

                    float x0 = floorf(src_x);
                    float y0 = floorf(src_y);
                    int ix = (int)x0;
                    int iy = (int)y0;
                    float fx = src_x - x0;
                    float fy = src_y - y0;

                    int ix1 = fminf(ix + 1, width - 1);
                    int iy1 = fminf(iy + 1, height - 1);
                    ix = fmaxf(ix, 0);
                    iy = fmaxf(iy, 0);

                    // Manually sample individual color channel using bilinear interpolation
                    float c00 = src_buf[iy * stride + ix * 3 + channel];
                    float c10 = src_buf[iy * stride + ix1 * 3 + channel];
                    float c01 = src_buf[iy1 * stride + ix * 3 + channel];
                    float c11 = src_buf[iy1 * stride + ix1 * 3 + channel];

                    float val = (1 - fx) * (1 - fy) * c00 +
                                fx * (1 - fy) * c10 +
                                (1 - fx) * fy * c01 +
                                fx * fy * c11;

                    dst[0][dy * dst_stride[0] + dx * 3 + channel] = (uint8_t)fminf(fmaxf(val, 0.0f), 255.0f);
                }
            }
        }
        break;

    default:
        fprintf(stderr, "Unsupported format: %d\n", pix_fmt);
        return -1;
    }

    return 0; // Success
}
