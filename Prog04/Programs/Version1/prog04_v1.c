#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 256
#define MAX_PATH_LENGTH 256

void execute_command(const char *command, const char *output_folder) {
    char output_file[MAX_PATH_LENGTH];
    static int idx = 1;

    // Format the output filename
    snprintf(output_file, sizeof(output_file), "%s/%s%d.txt", output_folder, command, idx++);
    
    char system_command[MAX_COMMAND_LENGTH + MAX_PATH_LENGTH];
    snprintf(system_command, sizeof(system_command), "%s > %s", command, output_file);
    system(system_command);
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
