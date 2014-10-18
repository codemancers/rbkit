def object_id_to_pointer_addr(object_id)
  (object_id * 2).to_s(18)
end

def pointer_addr_to_object_id(pointer_addr)
  pointer_addr.hex / 2
end
