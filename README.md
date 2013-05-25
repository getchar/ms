Hi, and thanks for your interest in `ms`.  You use it to write messages to yourself--to do lists and things like that.  Think of it as the command line equivalent of writing something on your hand, except you can decide for yourself how long it takes for the ink to wear off.  One recommended use is to run `ms` in your login script.  That way, every time you log on to your computer you're greeted with a list of tasks you need to work on.

Just run `make` to generate the binary of `ms`.  There's a man page in the directory called "man".  If you want to read it, run
    man ./man/ms.6

If you want to install `ms` automatically, just type 

    make install  

This will copy the binary to /usr/bin.  It will also put a copy of the man page in /usr/share/man/man6.  Running the program will cause a file called ".ms_file.txt" in your home directory.  If for some reason you already have a file called ".ms_file.txt" in your home directory, it will be overwritten.  Just a heads up.

This software and all associated documentation copyright Charlie Pashayan, 2012.
