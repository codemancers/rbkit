require "bundler/gem_tasks"
require "rspec/core/rake_task"

RSpec::Core::RakeTask.new

task :compile do
  symlink_bundle_path = File.absolute_path "lib/rbkit_tracer.#{RbConfig::MAKEFILE_CONFIG['DLEXT']}"
  original_bundle_path = File.absolute_path "ext/rbkit_tracer.#{RbConfig::MAKEFILE_CONFIG['DLEXT']}"
  Dir.chdir 'ext' do
    system("#{Gem.ruby} extconf.rb")
    system("make")
    File.delete(symlink_bundle_path) if File.symlink? symlink_bundle_path
    File.symlink(original_bundle_path, symlink_bundle_path)
  end
end

desc "Run each spec in isolated process"
task :run_spec => [:compile] do
  command_output = []
  Dir["spec/*_spec.rb"].each do |file|
    puts "Running #{file}.."
    command_output << system("bundle exec rspec #{file}")
  end
  if command_output.all?
    exit(0)
  else
    exit(-1)
  end
end

task :default => [:compile, :spec]
