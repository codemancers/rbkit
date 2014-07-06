$:<< File.join(File.dirname(__FILE__), "../lib")
$:<< File.join(File.dirname(__FILE__), "../ext")

require 'rbkit'

class Animal;end


puts "Using .class : #{Animal.new.class}"
puts "Using rb_obj_class : #{ObjectGraph.print_class(Animal.new)}"
ObjectGraph.print_class_using_class2name(Animal.new)

#ObjectGraph.print_heap_objects
