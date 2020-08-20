#include <stdint.h>
#include <stdio.h>

typedef struct {
  uint32_t m;
} m_t;

typedef struct {
  uint32_t a; /*<< sleep time */
  uint32_t b; /*<< bed time */
  uint32_t c;
  uint32_t d;

  m_t *p;
} sleep_result_t;

sleep_result_t srt[10] = {{0, 0, 0, 0, NULL}};

int main() {
  sleep_result_t x = {0, 2, 19, 8};
  sleep_result_t y = {0};

  sleep_result_t *p = &x;
  sleep_result_t *q = &y;

  *p = *q;

  printf("x: %u, %u, %u, %u\n", x.a, x.b, x.c, x.d);
}
