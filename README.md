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

If you don't want to use `rake-compiler` to compile the C extension, follow
these instructions :
```
cd ext
ruby extconf.rb
make
# Create a symlink at `lib/rbkit_tracer.bundle`
# that points to `ext/rbkit_tracer.bundle`
# (in order to use `rbkit` gem in Gemfiles using `path` option)
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
