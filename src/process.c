#include "mdt.h"

static mdt_wq_receiver *recv_list = NULL;

void mdt_wqrecv_add(mdt_wq_receiver *recv) {
  recv->next = recv_list;
  recv_list = recv;
};

static void mdt_receiver_synthsend_midi_in(wqueue_frame *frame, void *arg) {
  (void *)arg;
  mdt_write_synth(mdt_time, &frame->data.midi);
};

static mdt_wq_receiver mdt_receiver_synthsend = {NULL};

void mdt_wqrecv_init() {
  mdt_wqrecv_writer_init();
  mdt_receiver_synthsend.midi_in = mdt_receiver_synthsend_midi_in;
  mdt_receiver_synthsend.active = 1;
  mdt_wqrecv_add(&mdt_receiver_synthsend);
};

void mdt_process_frame(size_t time, wqueue_frame *frame) {
  MDT_RT_LOGF(RL_LOGLEVEL_INFO, "event at time %f", mdt_time_to_secs(time));

  if (frame->type == MDT_WQ_CALLFUN)
    frame->data.call.fun(frame, frame->data.call.arg);
  else
    for (mdt_wq_receiver *it = recv_list; it; it = it->next) {
      if (it->active) {
        if (frame->type == MDT_WQ_MIDI && it->midi_in)
          it->midi_in(frame, it->arg);
        else if (frame->type == MDT_WQ_WRITE_KOUT && it->midi_out_kb)
          it->midi_out_kb(frame, it->arg);
        else if (frame->type == MDT_WQ_WRITE_SYNTH && it->midi_out_synth)
          it->midi_out_synth(frame, it->arg);
        else if (frame->type == MDT_WQ_SYNTH_CONNECTED && it->reorder_synth)
          it->reorder_synth(frame, it->arg);
        else if (frame->type == MDT_WQ_KEYBOARD_CONNECTED && it->reorder_kb)
          it->reorder_kb(frame, it->arg);
      }
    }
};
