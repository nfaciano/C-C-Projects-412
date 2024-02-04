#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

// Read in the map to a 2D vector
bool readMap(const std::string& filePath, std::vector<std::vector<float>>& map, int& rows, int& cols);

// Finds the neighboring points to a point
std::vector<std::pair<int, int>> getNeighbors(int row, int col, int rows, int cols);

// Calcuate the lowest point path
std::vector<std::pair<int, int>> steepestDescent(const std::vector<std::vector<float>>& map, int startRow, int startCol);

// Write the results of the program
void show_results(std::string outputFolderPath, bool traceMode,int startRow, int startCol,std::vector<std::pair<int, int>> path,std::string fileName);

// Get the map file name
std::string extractMapName(const std::string& mapFilePath);

// Display the errors to the program output
void show_results(std::string outputFolderPath,std::string fileName, std::string Output_String);

int main(int argc, char *argv[]) {
    // Store the variables inputed into the program
    bool traceMode = false; // Stores true or false for if trace mode is needed
    std::string mapFilePath; // Like to the map path
    int startRow, startCol; // Stores the current pos to start
    std::string outputFolderPath; // Stores the path to the output folder 

    // Stores the info when trace is true
    if (std::string(argv[1]) == "-t") {
        traceMode = true;
        mapFilePath = argv[2];
        startRow = atoi(argv[3]);
        startCol = atoi(argv[4]);
        outputFolderPath = argv[5];
    }
    // Stores the info when the trace is false
    else{
        mapFilePath = argv[1];
        startRow = atoi(argv[2]);
        startCol = atoi(argv[3]);
        outputFolderPath = argv[4];
    }

    // Changes the start row to 0,0 scale instead of 1,1
    startRow -= 1;
    startCol -= 1;

    if (!outputFolderPath.empty() && outputFolderPath.back() == '/') {
        // Remove the last character
        outputFolderPath.pop_back();
    }

    // Gets the name of the files
    std::string fileName = extractMapName(mapFilePath);

    // Stores the map vector
    std::vector<std::vector<float>> map;

    // Stores the rows and cols 
    int rows, cols;

    // reads the map and exits if the file isnt able to be opened
    if(!readMap(mapFilePath, map, rows, cols)){
        show_results(outputFolderPath,fileName, "File not found");
        exit(12);
    }

    // checks if the inputed starting position is out of scope
    if (startCol > cols || startRow > rows){
        show_results(outputFolderPath,fileName,("Start point at row=" + std::to_string(startRow+1) + ", column=" + std::to_string(startCol+1) + " is invalid." ));
        return 0;
    }

    // Run the Steepest Descent Algorithm
    std::vector<std::pair<int, int>> path = steepestDescent(map, startRow, startCol);

    // Output Results
    show_results(outputFolderPath,traceMode,startRow,startCol,path,fileName);

    // exit the program
    return 0;
}

// Read in the map to a 2D vector
bool readMap(const std::string& filePath, std::vector<std::vector<float>>& map, int& rows, int& cols) {
    
    // Start our file stream
    std::ifstream inFile(filePath);

    // if file cant be opened return false
    if (!inFile.is_open()) {
        return 0;
    }

    // create the line variable 
    std::string line;

    // Read the dimensions of the map
    std::getline(inFile, line);
    std::istringstream iss(line);
    iss >> rows >> cols;

    // Resize the map to the dimensions read
    map.resize(rows, std::vector<float>(cols));

    // Read the map data
    for (int i = 0; i < rows; ++i) {
        std::getline(inFile, line);
        std::istringstream iss(line);
        for (int j = 0; j < cols; ++j) {
            iss >> map[i][j];
        }
    }

    // if properly compleated return true
    return 1;
}

// Finds the neighboring points to a point
std::vector<std::pair<int, int>> getNeighbors(int row, int col, int rows, int cols) {

    // create a vector to store the neighbors
    std::vector<std::pair<int, int>> neighbors;

    // search for neighbors
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue; // Skips the current point

            // bounds check
            int newRow = row + i;
            int newCol = col + j;
            // if requirments are met store neighbor
            if (newRow >= 0 && newRow < rows && newCol >= 0 && newCol < cols) {
                neighbors.push_back({newRow, newCol});
            }
        }
    }

    // Return the nighbors vector
    return neighbors;
}

// Calcuate the lowest point path
std::vector<std::pair<int, int>> steepestDescent(const std::vector<std::vector<float>>& map, int startRow, int startCol) {

    // Initiate the required variables for the function
    int rows = map.size(); // store the number of rows
    int cols = map[0].size(); // Store the number of Colums
    std::vector<std::pair<int, int>> path; // Store the path the program followed
    int currentRow = startRow; // Sets current row position to starting point
    int currentCol = startCol; // setts current column position to starting point
    path.push_back({currentRow, currentCol}); // Added starting point to start of the path followed

    while (true) {

        // Store the Neighbors of the current position to a vector
        std::vector<std::pair<int, int>> neighbors = getNeighbors(currentRow, currentCol, rows, cols);

        // Set the next point to the lowest value (currentrow and col will be differnt after first itteration)
        std::pair<int, int> nextPoint = {currentRow, currentCol};

        // Reset the lowest nearby elevation to the current position
        float lowestElevation = map[currentRow][currentCol];

        // Itterate through all the neighbors that were found
        for (const auto& neighbor : neighbors) {

            // Set Row and col to neighbor to investigate
            int row = neighbor.first;
            int col = neighbor.second;

            // If that neighbor is lower then the current lowest point store that at the next point and lowest point untill a new one arives
            if (map[row][col] < lowestElevation) {
                lowestElevation = map[row][col];
                nextPoint = neighbor;
            }
        }

        // If the next point is still == the point it was before then you have already found the lowest point
        if (nextPoint.first == currentRow && nextPoint.second == currentCol) {
            break;
        }

        // Set the current values to the lowest point found and store that to the path
        currentRow = nextPoint.first;
        currentCol = nextPoint.second;
        path.push_back({currentRow, currentCol});
    }

    // Return the path to the main function
    return path;
}

// Write the results of the program
void show_results(std::string outputFolderPath, bool traceMode, int startRow, int startCol, std::vector<std::pair<int, int>> path,std::string fileName) {

    // Concat Output String
    std::ofstream outFile(outputFolderPath +"/" + fileName +".txt");

    // If you cant open output path exit
    if (!outFile.is_open()) {
        exit(11);
    }

    // Checks for trace mode on or off
    if (traceMode) {
        outFile << "TRACE" << std::endl;
    } else {
        outFile << "NO_TRACE" << std::endl;
    }

    // Write to the file the starting point, end point, and amount of jumps to neighbors
    outFile << (startRow + 1) << " " << (startCol + 1) << " " << (path.back().first + 1) << " " << (path.back().second + 1) << " " << path.size() << std::endl;

    // If traceMode is on itterate the path followed and write to the file
    if (traceMode) {
        for (const auto& point : path) {
            outFile << (point.first + 1) << " " << (point.second + 1) << " ";
        }
        outFile << std::endl;
    }
}

// Display the errors to the program output
void show_results(std::string outputFolderPath,std::string fileName, std::string Output_String) {
    // Concat Output String
    std::ofstream outFile(outputFolderPath + "/" + fileName + ".txt");

    // If you cant open output path exit
    if (!outFile.is_open()) {
        exit(11);
    }

    // Output the error
    outFile << Output_String;
}

// Get the map file name
std::string extractMapName(const std::string& mapFilePath) {
    std::string fileName;
    int start = 0; // Start of the file name
    int end = mapFilePath.size(); // End of the file name

    // Find the start of the file name by backtracking
    for (int i = mapFilePath.size() - 1; i >= 0; i--) {
        if (mapFilePath[i] == '/' || mapFilePath[i] == '\\') {
            start = i + 1;
            break;
        }
    }

    // Find the end of the file name
    for (int i = start; i < mapFilePath.size(); i++) {
        if (mapFilePath[i] == '.') {
            end = i;
            break;
        }
    }

    // Extract the file name
    for (int i = start; i < end; i++) {
        fileName += mapFilePath[i];
    }
    return fileName;
}
