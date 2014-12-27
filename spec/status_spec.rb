require 'spec_helper'
require 'msgpack'

describe 'status' do
  describe 'object_space_enabled field' do
    context "when object trace is disabled" do
      before do
        Rbkit.start_profiling(enable_object_trace: false)
        @status = Rbkit.status
        Rbkit.stop_server
      end
      it 'should set object_trace_enabled to 0' do
        expect(@status[:object_trace_enabled]).to eql 0
      end
    end
    context "when object trace is enabled" do
      before do
        Rbkit.start_profiling(enable_object_trace: true)
        @status = Rbkit.status
        Rbkit.stop_server
      end
      it 'should set object_trace_enabled to 0' do
        expect(@status[:object_trace_enabled]).to eql 1
      end
    end
  end

  describe 'pwd field' do
    let(:field) { Rbkit.status[:pwd] }
    it 'should be equal to PWD' do
      expect(field).to eql Dir.pwd
    end
  end

  describe "process_name field" do
    let(:process_name) { Rbkit.status[:process_name] }
    it "should have process name" do
      expect(process_name).to eql Process.argv0
    end
  end

  describe 'pid field' do
    let(:field) { Rbkit.status[:pid] }
    it 'should be equal to Process PID' do
      expect(field).to eql Process.pid
    end
  end
end
