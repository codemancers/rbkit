require "bundler/gem_tasks"
require "rspec/core/rake_task"

RSpec::Core::RakeTask.new

task :compile do
  symlink_bundle_path = File.absolute_path "lib/rbkit_tracer.#{RbConfig::MAKEFILE_CONFIG['DLEXT']}"
  next if File.exists? symlink_bundle_path
  Dir.chdir 'ext' do
    system("#{Gem.ruby} extconf.rb")
    system("make")
    original_bundle_path = File.absolute_path "rbkit_tracer.#{RbConfig::MAKEFILE_CONFIG['DLEXT']}"
    File.symlink(original_bundle_path, symlink_bundle_path)
  end
end

task :default => [:compile, :spec]
