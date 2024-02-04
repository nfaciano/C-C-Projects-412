#!/bin/bash

commands=()

while true; do
    echo "Please enter a bash command:"
    read cmd

    # Check for redirection in the command
    if echo "$cmd" | grep -E ">\s*|<\s*" >/dev/null; then
        echo "Redirection detected. Please enter a command without redirection."
        continue
    fi

    # Execute the command
    eval "$cmd"

    # Ask the user for the next action
    echo "Choose an option: (R)ecord the command, (D)iscard it, (STOP) recording"
    read choice

    case $choice in
        R)
            commands+=("$cmd")
            ;;
        D)
            ;;
        STOP)
            echo "Enter path to save the recorded commands:"
            read filepath
            printf "%s\n" "${commands[@]}" > "$filepath"
            exit 0
            ;;
        *)
            echo "Invalid choice. Please enter R, D, or STOP."
            ;;
    esac
done
