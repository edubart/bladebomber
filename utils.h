#ifndef UTILS_H
#define UTILS_H

#include <riv.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
typedef riv_vec2f vec2;
typedef riv_vec2i vec2i;
typedef riv_recti recti;

static inline f64 sqr(f64 x) { return x*x; }
static inline f64 abst(f64 x) { return x >= 0.0 ? x : -x; }
static inline i64 absi(i64 x) { return x >= 0 ? x : -x; }
static inline f64 sign(f64 x) { return x > 0.0 ? 1.0 : (x < 0 ? -1 : 0); }
static inline i64 isign(f64 x) { return x > 0.0 ? 1 : (x < 0.0 ? -1 : 0); }
static inline f64 min(f64 a, f64 b) { return (a <= b) ? a : b; }
static inline f64 max(f64 a, f64 b) { return (a >= b) ? a : b; }
static inline i64 mini(i64 a, i64 b) { return (a <= b) ? a : b; }
static inline i64 maxi(i64 a, i64 b) { return (a >= b) ? a : b; }
static inline f64 clamp(f64 v, f64 vmin, f64 vmax) { return min(max(v,vmin),vmax); }
static inline f64 clampi(i64 v, i64 vmin, i64 vmax) { return mini(maxi(v,vmin),vmax); }
static inline i64 iceil(f64 x) { i64 ix = (i64)x; return (x < 0 || x == (f64)ix) ? ix : ix + 1; }
static inline i64 iround(f64 x) { return (i64)(x + 0.5); }
static inline i64 ifloor(f64 x) { return (i64)(x); }
static inline i64 itrunc(f64 x) { return (x >= 0.0) ? (i64)(x) : -(i64)(-x); }

// vec2
static inline vec2i ifloor_vec2(vec2 v) { return (vec2i){ifloor(v.x), ifloor(v.y)}; }
static inline vec2 sub_vec2(vec2 a, vec2 b) { return (vec2){a.x - b.x, a.y - b.y}; }
static inline vec2 add_vec2(vec2 a, vec2 b) { return (vec2){a.x + b.x, a.y + b.y}; }
static inline vec2 mul_vec2(vec2 a, vec2 b) { return (vec2){a.x * b.x, a.y * b.y}; }
static inline vec2 mul_vec2_scalar(vec2 a, f64 b) { return (vec2){a.x * b, a.y * b}; }
static inline vec2 div_vec2(vec2 a, vec2 b) { return (vec2){a.x / b.x, a.y / b.y}; }
static inline vec2 sqr_vec2(vec2 a) { return (vec2){a.x * a.x, a.y * a.y}; }
static inline vec2 abs_vec2(vec2 a) { return (vec2){abst(a.x), abst(a.y)}; }
static inline vec2 make_vec2(f64 v) { return (vec2){v, v}; }
static inline f64 sum_vec2(vec2 v) { return v.x + v.y; }
static inline f64 distsqr_vec2(vec2 a, vec2 b) { return sum_vec2(sqr_vec2(sub_vec2(a, b))); }

// recti
static inline recti expand_recti(recti r, i64 b) { return (recti){r.x - b, r.y - b, r.width + 2*b, r.height + 2*b}; }
static inline bool overlaps_recti(recti a, recti b) { return a.x + a.width > b.x && b.x + b.width > a.x && a.y + a.height > b.y && b.y + b.height > a.y; }

static recti riv_get_sprite_bbox(u32 n, u64 sps_id, i64 nx, i64 ny) {
  riv_spritesheet* spritesheet = &riv->spritesheets[sps_id];
  riv_image *image = &riv->images[spritesheet->image_id];
  i64 pitch = image->width;
  i64 ncols = pitch / spritesheet->cell_width;
  i64 sx = (n % ncols)*spritesheet->cell_width;
  i64 sy = (n / ncols)*spritesheet->cell_height;
  i64 sw = spritesheet->cell_width * nx;
  i64 sh = spritesheet->cell_height * ny;
  i64 color_key = image->color_key;
  i64 minx = sx+sw-1;
  i64 miny = sy+sh-1;
  i64 maxx = sx-1;
  i64 maxy = sy-1;
  u8 *pixels = image->pixels;
  for (i64 y=sy;y<sy+sh;++y) {
    for (i64 x=sx;x<sx+sw;++x) {
      if (pixels[y*pitch + x] != color_key) {
        if (x < minx) minx = x;
        if (y < miny) miny = y;
        if (x > maxx) maxx = x;
        if (y > maxy) maxy = y;
      }
    }
  }
  return (recti){minx - sx, miny - sy, maxx - minx + 1, maxy - miny + 1};
}

#endif
