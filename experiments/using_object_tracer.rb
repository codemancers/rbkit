class Foo
  def initialize
    @names = {}
  end

  def run
    1000.times do |i|
      @names[i] = "allocating #{i}"
    end
  end
end
