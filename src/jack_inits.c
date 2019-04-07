#include "mdt.h"

void mdt_jack_init_client() {
  mdt_jack_client = jack_client_open("MDT", JackNoStartServer, NULL);
  if (!mdt_jack_client) {
    MDT_LOGF(RL_LOGLEVEL_FATAL, "Could not open Jack client");
    RL_ABORT;
  }
};

static jack_port_t *init_port(const char *name, bool is_input) {
  jack_port_t *port =
      jack_port_register(mdt_jack_client, name, JACK_DEFAULT_MIDI_TYPE,
                         is_input ? JackPortIsInput : JackPortIsOutput, 0);
  if (!port) {
    MDT_LOGF(RL_LOGLEVEL_FATAL, "Could not init Jack port '%s'", name);
    RL_ABORT;
  }

  return port;
};

void mdt_jack_init_ports() {
  mdt_port_kin = init_port("Keyboard input", 1);
  mdt_port_kout = init_port("Keyboard output", 0);
  mdt_port_synth = init_port("Synth output", 0);
};
