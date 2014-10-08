#include <assert.h>
#include "zmq.h"
#include "rbkit_message_aggregator.h"

static msgpack_sbuffer * sbuf;
static void* message_array;
static size_t used_memsize;
static size_t total_capacity;
static size_t no_of_messages;
static unsigned long message_counter = 0;
static unsigned long max_unsigned_long = -1;

static unsigned long get_message_counter() {
  if(message_counter == max_unsigned_long)
    message_counter = 0;
  return message_counter++;
}

static int has_enough_space_for(size_t size) {
  return ((total_capacity - used_memsize) >= size);
}

static void double_the_capacity() {
  total_capacity *= 2;
  message_array = realloc(message_array, total_capacity);
  assert(message_array);
}

void message_list_new() {
  size_t initial_size = 1024; // Reserve 1 KB of memory
  message_array = malloc(initial_size);
  total_capacity = initial_size;
  used_memsize = 0;
  no_of_messages = 0;
}

void message_list_destroy() {
  free(message_array);
  used_memsize = 0;
  total_capacity = 0;
  no_of_messages = 0;
}

void message_list_clear() {
  used_memsize = 0;
  no_of_messages = 0;
}

// Copies the msgpack sbuffer to the end of
// a dynamically growing array
void add_message(msgpack_sbuffer *buffer) {
  while(!has_enough_space_for(buffer->size))
    double_the_capacity();
  memcpy(message_array + used_memsize, buffer->data, buffer->size);
  used_memsize += buffer->size;
  no_of_messages += 1;
}

// Creates a message containing all the available
// msgpack sbuffers in the array
void get_event_collection_message(msgpack_sbuffer *sbuf) {
  if(no_of_messages > 0) {
    msgpack_packer *pk = msgpack_packer_new(sbuf, msgpack_sbuffer_write);
    pack_event_header(pk, "event_collection", 4);
    pack_string(pk, "message_counter");
    msgpack_pack_unsigned_long(pk, get_message_counter());
    pack_string(pk, "payload");
    msgpack_pack_array(pk, no_of_messages);
    sbuf->data = realloc(sbuf->data, used_memsize + sbuf->size);
    assert(sbuf->data);
    memcpy(sbuf->data + sbuf->size, message_array, used_memsize);
    sbuf->size += used_memsize;

    msgpack_packer_free(pk);
  }
}
