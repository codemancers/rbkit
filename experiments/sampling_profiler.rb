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
server.start_sampling_profiler
main
server.stop_sampling_profiler
Rbkit.stop_server
