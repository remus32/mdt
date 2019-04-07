#include <jack/jack.h>
#include <jack/midiport.h>
#include <pthread.h>
#include <remlib.h>
#include <stdio.h>

#define MDT_LOGBUF_SIZE 256
char mdt_logbuf[MDT_LOGBUF_SIZE];

#define MDT_LOGF(LEVEL, ...) rl_logf(&rl_lctx_remlib, LEVEL, __VA_ARGS__)
#define MDT_RT_LOGF(LEVEL, ...)                                                \
  ({                                                                           \
    snprintf(mdt_logbuf, MDT_LOGBUF_SIZE, __VA_ARGS__);                        \
    mdt_bg_log(LEVEL);                                                         \
  })

void mdt_jack_init_client();
void mdt_jack_init_ports();
void mdt_jack_init_callbacks();
void mdt_jack_activate();

struct mdt_reorder_state {
  bool kin;
  bool kout;
  bool synth;
};
struct mdt_reorder_state mdt_reorder_state();
void mdt_cb_reorder();
void mdt_bg_log(uint8_t level);
void mdt_cb_process(size_t frames);
void mdt_bg_loop();

jack_client_t *mdt_jack_client;

jack_port_t *mdt_port_kin;
jack_port_t *mdt_port_kout;
jack_port_t *mdt_port_synth;

void *mdt_port_kout_buffer;
void *mdt_port_synth_buffer;

bool mdt_reordered_synth;
bool mdt_reordered_kb;

size_t mdt_time;
size_t mdt_process_basetime;

#define MDT_WQ_MIDI 1
#define MDT_WQ_WRITE_KOUT 2
#define MDT_WQ_WRITE_SYNTH 3
#define MDT_WQ_SYNTH_CONNECTED 4
#define MDT_WQ_KEYBOARD_CONNECTED 5
#define MDT_WQ_CALLFUN 6

struct mdt_midi;
typedef struct mdt_midi mdt_midi;
struct mdt_midi {
  uint8_t mesg[3];
};

void mdt_write_kout(size_t time, mdt_midi *mesg);
void mdt_write_synth(size_t time, mdt_midi *mesg);

struct wqueue_frame;
typedef struct wqueue_frame wqueue_frame;
typedef void (*mdt_process_cb_t)(wqueue_frame *, void *);
struct wqueue_frame {
  union {
    mdt_midi midi;
    struct {
      mdt_process_cb_t fun;
      void *arg;
    } call;
  } data;
  uint8_t type;
};
void mdt_wqueue_init();
void mdt_wqueue_loop(size_t frames);
wqueue_frame *mdt_wqueue_add(size_t time);

struct wqueue_timer;
typedef struct wqueue_timer wqueue_timer;
struct wqueue_timer {
  wqueue_timer *next;
  size_t expire;
  wqueue_frame frame;
  bool active;
  size_t repeat_interval;
};
void mdt_timer_add(wqueue_timer *);
void mdt_timer_rm(wqueue_timer *);

struct mdt_wq_receiver;
typedef struct mdt_wq_receiver mdt_wq_receiver;
struct mdt_wq_receiver {
  mdt_process_cb_t midi_in;
  mdt_process_cb_t midi_out_kb;
  mdt_process_cb_t midi_out_synth;

  mdt_process_cb_t reorder_kb;
  mdt_process_cb_t reorder_synth;

  void *arg;

  mdt_wq_receiver *next;
  bool active;
};
void mdt_wqrecv_init();
void mdt_wqrecv_writer_init();
void mdt_wqrecv_add(mdt_wq_receiver *);
void mdt_wqrecv_rm(mdt_wq_receiver *);

void mdt_process_frame(size_t, wqueue_frame *);

double mdt_time_to_secs(size_t);

void mdt_ledbuffer_init();
void mdt_ledbuffer_process();
uint8_t mdt_ledbuffer[16];
bool mdt_ledbuffer_dirty;
