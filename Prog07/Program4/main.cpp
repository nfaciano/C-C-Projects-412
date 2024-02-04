//
//  main.c
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2020-12-01, rev. 2023-12-04
//
//	This is public domain code.  By all means appropriate it and change is to your
//	heart's content.

#include <iostream>
#include <string>
#include <random>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <pthread.h>
#include <vector>
#include <unistd.h> // Include for usleep
#include <algorithm>

//
#include "gl_frontEnd.h"

//	feel free to "un-use" std if this is against your beliefs.
using namespace std;

#if 0
//-----------------------------------------------------------------------------
#pragma mark -
#pragma mark Private Functions' Prototypes
//-----------------------------------------------------------------------------
#endif

void initializeApplication(void);
GridPosition getNewFreePosition(void);
Direction newDirection(Direction forbiddenDir = Direction::NUM_DIRECTIONS);
TravelerSegment newTravelerSegment(const TravelerSegment& currentSeg, bool& canAdd);
void generateWalls(void);
void generatePartitions(void);
void updateTravelers(unsigned int& moveCount, unsigned int numSegmentGrowthMoves);
void updateSegmentPositions(Traveler& traveler);
void removeInactiveTravelers();


#if 0
//-----------------------------------------------------------------------------
#pragma mark -
#pragma mark Application-level Global Variables
//-----------------------------------------------------------------------------
#endif

//	Don't rename any of these variables
//-------------------------------------
//	The state grid and its dimensions (arguments to the program)
SquareType** grid;
unsigned int numRows = 0;	//	height of the grid
unsigned int numCols = 0;	//	width
unsigned int numTravelers = 0;	//	initial number
unsigned int numTravelersDone = 0;
unsigned int numLiveThreads = 0;		//	the number of live traveler threads
vector<SlidingPartition> partitionList;
unsigned int numSegmentGrowthMoves;
unsigned int moveCount = 0;
const int MAX_ATTEMPTS = 4;
GridPosition	exitPos;	//	location of the exit

//	travelers' sleep time between moves (in microseconds)
const int MIN_SLEEP_TIME = 1000;
int travelerSleepTime = 100000;

//	An array of C-string where you can store things you want displayed
//	in the state pane to display (for debugging purposes?)
//	Dont change the dimensions as this may break the front end
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char** message;
time_t launchTime;

//	Random generators:  For uniform distributions
const unsigned int MAX_NUM_INITIAL_SEGMENTS = 6;
random_device randDev;
default_random_engine engine(randDev());
uniform_int_distribution<unsigned int> unsignedNumberGenerator(0, numeric_limits<unsigned int>::max());
uniform_int_distribution<unsigned int> segmentNumberGenerator(0, MAX_NUM_INITIAL_SEGMENTS);
uniform_int_distribution<unsigned int> segmentDirectionGenerator(0, static_cast<unsigned int>(Direction::NUM_DIRECTIONS)-1);
uniform_int_distribution<unsigned int> headsOrTails(0, 1);
uniform_int_distribution<unsigned int> rowGenerator;
uniform_int_distribution<unsigned int> colGenerator;

float** travelerColors = nullptr;

std::vector<Traveler> sharedTravelers;
pthread_mutex_t lock;


#if 0
//-----------------------------------------------------------------------------
#pragma mark -
#pragma mark Functions called by the front end
//-----------------------------------------------------------------------------
#endif
//============================================================= =====================
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================
void handleKeyboardEvent(unsigned char c, int x, int y)
{
	int ok = 0;

	switch (c)
	{
		//	'esc' to quit
		case 27:
			exit(0);
			break;

		//	slowdown
		case ',':
			slowdownTravelers();
			ok = 1;
			break;

		//	speedup
		case '.':
			speedupTravelers();
			ok = 1;
			break;

		default:
			ok = 1;
			break;
	}
	if (!ok)
	{
		//	do something?
	}
}


//------------------------------------------------------------------------
//	You shouldn't have to touch this one.  Definitely if you don't
//	add the "producer" threads, and probably not even if you do.
//------------------------------------------------------------------------
void speedupTravelers(void)
{
	//	decrease sleep time by 20%, but don't get too small
	int newSleepTime = (8 * travelerSleepTime) / 10;
	
	if (newSleepTime > MIN_SLEEP_TIME)
	{
		travelerSleepTime = newSleepTime;
	}
}

void slowdownTravelers(void)
{
	//	increase sleep time by 20%.  No upper limit on sleep time.
	//	We can slow everything down to admistrative pace if we want.
	travelerSleepTime = (12 * travelerSleepTime) / 10;
}

void drawTravelers(void) {
    updateTravelers(moveCount, numSegmentGrowthMoves);
    pthread_mutex_lock(&lock);
    for (unsigned int k = 0; k < sharedTravelers.size(); k++) {
        // Check if the traveler is active and has been initialized (non-empty segmentList)
        if (sharedTravelers[k].isActive && !sharedTravelers[k].segmentList.empty()) {
            drawTraveler(sharedTravelers[k]);
        }
    }
    pthread_mutex_unlock(&lock);
}



void updateMessages(void)
{

	//	Here I hard-code a few messages that I want to see displayed
	//	in my state pane.  The number of live robot threads will
	//	always get displayed.  No need to pass a message about it.
	unsigned int numMessages = 4;
	sprintf(message[0], "We created %d travelers", numTravelers);
	sprintf(message[1], "%d travelers solved the maze", numTravelersDone);
	sprintf(message[2], "I like cheese");
	sprintf(message[3], "Simulation run time: %ld s", time(NULL)-launchTime);
	
	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//
	//	You *must* synchronize this call.
	//---------------------------------------------------------
	pthread_mutex_lock(&lock);
	drawMessages(numMessages, message);
	pthread_mutex_unlock(&lock);

}


//==================================================================================
//
//	This is a function that you have to edit and add to.
//
//==================================================================================

Direction getRandomDirection(const std::set<Direction>& excludedDirections) {
    std::vector<Direction> possibleDirections = {Direction::NORTH, Direction::EAST, Direction::SOUTH, Direction::WEST};
    possibleDirections.erase(std::remove_if(possibleDirections.begin(), possibleDirections.end(), 
                    [&](const Direction& dir) { return excludedDirections.find(dir) != excludedDirections.end(); }), 
                    possibleDirections.end());

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<> dirGen(0, possibleDirections.size() - 1);

    return possibleDirections[dirGen(rng)];
}


// Randomly choose a direction for the partition to move

// You would then create two new functions to get random directions for vertical and horizontal partitions:

Direction getRandomVerticalDirection() {
    static std::random_device rd;
    static std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(0, 1); // for two directions: north and south

    return uni(rng) == 0 ? Direction::NORTH : Direction::SOUTH;
}

Direction getRandomHorizontalDirection() {
    static std::random_device rd;
    static std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(0, 1); // for two directions: east and west

    return uni(rng) == 0 ? Direction::EAST : Direction::WEST;
}


bool canMovePartition(const SlidingPartition& part, Direction dir, SquareType** grid, unsigned int numRows, unsigned int numCols, const vector<SlidingPartition>& partitions, const std::vector<Traveler>& sharedTravelers, GridPosition exitPos) {
    for (const auto& pos : part.blockList) {
        int newRow = pos.row;
        int newCol = pos.col;

        // Adjust movement based on direction
        switch (dir) {
            case Direction::NORTH: newRow--; break;
            case Direction::SOUTH: newRow++; break;
            case Direction::EAST: newCol++; break;
            case Direction::WEST: newCol--; break;
        }

        // Check boundaries
        if (newRow < 0 || newRow >= numRows || newCol < 0 || newCol >= numCols) {
            return false;
        }

        // Check if the new position is the exit
        if (newRow == exitPos.row && newCol == exitPos.col) {
            return false;
        }

        // Check for free space, other partitions, and travelers
        if (grid[newRow][newCol] != SquareType::FREE_SQUARE) {
            // Check against other partitions
            for (const auto& otherPart : partitions) {
                if (&otherPart == &part) continue;
                for (const auto& otherPos : otherPart.blockList) {
                    if (newRow == otherPos.row && newCol == otherPos.col) {
                        return false;
                    }
                }
            }

            // Check against travelers
            for (const auto& traveler : sharedTravelers) {
                if (!traveler.isActive) continue;
                for (const auto& segment : traveler.segmentList) {
                    if (newRow == segment.row && newCol == segment.col) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}




void movePartition(SlidingPartition& part, Direction dir, SquareType** grid, unsigned int numRows, unsigned int numCols, const vector<SlidingPartition>& partitions, const std::vector<Traveler>& sharedTravelers, GridPosition exitPos) {
    if (!canMovePartition(part, dir, grid, numRows, numCols, partitions, sharedTravelers, exitPos)) {
        // Debugging: Log that the partition cannot move
        return;
    }

    // Clear old position from grid
    for (const auto& pos : part.blockList) {
        grid[pos.row][pos.col] = SquareType::FREE_SQUARE;
    }

    // Update partition position
    for (auto& pos : part.blockList) {
        switch (dir) {
            case Direction::NORTH: pos.row--; break;
            case Direction::SOUTH: pos.row++; break;
            case Direction::EAST:  pos.col++; break;
            case Direction::WEST:  pos.col--; break;
        }
    }

    // Mark new position on grid
    for (const auto& pos : part.blockList) {
        grid[pos.row][pos.col] = part.isVertical ? SquareType::VERTICAL_PARTITION : SquareType::HORIZONTAL_PARTITION;
    }
}



void safeMovePartition(SlidingPartition& part, Direction dir, SquareType** grid, unsigned int numRows, unsigned int numCols, const vector<SlidingPartition>& partitions, const std::vector<Traveler>& sharedTravelers, GridPosition exitPos) {
    pthread_mutex_lock(&lock);
    movePartition(part, dir, grid, numRows, numCols, partitions, sharedTravelers, exitPos);
    pthread_mutex_unlock(&lock);
}




void updatePartitions(vector<SlidingPartition>& partitions, SquareType** grid, unsigned int numRows, unsigned int numCols, const std::vector<Traveler>& sharedTravelers, GridPosition exitPos) {
    for (auto& part : partitions) {
        Direction moveDir = part.isVertical ? getRandomVerticalDirection() : getRandomHorizontalDirection();
        safeMovePartition(part, moveDir, grid, numRows, numCols, partitions, sharedTravelers, exitPos);
    }
}






bool isValidMove(GridPosition pos, Direction dir) {
    // Check for grid boundaries
    if (pos.row >= numRows || pos.row < 0 || pos.col >= numCols || pos.col < 0) {
        return false;
    }

    // Check the type of square at the next position
    SquareType nextSquareType = grid[pos.row][pos.col];
    return nextSquareType != SquareType::WALL &&
           nextSquareType != SquareType::VERTICAL_PARTITION &&
           nextSquareType != SquareType::HORIZONTAL_PARTITION;
}


bool isValidDirectionChange(Direction currentDir, Direction newDir) {
    if (currentDir == Direction::NORTH && newDir == Direction::SOUTH) return false;
    if (currentDir == Direction::SOUTH && newDir == Direction::NORTH) return false;
    if (currentDir == Direction::EAST && newDir == Direction::WEST) return false;
    if (currentDir == Direction::WEST && newDir == Direction::EAST) return false;
    return true;
}


void updateSegmentPositions(Traveler& traveler) {
	pthread_mutex_lock(&lock);  // Use global lock or a specific lock for the traveler

    for (int i = traveler.segmentList.size() - 1; i > 0; i--) {
        // Update the previous position to the current position before moving
        traveler.segmentList[i].prevRow = traveler.segmentList[i].row;
        traveler.segmentList[i].prevCol = traveler.segmentList[i].col;

        // Move the segment to the position of the segment in front of it
        traveler.segmentList[i] = traveler.segmentList[i - 1];
    }

    if (!traveler.previousPositions.empty()) {
        GridPosition nextPos = traveler.previousPositions.front();
        traveler.previousPositions.pop_front();

        // Update the head segment
        if (traveler.segmentList.size() > 0) {
            traveler.segmentList[0].prevRow = traveler.segmentList[0].row;
            traveler.segmentList[0].prevCol = traveler.segmentList[0].col;
            traveler.segmentList[0].row = nextPos.row;
            traveler.segmentList[0].col = nextPos.col;
        }
    }
	pthread_mutex_unlock(&lock);

}


void growSegment(Traveler& traveler) {
	//Locked when updateTravelers is called
	pthread_mutex_lock(&lock);  // Use global lock or a specific lock for the traveler

    if (!traveler.segmentList.empty()) {
        TravelerSegment& lastSegment = traveler.segmentList.back();

        // New segment takes the current position of the last segment as its previous position
        TravelerSegment newSegment = {lastSegment.row, lastSegment.col, lastSegment.dir, lastSegment.prevRow, lastSegment.prevCol};
        traveler.segmentList.push_back(newSegment);
    }
	pthread_mutex_unlock(&lock);
}




void moveTravelerHead(Traveler& traveler) {
    // Locked by thread function

	// Do not process inactive travelers
    if (!traveler.isActive) {
        return;
    }

    bool hasMoved = false;
    Direction newDir;
    GridPosition nextPosition;
    if (traveler.segmentList.empty()) {
        // Handle the error or simply return to avoid further processing
        return;
    }

    // Attempt to move in a different direction if the last move was unsuccessful
    if (!traveler.hasMoved) {
        traveler.triedDirections.insert(traveler.segmentList.front().dir);
    }

    // Attempt to find a new direction that hasn't been tried yet
    while (!hasMoved && traveler.triedDirections.size() < static_cast<unsigned int>(Direction::NUM_DIRECTIONS)) {
        newDir = getRandomDirection(traveler.triedDirections);

        // Skip if this direction has been tried already from the current position
        if (traveler.triedDirections.find(newDir) != traveler.triedDirections.end()) {
            continue;
        }

        TravelerSegment& headSegment = traveler.segmentList.front();
        nextPosition = {headSegment.row, headSegment.col};

        // Calculate the new position based on the current direction
        switch (newDir) {
            case Direction::NORTH: nextPosition.row--; break;
            case Direction::SOUTH: nextPosition.row++; break;
            case Direction::EAST:  nextPosition.col++; break;
            case Direction::WEST:  nextPosition.col--; break;
        }

        // Check if the new direction and next position are valid
        if (isValidMove(nextPosition, newDir)) {
            // Check if the traveler has reached the exit
            if (nextPosition.row == exitPos.row && nextPosition.col == exitPos.col) {
				traveler.isActive = false;

				// Clear the traveler's segments from the grid
				for (const auto& segment : traveler.segmentList) {
					grid[segment.row][segment.col] = SquareType::FREE_SQUARE;
				}

				// Increment the number of travelers who have solved the maze
				pthread_mutex_lock(&lock); // Lock for modifying shared variable
				numTravelersDone++;
				pthread_mutex_unlock(&lock); // Unlock after modification

				// No further movement needed as traveler reached the exit
				break;
			}

            // Move the head and update the direction
            headSegment.row = nextPosition.row;
            headSegment.col = nextPosition.col;
            headSegment.dir = newDir;
            hasMoved = true;

            // Record the old head position and clear tried directions
            traveler.previousPositions.push_front({headSegment.row, headSegment.col});
            traveler.triedDirections.clear();
        } else {
            // Mark this direction as tried
            traveler.triedDirections.insert(newDir);
        }
    }

    // Only update segment positions if the head has moved
    if (hasMoved) {
        updateSegmentPositions(traveler);
        traveler.hasMoved = true;
    } else {
        traveler.hasMoved = false;
    }
}


void removeInactiveTravelers() {
    pthread_mutex_lock(&lock); // Lock the global mutex
    sharedTravelers.erase(
        std::remove_if(sharedTravelers.begin(), sharedTravelers.end(),
                       [](const Traveler& t) { return !t.isActive; }),
        sharedTravelers.end());
    pthread_mutex_unlock(&lock); // Unlock the global mutex
}




void updateTravelers(unsigned int& moveCount, unsigned int numSegmentGrowthMoves) {
    for (int i = 0; i < sharedTravelers.size(); i++) {
        // Skip inactive or empty travelers
        if (!sharedTravelers[i].isActive || sharedTravelers[i].segmentList.empty()) {
            continue;
        }

        pthread_mutex_lock(&sharedTravelers[i].lock); // Lock individual traveler
        moveTravelerHead(sharedTravelers[i]);
        pthread_mutex_unlock(&sharedTravelers[i].lock); // Unlock individual traveler

        // Check if the traveler reached the exit
        pthread_mutex_lock(&lock); // Lock for modifying sharedTravelers vector
        if (sharedTravelers[i].segmentList.front().row == exitPos.row &&
            sharedTravelers[i].segmentList.front().col == exitPos.col) {
            // Mark traveler as inactive instead of erasing
            sharedTravelers[i].isActive = false;
            numTravelersDone++;
            // No need to decrement i as we are not erasing the traveler
        }
        pthread_mutex_unlock(&lock); // Unlock after modifying the vector

        // Only grow segments for active travelers
        if (sharedTravelers[i].isActive && moveCount % numSegmentGrowthMoves == 0 && sharedTravelers[i].hasMoved) {
            pthread_mutex_lock(&sharedTravelers[i].lock); // Lock individual traveler
            growSegment(sharedTravelers[i]);
            pthread_mutex_unlock(&sharedTravelers[i].lock); // Unlock individual traveler
        }
    }
    moveCount++;
}






void initializeApplication(void) {
    // Initialize random generators
    rowGenerator = uniform_int_distribution<unsigned int>(0, numRows - 1);
    colGenerator = uniform_int_distribution<unsigned int>(0, numCols - 1);

    // Allocate the grid
    grid = new SquareType*[numRows];
    for (unsigned int i = 0; i < numRows; i++) {
        grid[i] = new SquareType[numCols];
        for (unsigned int j = 0; j < numCols; j++) {
            grid[i][j] = SquareType::FREE_SQUARE;
        }
    }

    // Initialize messages
    message = new char*[MAX_NUM_MESSAGES];
    for (unsigned int k = 0; k < MAX_NUM_MESSAGES; k++) {
        message[k] = new char[MAX_LENGTH_MESSAGE + 1];
    }

    // Initialize grid with walls and partitions
    generateWalls();
    generatePartitions();

    // Generate a random exit position
    exitPos = getNewFreePosition();
    grid[exitPos.row][exitPos.col] = SquareType::EXIT;
    // Initialize traveler colors (used by threads)
    travelerColors = createTravelerColors(numTravelers);
	travelerColors = createTravelerColors(numTravelers);

    sharedTravelers.resize(numTravelers);
    for (int i = 0; i < sharedTravelers.size(); i++) {
        pthread_mutex_init(&sharedTravelers[i].lock, NULL);
    }


	pthread_mutex_init(&lock, NULL);

}

void* travelerThreadFunction(void* arg) {
    int id = *(int*)arg;
    delete (int*)arg;
	//pthread_mutex_lock(&lock);
    // Initialize and set up the traveler
    Traveler& traveler = sharedTravelers[id];
    pthread_mutex_lock(&traveler.lock);


    // Initialize random generators for this thread
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<unsigned int> rowDistr(0, numRows - 1);
    std::uniform_int_distribution<unsigned int> colDistr(0, numCols - 1);
    std::uniform_int_distribution<unsigned int> dirDistr(0, 3); // For 4 directions
    std::uniform_int_distribution<unsigned int> segmentDistr(0, MAX_NUM_INITIAL_SEGMENTS);
    
    // Create the head of the traveler
    GridPosition startPos = {rowDistr(eng), colDistr(eng)};
    Direction startDir = static_cast<Direction>(dirDistr(eng));
    TravelerSegment startSegment = {startPos.row, startPos.col, startDir};
    traveler.segmentList.push_back(startSegment);
    grid[startPos.row][startPos.col] = SquareType::TRAVELER; // Mark the grid

    // Assign color to the traveler
    for (int c = 0; c < 4; c++) {
        traveler.rgba[c] = travelerColors[id][c];
    }

    // Add additional segments to the traveler
    unsigned int numAddSegments = segmentDistr(eng);
    TravelerSegment currSeg = startSegment;
    bool canAddSegment = true;
    for (unsigned int i = 0; i < numAddSegments && canAddSegment; i++) {
        TravelerSegment newSeg = newTravelerSegment(currSeg, canAddSegment);
        if (canAddSegment) {
            traveler.segmentList.push_back(newSeg);
            grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
            currSeg = newSeg;
        }
    }
    pthread_mutex_unlock(&traveler.lock); // Unlock after initialization


    // Main loop for traveler movement
    while (true) {
        pthread_mutex_lock(&traveler.lock); // Lock before modifying traveler's state
        moveTravelerHead(traveler);   

        // Check if segmentList is empty before accessing its elements
        if (traveler.segmentList.empty()) {
            // Handle the error or exit the thread

            break; // or return NULL;
        }

        
		pthread_mutex_unlock(&traveler.lock);
		updatePartitions(partitionList, grid, numRows, numCols, sharedTravelers,exitPos);


        usleep(1000000); // 100000 microseconds = 100 milliseconds
    }
    return NULL;
}






//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function besides
//	initialization of the various global variables and lists
//------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	//	We know that the arguments  of the program  are going
	//	to be the width (number of columns) and height (number of rows) of the
	//	grid, the number of travelers, etc.
	//	So far, I hard code-some values
	numRows = 30;
	numCols = 35;
	numTravelers = 8;
	numLiveThreads = 0;
	numTravelersDone = 0;
    numSegmentGrowthMoves = INT_MAX;  // Default value for segment growth


    if (argc >= 5) {
        numRows = atoi(argv[1]);
        numCols = atoi(argv[2]);
        numTravelers = atoi(argv[3]);
        numSegmentGrowthMoves = atoi(argv[4]);  // New argument for segment growth
    } else if (argc == 4) {
        numRows = atoi(argv[1]);
        numCols = atoi(argv[2]);
        numTravelers = atoi(argv[3]);
        // numSegmentGrowthMoves keeps the default value
    } else {
        cout << "Usage: " << argv[0] << " numRows numCols numTravelers [numSegmentGrowthMoves]" << endl;
        return 1;
    }

	//	Even though we extracted the relevant information from the argument
	//	list, I still need to pass argc and argv to the front-end init
	//	function because that function passes them to glutInit, the required call
	//	to the initialization of the glut library.
	initializeFrontEnd(argc, argv);
	
	//	Now we can do application-level initialization
	initializeApplication();

	launchTime = time(NULL);
	// Initialize traveler colors
	sharedTravelers.resize(numTravelers);

    // Create threads for each traveler
    // Create threads for each traveler
    vector<pthread_t> threads(numTravelers);
    for (int i = 0; i < numTravelers; ++i) {
        int *id = new int(i);
        if (pthread_create(&threads[i], NULL, travelerThreadFunction, id) != 0) {
            cerr << "Error creating thread " << i << endl;
        }
    }

    // Enter the main loop of the program
    glutMainLoop();
    
    // Wait for all threads to finish
    for (int i = 0; i < numTravelers; ++i) {
        pthread_join(threads[i], NULL);
    }

    // Remove inactive travelers
    removeInactiveTravelers();

    // Destroy individual traveler locks
    for (int i = 0; i < numTravelers; ++i) {
        pthread_mutex_destroy(&sharedTravelers[i].lock);
    }
    pthread_mutex_destroy(&lock);

    // Clean up dynamic memory allocations
    for (unsigned int i = 0; i < numRows; i++) {
        delete [] grid[i];
    }
    delete [] grid;
    for (int k = 0; k < MAX_NUM_MESSAGES; k++) {
        delete [] message[k];
    }
    delete [] message;
    for (unsigned int k = 0; k < numTravelers; k++) {
        delete [] travelerColors[k];
    }
    delete [] travelerColors;

    return 0;
}




//------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Generation Helper Functions
#endif
//------------------------------------------------------

GridPosition getNewFreePosition(void)
{
	GridPosition pos;

	bool noGoodPos = true;
	while (noGoodPos)
	{
		unsigned int row = rowGenerator(engine);
		unsigned int col = colGenerator(engine);
		if (grid[row][col] == SquareType::FREE_SQUARE)
		{
			pos.row = row;
			pos.col = col;
			noGoodPos = false;
		}
	}
	return pos;
}

Direction newDirection(Direction forbiddenDir)
{
	bool noDir = true;

	Direction dir = Direction::NUM_DIRECTIONS;
	while (noDir)
	{
		dir = static_cast<Direction>(segmentDirectionGenerator(engine));
		noDir = (dir==forbiddenDir);
	}
	return dir;
}


TravelerSegment newTravelerSegment(const TravelerSegment& currentSeg, bool& canAdd)
{
    TravelerSegment newSeg;
    switch (currentSeg.dir)
    {
        case Direction::NORTH:
            if (currentSeg.row < numRows - 1 &&
                grid[currentSeg.row + 1][currentSeg.col] == SquareType::FREE_SQUARE &&
                grid[currentSeg.row + 1][currentSeg.col] != SquareType::WALL)  // Check if not a wall
            {
                newSeg.row = currentSeg.row + 1;
                newSeg.col = currentSeg.col;
                newSeg.dir = newDirection(Direction::SOUTH);
                grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
                canAdd = true;
            }
            else
                canAdd = false;
            break;

        case Direction::SOUTH:
            if (currentSeg.row > 0 &&
                grid[currentSeg.row - 1][currentSeg.col] == SquareType::FREE_SQUARE &&
                grid[currentSeg.row - 1][currentSeg.col] != SquareType::WALL)  // Check if not a wall
            {
                newSeg.row = currentSeg.row - 1;
                newSeg.col = currentSeg.col;
                newSeg.dir = newDirection(Direction::NORTH);
                grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
                canAdd = true;
            }
            else
                canAdd = false;
            break;

        case Direction::WEST:
            if (currentSeg.col < numCols - 1 &&
                grid[currentSeg.row][currentSeg.col + 1] == SquareType::FREE_SQUARE &&
                grid[currentSeg.row][currentSeg.col + 1] != SquareType::WALL)  // Check if not a wall
            {
                newSeg.row = currentSeg.row;
                newSeg.col = currentSeg.col + 1;
                newSeg.dir = newDirection(Direction::EAST);
                grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
                canAdd = true;
            }
            else
                canAdd = false;
            break;

        case Direction::EAST:
            if (currentSeg.col > 0 &&
                grid[currentSeg.row][currentSeg.col - 1] == SquareType::FREE_SQUARE &&
                grid[currentSeg.row][currentSeg.col - 1] != SquareType::WALL)  // Check if not a wall
            {
                newSeg.row = currentSeg.row;
                newSeg.col = currentSeg.col - 1;
                newSeg.dir = newDirection(Direction::WEST);
                grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
                canAdd = true;
            }
            else
                canAdd = false;
            break;
        
        default:
            canAdd = false;
    }
    
    return newSeg;
}


void generateWalls(void)
{
	const unsigned int NUM_WALLS = (numCols+numRows)/4;

	//	I decide that a wall length  cannot be less than 3  and not more than
	//	1/4 the grid dimension in its Direction
	const unsigned int MIN_WALL_LENGTH = 3;
	const unsigned int MAX_HORIZ_WALL_LENGTH = numCols / 3;
	const unsigned int MAX_VERT_WALL_LENGTH = numRows / 3;
	const unsigned int MAX_NUM_TRIES = 20;

	bool goodWall = true;
	
	//	Generate the vertical walls
	for (unsigned int w=0; w< NUM_WALLS; w++)
	{
		goodWall = false;
		
		//	Case of a vertical wall
		if (headsOrTails(engine))
		{
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
			{
				//	let's be hopeful
				goodWall = true;
				
				//	select a column index
				unsigned int HSP = numCols/(NUM_WALLS/2+1);
				unsigned int col = (1+ unsignedNumberGenerator(engine)%(NUM_WALLS/2-1))*HSP;
				unsigned int length = MIN_WALL_LENGTH + unsignedNumberGenerator(engine)%(MAX_VERT_WALL_LENGTH-MIN_WALL_LENGTH+1);
				
				//	now a random start row
				unsigned int startRow = unsignedNumberGenerator(engine)%(numRows-length);
				for (unsigned int row=startRow, i=0; i<length && goodWall; i++, row++)
				{
					if (grid[row][col] != SquareType::FREE_SQUARE)
						goodWall = false;
				}
				
				//	if the wall first, add it to the grid
				if (goodWall)
				{
					for (unsigned int row=startRow, i=0; i<length && goodWall; i++, row++)
					{
						grid[row][col] = SquareType::WALL;
					}
				}
			}
		}
		// case of a horizontal wall
		else
		{
			goodWall = false;
			
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodWall; k++)
			{
				//	let's be hopeful
				goodWall = true;
				
				//	select a column index
				unsigned int VSP = numRows/(NUM_WALLS/2+1);
				unsigned int row = (1+ unsignedNumberGenerator(engine)%(NUM_WALLS/2-1))*VSP;
				unsigned int length = MIN_WALL_LENGTH + unsignedNumberGenerator(engine)%(MAX_HORIZ_WALL_LENGTH-MIN_WALL_LENGTH+1);
				
				//	now a random start row
				unsigned int startCol = unsignedNumberGenerator(engine)%(numCols-length);
				for (unsigned int col=startCol, i=0; i<length && goodWall; i++, col++)
				{
					if (grid[row][col] != SquareType::FREE_SQUARE)
						goodWall = false;
				}
				
				//	if the wall first, add it to the grid
				if (goodWall)
				{
					for (unsigned int col=startCol, i=0; i<length && goodWall; i++, col++)
					{
						grid[row][col] = SquareType::WALL;
					}
				}
			}
		}
	}
}


void generatePartitions(void)
{
	const unsigned int NUM_PARTS = (numCols+numRows)/4;

	//	I decide that a partition length  cannot be less than 3  and not more than
	//	1/4 the grid dimension in its Direction
	const unsigned int MIN_PARTITION_LENGTH = 3;
	const unsigned int MAX_HORIZ_PART_LENGTH = numCols / 3;
	const unsigned int MAX_VERT_PART_LENGTH = numRows / 3;
	const unsigned int MAX_NUM_TRIES = 20;

	bool goodPart = true;

	for (unsigned int w=0; w< NUM_PARTS; w++)
	{
		goodPart = false;
		
		//	Case of a vertical partition
		if (headsOrTails(engine))
		{
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
			{
				//	let's be hopeful
				goodPart = true;
				
				//	select a column index
				unsigned int HSP = numCols/(NUM_PARTS/2+1);
				unsigned int col = (1+ unsignedNumberGenerator(engine)%(NUM_PARTS/2-2))*HSP + HSP/2;
				unsigned int length = MIN_PARTITION_LENGTH + unsignedNumberGenerator(engine)%(MAX_VERT_PART_LENGTH-MIN_PARTITION_LENGTH+1);
				
				//	now a random start row
				unsigned int startRow = unsignedNumberGenerator(engine)%(numRows-length);
				for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
				{
					if (grid[row][col] != SquareType::FREE_SQUARE)
						goodPart = false;
				}
				
				//	if the partition is possible,
				if (goodPart)
				{
					//	add it to the grid and to the partition list
					SlidingPartition part;
					part.isVertical = true;
					for (unsigned int row=startRow, i=0; i<length && goodPart; i++, row++)
					{
						grid[row][col] = SquareType::VERTICAL_PARTITION;
						GridPosition pos = {row, col};
						part.blockList.push_back(pos);
					}
					partitionList.push_back(part); // Ensure this line is present

				}
			}
		}
		// case of a horizontal partition
		else
		{
			goodPart = false;
			
			//	I try a few times before giving up
			for (unsigned int k=0; k<MAX_NUM_TRIES && !goodPart; k++)
			{
				//	let's be hopeful
				goodPart = true;
				
				//	select a column index
				unsigned int VSP = numRows/(NUM_PARTS/2+1);
				unsigned int row = (1+ unsignedNumberGenerator(engine)%(NUM_PARTS/2-2))*VSP + VSP/2;
				unsigned int length = MIN_PARTITION_LENGTH + unsignedNumberGenerator(engine)%(MAX_HORIZ_PART_LENGTH-MIN_PARTITION_LENGTH+1);
				
				//	now a random start row
				unsigned int startCol = unsignedNumberGenerator(engine)%(numCols-length);
				for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
				{
					if (grid[row][col] != SquareType::FREE_SQUARE)
						goodPart = false;
				}
				
				//	if the wall first, add it to the grid and build SlidingPartition object
				if (goodPart)
				{
					SlidingPartition part;
					part.isVertical = false;
					for (unsigned int col=startCol, i=0; i<length && goodPart; i++, col++)
					{
						grid[row][col] = SquareType::HORIZONTAL_PARTITION;
						GridPosition pos = {row, col};
						part.blockList.push_back(pos);
					}
					partitionList.push_back(part);
				}
			}
		}
	}
}
