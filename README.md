[![Build Status](https://api.travis-ci.com/JoelStienlet/RingBuffer.svg?branch=main)](https://www.travis-ci.com/github/JoelStienlet/RingBuffer)
[![GitHub license](https://img.shields.io/badge/license-MIT-brightgreen)](https://github.com/JoelStienlet/RingBuffer/blob/main/LICENSE)
[![codecov](https://codecov.io/gh/JoelStienlet/RingBuffer/branch/main/graph/badge.svg?token=YLGvWF43U9)](https://codecov.io/gh/JoelStienlet/RingBuffer)
[![Percentage of issues still open](http://isitmaintained.com/badge/open/JoelStienlet/g3logPython.svg)](http://isitmaintained.com/project/JoelStienlet/RingBuffer "Percentage of issues still open")
[![Languages](https://img.shields.io/badge/languages-C-blue)](https://img.shields.io)

## License
MIT license

## Features
- Focussed and limited to index management
- Suitable for embedded systems
- can be used for DMA (see example 2)
- lockless
- core in pure C

## Why another Ring Buffer?
1- The main difference between this implementation and others is that this one focusses exclusively on index management, thus allowing for maximum flexibility. All other projects that I've seen insist on somehow interacting with the data. We don't.
Some advantages of doing less:
- With one index manager only, you can manage multiple independent buffers containing different kind of data sampled at the same times.
- The item's size is obviously arbitrary: you can store any type you want.
- Can be used with malloc() in environments where it is available, but also with static buffers on small embedded systems.

2- 100% Code coverage: Ring Buffers and similar fundamental tools need to be reliable. We put some effort in ensuring that every line of code in the core has passed at least one test.
``` make coverage ```
Note that although it proudly displays "100%", you shouldn't be fooled, and manual inspection will still reveal some lines that haven't been reached with the default settings.
In order to enter some corner-case safety tests, the code has to be built with invalid definitions in custom_circ_buf.h. A full coverage test will thus require some manual inspection too.

## How to build it?

This library is not intended for building as a shared library. Mainly because the source will strongly depend on the custom types you choose in custom_circ_buf.h. Just include the source file in your project.

## How to use it?

The best way to start is having a look at example 1 (Be careful that this is only intendend to display the general look and feel of the API, in production you must obviously manage the possible error conditions that could happen). Then have a look at circ_buf.h to get a more precise idea of the API.

## Current status:

We're working on Travis and codecov integration. For now Travis is displaying "Abuse detected" without any further indication.

## Other interesting Ring Buffer implementations:

- A ring buffer that always presents a linear memory region, using mmap (obviously requiring a CPU providing memory paging) (note the AGPL-3 license for that one):
 [https://github.com/lava/linear_ringbuffer](https://github.com/lava/linear_ringbuffer)


 
