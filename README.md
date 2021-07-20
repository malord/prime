Portable RuntIME
================

Prime is my portable runtime, which I've used on most of my C++ projects since 2000.

Extras/PrimeConfig.h needs to be copied and placed in the directory _above_ Prime. You can then configure which supporting libraries you have available and therefore enable/disable relevant portions of Prime. I suspect that if you've found this repo then you have a project which already uses Prime, so this will have already been done.

There's no autoconf/automake or any form of automated configuration, it's mostly done via preprocessor defines. This was done because much of my early work was in games where individual platforms would have poor/non-existant support for autotools.

Another decision affected by my games programming experience was to eschew C++'s RTTI and exception facilities, which are frequently disabled in games development. Functions which can fail return a `bool` but they also take a pointer to a `Log` instance which is used for reporting errors. I originally expected that the `Log` instance would have the ability to store details on the error (e.g., the `errno`) but I found I never needed that.

Some interesting features of the code:

- All I/O is done via the `Stream` class, which means you can get a `Stream` to a zlib compressed lump inside a zip archive and read it as though it's a regular file.
- There's built in support for XML (both pull parsing and expat style), JSON, CSV, multipart MIME, ZIP files, chunked encoding and so on.
- I spent a lot of time working on custom scripting languages and parsing configuration files, so there's a pretty good `TextReader` class which allows text to be read from RAM or streamed (from a `Stream`) in in blocks from an external file. `TextReader` is the basis of `JSONReader`, `XMLPullParser`, `CSVParser` and so on.
- There's an HTTP/1.1 web server and a bit of a framework around that and a bunch of extra networking support code, e.g., SOCKS proxy support and HTTP(s) clients.
- There are emulated threading primitives, such as barriers, Windows style events, read write locks and semaphores (look in the Emulated/ folder).
- There's C++ code for reading/writing Apple binary plists.
- There's a Python style database module (with SQLite and MySQL implementations - which you must link with mariadb to avoid GPL3 issues).

If you've inherrited a project which uses Prime, feel free to create pull requests or issues if you encounter any issues. The interface hasn't changed much for years so hopefully the latest version will just work without changes in your project.
