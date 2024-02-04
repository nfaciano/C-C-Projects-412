#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <filesystem>
#include <sstream>
#include <set>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdio> 


namespace fs = std::filesystem;


// Point structure to represent a coordinate (x, y) on the map.
struct Point {
    int x, y;

    // Overload the '<' operator to allow easy comparison of points.
    bool operator<(const Point& other) const {
        return x < other.x || (x == other.x && y < other.y);
    }

    // Overload the '<<' operator to easily print points.
    friend std::ostream& operator<<(std::ostream& os, const Point& p) {
        // Adjust back to 1-based indexing for output.
        os << p.x + 1 << " " << p.y + 1;
        return os;
    }
};

class ElevationMap {
private:
    std::vector<std::vector<float>> map; // 2D vector to hold elevation data.
    int height, width; // Dimensions of the map.

    // Function to get the neighboring point with the lowest elevation.
    Point getLowestNeighbor(int x, int y) const{
        // Directions for neighboring points.
        Point moves[8] = {
            {0, -1},  // North
            {-1,  0}, // West
            {1,  0},  // East
            {0,  1},  // South
            {-1, -1}, // Northwest
            {1, -1},  // Northeast
            {-1,  1}, // Southwest
            {1,  1}   // Southeast
        };

        Point lowestPoint = {x, y};
        float lowestElevation = map[y][x];

        // Check all neighbors.
        for (const Point& move : moves) {
            int newX = x + move.x;
            int newY = y + move.y;

            // Ensure the new point is within the map boundaries.

            if (newX >= 0 && newX < width && newY >= 0 && newY < height) {

                if (map[newY][newX] < lowestElevation) {
                    lowestElevation = map[newY][newX];
                    lowestPoint = {newX, newY};
                }
            }

        }
        return lowestPoint;
    }

public:
    // Constructor to initialize the elevation map from a file.

        // Getter for width
    int getWidth() const {
        return width;
    }

    // Getter for height
    int getHeight() const {
        return height;
    }
     ElevationMap(const std::string& filename) {
        FILE* file = fopen(filename.c_str(), "r");
        if (!file) {
            std::cerr << "Error opening file: " << filename << std::endl;
            exit(12);
        }

        // Read the map dimensions.
        fscanf(file, "%d %d", &height, &width);

        // Resize the map and read the elevation values.
        map.resize(height, std::vector<float>(width, 0));
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                fscanf(file, "%f", &map[i][j]);
            }
        }
        fclose(file);
    }

    // Check if a point is a valid starting position.
    bool isValidStartPoint(int x, int y) const {
    // Adjust the check for 0-based indexing
    return x >= 0 && x < getWidth() && y >= 0 && y < getHeight();
}

void saveMapToFile(const std::string& outputFile) {
    FILE* file = fopen(outputFile.c_str(), "w");
    if (!file) {
        std::cerr << "Error opening file for writing: " << outputFile << std::endl;
        exit(12);
    }

    // Write the map dimensions.
    fprintf(file, "%d %d\n", height, width);

    // Write the elevation values to the file.
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            fprintf(file, "%.2f ", map[i][j]);
        }
        fprintf(file, "\n");
    }
    fclose(file);
}


    // Simulate the descent from a starting point, and return the path taken and number of moves.
    std::pair<std::vector<Point>, int> startDescent(int x, int y) const {
        std::vector<Point> visitedPoints;

        Point current = {x, y};
        while (true) {
            visitedPoints.push_back(current);
            Point next = getLowestNeighbor(current.x, current.y);

            // If we're at a local minimum (current position is the lowest), break.
            if (next.x == current.x && next.y == current.y) {
                break;
            }

            current = next;
        }

        int moves = visitedPoints.size() - 1;
        return {visitedPoints, moves};
    }
};
pid_t startSkierProcess(const ElevationMap& elevationMap, int x, int y, const fs::path& tempDir, bool traceMode);
bool parseArguments(int argc, char* argv[], bool& traceMode, std::string& mapFilename, std::vector<std::pair<int, int>>& startPoints, std::string& outputDirectory);
void cleanup(const std::string& tempDir);
bool checkMapFile(const char* filePath);
void createTemporaryDirectory(const fs::path& tempDir);
void waitForChildren(const std::vector<pid_t>& childPIDs);
void collectAndCombineResults(const fs::path& tempDir, const std::vector<pid_t>& childPIDs, const std::string& traceString, int numSkiers, const fs::path& outputPath);


ElevationMap initializeElevationMap(const std::string& mapFilename);

int main(int argc, char* argv[]) {
    bool traceMode = false;
    std::string mapFile,outputDirectory;
    std::vector<std::pair<int, int>> startPoints;
    // Parse arguments
    if (!parseArguments(argc, argv, traceMode, mapFile, startPoints, outputDirectory)) {
        return 1; // parseArguments will handle its own error messages
    }
    // Check map file
    if (!checkMapFile(mapFile.c_str())) {
        return 2; // checkMapFile will handle its own error messages
    }
    // Load the elevation map (assuming ElevationMap is a class that you have defined)
    ElevationMap elevationMap(mapFile);
    // Create a temporary directory for skier outputs
    fs::path tempDir =  "tempSkierOutput";
    createTemporaryDirectory(tempDir);

    // Start processes for skiers
    std::vector<pid_t> childPIDs;
    for (const auto& [x, y] : startPoints) {
        pid_t pid = startSkierProcess(elevationMap, x, y, tempDir, traceMode);
        if (pid <= 0) {
            std::cerr << "Failed to start skier process." << std::endl;
            return 3; // Error message handled by startSkierProcess
        }
        childPIDs.push_back(pid);
    }
    // Wait for all child processes to finish
    waitForChildren(childPIDs);
    // Collect results from skier processes and combine them
    fs::path outputDirectoryPath = fs::path(outputDirectory);
    if (!fs::exists(outputDirectoryPath)) {
        fs::create_directories(outputDirectoryPath);
    }
    fs::path outputPath = outputDirectoryPath / (fs::path(mapFile).stem().string() + "_combined.txt");
    collectAndCombineResults(tempDir, childPIDs, traceMode ? "TRACE" : "NO_TRACE", startPoints.size(), outputPath);
    // Cleanup: remove the temporary directory
    fs::remove_all(tempDir);

    return 0;
}

bool parseArguments(int argc, char* argv[], bool& traceMode, std::string& mapFile, std::vector<std::pair<int, int>>& startPoints, std::string& outputDirectory) {
    int startArgIndex = 1;
    // Check if trace mode is enabled
    if (argc > 1 && std::string(argv[1]) == "-t") {
        traceMode = true;
        startArgIndex++;
    }

    // Check if there are enough arguments to proceed
    if (argc < startArgIndex + 3) { // Expect at least map file, one start point pair, and output directory
        std::cerr << "Usage: " << argv[0] << " [-t] <map_file> <start_points>... <output_directory>\n";
        return false;
    }
    
    // Parse map file
    mapFile = argv[startArgIndex++];
    
    // Parse start points - expecting pairs of arguments for each start point
    while (startArgIndex < argc - 1) { // The last argument is the output directory
        // Make sure there's a pair of arguments for each start point
        if (startArgIndex + 1 >= argc - 1) {
            std::cerr << "Expected a pair of coordinates but got an odd number of arguments\n";
            return false;
        }

        int row, column;
        // Attempt to parse the row and column values from the arguments
        try {
            row = std::stoi(argv[startArgIndex++]);
            column = std::stoi(argv[startArgIndex++]);
        } catch (const std::invalid_argument& ia) {
            std::cerr << "Invalid start point format: " << argv[startArgIndex - 2] << " " << argv[startArgIndex - 1] << "\n";
            return false;
        } catch (const std::out_of_range& oor) {
            std::cerr << "Start point out of range: " << argv[startArgIndex - 2] << " " << argv[startArgIndex - 1] << "\n";
            return false;
        }

        // Adjust for 0-based indexing
        startPoints.emplace_back(row - 1, column - 1);
    }
    
    // Parse output directory
    outputDirectory = argv[startArgIndex];
    
    return true;
}

bool checkMapFile(const char* filePath) {
    if (!fs::exists(filePath)) {
        std::cerr << "Error: file not found\n";
        return false;
    }
    return true;
}

void createTemporaryDirectory(const fs::path& tempDir) {
    if (!fs::create_directory(tempDir) && !fs::exists(tempDir)) {
        std::cerr << "Failed to create temporary directory: " << tempDir << std::endl;
        exit(1); // This might need to be replaced with proper error handling
    }
}



pid_t startSkierProcess(const ElevationMap& elevationMap, int x, int y, const fs::path& tempDir, bool traceMode) {
    pid_t pid = fork();
    if (pid == 0) { // Child (skier) process
        fs::path filename = tempDir / ("skier_" + std::to_string(getpid()) + ".txt");

        FILE* skierFile = fopen(filename.c_str(), "w");
        if (!skierFile) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            exit(1);
        }

        if (!elevationMap.isValidStartPoint(x, y)) {
            fprintf(skierFile, "Start point at row=%d, column=%d is invalid\n", x + 1, y + 1);
            fclose(skierFile);
            exit(0);
        }

        auto [visitedPoints, moves] = elevationMap.startDescent(x, y);

        // Print the starting point, the end point, and the number of moves
        const Point& startPoint = visitedPoints.front();
        const Point& endPoint = visitedPoints.back();
        fprintf(skierFile, "%d %d %d %d %d\n", startPoint.y + 1, startPoint.x + 1, endPoint.y + 1, endPoint.x + 1, moves);

        // If in trace mode, also print all the points on the path
        if (traceMode) {
            for (const auto& point : visitedPoints) {
                fprintf(skierFile, "%d %d ", point.y + 1, point.x + 1);
            }
            fprintf(skierFile, "\n");
        }

        fclose(skierFile);
        exit(0);
    } else if (pid < 0) {
        std::cerr << "Fork failed!" << std::endl;
        exit(1);
    }
    return pid;
}





void waitForChildren(const std::vector<pid_t>& childPIDs) {
    int status;
    for (pid_t pid : childPIDs) {
        if (waitpid(pid, &status, 0) == -1) {
            std::cerr << "Error waiting for child process with PID " << pid << std::endl;
        } else {
            if (WIFEXITED(status)) {
                int exitStatus = WEXITSTATUS(status);
                std::cerr << "Child process with PID " << pid << " exited with status " << exitStatus << std::endl;
            } else {
                std::cerr << "Child process with PID " << pid << " did not exit normally" << std::endl;
            }
        }
    }
    std::cerr << "All child processes have finished." << std::endl;
}


void collectAndCombineResults(const fs::path& tempDir, const std::vector<pid_t>& childPIDs, const std::string& traceString, int numSkiers, const fs::path& outputPath) {

    FILE* resultFile = fopen(outputPath.c_str(), "w");
    if (!resultFile) {
        std::cerr << "Failed to open combined output file: " << outputPath << std::endl;
        exit(3);
    }

    fprintf(resultFile, "%s\n", traceString.c_str());
    fprintf(resultFile, "%d\n", numSkiers);

    for (pid_t pid : childPIDs) {
        fs::path filename = tempDir / ("skier_" + std::to_string(pid) + ".txt");

        FILE* skierFile = fopen(filename.c_str(), "r");
        if (!skierFile) {
            std::cerr << "Failed to open skier result file: " << filename << std::endl;
            // If you expect the file to exist and it doesn't, this is an error condition.
            // Handle it appropriately, e.g., by continuing to the next file, or by exiting.
            continue;
        }

        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), skierFile) != NULL) {
            fprintf(resultFile, "%s", buffer);
        }
        fclose(skierFile);
    }

    fclose(resultFile);
}



