#include "contiki.h"
#include "net/rime/rime.h"
#include "sys/node-id.h"
#include "lib/random.h"

struct msg { uint8_t id; int16_t x, y; };
static const int pos[4][2] = {{0,0},{40,0},{0,40},{40,40}};  /* metres */
static const struct broadcast_callbacks cb = {NULL};
static struct broadcast_conn bc;

PROCESS(anchor_proc, "Anchor");
AUTOSTART_PROCESSES(&anchor_proc);

PROCESS_THREAD(anchor_proc, ev, data)
{
  static struct etimer et;
  static struct msg m;
  PROCESS_BEGIN();
  broadcast_open(&bc, 129, &cb);
  m.id = node_id;
  m.x = pos[node_id - 1][0];
  m.y = pos[node_id - 1][1];
  while(1) {
    etimer_set(&et, CLOCK_SECOND + random_rand() % CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    packetbuf_copyfrom(&m, sizeof(m));
    broadcast_send(&bc);
  }
  PROCESS_END();
}