$:<< File.join(File.dirname(__FILE__), "../lib")
$:<< File.join(File.dirname(__FILE__), "../ext")

require 'rbkit'

def main
  i = 0
  threads = []
  threads << Thread.new {
    while(i < 300)
      sleep(0.01)
      i += 1
    end
  }
  threads << Thread.new {
    while(i < 300)
      sleep(0.01)
      i += 1
    end
  }
  threads.each(&:join)
end

server = Rbkit.start_server
server.start_cpu_profiling(mode: :sampling, clock_type: :wall)
main
server.stop_cpu_profiling
Rbkit.stop_server
