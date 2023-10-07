#!/bin/bash

for c in "bit decoder fixed frame huffman layer3 layer12 stream synth timer version"; do
  gcc -O2 -c ${c}.c
done

