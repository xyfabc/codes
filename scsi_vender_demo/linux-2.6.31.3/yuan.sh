#!/bin/bash
while true; do
	echo "make clean"
	make clean
	echo "make uImage"
	make uImage
done
