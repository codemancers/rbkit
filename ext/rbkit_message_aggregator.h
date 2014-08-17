#ifndef RBKIT_MESSAGE_AGGREGATOR_H
#define RBKIT_MESSAGE_AGGREGATOR_H

#include "msgpack.h"
#include "zmq.h"
#include "rbkit_list.h"

void message_list_new();
void add_message(msgpack_sbuffer *);
msgpack_sbuffer * get_messages_as_msgpack_array();
void message_list_destroy();
void message_list_clear();

#endif
