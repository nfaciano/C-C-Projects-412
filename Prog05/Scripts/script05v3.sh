#!/bin/bash

# Assign arguments to variables
WATCH_FOLDER="$1"
OUTPUT_FOLDER="$2"

# Create the watch folder if it doesn't exist
if [ ! -d "$WATCH_FOLDER" ]; then
    mkdir -p "$WATCH_FOLDER"
fi

# Create the output folder if it doesn't exist
if [ ! -d "$OUTPUT_FOLDER" ]; then
    mkdir -p "$OUTPUT_FOLDER"
fi

# Function to process a map file
process_map_file() {
    local map_file="$1"
    # Assuming the general dispatcher is a compiled C++ program called 'general_dispatcher'
    # Pass the map file to the general dispatcher to handle
    ./general_dispatcher "$map_file" "$OUTPUT_FOLDER"
}

# Function to process a task file
process_task_file() {
    local task_file="$1"
    # Read the task file line by line
    while IFS= read -r line || [[ -n "$line" ]]; do
        # Assuming the first line is TRACE or NO_TRACE
        if [[ $line == TRACE* || $line == NO_TRACE* ]]; then
            trace_mode=$line
        # Assuming the second line is the path to the map file
        elif [[ $line == /* ]]; then
            map_file_path=$line
        # If the line contains 'END', signal the end of processing
        elif [[ $line == END* ]]; then
            end_command=$line
            if [[ $end_command == "END ALL" ]]; then
                # Signal all skier dispatchers to terminate
                # Placeholder for actual command to terminate all skier dispatchers
                echo "Terminating all skier dispatchers"
            else
                # Extract the map file name and signal the specific skier dispatcher to terminate
                map_name=$(echo $end_command | cut -d ' ' -f 2)
                # Placeholder for actual command to terminate a specific skier dispatcher
                echo "Terminating skier dispatcher for map: $map_name"
            fi
        # Otherwise, the line should contain row and column indices for a skier
        else
            # Extract row and column indices
            IFS=' ' read -ra ADDR <<< "$line"
            row_index="${ADDR[0]}"
            col_index="${ADDR[1]}"
            # Signal the skier dispatcher to start a skier at the given row and column
            # Placeholder for actual command to start a skier
            echo "Starting skier at row $row_index and column $col_index"
        fi
    done < "$task_file"
}

# Main loop to watch for new files
while true; do
    # Check for new files in the watch folder
    for file in "$WATCH_FOLDER"/*; do
        # Skip if the directory is empty
        [ -e "$file" ] || continue

        # If the file has already been processed, skip it
        [[ -n "${processed_files[$file]}" ]] && continue

        # Mark the file as processed
        processed_files["$file"]=1

        # Process the file based on its type
        if [[ $file == *.map ]]; then
            process_map_file "$file"
        elif [[ $file == *.task ]]; then
            process_task_file "$file"
        fi
    done
    # Sleep for a while before checking again
    sleep 5
done
