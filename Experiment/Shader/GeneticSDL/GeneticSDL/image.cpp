#include "image.h"

uint64_t Image::
Compare(const Image &src) const {
    uint64_t error = 0;
    const unsigned char *s = (const unsigned char *)src.surface_->pixels;
    const unsigned char *d = (const unsigned char *)surface_->pixels;
    uint16_t pitch = surface_->pitch;
    uint16_t bpp = surface_->format->BytesPerPixel;
    int h = surface_->h;
#if 0
    bpp *= 2;
    pitch *= 2;
#endif
    while (--h) {
        const unsigned char *dline = d, *sline = s;
        int w = surface_->w;

        while (--w) { // expect pixels as RGBA!
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            int er = (int)(dline[0]) - (int)(sline[0]);
            int eg = (int)(dline[1]) - (int)(sline[1]);
            int eb = (int)(dline[2]) - (int)(sline[2]);
#else
            int er = (int)(dline[1]) - (int)(sline[1]);
            int eg = (int)(dline[2]) - (int)(sline[2]);
            int eb = (int)(dline[3]) - (int)(sline[3]);
#endif
            error += ((er * er) + (eb * eb) + (eg * eg));

            dline += bpp;
            sline += bpp;
        }
        d += pitch;
        s += pitch;
    }
    return error;
}

void Image::
Polygon(const Sint16 * vx, const Sint16 * vy, int n, Uint32 color)
{
    int i;
    int y, xa, xb;
    int miny, maxy;
    int x1, y1;
    int x2, y2;
    int ind1, ind2;
    int ints;
    int gfxPrimitivesPolyInts[n];
    uint8_t alpha = color & 0x000000ff;
    uint32_t R = color >> 24;
    uint32_t G = (color >> 16) & 0xff;
    uint32_t B = (color >> 8) & 0xff;

    R *= alpha;
    G *= alpha;
    B *= alpha;

    alpha = 255 - alpha;
    /*
     * Check visibility of clipping rectangle
     */
    if ((surface_->clip_rect.w==0) || (surface_->clip_rect.h==0)) return;

    /*
     * Sanity check number of edges
     */
    if (n < 3) return;
     
    /*
     * Determine Y maxima 
     */
    miny = vy[0];
    maxy = vy[0];
    for (i = 1; (i < n); i++) {
        if (vy[i] < miny) {
            miny = vy[i];
        } else if (vy[i] > maxy) {
            maxy = vy[i];
        }
    }

    /*
     * Draw, scanning y 
     */
    for (y = miny; (y <= maxy); y++) {
        ints = 0;
        for (i = 0; (i < n); i++) {
            if (!i) {
                ind1 = n - 1;
                ind2 = 0;
            } else {
                ind1 = i - 1;
                ind2 = i;
            }
            y1 = vy[ind1];
            y2 = vy[ind2];
            if (y1 < y2) {
                x1 = vx[ind1];
                x2 = vx[ind2];
            } else if (y1 > y2) {
                y2 = vy[ind1];
                y1 = vy[ind2];
                x2 = vx[ind1];
                x1 = vx[ind2];
            } else {
                continue;
            }
            if ( ((y >= y1) && (y < y2)) || ((y == maxy) && (y > y1) && (y <= y2)) ) {
                gfxPrimitivesPolyInts[ints++] = ((65536 * (y - y1)) / (y2 - y1)) * (x2 - x1) + (65536 * x1);
            } 	    
        }

        qsort(gfxPrimitivesPolyInts, ints, sizeof(int), CompareInt);

        for (i = 0; (i < ints); i += 2) {
            xa = gfxPrimitivesPolyInts[i] + 1;
            xa = (xa >> 16) + ((xa & 32768) >> 15);
            xb = gfxPrimitivesPolyInts[i+1] - 1;
            xb = (xb >> 16) + ((xb & 32768) >> 15);
            HLineAlpha(xa, xb, y, R, G, B, alpha);
        }
    }
}

void Image::
HLineAlpha(Sint16 x1, Sint16 x2, Sint16 y, Uint32 r, Uint32 g, Uint32 b, Uint8 alpha) {
        Uint32 R, G, B;
        Uint32 *row;

        row = (Uint32 *) surface_->pixels + y * surface_->pitch / 4 + x1;
        x2 -= x1;

        for (; x2 >= 0; x2--, row++) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            R = ((unsigned char *)row)[0];
            G = ((unsigned char *)row)[1];
            B = ((unsigned char *)row)[2];
#else
            R = ((unsigned char *)row)[3];
            G = ((unsigned char *)row)[2];
            B = ((unsigned char *)row)[1];
#endif           
            R *= alpha;
            G *= alpha;
            B *= alpha;
            R += r;
            G += g;
            B += b;

            // every component here has to be divided /256, but we can avoid this
            *row = ((R & 0xff00) << 16) | ((G & 0xff00) << 8) | (B & 0xff00) | 0xff;
        }
    }

