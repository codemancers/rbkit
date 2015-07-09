require 'spec_helper'

describe 'status' do
  let(:enable_object_trace) { false }
  let(:enable_cpu_profiling) { false }
  let(:clock_type) { :wall }
  let(:cpu_profiling_mode) { :sampling }
  before do
    @server = Rbkit.start_profiling(enable_object_trace: enable_object_trace, enable_cpu_profiling: enable_cpu_profiling,
                                   clock_type: clock_type, cpu_profiling_mode: cpu_profiling_mode)
    @status = @server.status
    @server.stop
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

  describe 'cpu_profiling_enabled field' do
    context "when cpu profiling is disabled" do
      it 'should set cpu_profiling_enabled to 0' do
        expect(@status[:cpu_profiling_enabled]).to eql 0
      end
    end
    context "when cpu profiling is enabled" do
      let(:enable_cpu_profiling) { true }
      it 'should set cpu_profiling_enabled to 0' do
        expect(@status[:cpu_profiling_enabled]).to eql 1
      end
    end
  end

  describe 'clock_type field' do
    context 'when using wall time' do
      let(:clock_type) { :wall }
      it 'should be wall' do
        expect(@status[:clock_type]).to eql :wall
      end
    end
    context 'when using cpu time' do
      let(:clock_type) { :cpu }
      it 'should be cpu' do
        expect(@status[:clock_type]).to eql :cpu
      end
    end
  end

  describe 'cpu_profiling_mode field' do
    context 'when using sampling profiling mode' do
      let(:cpu_profiling_mode) { :sampling }
      it 'should be sampling' do
        expect(@status[:cpu_profiling_mode]).to eql :sampling
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
      expect(Rbkit::PROTOCOL_VERSION).to eql "2.1"
      expect(@status[:rbkit_protocol_version]).to eql Rbkit::PROTOCOL_VERSION
    end
  end
end
