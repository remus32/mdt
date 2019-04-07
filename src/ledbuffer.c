#include "mdt.h"
#include <string.h>

static mdt_wq_receiver ledrecv = {NULL};
static uint8_t oldledbuffer[16];
static bool fullrefresh = 1;

static void kb_reordered(wqueue_frame *fr, void *arg) {
  (void *)fr;
  (void *)arg;

  mdt_ledbuffer_dirty = 1;
  fullrefresh = 1;
}

void mdt_ledbuffer_init() {
  memset(mdt_ledbuffer, 0, sizeof(mdt_ledbuffer));
  memset(oldledbuffer, 0, sizeof(oldledbuffer));
  mdt_ledbuffer_dirty = 0;

  ledrecv.reorder_kb = kb_reordered;
  ledrecv.active = 1;
  mdt_wqrecv_add(&ledrecv);
};

void mdt_ledbuffer_process() {
  mdt_midi midi;
  for (size_t i = 0; i < 16; i++) {
    if (fullrefresh || oldledbuffer[i] != mdt_ledbuffer[i]) {
      midi = (mdt_midi){.mesg = {159, 36 + i, mdt_ledbuffer[i]}};
      mdt_write_kout(mdt_time, &midi);
      oldledbuffer[i] = mdt_ledbuffer[i];
    }
  }
  fullrefresh = 0;
  mdt_ledbuffer_dirty = 0;
};
