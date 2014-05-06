### How to debug a extension coming from C extension ###

1. Build a Ruby which is compiled with debug symbols:

        ./configure --prefix="$HOME/.rbenv/versions/debug_ruby" --enable-shared --disable-install-doc debugflags="-g" --with-opt-dir="$(brew --prefix openssl):$(brew --prefix libyaml):$(brew --prefix gdbm):$(brew --prefix libffi)"

I used above configure command to compile a debug symbol enabled Ruby.

2. Build your extension with debug enabled.

        1. delete existing Makefile and object/bundle/so files.
        2. export CFLAGS=-g
        3. ruby extconf.rb
        4. make

3. Run your program that makes use of C extension with:

        ~> lldb /Users/gnufied/.rbenv/versions/debug_ruby/bin/ruby -> needs absolute path
        lldb> process launch -- experiments/using_rbkit.rb

4. LLDB is slightly different from `GDB` and more information about it can be found on,
http://lldb.llvm.org/tutorial.html . I specially found following commands useful:

        1. expr
        2. frame variable
        3. frame variable <variable_name>

