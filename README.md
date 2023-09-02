<img src="./logo.png" height="30px" /> Rbkit
============================================

## [WARNING]
This project is not maintained at the moment. We have plans to work on a universal profiling protocol after which we intend to rewrite major parts of Rbkit.

[![Join the chat at https://gitter.im/code-mancers/rbkit](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/code-mancers/rbkit?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

[![Gem Version](https://badge.fury.io/rb/rbkit.svg)](http://badge.fury.io/rb/rbkit)
[![Build Status](https://travis-ci.org/code-mancers/rbkit.svg?branch=tests)](https://travis-ci.org/code-mancers/rbkit)

`rbkit` is a Ruby gem that plugs into your ruby process, taps profiling data
in realtime and sends it across the wire to the [rbkit-client](https://github.com/code-mancers/rbkit-client)
as packed messages.

## Usage

#### Install prerequisites

Rbkit requires the following executables to be present :

* libtool
* autoconf
* automake

Install these with your operating system's package manager before proceeding
to install Rbkit gem.

#### Add `rbkit` to Gemfile

Add the following to the project's Gemfiles

```
gem 'rbkit', path: <RBKIT_PATH>
```


and run `bundle install`

#### Installation without Rubygems

You can also install `rbkit` without bundler/rubygems. This can
be useful, if you are profiling a Ruby app where you want to measure
overhead of Rubygems/Bundler.

Just clone the repository or download from a release tag and run:

```
# Run from root of rbkit directory.
~> ruby setup.rb
```

This should install `rbkit`
in ruby's `site_dir` and then you don't need `rbkit` added to your
`Gemfile` for requiring `rbkit`.

#### Inject `rbkit` into code

Wherever you want to start profiling, add the following :

```ruby
require 'rbkit' # Not needed in Rails
Rbkit.start_server
```

If using Rails, and you want to measure everything from the boot process,
a good place to put this would be at the end of `config/boot.rb`.

## Rbkit API

### Rbkit.start_server

Without any arguments, it's same as :
```ruby
Rbkit.start_server(pub_port: nil, request_port: nil)
```

Starts the Rbkit server and waits for a client to connect and issue
commands to the request_port, until then there's zero performance overhead.
Profiling data is sent asynchronously over pub_port.
This method can be called early in a ruby application so that
whenever profiling needs to be done, the client can attach itself to the
inactive server, do the profiling and leave. Returns an instance of
`Rbkit::Server` if server was started successfully, else returns false.


|argument      | valid values | default value | description                                       |
|--------------|--------------|---------------|---------------------------------------------------|
|pub_port      | nil, fixnum  | nil           | Override default message publishing port of 5555  |
|request_port  | nil, fixnum  | nil           | Override default command listener port of 5556    |


### Rbkit.start_profiling

Without any arguments, it's same as :
```ruby
Rbkit.start_profiling(
  pub_port: nil,
  request_port: nil,
  enable_object_trace: true,
  enable_gc_stats: true,
  enable_cpu_profiling: true,
  clock_type: :wall,
  cpu_profiling_mode: :sampling,
  cpu_sampling_interval_usec: 1000,
  cpu_sampling_depth: 10
)
```

Starts the server with all tracepoints enabled by default. User can
optionally disable tracepoints using the optional arguments.
This method can be used to profile the startup process of a ruby
application where sending commands from the client to enable
profiling is not feasible. Returns an instance of `Rbkit::Server`
if server was started successfully, else returns false.

Arguments:

|argument                   | valid values | default value | description                                      |
|---------------------------|--------------|---------------|--------------------------------------------------|
|pub_port                   | nil, fixnum  | nil           | Override default message publishing port of 5555 |
|request_port               | nil, fixnum  | nil           | Override default command listener port of 5556   |
|enable_object_trace        | true/false   | true          | Enables object creation/deletion events          |
|enable_gc_stats            | true/false   | true          | Enables GC stats which is sent every 5 seconds   |
|enable_cpu_profiling       | true/false   | true          | Enables CPU profiling                            |
|clock_type                 | :wall/:cpu   | :wall         | Specifies clock type to use in CPU profiling     |
|cpu_profiling_mode         | :sampling    | :sampling     | CPU profiling mode - currently only sampling     |
|cpu_sampling_interval_usec | Fixnums      | 1000          | CPU Sampling interval un usec, if sampling mode  |
|cpu_sampling_depth         | Fixnums      | 10            | Depth of stack frame collected by CPU sampler    |


## Development

#### Install zmq and msgpack

If zmq and msgpack are not installed, Rbkit automatically downloads
and installs the two libraries from source during gem installation.
But if you are developing Rbkit, it makes sense to have these
preinstalled:

On OSX - Using `homebrew` following command should suffice:

```
~> brew install zeromq
~> brew install msgpack
```

On Linux - we recommend to download these libraries
from their respective home pages and manually compiling
and installing.

Recommended versions are:

Zeromq: 4.0.5_2
Msgpack: 0.5.9

#### Clone the repo

`git clone git@github.com:code-mancers/rbkit.git`

We'll call this `<RBKIT_PATH>`.

#### Set RBKIT_DEV environment flag

Set the environment variable `RBKIT_DEV` to true.
If using bash, put `export RBKIT_DEV=true` in your `~/.bashrc`.

This compiles the C extension with debug flag and also sets a macro named
`RBKIT_DEV` inside the C extension.

#### Compile the C extension

Two ways to do this :

##### Using rake

```
cd <RBKIT_PATH>
bundle install
bundle exec rake compile

```

##### Or do it manually

```
cd <RBKIT_PATH/ext>
ruby extconf.rb
make
# Create a symlink at `lib/rbkit_server.bundle` (or .so if on linux)
# that points to `ext/rbkit_server.bundle`
# (in order to use `rbkit` gem in Gemfiles using `path` option)
```

#### Run the tests

```
bundle exec rake
```

## TODO

TODOs are tracked as github issues.
