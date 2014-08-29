#ifndef RBKIT_MESSAGE_AGGREGATOR_H
#define RBKIT_MESSAGE_AGGREGATOR_H

#include "msgpack.h"
#include "zmq.h"

void message_list_new();
void add_message(msgpack_sbuffer *);
msgpack_sbuffer * get_event_collection_message();
void message_list_destroy();
void message_list_clear();

#endif
