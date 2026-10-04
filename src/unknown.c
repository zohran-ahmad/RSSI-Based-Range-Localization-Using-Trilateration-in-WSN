#include "contiki.h"
#include "net/rime/rime.h"
#include "sys/node-id.h"
#include "lib/random.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define NA 4              /* anchors used: 3 or 4 */
#define AVG 1             /* 1 = average RSSI, 0 = last reading */
#define SIGMA 2           /* noise std dev in dB, 0 = off */
#define RSSI0 (-40.4f)    /* from calibration */
#define PLE 1.96f

struct msg { uint8_t id; int16_t x, y; };
static float ax[NA], ay[NA], sum[NA], last[NA];
static int cnt[NA];

static void recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  struct msg m;
  int i;
  float r = (int16_t)packetbuf_attr(PACKETBUF_ATTR_RSSI);
  r += SIGMA * 1.732f * (((int)(random_rand() % 2001) - 1000) / 1000.0f);
  memcpy(&m, packetbuf_dataptr(), sizeof(m));
  if(m.id < 1 || m.id > NA) return;
  i = m.id - 1;
  ax[i] = m.x; ay[i] = m.y;
  last[i] = r; sum[i] += r; cnt[i]++;
}
static const struct broadcast_callbacks cb = {recv};
static struct broadcast_conn bc;

static float dist(int i)    /* RSSI -> distance */
{
  float r = AVG ? sum[i] / cnt[i] : last[i];
  return powf(10.0f, (RSSI0 - r) / (10.0f * PLE));
}

PROCESS(unknown_proc, "Unknown");
AUTOSTART_PROCESSES(&unknown_proc);

PROCESS_THREAD(unknown_proc, ev, data)
{
  static struct etimer et;
  static int i;
  static float a11, a12, a22, b1, b2, d1, di, ex, ey, b, det, x, y;
  PROCESS_BEGIN();
  broadcast_open(&bc, 129, &cb);
  etimer_set(&et, 10 * CLOCK_SECOND);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    etimer_reset(&et);
    for(i = 0; i < NA; i++) if(cnt[i] == 0) break;
    if(i < NA) continue;                 /* wait until all anchors heard */
    a11 = a12 = a22 = b1 = b2 = 0;
    d1 = dist(0);
    for(i = 1; i < NA; i++) {            /* least-squares trilateration */
      di = dist(i);
      ex = 2 * (ax[0] - ax[i]);
      ey = 2 * (ay[0] - ay[i]);
      b = di*di - d1*d1 - ax[i]*ax[i] - ay[i]*ay[i] + ax[0]*ax[0] + ay[0]*ay[0];
      a11 += ex*ex; a12 += ex*ey; a22 += ey*ey;
      b1 += ex*b;   b2 += ey*b;
    }
    det = a11*a22 - a12*a12;
    x = (b1*a22 - b2*a12) / det;
    y = (a11*b2 - a12*b1) / det;
    printf("P,%d,%d,%d\n", node_id, (int)(x*100), (int)(y*100));  /* cm */
    for(i = 0; i < NA; i++) { sum[i] = 0; cnt[i] = 0; }
  }
  PROCESS_END();
}