require "objspace"

module Rbkit
  class RbkitGC
    # Returns a standardized hash containing the data
    # returned by GC.stat
    # @return [Hash] Keys :
    #   [count] Count of major and minor GCs so far
    #   [minor_gc_count] Count of minor GCs
    #   [major_gc_count] Count of major GCs
    #   [heap_allocated_pages] Count of allocated pages (heap_eden_pages + heap_tomb_pages)
    #   [heap_eden_pages] Count of pages which has atleast one live object
    #   [heap_tomb_pages] Count of pages which don't have any object yet
    #   [heap_allocatable_pages] Count of pages that will be allocated if Ruby runs out of heap
    #   [heap_sorted_length] Count of total number of sorted pages (>= heap_allocated_pages + heap_allocatable_pages)
    #   [heap_live_slots] Count of slots in all pages having live objects
    #   [heap_free_slots] Count of free slots in all pages
    #   [heap_final_slots] Count of zombie objects
    #   [heap_swept_slots] Count of slots swept after last GC
    #   [old_objects] Count of old generation objects
    #   [old_objects_limit] Old generation object count after which GC is triggered
    #   [total_allocated_objects] Number of created objects in the lifetime of the process
    #   [total_freed_objects] Number of freed objects in the lifetime of the process
    #   [malloc_increase_bytes] Malloc'ed bytes since last GC
    #   [malloc_increase_bytes_limit] Minor GC is triggered when malloc_increase_bytes exceeds this value
    #   [oldmalloc_increase_bytes] Malloc'ed bytes for old objects since last major GC
    #   [oldmalloc_increase_bytes_limit] Major GC is triggered with oldmalloc_increase_bytes exceeds this value
    #   [total_heap_size] heap_allocated_pages * max slots per page * size of one slot
    #   [total_memsize] ObjectSpace.memsize_of_all
    def self.stat
      stats = {}
      data = GC.stat

      stats[:count] = data[:count]
      stats[:minor_gc_count] = data[:minor_gc_count]
      stats[:major_gc_count] = data[:major_gc_count]

      if RUBY_VERSION >= "2.2.0"
        [
          :heap_allocated_pages, :heap_eden_pages, :heap_tomb_pages,
          :heap_allocatable_pages, :heap_sorted_length, :heap_live_slots,
          :heap_free_slots, :heap_final_slots, :heap_swept_slots,
          :old_objects, :old_objects_limit, :total_allocated_objects,
          :total_freed_objects, :malloc_increase_bytes,
          :malloc_increase_bytes_limit, :oldmalloc_increase_bytes,
          :oldmalloc_increase_bytes_limit
        ].each do |key|
          stats[key] = data[key]
        end
      elsif RUBY_VERSION >= "2.1.0"
        stats[:heap_allocated_pages] = data[:heap_used]
        stats[:heap_eden_pages] = data[:heap_eden_page_length]
        stats[:heap_tomb_pages] = data[:heap_tomb_page_length]
        stats[:heap_allocatable_pages] = data[:heap_increment]
        stats[:heap_sorted_length] = data[:heap_length]
        stats[:heap_live_slots] = data[:heap_live_slot]
        stats[:heap_free_slots] = data[:heap_free_slot]
        stats[:heap_final_slots] = data[:heap_final_slot]
        stats[:heap_swept_slots] = data[:heap_swept_slot]
        stats[:old_objects] = data[:old_object]
        stats[:old_objects_limit] = data[:old_object_limit]
        stats[:total_allocated_objects] = data[:total_allocated_object]
        stats[:total_freed_objects] = data[:total_freed_object]
        stats[:malloc_increase_bytes] = data[:malloc_increase]
        stats[:malloc_increase_bytes_limit] = data[:malloc_limit]
        stats[:oldmalloc_increase_bytes] = data[:oldmalloc_increase]
        stats[:oldmalloc_increase_bytes_limit] = data[:oldmalloc_limit]
      end

      no_of_allocated_pages = stats[:heap_allocated_pages] rescue 0
      max_objects_per_page = GC::INTERNAL_CONSTANTS[:HEAP_OBJ_LIMIT]
      size_of_one_obj = GC::INTERNAL_CONSTANTS[:RVALUE_SIZE]
      stats[:total_heap_size] = no_of_allocated_pages * max_objects_per_page *
        size_of_one_obj
      stats[:total_memsize] = ObjectSpace.memsize_of_all
      stats
    end
  end
end
