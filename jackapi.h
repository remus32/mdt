#include <jack/jack.h>
#include <jack/midiport.h>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <format>
#include <functional>

namespace jack {
  struct client {
    jack_client_t *handle;

    template<class A>
    void set_reader_cb(A fun) {
      auto fptr = new std::function<int()>(fun);
      jack_set_graph_order_callback(handle, [](void *arg) { return (*(std::function<int()>*)arg)(); }, (void*)fptr);
    };
    template<class A>
    void set_process_cb(A fun) {
      auto fptr = new std::function<int(jack_nframes_t)>(fun);
      jack_set_process_callback(handle, [](jack_nframes_t nframes, void *arg) { return (*(std::function<int(jack_nframes_t)>*)arg)(nframes); }, (void*)fptr);
    };


    inline void close() {
      int i = jack_client_close(handle);
      assert(i == 0);
    };
    inline void set_activated(bool i) {
      int ret = i ? jack_activate(handle) : jack_deactivate(handle);
      assert(ret == 0);
    };
    inline void port_connect(const char *a, const char *b) {
      int i = jack_connect(handle, a, b);
      assert(i == 0 || i == EEXIST);
    };

    inline std::vector<std::string> get_ports(const char *name_pattern = NULL, const char *type_pattern = NULL) {
      const char **ret = jack_get_ports(handle, name_pattern, type_pattern, 0);
      if(!ret) return {};
      std::vector<std::string> str;
      for(const char **s = ret;*s;s++) {
        str.emplace_back(*s);
      };
      free(ret);
      return str;
    };

    inline client(const char *name): handle(jack_client_open(name, JackNoStartServer, NULL)) {
      assert(handle);
    };
    client() = delete;
    client(client &cl) = delete;
    client(client &&cl) = delete;

    inline ~client() { close(); }
  };

  struct port {
    client &cl;
    jack_port_t *handle;

    inline const char *name() {
      return jack_port_name(handle);
    };
    inline int num() {
      return jack_port_connected(handle);
    };

    inline port(client &_cl, const char *name, bool inout, const char *type = JACK_DEFAULT_MIDI_TYPE):
      cl(_cl), handle(jack_port_register(_cl.handle, name, type, inout ? JackPortIsInput : JackPortIsOutput, 0)) {
      assert(handle);
    };
  };

  struct midi_event {
    jack_nframes_t time;
    std::vector<uint8_t> data;

    inline midi_event(jack_nframes_t _t, uint8_t *ptr, size_t len): time(_t), data(ptr, ptr + len) {};
    inline midi_event(const jack_midi_event_t &ev): midi_event(ev.time, ev.buffer, ev.size) {};
    inline midi_event(jack_nframes_t _t, std::initializer_list<uint8_t> i): time(_t), data(i) {};
  };

  struct midi_read_buffer {
    port &p;
    void *buffer;

    inline jack_nframes_t evcount() {
      return jack_midi_get_event_count(buffer);
    }
    inline std::vector<midi_event> events() {
 	    jack_midi_event_t evbuf;
      std::vector<midi_event> vec;
      auto evc = evcount();
      for (size_t i = 0; i < evc; i++) {
        int r = jack_midi_event_get(&evbuf, buffer, i);
        assert(r == 0);

        vec.emplace_back(evbuf);
      }
      return vec;
    }

    inline midi_read_buffer(port &_p, jack_nframes_t nf): p(_p), buffer(jack_port_get_buffer(_p.handle, nf)) {
      assert(buffer);
    };
  };
  struct midi_write_buffer {
    port &p;
    void *buffer;

    inline void write(const midi_event &ev) {
      int i = jack_midi_event_write(buffer, ev.time, ev.data.data(), ev.data.size());
      if(i == ENOBUFS) std::cerr << "ERROR: WRITE ENOBUFS" << '\n';
      assert(i == 0);
    };

    inline midi_write_buffer(port &_p, jack_nframes_t nf): p(_p), buffer(jack_port_get_buffer(_p.handle, nf)) {
      assert(buffer);
      jack_midi_clear_buffer(buffer);
    };
  };
};
