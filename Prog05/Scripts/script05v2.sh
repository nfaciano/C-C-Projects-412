#!/bin/bash

# Check if the correct number of arguments are provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <path to watch folder> <path to output folder>"
    exit 1
fi

# Compile the programs
g++ Programs/Version3/prog05v3.cpp --std=c++20 -o Compiled/prog05v3

# Make the compiled programs executable
chmod +x Compiled/*

WATCH_FOLDER=$1
OUTPUT_FOLDER=$2
TEMP_FOLDER="$OUTPUT_FOLDER/Temp"

# Create the watch folder if it doesn't exist
if [ ! -d "$WATCH_FOLDER" ]; then
    mkdir -p "$WATCH_FOLDER"
fi

# Create or empty the output folder
if [ ! -d "$OUTPUT_FOLDER" ]; then
    mkdir -p "$OUTPUT_FOLDER"
else
    rm -rf "$OUTPUT_FOLDER"/*
fi

mkdir -p "$TEMP_FOLDER"

process_task_file() {
    TASK_FILE=$1
    TRACE_FLAG=""
    MAP_FILE=""
    SKIER_COUNT=0
    SKIER_POINTS=()
    
    # Read the task file
    while IFS= read -r line || [[ -n "$line" ]]; do
        if [[ "$line" == "TRACE" ]]; then
            TRACE_FLAG="-t"
        elif [[ "$line" == "NO TRACE" ]]; then
            TRACE_FLAG=""
        elif [[ -z "$MAP_FILE" ]]; then
            MAP_FILE="$line"
        elif [[ "$SKIER_COUNT" -eq 0 ]]; then
            SKIER_COUNT="$line"
        else
            SKIER_POINTS+=("$line")
        fi
    done < "$TASK_FILE"

    # Construct the output file name
    OUTPUT_FILE="$OUTPUT_FOLDER/"

    # Construct the command to run the skier dispatcher
    CMD="Compiled/prog05v3 $TRACE_FLAG $MAP_FILE"
    for point in "${SKIER_POINTS[@]}"; do
        CMD="$CMD $point"
    done
    CMD="$CMD $OUTPUT_FILE"

    # Run the skier dispatcher command in the background
    eval $CMD &
}

# Start the polling loop
while true; do
    # Check for new .task files
    for FILE in "$WATCH_FOLDER"/*.task; do
        # Skip if no files found
        [ -e "$FILE" ] || continue

        # Process the file if it's new
        if [ ! -f "$TEMP_FOLDER/$(basename "$FILE")" ]; then
            if grep -q "^END$" "$FILE"; then
                echo "Terminating all skier processes."
                # Implement process termination logic here
                # This is a placeholder; actual implementation will depend on how you're managing processes
                pkill -P $$ # This will kill all child processes of this script's process
                exit 0
            else
                process_task_file "$FILE"
                # Mark the file as processed
                touch "$TEMP_FOLDER/$(basename "$FILE")"
            fi
        fi
    done

    # Sleep for a while before checking again
    sleep 5
done
