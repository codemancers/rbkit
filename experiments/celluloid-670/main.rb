require 'pathname'
ENV['BUNDLE_GEMFILE'] ||= File.expand_path("../Gemfile",
  Pathname.new(__FILE__).realpath)

require 'rubygems'
require 'bundler/setup'
require 'celluloid'
require 'rbkit'

Thread.new do
  loop do
    puts `ps -o rss -p #{$$}`.strip.split.last.to_i / 1024
    sleep 2
  end
end

Rbkit.profile(memory: true) do

loop do
  f = Celluloid::Future.new { 1 }
  f.value
  sleep 0.005
end

end
