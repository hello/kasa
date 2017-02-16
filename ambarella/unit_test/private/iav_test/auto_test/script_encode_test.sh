#!/bin/sh
echo "run ./test_encode -i1080p -V480p --hdmi"
test_encode -i1080p -V480p --hdmi
test_encode -A -h1080p -B -h480p -C -hcif -D -hcif
sleep 1
# test_image -i 0 &
sleep 1
encode_test