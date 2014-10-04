require "bundler/gem_tasks"
require "rspec/core/rake_task"

RSpec::Core::RakeTask.new

task :compile do
  Dir.chdir 'ext' do
    system("#{Gem.ruby} extconf.rb")
    system("make")
    original_bundle_path = File.absolute_path 'rbkit_tracer.bundle'
    symlink_bundle_path = File.absolute_path '../lib/rbkit_tracer.bundle'
    File.symlink(original_bundle_path, symlink_bundle_path)
  end
end

task :default => [:compile, :spec]
