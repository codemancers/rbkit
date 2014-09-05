$:<< File.join(File.dirname(__FILE__), "../lib")
$:<< File.join(File.dirname(__FILE__), "../ext")

require 'rbkit'

Rbkit.start_profiling(5555)

class Foo
  def initialize(name)
    @name = name
  end
end

stuff = {}
100.times do |i|
  foo = Foo.new("hemant-#{i}")
  stuff["name#{i}"] = "hemant-#{i}"
end

gets
