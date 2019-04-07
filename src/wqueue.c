#include "mdt.h"

#define LEAF_COUNT 1024

struct wqueue_leaf;
typedef struct wqueue_leaf wqueue_leaf;
struct wqueue_leaf {
  size_t time;
  wqueue_leaf *next;

  wqueue_frame data;
};
static wqueue_leaf leaf_bank[LEAF_COUNT];
static wqueue_leaf *wqueue;
static wqueue_leaf *leaf_bin;

static wqueue_timer *timer_list;

void mdt_timer_add(wqueue_timer *timer) {
  timer->next = timer_list;
  timer_list = timer;
};
void mdt_timer_rm(wqueue_timer *timer) {
  wqueue_timer **last_ptr = &timer_list;
  for (wqueue_timer *it = timer_list; it; it = it->next) {
    if (it == timer) {
      *last_ptr = it->next;
      it->next = NULL;
      break;
    }
    last_ptr = &it->next;
  }
};

size_t mdt_time;

void mdt_wqueue_init() {
  timer_list = NULL;
  wqueue = NULL;
  leaf_bin = NULL;

  for (size_t i = 0; i < LEAF_COUNT; i++) {
    leaf_bank[i].next = leaf_bin;
    leaf_bin = leaf_bank + i;
  }

  mdt_time = 0;
};

static void sort_leaf(wqueue_leaf *new_leaf) {
  size_t time = new_leaf->time;
  wqueue_leaf *prev_leaf = NULL;
  for (wqueue_leaf *it = wqueue; it; it = it->next) {
    if (it->time <= time)
      prev_leaf = it;
    else
      break;
  }
  if (prev_leaf) {
    new_leaf->next = prev_leaf->next;
    prev_leaf->next = new_leaf;
  } else {
    new_leaf->next = wqueue;
    wqueue = new_leaf;
  }
};

wqueue_frame *mdt_wqueue_add(size_t time) {
  if (!leaf_bin) {
    MDT_LOGF(RL_LOGLEVEL_FATAL, "WQueue full");
    RL_ABORT;
  }

  wqueue_leaf *leaf = leaf_bin;
  leaf_bin = leaf->next;
  leaf->time = time;
  sort_leaf(leaf);

  return &leaf->data;
};

void mdt_wqueue_loop(size_t frames) {
  size_t base_time = mdt_time;
  size_t end_time = base_time + frames;

  int bestway;
  size_t besttime;
  wqueue_timer *besttimer = NULL;

  for (;;) {
    bestway = 0;
    besttime = ~0;

    if (wqueue) {
      bestway = 1;
      besttime = wqueue->time;
    }
    for (wqueue_timer *it = timer_list; it; it = it->next) {
      if (it->active) {
        if (it->expire < besttime) {
          bestway = 2;
          besttimer = it;
          besttime = it->expire;
        }
      }
    }

    if (!bestway || (besttime >= end_time))
      break;

    mdt_time = besttime;

    if (bestway == 1) {
      wqueue_frame *data = &wqueue->data;

      wqueue_leaf *next_leaf = wqueue->next;
      wqueue->next = leaf_bin;
      leaf_bin = wqueue;
      wqueue = next_leaf;

      mdt_process_frame(mdt_time, data);
    } else if (bestway == 2) {
      if (besttimer->repeat_interval) {
        besttimer->expire += besttimer->repeat_interval;
      } else
        besttimer->active = 0;
      mdt_process_frame(mdt_time, &besttimer->frame);
    } else
      RL_NOIMPL;

    if (mdt_ledbuffer_dirty)
      mdt_ledbuffer_process();
  }

  mdt_time = end_time;
};
