module Rbkit
  class Timer
    attr_accessor :last_fired_at, :interval

    def initialize(interval, &timer_block)
      @interval = interval
      @timer_block = timer_block
      @last_fired_at = Time.now
    end

    def run
      if Time.now - last_fired_at > interval
        @timer_block.call
        @last_fired_at = Time.now
      end
    end
  end
end
