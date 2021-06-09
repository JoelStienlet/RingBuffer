[![Build Status](https://travis-ci.org/JoelStienlet/RingBuffer.svg?branch=master)](https://travis-ci.org/JoelStienlet/RingBuffer)
[![GitHub license](https://img.shields.io/badge/license-MIT-brightgreen)](https://github.com/JoelStienlet/RingBuffer/blob/master/LICENSE)
[![codecov](https://codecov.io/gh/JoelStienlet/g3logPython/branch/master/graph/badge.svg)](https://codecov.io/gh/JoelStienlet/RingBuffer)
[![Percentage of issues still open](http://isitmaintained.com/badge/open/JoelStienlet/g3logPython.svg)](http://isitmaintained.com/project/JoelStienlet/RingBuffer "Percentage of issues still open")
[![Languages](https://img.shields.io/badge/languages-Python3/C-blue)](https://img.shields.io)

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
-ex: with one index manager only, you can manage multiple independent buffers containing different kind of data sampled at the same times.
-ex: can be used with malloc() in environments where it is available, but also with static buffers on small embedded systems.

2- Code coverage: Ring Buffers and similar fundamental tools need to be reliable. We put some effort in ensuring that every line of code in the core has passed some test.


