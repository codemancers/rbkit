#ifndef RBKIT_MESSAGE_AGGREGATOR_H
#define RBKIT_MESSAGE_AGGREGATOR_H

#include "msgpack.h"

// No of events that will be grouped
// into one event_collection message
#define MESSAGE_BATCH_SIZE 400

void message_list_new();
void queue_message(msgpack_sbuffer *);
void get_event_collection_message(msgpack_sbuffer *);
void message_list_destroy();
void message_list_clear();
size_t queued_message_count();

#endif
