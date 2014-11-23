require 'spec_helper'

describe 'Messages printed by Rbkit on errors' do
  let(:error_message) {
    "Rbkit server couldn't bind to socket, check if it is already" \
      " running. Profiling data will not be available."
  }

  context 'when Rbkit.start_server is run twice' do
    it 'should print helpful message and return false the second time' do
      expect($stderr).to receive(:puts).with(error_message).once
      expect(Rbkit.start_server).to be_truthy
      expect(Rbkit.start_server).to be_falsey
      Rbkit.stop_server
    end
  end
  context 'when Rbkit.start_profiling is run twice' do
    it 'should print helpful message and return false the second time' do
      expect($stderr).to receive(:puts).with(error_message).once
      expect(Rbkit.start_profiling).to be_truthy
      expect(Rbkit.start_profiling).to be_falsey
      Rbkit.stop_server
    end
  end
  context 'when Rbkit.stop_server is run without starting the server' do
    it 'should print helpful message and return false' do
      expect($stderr).to receive(:puts).with("Cannot stop Rbkit server. Is it running?").once
      expect(Rbkit.stop_server).to be_falsey
    end
  end
  context 'when Rbkit.stop_server is run twice' do
    it 'should print helpful message and return false the second time' do
      expect($stderr).to receive(:puts).with("Cannot stop Rbkit server. Is it running?").once
      expect(Rbkit.start_server).to be_truthy
      expect(Rbkit.stop_server).to be_truthy
      expect(Rbkit.stop_server).to be_falsey
    end
  end
  context 'during normal operation' do
    it 'should not print anything' do
      expect($stderr).not_to receive(:puts)
      expect(Rbkit.start_server).to be_truthy
      expect(Rbkit.stop_server).to be_truthy
      expect(Rbkit.start_server).to be_truthy
      expect(Rbkit.stop_server).to be_truthy
    end
  end
  context 'after a series of failed attempts' do
    it 'should be able to resume normal operation' do
      allow($stderr).to receive(:puts)
      expect(Rbkit.stop_server).to be_falsey
      expect(Rbkit.start_server).to be_truthy
      expect(Rbkit.start_server).to be_falsey
      expect(Rbkit.stop_server).to be_truthy
      expect(Rbkit.stop_server).to be_falsey

      expect(Rbkit.start_server).to be_truthy
      expect(Rbkit.stop_server).to be_truthy
    end
  end
end
