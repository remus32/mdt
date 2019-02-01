#include "jackapi.h"
#include "notify.h"
#include <cstdio>
#include <variant>
#include <functional>
#include <iostream>
#include <queue>

namespace {
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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
    if(kb_out.num() == 0) connect_ports_kb("system:midi_playback_[0-9]+", 1, kb_out.name(), 0);
    if(kb_in.num() == 0)  connect_ports_kb("system:midi_capture_[0-9]+", 2, kb_in.name(), 1);
    if(kb_in.num() && kb_out.num()) std::cerr << "Keyboard connected" << '\n';

    if(synth_out.num() == 0) {
      bool b = connect_ports_synth();
      if(b) std::cerr << "Synth connected" << '\n';
    };
  };

  enum subev_t {
    SUBEV_REORDER;
  };
  struct event {
    size_t abstime;
    std::variant<jack::midi_event, std::function<void(mdt &)>, subev_t> data;

    template<class A>
    event(size_t _t, const A &_d): abstime(_t), data(_d) {};
  };
  struct event_cmp {
    bool operator()(const event &a, const event &b) const {
      return a.abstime > b.abstime;
    };
  };
  std::priority_queue<event, std::vector<event>, event_cmp> event_queue;

  template<class Fun>
  void set_timer(size_t delay, Fun f) {
    event_queue.emplace(frames + delay, f);
  };
  template<class Fun>
  void set_timer_repeat(size_t delay, Fun f) {
    auto fun = [this, delay, f](mdt &ev) {
      set_timer_repeat(delay, f);
      f(ev);
    };
    event_queue.emplace(frames + delay, fun);
  };

  uint8_t ledbuf_old[16] = {1};
  uint8_t ledbuf[16] = {0};
  bool update_ledbuf = 1;

  size_t ledctx = 0;

  struct processor {
    mdt &m;

    size_t timebase;

    jack::midi_write_buffer kbwbuf;
    jack::midi_write_buffer snwbuf;

    jack_nframes_t localtime() {
      return m.frames - timebase;
    };

    processor(mdt &_m, size_t tb, jack_nframes_t nf): m(_m), timebase(tb), kbwbuf(m.kb_out, nf), snwbuf(m.synth_out, nf) {};

    void process_midi(size_t abstime, const jack::midi_event me) {
      size_t ourctx = ++(m.ledctx);
      std::cerr << "ourctx == " << ourctx << '\n';
      if(ourctx == 1) {
        m.ledbuf[0] = 1;
        m.update_ledbuf = 1;
      };
      m.set_timer(44100, [ourctx](mdt &m) {
        std::cerr << "ourctx was " << ourctx << "; now at " << m.ledctx << '\n';
        if(m.ledctx == ourctx) {
          m.ledbuf[0] = 0;
          m.update_ledbuf = 1;
          m.ledctx = 0;
        };
      });
      snwbuf.write(me);
    }
  };

  void process(jack_nframes_t nf) {
    size_t timebase = frames;

    jack::midi_read_buffer rbuf(kb_in, nf);
    for(auto &ev : rbuf.events()) {
      event_queue.emplace(event(timebase + ev.time, ev));

      // kbwbuf.write(jack::midi_event(ev.time, {159, 36, 54}));
      // kbwbuf.write(jack::midi_event(ev.time + 1, {159, 36, 0}));
    };

    processor prc(*this, timebase, nf);
    while(!event_queue.empty()) {
      auto &ev = event_queue.top();
      assert(ev.abstime >= timebase);
      if(ev.abstime >= (timebase + nf)) break;
      frames = ev.abstime;

      std::visit(overloaded {
        [&](const jack::midi_event &me) { prc.process_midi(ev.abstime, me); },
        [&](const std::function<void(mdt &)> &fun) { fun(*this); }
      }, ev.data);

      event_queue.pop();

      if(update_ledbuf) {
        for (uint8_t i = 0; i < 16; i++) {
          if(ledbuf[i] != ledbuf_old[i]) {
            prc.kbwbuf.write(jack::midi_event(prc.localtime(), {159, (uint8_t)(36 + i), ledbuf[i]}));
            ledbuf_old[i] = ledbuf[i];
          }
        }
        update_ledbuf = 0;
      }
    };

    frames = timebase + nf;
  };

  void blink_at(size_t dl, uint8_t led, uint8_t color) {
    set_timer_repeat(dl, [led, color](mdt &m) {
      m.ledbuf[led] = m.ledbuf[led] ? 0 : color;
      m.update_ledbuf = 1;
    });
  };

  int main(int argc, char const *argv[]) {
    // for (size_t i = 0; i < 100; i++) {
    //   event_queue.emplace(i * 10, jack::midi_event(68, {159, 36, 54}));
    // }

    event_queue.emplace(44100, [](mdt &m) {
      size_t base = 1000;
      uint8_t c = 1;
      for (uint8_t i = 0; i < 16; i++) {
        m.ledbuf[i] = 1;
        //m.blink_at(base * i, i, c);
      }
      m.update_ledbuf = 1;
    });

    cl.set_reader_cb([&]() {
       graph_reorder_cv.notify(0);
       return 0;
    });
    cl.set_process_cb([this](jack_nframes_t nf) {
      process(nf);
      cidx++;
      return 0;
    });
    cl.set_shutdown_cb([]() { exit(0); });
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
