#include "mdt.h"

static wqueue_timer timers[1];

static void timer_expired(wqueue_frame *fr, void *arg) {
  mdt_ledbuffer[(uint8_t)arg] = !mdt_ledbuffer[(uint8_t)arg];
  mdt_ledbuffer_dirty = 1;
}
static void userland() {
  for (size_t i = 0; i < 1; i++) {
    size_t interval = (i + 1) * 1000;
    timers[i] = (wqueue_timer){
        .expire = 0,
        .frame = {.type = MDT_WQ_CALLFUN,
                  .data = {.call = {.fun = timer_expired, .arg = (void *)i}}},
        .active = 1,
        .repeat_interval = interval};
    mdt_timer_add(&timers[i]);
  }
}

int main(int argc, char const *argv[]) {

  rl_loglevel_min = RL_LOGLEVEL_FINE;

  printf("Loading MDT\n");

  mdt_wqueue_init();
  mdt_wqrecv_init();
  mdt_ledbuffer_init();

  mdt_jack_init_client();
  mdt_jack_init_ports();
  mdt_jack_init_callbacks();

  // userland();

  mdt_jack_activate();

  mdt_bg_loop();
  return 0;
}
