#include "mdt.h"
#include <errno.h>

bool mdt_reordered_synth = 0;
bool mdt_reordered_kb = 0;

static void connect_ports(jack_port_t *base_port, const char *port_mask,
                          int min_ports, size_t idx, bool invert, bool *mark) {
  *mark = 0;
  if (jack_port_connected(base_port) == 0) {
    const char **ports = jack_get_ports(mdt_jack_client, port_mask, NULL, 0);
    if (!ports)
      return;
    size_t ports_n = rl_narray_size((void **)ports);
    if (ports_n >= min_ports) {
      const char *base_port_name = jack_port_name(base_port);
      const char *target_port = ports[ports_n - 1 - idx];

      const char *port_a = invert ? target_port : base_port_name;
      const char *port_b = invert ? base_port_name : target_port;

      int i = jack_connect(mdt_jack_client, port_a, port_b);
      if (i != 0 && i != EEXIST) {
        MDT_LOGF(RL_LOGLEVEL_FATAL, "Could not connect port '%s' to '%s' (%i)",
                 port_a, port_b, i);
        RL_ABORT;
      }
      *mark = 1;
      MDT_LOGF(RL_LOGLEVEL_INFO, "%s connected", base_port_name);
    }
    jack_free(ports);
  }
}

void mdt_cb_reorder() {
  bool r_a, r_b;

  connect_ports(mdt_port_kin, "system:midi_capture_[0-9]+", 3, 1, 1, &r_a);
  connect_ports(mdt_port_kout, "system:midi_playback_[0-9]+", 3, 0, 0, &r_b);
  connect_ports(mdt_port_synth, "qsynth:midi_[0-9]+", 1, 0, 0,
                &mdt_reordered_synth);

  assert(r_a ^ r_b == 0);
  mdt_reordered_kb = r_a;
};
