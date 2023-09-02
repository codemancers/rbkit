def io_intensive_operation
  r, w = IO.pipe
  IO.select([r], nil, nil, 1)
ensure
  r.close
  w.close
end

def cpu_intensive_operation
  100000.times{|x| Math.sqrt(x)}
end

class SampleClassForTest
  class Sample2
    def baz(block)
      instance_eval 'def zab(block) block.call end'
      [self, zab(block)]
    end
  end

  def self.bar(block)
    Sample2.new.baz(block)
  end

  def foo(block)
    self.class.bar(block)
  end
end
