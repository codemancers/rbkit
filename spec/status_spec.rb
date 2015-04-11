require 'spec_helper'
require 'msgpack'

describe 'status' do
  let(:enable_object_trace) { false }
  before do
    server = Rbkit.start_profiling(enable_object_trace: enable_object_trace)
    @status = server.status
    Rbkit.stop_server
  end

  describe 'object_space_enabled field' do
    context "when object trace is disabled" do
      it 'should set object_trace_enabled to 0' do
        expect(@status[:object_trace_enabled]).to eql 0
      end
    end
    context "when object trace is enabled" do
      let(:enable_object_trace) { true }
      it 'should set object_trace_enabled to 0' do
        expect(@status[:object_trace_enabled]).to eql 1
      end
    end
  end

  describe 'pwd field' do
    it 'should be equal to PWD' do
      expect(@status[:pwd]).to eql Dir.pwd
    end
  end

  describe "process_name field" do
    let(:process_name) { Rbkit.status[:process_name] }
    it "should have process name" do
      expect(@status[:process_name]).to eql Process.argv0
    end
  end

  describe 'pid field' do
    it 'should be equal to Process PID' do
      expect(@status[:pid]).to eql Process.pid
    end
  end

  describe 'rbkit_server_version field' do
    it 'should be equal to Rbkit version' do
      expect(@status[:rbkit_server_version]).to eql Rbkit::VERSION
    end
  end

  describe 'rbkit_protocol_version field' do
    it 'should be equal to Rbkit version' do
      expect(Rbkit::PROTOCOL_VERSION).to eql "2.0"
      expect(@status[:rbkit_protocol_version]).to eql Rbkit::PROTOCOL_VERSION
    end
  end
end
