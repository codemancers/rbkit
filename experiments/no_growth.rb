$:<< File.join(File.dirname(__FILE__), "../lib")
$:<< File.join(File.dirname(__FILE__), "../ext")

require 'rbkit'

Rbkit.start_profiling

stuff = {}
stuff["hello"] = "world"
gets
