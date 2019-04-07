#include "mdt.h"

static pthread_mutex_t reorder_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t reorder_cv = PTHREAD_COND_INITIALIZER;
static size_t reorder_op = 0;
static uint8_t log_level;

void mdt_bg_log(uint8_t level) {
  pthread_mutex_lock(&reorder_mutex);
  reorder_op = 2;
  log_level = level;
  pthread_cond_signal(&reorder_cv);
  pthread_mutex_unlock(&reorder_mutex);
};

void mdt_bg_loop() {
  for (;;) {
    reorder_op = 0;

    pthread_mutex_lock(&reorder_mutex);
    while (reorder_op == 0)
      pthread_cond_wait(&reorder_cv, &reorder_mutex);
    pthread_mutex_unlock(&reorder_mutex);

    if (reorder_op == 1)
      mdt_cb_reorder();
    else if (reorder_op == 2)
      MDT_LOGF(log_level, "%s", mdt_logbuf);
    else
      RL_NOIMPL;
  }
};
static int callback_reorder(void *arg) {
  pthread_mutex_lock(&reorder_mutex);
  reorder_op = 1;
  pthread_cond_signal(&reorder_cv);
  pthread_mutex_unlock(&reorder_mutex);
  return 0;
};

static void callback_shutdown(void *arg) {
  (void *)arg;
  exit(0);
}

static int callback_process(jack_nframes_t frames, void *arg) {
  (void *)arg;
  mdt_cb_process(frames);
  return 0;
}

void mdt_jack_init_callbacks() {
  jack_on_shutdown(mdt_jack_client, callback_shutdown, NULL);
  int i;
  i = jack_set_graph_order_callback(mdt_jack_client, callback_reorder, NULL);
  assert(i == 0);
  i = jack_set_process_callback(mdt_jack_client, callback_process, NULL);
  assert(i == 0);
};
