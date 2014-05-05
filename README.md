1. This is a Ruby C extension that expose object allocation stats

For compiling it, install:

*. brew install zeromq

*. brew install msgpack

At some point, we will bundle these two C libraries during gem installations
but for now, this has to suffice.

2. After that, run

ruby extconf.rb

3. and then

make

4. Tasks to do:

* [X] implement support for disabling trackpoints
* [X] implement support for cleaning zmq context on shutdown.
* [X] Write some ruby code that installs at_exit block for socket cleanup.
* [X] Fix memory leak around event names.
* [X] Allow user to specify port where profiler should start.
* [ ] Implement support for returning object classes along with event names.
