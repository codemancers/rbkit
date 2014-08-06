$:<< File.join(File.dirname(__FILE__), "../lib")
$:<< File.join(File.dirname(__FILE__), "../ext")

require 'benchmark'
require 'rbkit'


def do_stuff
  hash = {}
  10_00_000.times do |i|
    hash[:"key#{i}"] = "value#{i}"
  end
end

Benchmark.bm do |bm|
  bm.report('Running code without Rbkit :') do
    do_stuff
  end

  Rbkit.start_profiling
  bm.report('Running code with Rbkit tracing :') do
    do_stuff
  end

  bm.report('Time taken for objectspace snapshot :') do
    Rbkit.send_objectspace_dump
  end

  bm.report('Running code after objectspace snapshot :') do
    do_stuff
  end

  Rbkit.stop_server
  bm.report('Running code after disabling rbkit :') do
    do_stuff
  end
end
