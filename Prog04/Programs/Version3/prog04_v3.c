#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_COMMAND_LENGTH 256
#define MAX_PATH_LENGTH 256

void execute_command(const char *command, const char *output_folder) {
    char output_file[MAX_PATH_LENGTH];
    static int idx = 1;

    // Format the output filename
    snprintf(output_file, sizeof(output_file), "%s/%s%d.txt", output_folder, command, idx++);
    
    pid_t pid = fork(); // Create a new process

    if (pid < 0) {
        perror("Failed to fork");
        exit(1);
    }
    
    if (pid == 0) { // This block will be run by the child process
        // Redirect standard output to the output file
        freopen(output_file, "w", stdout);
        
        char *args[] = {"/bin/sh", "-c", (char *)command, NULL};
        execvp(args[0], args);

        // If execvp fails
        perror("Failed to exec");
        exit(1);
    } else { // This block will be run by the parent
        wait(NULL); // Wait for child to exit
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <output_folder> <command_file1> <command_file2> ... \n", argv[0]);
        exit(1);
    }

    const char *output_folder = argv[1];

    for (int i = 2; i < argc; ++i) {
        FILE *file = fopen(argv[i], "r");
        if (!file) {
            perror("Error opening command file");
            continue;
        }

        char command[MAX_COMMAND_LENGTH];
        while (fgets(command, sizeof(command), file)) {
            // Remove trailing newline character
            command[strcspn(command, "\n")] = 0;
            execute_command(command, output_folder);
        }
        fclose(file);
    }
    return 0;
}
