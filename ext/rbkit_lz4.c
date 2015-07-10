#include "ruby/ruby.h"
#include "lz4.h"
#include "rbkit_lz4.h"

static VALUE lz4_error;

static VALUE rb_lz4_uncompress(VALUE self, VALUE input) {
  char *src_p = RSTRING_PTR(input);
  size_t src_length  = RSTRING_LEN(input);
  size_t uncompressed_size = *((size_t *)src_p);
  VALUE result = rb_str_new(NULL, uncompressed_size);
  char *dest = RSTRING_PTR(result);
  int read_bytes = LZ4_decompress_safe(src_p + sizeof(size_t), dest, (int)(src_length - sizeof(size_t)), (int)uncompressed_size);
  if (read_bytes < 0) {
    rb_raise(lz4_error, "Cannot uncompress with LZ4");
  }
  return result;
}

int lz4_max_compressed_length(int uncompressed_length) {
  return (LZ4_compressBound(uncompressed_length) + sizeof(size_t));
}

int lz4_compress(char *source, char *dest, size_t in_size, int max_dest_size) {
  int bytes_after_compressing;
  memcpy(dest, &in_size, sizeof(size_t));

  bytes_after_compressing = LZ4_compress_default(source, dest + sizeof(size_t), (int)in_size, max_dest_size - sizeof(size_t));
  if (bytes_after_compressing < 0) {
    rb_raise(lz4_error, "Cannot compress with LZ4");
  }
  return bytes_after_compressing + sizeof(size_t);
}

void Init_rbkit_lz4() {
  VALUE rbkit_module = rb_define_module("Rbkit");
  VALUE rbkit_lz4 = rb_define_class_under(rbkit_module, "LZ4", rb_cObject);
  VALUE klass = rb_singleton_class(rbkit_lz4);
  rb_define_method(klass, "uncompress", rb_lz4_uncompress, 1);
  lz4_error = rb_define_class_under(rbkit_lz4, "Error", rb_eStandardError);
}
