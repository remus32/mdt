#include "mdt.h"
#include <string.h>

void mdt_cb_process(size_t frames) {
  void *j_input_buffer = jack_port_get_buffer(mdt_port_kin, frames);
  size_t ev_count = jack_midi_get_event_count(j_input_buffer);
  jack_midi_event_t jev;
  mdt_process_basetime = mdt_time;
  for (size_t i = 0; i < ev_count; i++) {
    int ec = jack_midi_event_get(&jev, j_input_buffer, i);
    assert(ec == 0);

    wqueue_frame *frame = mdt_wqueue_add(mdt_time + jev.time);
    frame->type = MDT_WQ_MIDI;
    assert(jev.size <= 3);
    memcpy(frame->data.midi.mesg, jev.buffer, 3);
  }

  mdt_port_kout_buffer = jack_port_get_buffer(mdt_port_kout, frames);
  jack_midi_clear_buffer(mdt_port_kout_buffer);
  mdt_port_synth_buffer = jack_port_get_buffer(mdt_port_synth, frames);
  jack_midi_clear_buffer(mdt_port_synth_buffer);

  if (mdt_reordered_synth) {
    wqueue_frame *fr = mdt_wqueue_add(mdt_time);
    fr->type = MDT_WQ_SYNTH_CONNECTED;
    mdt_reordered_synth = 0;
  }
  if (mdt_reordered_kb) {
    wqueue_frame *fr = mdt_wqueue_add(mdt_time);
    fr->type = MDT_WQ_KEYBOARD_CONNECTED;
    mdt_reordered_kb = 0;
  }

  mdt_wqueue_loop(frames);
};
