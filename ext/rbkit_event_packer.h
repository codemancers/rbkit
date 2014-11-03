#ifndef RBKIT_MESSAGE_PACKER
#define RBKIT_MESSAGE_PACKER
#include "msgpack.h"
#include "rbkit_event.h"

void pack_event(rbkit_event_header *event_header, msgpack_packer *packer);

#endif
