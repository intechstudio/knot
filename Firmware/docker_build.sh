#!/bin/bash

# Check if Docker is installed
if command -v docker &> /dev/null; then
    CONTAINER_TOOL="docker"
# Check if Podman is installed
elif command -v podman &> /dev/null; then
    CONTAINER_TOOL="podman"
else
    echo "Neither Docker nor Podman found. Please install one of them to proceed."
    exit 1
fi

$CONTAINER_TOOL build -t idf-pico-merged .
