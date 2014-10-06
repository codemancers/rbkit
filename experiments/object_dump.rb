$:<< File.join(File.dirname(__FILE__), "../lib")
$:<< File.join(File.dirname(__FILE__), "../ext")

require 'rbkit'

Rbkit.start_profiling

class Foo
  def initialize(name)
    @name = name
  end
end

stuff = {}
100.times do |i|
  sleep 0.01
  foo = Foo.new("hemant-#{i}")
  stuff["name#{i}"] = "hemant-#{i}"
end

Rbkit.dump_objects
