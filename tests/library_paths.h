// Keep fixture data and transient navigation state outside the real desktop card.
#include "defines.h"
#undef SDCARD_PATH
#define SDCARD_PATH TEST_LIBRARY_ROOT "/sdcard"
#undef LAST_PATH
#define LAST_PATH TEST_LIBRARY_ROOT "/last.txt"

#undef RESUME_SLOT_PATH
#define RESUME_SLOT_PATH TEST_LIBRARY_ROOT "/resume_slot.txt"
