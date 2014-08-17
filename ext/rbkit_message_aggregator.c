#include "zmq.h"
#include "rbkit_message_aggregator.h"

// Struct that holds each message
// to form a linked list of accumulated messages
typedef struct _message {
  void *data;
  size_t size;
} message;

static list_t *message_list;
static size_t total_memsize;
static msgpack_sbuffer * sbuf;

void message_list_new() {
  message_list = list_new();
  total_memsize = 0;
}

void message_list_destroy() {
  list_destroy(message_list);
  total_memsize = 0;
}

void message_list_clear() {
  list_clear(message_list);
  total_memsize = 0;
}

// Makes a copy of the msgpack sbuffer, and pushes
// the pointer to sbuffer to the list
void add_message(msgpack_sbuffer *buffer) {
  total_memsize += buffer->size;
  void *data = malloc(buffer->size);
  memcpy(data, buffer->data, buffer->size);
  message *msg = malloc(sizeof(message));
  msg->size = buffer->size;
  msg->data = data;
  list_append(message_list, msg);
}

// Creates a msgpack array containing all the available
// msgpack sbuffers in the list and frees them.
msgpack_sbuffer * get_messages_as_msgpack_array() {
  sbuf = msgpack_sbuffer_new();
  if(rbkit_list_size(message_list) > 0) {
    msgpack_packer *pk = msgpack_packer_new(sbuf, msgpack_sbuffer_write);
    pack_event_header(pk, "event_collection", 3);
    pack_string(pk, "payload");
    msgpack_pack_array(pk, list_size(message_list));
    sbuf->data = realloc(sbuf->data, total_memsize + sbuf->size);

    message *msg = list_first(message_list);
    while(msg) {
      memcpy(sbuf->data + sbuf->size, msg->data, msg->size);
      sbuf->size += msg->size;
      free(msg->data);
      free(msg);
      msg = list_next(message_list);
    }

    msgpack_packer_free(pk);
  }
  return sbuf;
}
