#slarchive, a SeedLink client for archiving data streams

For usage information see the [slarchive manual](doc/slarchive.md)
in the 'doc' directory.

## Building and Installation

In most environments a simple 'make' will compile slarchive.

Using GCC, running 'make static' will compile a static version of slarchive
if possible.

GCC can be explicitly used by running 'make gcc'.

SunOS/Solaris:

In order to compile under Solaris the 'src/Makefile' needs to be edited.
See the file for instructions.

For further installation simply copy the resulting binary and man page
(in the 'doc' directory) to appropriate system directories.

## License

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

[http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0)

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Copyright (C) 2023 Chad Trabant, EarthScope Data Services