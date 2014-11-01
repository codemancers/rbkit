#ifndef RBKIT_MESSAGE_PACKER
#define RBKIT_MESSAGE_PACKER
#include "msgpack.h"
#include "rbkit_event.h"

// Object dump will be split into multiple
// messages. This macro defines the number
// of object data that should be packed
// as the payload of one message.
#define MAX_OBJECT_DUMPS_IN_MESSAGE 20

void pack_event(rbkit_event_header *event_header, msgpack_packer *packer);

#endif
