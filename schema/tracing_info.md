## How we send tracing information ##

1. For now Tracing information is sent via simple strings like:

        obj_created <klass_name> <object_id_in_hex>
        obj_destroyed <klass_name> <object_id_in_hex>
        gc_start
        gc_end_s
