#!/bin/bash

# Check if Docker is installed
if command -v docker &> /dev/null; then
    CONTAINER_TOOL="docker"
    ARGS="--privileged"
# Check if Podman is installed
elif command -v podman &> /dev/null; then
    CONTAINER_TOOL="podman"
    ARGS="--group-add keep-groups"
else
    echo "Neither Docker nor Podman found. Please install one of them to proceed."
    exit 1
fi

$CONTAINER_TOOL run $ARGS --network=host -it -v /dev:/dev -v $PWD:/project -w /project/Firmware knot-build
