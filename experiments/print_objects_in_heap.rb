$:<< File.join(File.dirname(__FILE__), "../lib")
$:<< File.join(File.dirname(__FILE__), "../ext")

require 'rbkit'

class Animal;end

ObjectGraph.print_class_using_class2name(Animal.new)

ObjectGraph.print_heap_objects
