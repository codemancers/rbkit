require 'mkmf'
require 'fileutils'

CWD = File.expand_path('../', __FILE__)

def red(str);   "\033[31m#{str}\033[0m" end
def green(str); "\033[32m#{str}\033[0m" end

if ENV['RBKIT_DEV']
  puts green("Dev environment enabled.")
  $CFLAGS << ' -g' # Enable debug flag
  $defs << '-DRBKIT_DEV' # Set macro named RBKIT_DEV
end

def download_file(url)
  if find_executable('curl')
    system("curl -L -O #{url}")
  elsif find_executable('wget')
    system("wget #{url}")
  else
    fail "Cannot download #{url}. You need either curl or wget to continue"
  end
end

def download_and_install_zeromq_from_source
  url = "http://download.zeromq.org/zeromq-4.0.4.tar.gz"
  filename = "zeromq-4.0.4.tar.gz"
  basename = File.basename(filename, '.tar.gz')
  dist_path = "#{CWD}/#{basename}/dist"

  unless File.exists? "#{dist_path}/lib/libzmq.a"
    ['libtool', 'autoconf', 'automake'].each do |executable|
      fail "#{executable} needed by zeromq not found." unless find_executable(executable)
    end
    puts green("Downloading and installing zeromq from source")
    download_file(url)
    system("tar zxvf #{filename}")
    Dir.chdir(basename) do
      system("./configure CPPFLAGS='-fPIC' --prefix='#{dist_path}'")
      system("cd src && make && make install")
    end
  end
  FileUtils.cp "#{dist_path}/lib/libzmq.a", "#{CWD}/libzmq.a"
  $INCFLAGS[0,0] = "-I#{dist_path}/include "
end

def download_and_install_msgpack_from_source
  url = "https://github.com/msgpack/msgpack-c/releases/download/cpp-0.5.9/msgpack-0.5.9.tar.gz"
  filename = "msgpack-0.5.9.tar.gz"
  basename = File.basename(filename, '.tar.gz')
  dist_path = "#{CWD}/#{basename}/dist"

  unless File.exists? "#{dist_path}/lib/libmsgpack.a"
    puts green("Downloading and installing msgpack from source")
    download_file(url)
    system("tar zxvf #{filename}")
    Dir.chdir(basename) do
      system("./configure CFLAGS='-fPIC' --prefix='#{dist_path}'")
      system("cd src && make && make install")
    end
  end

  FileUtils.cp "#{dist_path}/lib/libmsgpack.a", "#{CWD}/libmsgpack.a"
  $INCFLAGS[0,0] = "-I#{dist_path}/include "
end

# Ensure Ruby 2.1.x API exists before proceeding
unless(have_func('rb_postponed_job_register_one') &&
  have_func('rb_profile_frames') &&
  have_func('rb_tracepoint_new') &&
  have_const('RUBY_INTERNAL_EVENT_NEWOBJ'))
  fail 'Missing Ruby 2.1.x API. Rbkit only works with Ruby 2.1+'
end

# Install zmq if not available
unless(have_library("zmq") && have_header("zmq.h"))
  if Gem.win_platform?
    puts red("On Windows? You'll have to install zeromq by yourself.")
  else
    download_and_install_zeromq_from_source
  end

  unless have_library('stdc++') and have_library('zmq') and have_header('zmq.h')
    fail 'zeromq build failed.'
  end
end

# Install msgpack if not available
unless(have_library("msgpack") && have_header("msgpack.h"))
  if Gem.win_platform?
    puts red("On Windows? You'll have to install msgpack by yourself.")
  else
    download_and_install_msgpack_from_source
  end

  unless have_library('msgpack') and have_header('msgpack.h')
    fail 'msgpack build failed.'
  end
end

create_makefile('rbkit_tracer')
