$:<< File.join(File.dirname(__FILE__), "../lib")

require 'rbkit'

Rbkit.start_profiling(5555)

class Foo
  def initialize(name)
    @name = name
  end
end

stuff = {}
10_00_0000.times do |i|
  sleep 0.01
  foo = Foo.new("hemant-#{i}")
  stuff["name#{i}"] = "hemant-#{i}"
end
