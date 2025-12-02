# Porting guide for ulog

## Establish a platform name

To ensure consistency and clarity across all supported platforms,
we adopt a standardized naming convention for platform identifiers used in
 * macros
 * header paths
 * build configurations.

The Naming Convention is:
<arch>_<os>_<abi>[_<vendor>]

### Components:

* arch: CPU architecture
Examples: avr, arm, aarch64, x86_64, riscv64
* os: Operating system or kernel
Examples: linux, baremetal, windows, freertos, asx
* abi: Application Binary Interface or libc variant
Examples: gnueabi, gnu, eabi, mingw
* vendor (optional): Board or toolchain vendor
Examples: nvidia, raspberry, nxp

### Examples

arm_baremetal_eabi
arm_linux_gnueabi
aarch64_linux_gnu
x86_64_linux_gnu
avr_asx_gnu

### Rules
Use lowercase for all components.
Separate components with underscores (_).
Include arch, os, and abi at minimum.
Add vendor only if necessary to distinguish variants.
Avoid distribution-specific names (e.g., ubuntu, poky) unless required for compatibility.

## Copy templates

Copy the template files:
1. ulog_port_header_template.h -> ulog_port_header_<your platform name>.h
2. ulog_port_impl_template.h -> ulog_port_impll_<your platform name>.h

## Edit both files

Follow the guidelines from the template. Check other implementation for ideas.

## Activate the plaform

Edit the file ulog_port_selection.h and add detection of the platform based on 
existing compiler defines.

## Manage the linker script

To work, the linker must create .logs sections in the resulting .elf file.

The memory and sections items of the linker script must be edited.

### MEMORY section

The 'logmeta' section must be added.
This section contains all the logs supported by the application.
It is at 0x100000 by default but can be changed. The ulogger viewer will get the log metadata from this section.

  MEMORY
  {
    text   (rx)   : ...
    data   (rw!x) : ...
    ...
    logmeta (r) : ORIGIN = 0x10000, LENGTH = 0x100000
  }

### logs in sections

The sections has a log section where all logs are added.

  SECTIONS
  {
    .logs () :
    {
      KEEP(*(.logs))
    } > logmeta

    ...
  }