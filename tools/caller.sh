#!/usr/bin/env bash

# Change the range for your needs, 0..9 or a..z or A..Z etc
for c in {A..Z}; do
	python3 png_converter.py "../resources/main/$c.png"
done
