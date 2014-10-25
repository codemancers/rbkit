#ifndef RBKIT_MESSAGE_AGGREGATOR_H
#define RBKIT_MESSAGE_AGGREGATOR_H

#include "msgpack.h"

void message_list_new();
void add_message(msgpack_sbuffer *);
void * get_event_collection_buffer();
size_t get_event_collection_buffer_size();
size_t get_event_collection_message_count();
void message_list_destroy();
void message_list_clear();

#endif
