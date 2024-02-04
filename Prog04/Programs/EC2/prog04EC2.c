/* Long Commands Handling */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_PATH_LENGTH 256

void execute_command(char *command, const char *output_folder) {
    char output_file[MAX_PATH_LENGTH];
    snprintf(output_file, sizeof(output_file), "%s/%s.txt", output_folder, command);
    
    pid_t pid = fork();

    if (pid < 0) {
        perror("Failed to fork");
        exit(1);
    }
    
    if (pid == 0) {
        freopen(output_file, "w", stdout);
        char *args[] = {"/bin/sh", "-c", command, NULL};
        execvp(args[0], args);

        perror("Failed to exec");
        exit(1);
    } else {
        wait(NULL);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <output_folder> <command_file1> <command_file2> ... \n", argv[0]);
        exit(1);
    }

    const char *output_folder = argv[1];

    char combined_command[MAX_COMMAND_LENGTH];

    for (int i = 2; i < argc; ++i) {
        FILE *file = fopen(argv[i], "r");
        if (!file) {
            perror("Error opening command file");
            continue;
        }

        char line[MAX_COMMAND_LENGTH];
        while (fgets(line, sizeof(line), file)) {
            // Remove trailing newline character
            line[strcspn(line, "\n")] = 0;

            strcpy(combined_command, line);
            
            // Handle multi-line commands
            while (line[strlen(line) - 1] == '\\') {
                fgets(line, sizeof(line), file);
                line[strcspn(line, "\n")] = 0;
                strcat(combined_command, line);
            }

            execute_command(combined_command, output_folder);
        }
        fclose(file);
    }
    return 0;
}
