require 'spec_helper'

describe 'gc_stat' do
  let(:stat) { Rbkit::RbkitGC.stat }
  it 'should have the correct gc stat keys' do
    expect(stat.keys).to eql [
        :count,
        :minor_gc_count,
        :major_gc_count,
        :heap_allocated_pages,
        :heap_eden_pages,
        :heap_tomb_pages,
        :heap_allocatable_pages,
        :heap_sorted_length,
        :heap_live_slots,
        :heap_free_slots,
        :heap_final_slots,
        :heap_swept_slots,
        :old_objects,
        :old_objects_limit,
        :total_allocated_objects,
        :total_freed_objects,
        :malloc_increase_bytes,
        :malloc_increase_bytes_limit,
        :oldmalloc_increase_bytes,
        :oldmalloc_increase_bytes_limit,
        :total_heap_size,
        :total_memsize ]
  end
end

