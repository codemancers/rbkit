require "bundler/gem_tasks"
require "rspec/core/rake_task"

RSpec::Core::RakeTask.new

task :compile do
  symlink_bundle_path = File.absolute_path "lib/rbkit_server.#{RbConfig::MAKEFILE_CONFIG['DLEXT']}"
  original_bundle_path = File.absolute_path "ext/rbkit_server.#{RbConfig::MAKEFILE_CONFIG['DLEXT']}"
  Dir.chdir 'ext' do
    if !system("#{Gem.ruby} extconf.rb") || !system("make")
      fail "Cannot compile rbkit"
    end
    File.delete(symlink_bundle_path) if File.symlink? symlink_bundle_path
    File.symlink(original_bundle_path, symlink_bundle_path)
  end
end

desc "Run each spec in isolated process"
task :run_specs_in_separate_processes => [:compile] do
  command_output = []
  threads = []
  pub_port = 6666
  req_port = 8888
  Dir["spec/*_spec.rb"].each do |file|
    threads << Thread.new(pub_port+=1, req_port+=1) do |pub_port, req_port|
      puts "Running #{file}.."
      command_output << system("RBKIT_PUB_PORT=#{pub_port} RBKIT_REQ_PORT=#{req_port} bundle exec rspec #{file}")
    end
  end
  threads.each(&:join)
  if command_output.all?
    exit(0)
  else
    exit(-1)
  end
end

task :default => [:run_specs_in_separate_processes]
