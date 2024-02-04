#!/bin/bash

# Check for correct number of arguments
if [ "$#" -lt 4 ]; then
    echo "Usage: $0 <path_to_executable> <num_threads> <path_to_output_image> <path_to_image_stack_folder>"
    exit 1
fi

# Assign arguments to variables
EXECUTABLE=$1
NUM_THREADS=$2
OUTPUT_PATH=$3
IMAGE_STACK_FOLDER=$4

# Create an array of image paths
IMAGE_PATHS=($(ls $IMAGE_STACK_FOLDER/*.tga))

# Run the stack focusing program
$EXECUTABLE $NUM_THREADS $OUTPUT_PATH "${IMAGE_PATHS[@]}"

