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

void drawTravelers(void) {
	updateTravelers(moveCount, numSegmentGrowthMoves);

    for (unsigned int k = 0; k < sharedTravelers.size(); k++) {
        // Check if the traveler has been initialized (non-empty segmentList)
        if (!sharedTravelers[k].segmentList.empty()) {
            drawTraveler(sharedTravelers[k]);
        }
    }
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
	drawMessages(numMessages, message);
}

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

// Checks if a move for a traveler is valid within the grid and not blocked
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

// Checks that a traveler doesn't reverse direction
bool isValidDirectionChange(Direction currentDir, Direction newDir) {
	// Prevent reversal of direction
    if (currentDir == Direction::NORTH && newDir == Direction::SOUTH) return false;
    if (currentDir == Direction::SOUTH && newDir == Direction::NORTH) return false;
    if (currentDir == Direction::EAST && newDir == Direction::WEST) return false;
    if (currentDir == Direction::WEST && newDir == Direction::EAST) return false;
    return true;
}

// Updates the positions of all segments of a traveler
void updateSegmentPositions(Traveler& traveler) {
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
}

// Adds a new segment to the end of a traveler
void growSegment(Traveler& traveler) {
    if (!traveler.segmentList.empty()) {
        TravelerSegment& lastSegment = traveler.segmentList.back();

        // New segment takes the current position of the last segment as its previous position
        TravelerSegment newSegment = {lastSegment.row, lastSegment.col, lastSegment.dir, lastSegment.prevRow, lastSegment.prevCol};
        traveler.segmentList.push_back(newSegment);
    }
}



// Moves the head of a traveler in a random valid direction
void moveTravelerHead(Traveler& traveler) {
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
        Direction newDir = getRandomDirection(traveler.triedDirections);


        // Skip if this direction has been tried already from the current position
        if (traveler.triedDirections.find(newDir) != traveler.triedDirections.end()) {
            continue;
        }

        TravelerSegment &headSegment = traveler.segmentList.front();
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






// Updates the traveler moving them and growing segments as necessary
void updateTravelers(unsigned int& moveCount, unsigned int numSegmentGrowthMoves) {
    for (int i = 0; i < sharedTravelers.size(); i++) {
        if (sharedTravelers[i].segmentList.empty()) {
            continue; // Skip to the next traveler
        }

        moveTravelerHead(sharedTravelers[i]);
        
        if (sharedTravelers[i].segmentList.front().row == exitPos.row &&
            sharedTravelers[i].segmentList.front().col == exitPos.col) {
            sharedTravelers.erase(sharedTravelers.begin() + i);
            i--;
            numTravelersDone++;
            continue;
        }
        if (moveCount % numSegmentGrowthMoves == 0 && sharedTravelers[i].hasMoved) {
            growSegment(sharedTravelers[i]);
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
}

void* travelerThreadFunction(void* arg) {
    int id = *(int*)arg;
    delete (int*)arg;

    // Initialize and set up the traveler
    Traveler& traveler = sharedTravelers[id];

    // Initialize random generators for this thread
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<unsigned int> rowDistr(0, numRows - 1);
    std::uniform_int_distribution<unsigned int> colDistr(0, numCols - 1);
    std::uniform_int_distribution<unsigned int> dirDistr(0, 3); // For 4 directions
    std::uniform_int_distribution<unsigned int> segmentDistr(0, MAX_NUM_INITIAL_SEGMENTS);
    
    // Create the head of the traveler
    GridPosition startPos = getNewFreePosition(); // Use the function to get a free position
    Direction startDir = static_cast<Direction>(dirDistr(eng));
    TravelerSegment startSegment = {startPos.row, startPos.col, startDir};
    traveler.segmentList.push_back(startSegment);
    grid[startPos.row][startPos.col] = SquareType::TRAVELER;

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

    // Main loop for traveler movement
    while (true) {
        moveTravelerHead(traveler);

        // Check if segmentList is empty before accessing its elements
        if (traveler.segmentList.empty()) {
            // Handle the error or exit the thread
            break; // or return NULL;
        }

        // Check if the traveler has reached the exit
        if (traveler.segmentList.front().row == exitPos.row &&
            traveler.segmentList.front().col == exitPos.col) {
            break;
        }

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
    vector<pthread_t> threads(numTravelers);
    // Initialize sharedTravelers here
    for (int i = 0; i < numTravelers; ++i) {
        int *id = new int(i);
        if (pthread_create(&threads[i], NULL, travelerThreadFunction, id) != 0) {
            cerr << "Error creating thread " << i << endl;
        }
    }





	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that 
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
	
    // Join threads after GLUT main loop

    // Clean up
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
			if (	currentSeg.row < numRows-1 &&
					grid[currentSeg.row+1][currentSeg.col] == SquareType::FREE_SQUARE)
			{
				newSeg.row = currentSeg.row+1;
				newSeg.col = currentSeg.col;
				newSeg.dir = newDirection(Direction::SOUTH);
				grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case Direction::SOUTH:
			if (	currentSeg.row > 0 &&
					grid[currentSeg.row-1][currentSeg.col] == SquareType::FREE_SQUARE)
			{
				newSeg.row = currentSeg.row-1;
				newSeg.col = currentSeg.col;
				newSeg.dir = newDirection(Direction::NORTH);
				grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case Direction::WEST:
			if (	currentSeg.col < numCols-1 &&
					grid[currentSeg.row][currentSeg.col+1] == SquareType::FREE_SQUARE)
			{
				newSeg.row = currentSeg.row;
				newSeg.col = currentSeg.col+1;
				newSeg.dir = newDirection(Direction::EAST);
				grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
				canAdd = true;
			}
			//	no more segment
			else
				canAdd = false;
			break;

		case Direction::EAST:
			if (	currentSeg.col > 0 &&
					grid[currentSeg.row][currentSeg.col-1] == SquareType::FREE_SQUARE)
			{
				newSeg.row = currentSeg.row;
				newSeg.col = currentSeg.col-1;
				newSeg.dir = newDirection(Direction::WEST);
				grid[newSeg.row][newSeg.col] = SquareType::TRAVELER;
				canAdd = true;
			}
			//	no more segment
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

