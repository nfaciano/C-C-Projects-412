/* Combined Log File */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_PATH_LENGTH 256

static int log_idx = 1;
char log_file[MAX_PATH_LENGTH];

void write_to_log(const char *command) {
    FILE *log = fopen(log_file, "a");
    if (!log) {
        perror("Error opening log file");
        exit(1);
    }
    fprintf(log, "%s\n", command);
    fclose(log);
}

void execute_command(char *command, const char *output_folder) {
    // Write command to log
    write_to_log(command);

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
    snprintf(log_file, sizeof(log_file), "%s/log%d.txt", output_folder, log_idx++);

    char line[MAX_COMMAND_LENGTH];

    for (int i = 2; i < argc; ++i) {
        FILE *file = fopen(argv[i], "r");
        if (!file) {
            perror("Error opening command file");
            continue;
        }

        while (fgets(line, sizeof(line), file)) {
            // Remove trailing newline character
            line[strcspn(line, "\n")] = 0;

            execute_command(line, output_folder);
        }
        fclose(file);
    }
    return 0;
}
