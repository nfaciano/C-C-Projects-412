#!/bin/bash

if [[ "$#" -ne 3 ]]; then
    echo "Usage: $0 <path_to_watch_folder> <version_index> <path_to_output_folder>"
    exit 1
fi

watch_folder="$1"
version_index="$2"
output_folder="$3"

# Ensure watch folder exists
mkdir -p "$watch_folder"

# Build executables (assuming you named them version1, version2, version3)
gcc version1.c -o version1
gcc version2.c -o version2
gcc version3.c -o version3

# Function to process a file
process_file() {
    local file="$1"
    case "$version_index" in
        1)
            ./version1 "$output_folder" "$file"
            ;;
        2)
            ./version2 "$output_folder" "$file"
            ;;
        3)
            ./version3 "$output_folder" "$file"
            ;;
        *)
            echo "Invalid version index!"
            exit 1
            ;;
    esac
}

processed_files=()

while true; do
    # Sleep for a bit and then check for new files
    sleep 5

    for file in "$watch_folder"/*; do
        # Skip if not a file
        [[ ! -f "$file" ]] && continue

        # Skip if already processed
        if printf "%s\n" "${processed_files[@]}" | grep -q "^$file$"; then
            continue
        fi

        # If it's a .dat file, process it
        if [[ "$file" == *.dat ]]; then
            process_file "$file"
            processed_files+=("$file")
        fi
    done

    for dir in "$watch_folder"/*/; do
        # Skip if not a directory
        [[ ! -d "$dir" ]] && continue

        for file in "$dir"/*.dat; do
            # Skip if already processed
            if printf "%s\n" "${processed_files[@]}" | grep -q "^$file$"; then
                continue
            fi

            process_file "$file"
            processed_files+=("$file")
        done
    done
done
