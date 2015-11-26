# Rbkit Message Protocol v2.1

This is the documentation for the protocol that's used when Rbkit server
and client communicate with each other. A client can work with a server
as long as they both speak the same protocol version. Find out the protocol
version used by finding the value of the constant `Rbkit::PROTOCOL_VERSION`.

Refer [issue #11](https://github.com/code-mancers/rbkit/issues/11) for some history.

## Event types

EventType is an integer value indicating the type of event
contained in the message. It's basically the following enum : 

```c
enum EventType {
  allocation_snapshot,
  gc_start,
  gc_end_m,
  gc_end_s,
  object_space_dump,
  gc_stats,
  event_collection,
  handshake,
  cpu_sample,
}
```

ie, 

```ruby
# Rbkit::EVENT_TYPES
{
  "allocation_snapshot" => 0
  "gc_start"            => 1,
  "gc_end_m"            => 2,
  "gc_end_s"            => 3,
  "object_space_dump"   => 4,
  "gc_stats"            => 5,
  "event_collection"    => 6,
  "handshake"           => 7,
  "cpu_sample"          => 8
}
```

The keys of all event message hashes are integer values whose enum names
are used below. `Rbkit::MESSAGE_FIELDS` gives you the exhaustive list of
enums used. 

```ruby
# Rbkit::MESSAGE_FIELDS
{
  'event_type'             =>  0,
  'timestamp'              =>  1,
  'payload'                =>  2,
  'object_id'              =>  3,
  'class_name'             =>  4,
  'references'             =>  5,
  'file'                   =>  6,
  'line'                   =>  7,
  'size'                   =>  8,
  'message_counter'        =>  9,
  'correlation_id'         => 10,
  'complete_message_count' => 11,
  'method_name'            => 12,
  'label'                  => 13,
  'singleton_method'       => 14,
  'thread_id'              => 15,
  'stacktrace'             => 16,
  'count'                  => 17
}
```

`correlation_id` can be used to split a large event across several batches.
All split events will have same `correlation_id`.

## Message frames

Generic message frame is of the format :

```yaml
{
  event_type: <EventType>,
  timestamp: <timestamp in milliseconds>,
  payload: <PAYLOAD> # Optional
}
```

### Message frame for event_collection :

All events are aggregated in the server and sent as the payload of a special
type of event named `event_collection`. The payload will be an array containing
other event messages.

```yaml
{
  event_type: "event_collection",
  message_counter: <incrementing counter>,
  timestamp: <timestamp in milliseconds>,
  payload: [
    {event_type: <EVENT_TYPE>, timestamp: <TIMESTAMP>, payload: <PAYLOAD>},
    {event_type: <EVENT_TYPE>, timestamp: <TIMESTAMP>, payload: <PAYLOAD>}
  ]
}
```

### Message frame for HANDSHAKE :

Handshake is a special message which is sent synchronously over the REQ-REP
socket pair and not on the PUB socket. When a "handshake" command is send by
the client, the server responses with the Rbkit status. The handshake reply
is of the following format :

```yaml
{
  event_type: "handshake",
  timestamp: <timestamp in milliseconds>,
  payload: {
    "rbkit_server_version" => <Version of Rbkit server>,
    "rbkit_protocol_version" => <Version of message protocol used in Rbkit server>,
    "process_name" => <Name of the process>,
    "pwd" => <working directory of the app>,
    "pid" => <PID of the ruby process>,
    "object_trace_enabled" => <0 or 1>,
    "cpu_profiling_enabled" => <0 or 1>,
    "clock_type" => <:wall or :cpu>,
    "cpu_profiling_mode" => <:sampling>
  }
}
```

### Message frame for allocation_snapshot :

allocation_snapshot event is not part of event collections. These events
are sent to the client in a constant interval of 6 seconds (TODO: Make this configurable).

```yaml
{
  event_type: allocation_snapshot,
  timestamp: <timestamp in milliseconds>,
  payload: {
    "stacktrace" => {
      <STACK_TRACE_ID> => [ <STACKTRACE1>, <STACKTRACE2>, ..],
      ...
    },
    "allocations" => {
      <FILE_PATH> => {
        <"#{LINE} #{TYPE} #{SIZE}"> => {
          "count" => <COUNT>,
          "stacktrace" => <STACK_TRACE_ID>
        }
      }
    }
  }
}
```

### Message frame for FREE_OBJECT :

```yaml
{
  event_type: obj_destroyed,
  timestamp: <timestamp in milliseconds>,
  payload: {
    object_id: <OBJECT_ID>
  }
}
```

### Message frame for GC_START :

When the GC_START event is triggered, no payload is sent.

```yaml
{
  event_type: gc_start,
  timestamp: <timestamp in milliseconds>,
}
```

### Message frame for GC_END_SWEEP :

When the GC_END_SWEEP event is triggered, no payload is sent.

```yaml
{
  event_type: gc_end_s,
  timestamp: <timestamp in milliseconds>,
}
```

### Message frame for OBJECT_SPACE_DUMP :

Object space dump is split into multiple messages. Each message is of
the following format :

```yaml
{
  event_type: object_space_dump
  timestamp: <timestamp in milliseconds>,
  correlation_id: <ID_INDICATING_EVENT_THIS_MESSAGE_IS_PART_OF>,
  payload: [
    {
      object_id: <OBJECT_ID>,
      class_name: <CLASS_NAME>,
      references: [<OBJECT_ID>, <OBJECT_ID>, ... ],
      file: <FILE_PATH>,
      line: <LINE_NUMBER>,
      size: <SIZE>
    },
    ...
  ]
}
```

### Message frame for CPU_SAMPLE :

CPU Samples are split into multiple messages. Each message is of
the following format :

```yaml
{
  event_type: cpu_sample
  timestamp: <timestamp in milliseconds>,
  payload: [
    {
      method_name: <name of method >,
      label: <method label (See below for details)>,
      file: <filename>,
      line: <line no>,
      singleton_method: <1/0>,
      thread_id: <thread id>
    }, #Frame 1
    ...
  ] # Array of frames in the sample
}
```

Ruby VM gives us something called a "full label" which will give us many details
about each frame in the call stack. Some examples:

```ruby
"block (2 levels) in SampleClass#foobar" # foobar is an instance method of SampleClass. The frame is 2 blocks deep inside foobar.
"#{obj.inspect}.zab" # Frame is in zab, a singleton method defined on an object obj
"SampleClass#baz" # Frame is in baz, an instance method defined in SampleClass
"SampleClass.bar" # Frame is in bar, a class method defined in SampleClass
```

### Message from GC stats:
**Note: GC stat payload uses strings for keys**

```yaml
{
  event_type: "gc_stats",
  timestamp: <timestamp in milliseconds>,
  payload: {
    count: # Count of major and minor GCs so far
    minor_gc_count: # Count of minor GCs
    major_gc_count: # Count of major GCs
    heap_allocated_pages: # Count of allocated pages (heap_eden_pages + heap_tomb_pages)
    heap_eden_pages: # Count of pages which has atleast one live object
    heap_tomb_pages: # Count of pages which don't have any object yet
    heap_allocatable_pages: # Count of pages that will be allocated if Ruby runs out of heap
    heap_sorted_length: # Count of total number of sorted pages (>= heap_allocated_pages + heap_allocatable_pages)
    heap_live_slots: # Count of slots in all pages having live objects
    heap_free_slots: # Count of free slots in all pages
    heap_final_slots: # Count of zombie objects
    heap_swept_slots: # Count of slots swept after last GC
    old_objects: # Count of old generation objects
    old_objects_limit: # Old generation object count after which GC is triggered
    total_allocated_objects: # Number of created objects in the lifetime of the process
    total_freed_objects: # Number of freed objects in the lifetime of the process
    malloc_increase_bytes: # Malloc'ed bytes since last GC
    malloc_increase_bytes_limit: # Minor GC is triggered when malloc_increase_bytes exceeds this value
    oldmalloc_increase_bytes: # Malloc'ed bytes for old objects since last major GC
    oldmalloc_increase_bytes_limit: # Major GC is triggered with oldmalloc_increase_bytes exceeds this value
    total_heap_size: # heap_allocated_pages * max slots per page * size of one slot
    total_memsize: # ObjectSpace.memsize_of_all
  }
}
```
