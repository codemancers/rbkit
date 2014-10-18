class Bar;end
class ShortLivedBar;end
class Foo
  attr_reader :array, :bar
  def initialize
    @bar = Bar.new
    ShortLivedBar.new #not reference outside this scope
    @array = [1, 2, 3, 4, 5, 6, 7]
  end
end

def foo_bar_source_file
  __FILE__
end

def bar_obj_line
  # Warning! Hardcoded line number below ¯\_(ツ)_/¯
  6
end

def array_line
  # Warning! Hardcoded line number below ¯\_(ツ)_/¯
  8
end
