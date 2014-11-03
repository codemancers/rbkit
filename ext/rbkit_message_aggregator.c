#include "rbkit_message_aggregator.h"
#include "rbkit_event_packer.h"

static void* message_array;
static size_t used_memsize;
static size_t total_capacity;
static size_t no_of_messages;

static int has_enough_space_for(size_t size) {
  return ((total_capacity - used_memsize) >= size);
}

static void double_the_capacity() {
  total_capacity *= 2;
  message_array = realloc(message_array, total_capacity);
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
  if(no_of_messages == 0)
    return;

  rbkit_event_collection_event *event = new_rbkit_event_collection_event(message_array, used_memsize, no_of_messages);
  msgpack_packer* pk = msgpack_packer_new(sbuf, msgpack_sbuffer_write);
  pack_event((rbkit_event_header *)event, pk);
  free(event);
  msgpack_packer_free(pk);
}
