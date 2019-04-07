#include "mdt.h"

double mdt_time_to_secs(size_t time) {
  return (double)time / jack_get_sample_rate(mdt_jack_client);
};
