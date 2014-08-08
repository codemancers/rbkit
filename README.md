1. This is a Ruby C extension that expose object allocation stats

For compiling it, install:

```
~> brew install zeromq
~> brew install msgpack
```

At some point, we will bundle these two C libraries during gem installations
but for now, this has to suffice.

2. After that, run

```
bundle install
bundle exec rake compile
```

3. Tasks to do:

* [X] implement support for disabling trackpoints
* [X] implement support for cleaning zmq context on shutdown.
* [X] Write some ruby code that installs at_exit block for socket cleanup.
* [X] Fix memory leak around event names.
* [X] Allow user to specify port where profiler should start.
* [X] Implement support for returning object classes along with event names.
* [X] Send objectspace dump over zmq
* [X] Client should be able to trigger GC
* [X] Use PUB-SUB socket pair for sending events from server
* [X] Use REQ-REP socket pair to accept commands from client
* [X] Collect file and line no of object allocation


4. Dumping events to a file

Sometimes its useful to dump events to file so that we can write tests on
them. Here is a way to do it.

```sh
  # enable macro DUMP_MSBPACK_OBJECTS in rbkit_tracer.c file
  # and recompile.

  # make a directory /tmp/rbkitevents
  $ mkdir -p /tmp/rbkitevents

  # simply run ruby with rbkit, and you can see dumps
  $ ruby experiments/using_rbkit.rb
```
