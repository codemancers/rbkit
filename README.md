<img src="./logo.png" height="30px" /> Rbkit
============================================

[![Gem Version](https://badge.fury.io/rb/rbkit.svg)](http://badge.fury.io/rb/rbkit)
[![Build Status](https://travis-ci.org/code-mancers/rbkit.svg?branch=tests)](https://travis-ci.org/code-mancers/rbkit)

`rbkit` is a Ruby gem that plugs into your ruby process, taps profiling data
in realtime and sends it across the wire to the [rbkit-client](https://github.com/code-mancers/rbkit-client)
as packed messages.

## Usage

Rbkit is not ready for production yet. To use the development version of Rbkit
in your Ruby project, follow the instructions under `Development` section and
then follow these steps :

#### Add `rbkit` to Gemfile

Add the following to the project's Gemfiles

```
gem 'rbkit', path: <RBKIT_PATH>`
```


and run `bundle install`

#### Inject `rbkit` into code

Wherever you want to start profiling, add the following :

```ruby
require 'rbkit'
Rbkit.start_profiling
```

If using Rails, and you want to measure everything from the boot process,
a good place to put this would be at the end of `config/boot.rb`.

You can pass the following keyword arguments to `Rbkit.start_profiling` :

|argument             | valid values | default value | description                                    |
|---------------------|--------------|---------------|------------------------------------------------|
|enable_object_trace  | true/false   | true          | Enables object creation/deletion events        |
|enable_gc_stats      | true/false   | true          | Enables GC stats which is sent every 5 seconds |


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
# Create a symlink at `lib/rbkit_tracer.bundle` (or .so if on linux)
# that points to `ext/rbkit_tracer.bundle`
# (in order to use `rbkit` gem in Gemfiles using `path` option)
```

## TODO

TODOs are tracked as github issues.
