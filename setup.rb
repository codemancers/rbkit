require 'rbconfig'
require 'fileutils'

class Setup
  attr_accessor :site_dir, :site_lib_dir, :site_arch_dir

  def initialize
    @site_dir = RbConfig::CONFIG["sitedir"]
    @site_lib_dir = RbConfig::CONFIG["sitelibdir"]
    @site_arch_dir = RbConfig::CONFIG["sitearchdir"]
  end

  def compile
    Dir.chdir 'ext' do
      if File.exist?("Makefile")
        system("make clean")
      end

      system("#{Gem.ruby} extconf.rb")
      system("make")
    end
  end

  def copy_files
    ext_path =
      File.absolute_path "ext/rbkit_tracer.#{RbConfig::MAKEFILE_CONFIG['DLEXT']}"

    cleanup
    FileUtils.cp_r "lib/.", site_lib_dir, verbose: true
    FileUtils.cp_r ext_path, site_arch_dir, verbose: true
  end

  def cleanup
    puts "Removing existing installation"
    FileUtils.rm_r site_lib_dir + "/rbkit", force: true
    FileUtils.rm_r site_lib_dir + "/rbkit.rb", force: true
    FileUtils.rm_r site_arch_dir + "/rbkit_tracer.#{RbConfig::MAKEFILE_CONFIG['DLEXT']}", force: true
  end
end

if __FILE__ == $0
  cmd_arg = ARGV[0] ? ARGV[0].strip : 'setup'
  setup_script = Setup.new
  case cmd_arg
  when 'remove'
    setup_script.cleanup
  else
    setup_script.compile
    setup_script.copy_files
  end
end
