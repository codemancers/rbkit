Refer [issue #11](https://github.com/code-mancers/rbkit/issues/11) for some history.

## Event types

EventType is an integer value indicating the type of event
contained in the message. It's basically the following enum : 

```c
enum EventType {
  obj_created,
  obj_destroyed,
  gc_start,
  gc_end_m,
  gc_end_s,
  object_space_dump,
  gc_stats,
  event_collection,
  handshake
}
```

ie, 

```ruby
# Rbkit::EVENT_TYPES
{
  "obj_created"       => 0,
  "obj_destroyed"     => 1,
  "gc_start"          => 2,
  "gc_end_m"          => 3,
  "gc_end_s"          => 4,
  "object_space_dump" => 5,
  "gc_stats"          => 6,
  "event_collection"  => 7,
  "handshake"         => 8
}
```

The keys of all event message hashes are integer values whose enum names
are used below. `Rbkit::MESSAGE_FIELDS` gives you the exhaustive list of
enums used. 

```ruby
# Rbkit::MESSAGE_FIELDS
{
  'event_type'      =>  0,
  'timestamp'       =>  1,
  'payload'         =>  2,
  'object_id'       =>  3,
  'class_name'      =>  4,
  'references'      =>  5,
  'file'            =>  6,
  'line'            =>  7,
  'size'            =>  8,
  'message_counter' =>  9
}
```

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
    "process_name" => <Name of the process>,
    "pwd" => <working directory of the app>,
    "pid" => <PID of the ruby process>,
    "object_trace_enabled" => <0 or 1>
  }
}
```

### Message frame for NEW_OBJECT :

When the NEW_OBJECT event is triggered, the object id and the class name
are sent as the payload.

```yaml
{
  event_type: obj_created,
  timestamp: <timestamp in milliseconds>,
  payload: {
    object_id: <OBJECT_ID>,
    class_name: <CLASS_NAME>
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

```yaml
{
  event_type: object_space_dump
  timestamp: <timestamp in milliseconds>,
  payload: [
    {
      object_id: <OBJECT_ID>,
      snapshot_no: <SNAPSHOT_COUNT>,
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

### Message from GC stats:
**Note: GC stat payload uses strings for keys**

```yaml
{
  event_type: "gc_stats",
  timestamp: <timestamp in milliseconds>,
  payload: {
     count:
     heap_user:
     heap_length:
     heap_increment:
     ...
  }
}
```
