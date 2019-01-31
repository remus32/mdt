#include "jackapi.h"
#include "notify.h"
#include <cstdio>
#include <iostream>

namespace {
struct process_cb {
  
};
struct mdt {
  uint64_t frames = 0;
  uint64_t cidx = 0;


  jack::client cl;

  jack::port kb_in;
  jack::port kb_out;
  jack::port synth_out;

  notifier graph_reorder_cv;

  mdt(): cl("MDT"), kb_in(cl, "Kb IN", 1), kb_out(cl, "Kb OUT", 0), synth_out(cl, "Synth OUT", 0) {};

  bool connect_ports_kb(const char *pattern, size_t idx, const char* a, bool invert) {
    auto r = cl.get_ports(pattern);
    if(r.size() >= 3) {
      auto target = r[r.size() - idx].c_str();
      cl.port_connect(invert ? target : a, invert ? a : target);
      return 1;
    };
    return 0;
  };
  bool connect_ports_synth() {
    auto r = cl.get_ports("qsynth:midi_[0-9]+");
    for(auto &i : r) std::cerr << i << '\n';
    if(r.size() > 0) {
      auto target = r.back().c_str();
      cl.port_connect(synth_out.name(), target);
      return 1;
    };
    return 0;
  };
  void connect_ports() {
    if(kb_out.num() == 0 || kb_in.num() == 0) {
      bool out = connect_ports_kb("system:midi_playback_[0-9]+", 1, kb_out.name(), 0);
      bool in = connect_ports_kb("system:midi_capture_[0-9]+", 2, kb_in.name(), 1);
      if(out && in) std::cerr << "Keyboard connected" << '\n';
    };

    if(synth_out.num() == 0) {
      bool b = connect_ports_synth();
      if(b) std::cerr << "Synth connected" << '\n';
    };
  };

  int main(int argc, char const *argv[]) {
    cl.set_reader_cb([&]() {
       graph_reorder_cv.notify(0);
       return 0;
    });
    cl.set_process_cb([&](jack_nframes_t nf) {
      frames += nf;

      jack::midi_read_buffer rbuf(kb_in, nf);
      jack::midi_write_buffer kbwbuf(kb_out, nf);
      jack::midi_write_buffer wbuf(synth_out, nf);
      for(auto &ev : rbuf.events()) wbuf.write(ev);
      kbwbuf.write(jack::midi_event(0, {159, 36, (rbuf.evcount()) ? 54 : 0}));
      cidx++;
      return 0;
    });
    cl.set_activated(1);

    for(;;) {
      graph_reorder_cv.wait();
      connect_ports();
    };

    return 0;
  };
};
};

int main(int argc, char const *argv[]) {
  mdt m;
  return m.main(argc, argv);
}
