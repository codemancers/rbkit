require "bundler/gem_tasks"
require "rspec/core/rake_task"
require 'rake/extensiontask'

Rake::ExtensionTask.new('rbkit_tracer') do |ext|
  ext.ext_dir = 'ext'
end

RSpec::Core::RakeTask.new

task :default => [:compile, :spec]
