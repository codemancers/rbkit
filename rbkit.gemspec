# -*- encoding: utf-8 -*-

GEM_NAME = "rbkit"

lib = File.expand_path("../lib", __FILE__)
$: << lib unless $:.include?(lib)

require "rbkit/version"

Gem::Specification.new do |s|
  s.name = GEM_NAME
  s.version = Rbkit::VERSION

  if s.respond_to? :required_rubygems_version=
    s.required_rubygems_version = Gem::Requirement.new(">= 0")
  end
  s.authors = ["Hemant Kumar", "Emil Soman", "Kashyap"]
  s.description = %q{Something small for process management}
  s.email = %q{hemant@codemancers.com emil@codemancers.com kashyap@codemancers.com}

  s.files         = `git ls-files`.split("\n")
  s.test_files    = `git ls-files -- {spec,features}/*`.split("\n")
  s.extensions = 'ext/extconf.rb'
  s.require_paths = ["lib"]

  s.homepage = %q{http://rbkit.codemancers.com}
  s.licenses = ["MIT"]
  s.require_paths = ["lib"]
  s.summary = %q{Ruby profiler for rest of us}
  s.add_development_dependency("rspec")
  s.add_development_dependency("rake")
end
