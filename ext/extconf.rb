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

def check_and_install_with_homebrew(package)
  puts green("Looks like you're on OSX. Do you want to use homebrew to install #{package}? (y/Y/n/N) :")
  use_brew = gets.chomp
  if(use_brew.downcase == "y")
    system("brew install #{package}")
  else
    false
  end
end

def download_file(url, target)
  if find_executable('curl')
    system("curl -o #{target} #{url}")
  elsif find_executable('wget')
    system("wget -O #{target} #{url}")
  else
    fail "Cannot download #{url}. You need either curl or wget to continue"
  end
end

def download_and_install_zeromq_from_source
  ['libtool', 'autoconf', 'automake'].each do |executable|
    fail "#{executable} needed by zeromq not found." unless find_executable(executable)
  end
  puts green("Downloading and installing zeromq from source")
  url = "http://download.zeromq.org/zeromq-4.0.4.tar.gz"
  filename = "zeromq-4.0.4.tar.gz"
  basename = File.basename(filename, '.tar.gz')
  download_file(url, filename)
  system("tar zxvf #{filename}")
  dist_path = "#{CWD}/#{basename}/dist"
  Dir.chdir('zeromq-4.0.4') do
    system("./configure --prefix='#{dist_path}'")
    system("make && make install")
  end
  FileUtils.cp "#{dist_path}/lib/libzmq.a", "#{CWD}/libzmq.a"
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
  elsif RUBY_PLATFORM =~ /darwin/
    unless check_and_install_with_homebrew("zeromq")
      download_and_install_zeromq_from_source
    end
  else
    download_and_install_zeromq_from_source
  end

  unless have_library('zmq') and have_header('zmq.h')
    fail 'zeromq build failed.'
  end
end

# Install msgpack if not available
unless(have_library("msgpack") && have_header("msgpack.h"))
  if RUBY_PLATFORM =~ /darwin/
    unless check_and_install_with_homebrew("msgpack")
      # Download and install msgpack
    end
  end
  unless(have_library("msgpack") && have_header("msgpack.h"))
    fail 'msgpack build failed'
  end
end

create_makefile('rbkit_tracer')
