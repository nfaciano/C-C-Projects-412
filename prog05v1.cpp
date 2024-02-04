#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <filesystem>
#include <sstream>
#include <set>

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
    Point getLowestNeighbor(int x, int y) {
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
    ElevationMap(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filename << std::endl;
            exit(12);
        }

        // Read the map dimensions.
        file >> height >> width;

        // Resize the map and read the elevation values.
        map.resize(height, std::vector<float>(width, 0));
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                file >> map[i][j];
            }
        }
        file.close();
    }

    // Check if a point is a valid starting position.
    bool isValidStartPoint(int x, int y) {
        return x >= 0 && x < width && y >= 0 && y < height;
    }

    // Simulate the descent from a starting point, and return the path taken and number of moves.
    std::pair<std::vector<Point>, int> startDescent(int x, int y) {
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

int main(int argc, char* argv[]) {
    bool traceMode = false;
    int startArgIndex = 1;
    std::string traceString = "NO_TRACE";

    // Ensure the right number of arguments are provided.
    if (argc < 5) {
        std::cerr << "Insufficient arguments provided." << std::endl;
        return 1;
    }

    // Check if trace mode is enabled.
     if (std::string(argv[1]) == "-t") {
        traceMode = true;
        traceString = "TRACE";
        startArgIndex++;
    }

    if (!fs::exists(fs::path(argv[startArgIndex + 3]))) {
        return 11;
    }

    // Check for the existence of the map file.
    if (!fs::exists(argv[startArgIndex])) {
        std::ofstream errorFile(fs::path(argv[startArgIndex + 3]) / (fs::path(argv[startArgIndex]).stem().string() + ".txt"));
        errorFile << "Error: file not found\n";
        errorFile.close();
        return 2;
    }

    ElevationMap elevationMap(argv[startArgIndex]);
    int x = std::stoi(argv[startArgIndex + 1]) - 1; // Convert from 1-based to 0-based indexing.
    int y = std::stoi(argv[startArgIndex + 2]) - 1; // Convert from 1-based to 0-based indexing.

    if (!elevationMap.isValidStartPoint(x, y)) {
        std::cerr << "Invalid starting point." << std::endl;
        return 3;
    }

    // Perform the descent simulation and obtain the results.
   auto [visitedPoints, moves] = elevationMap.startDescent(x, y);
    std::ofstream resultFile(fs::path(argv[startArgIndex + 3]) / (fs::path(argv[startArgIndex]).stem().string() + ".txt"));

    resultFile << traceString << std::endl;
    resultFile << visitedPoints[0] << " " << visitedPoints.back() << " " << visitedPoints.size() << std::endl;
    
    if (traceMode) {
        for (const Point& point : visitedPoints) {
            resultFile << point << " ";
        }
        resultFile << std::endl;
    }

    resultFile.close();
    return 0;
}
