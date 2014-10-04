<img src="./logo.png" height="30px" /> Rbkit
============================================

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
```
require 'rbkit'
Rbkit.start_profiling
```
If using Rails, and you want to measure everything from the boot process,
a good place to put this would be at the end of `config/boot.rb`.

## Development

#### Install zmq and msgpack

On OSX - Using `homebrew` following command should suffice:

```
~> brew install zeromq
~> brew install msgpack
```

On Linux - we recommend to download these libraries
from their respective home pages and manually compiling
and installing.

At some point, we will bundle these two C libraries during gem installations
but for now, this has to suffice.

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

##### Using rake-compiler

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
# Create a symlink at `lib/rbkit_tracer.bundle`
# that points to `ext/rbkit_tracer.bundle`
# (in order to use `rbkit` gem in Gemfiles using `path` option)
```

## TODO

TODOs are tracked as github issues.
