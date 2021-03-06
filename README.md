# IS1200 project
Goals: design a kernel/loader that will load and run programs
from an EEPROM chip with a filesystem (FAT) on it, and provide
some helper functions (display, filesystem, etc) through syscalls.

## sendelf_crc32 usage
When running `make picocom`, `./helpers/sendelf_crc32` will be used
to transfer parts of the ELF file to the chipKIT. In picocom use
`C-a C-s` to send a file.
