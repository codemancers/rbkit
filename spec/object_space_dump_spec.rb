require 'spec_helper'
describe "Object space dump" do
  it "should #TODO" do
    Rbkit.start_server
    Object.new
    Object.new
    require 'pry'; binding.pry 
  end
end
