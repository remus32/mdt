#include "mdt.h"
#include <string.h>

static void add_write_ev(uint8_t type, size_t time, mdt_midi *mesg) {
  assert(time >= mdt_time);
  wqueue_frame *fr = mdt_wqueue_add(time);
  fr->type = type;
  memcpy(&fr->data.midi, mesg, sizeof(mdt_midi));
}

void mdt_write_kout(size_t time, mdt_midi *mesg) {
  add_write_ev(MDT_WQ_WRITE_KOUT, time, mesg);
};
void mdt_write_synth(size_t time, mdt_midi *mesg) {
  add_write_ev(MDT_WQ_WRITE_SYNTH, time, mesg);
};

static void jack_write(void *buffer, size_t time, wqueue_frame *frame) {
  assert(buffer);
  size_t target_time = time - mdt_process_basetime;
  assert(target_time < 1024);
  int i = jack_midi_event_write(buffer, target_time, frame->data.midi.mesg, 3);
  assert(i == 0);
};

static void mdt_process_write_kout(wqueue_frame *frame, void *arg) {
  jack_write(mdt_port_kout_buffer, mdt_time, frame);
};
static void mdt_process_write_synth(wqueue_frame *frame, void *arg) {
  jack_write(mdt_port_synth_buffer, mdt_time, frame);
};
static mdt_wq_receiver mdt_receiver_midiwriter = {NULL};
void mdt_wqrecv_writer_init() {
  mdt_receiver_midiwriter.midi_out_kb = mdt_process_write_kout;
  mdt_receiver_midiwriter.midi_out_synth = mdt_process_write_synth;
  mdt_receiver_midiwriter.active = 1;
  mdt_wqrecv_add(&mdt_receiver_midiwriter);
};
