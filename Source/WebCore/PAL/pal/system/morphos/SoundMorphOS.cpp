#include "config.h"
#include "Sound.h"

#include <proto/intuition.h>

namespace PAL {

void systemBeep() { DisplayBeep(NULL); }

} // namespace PAL
