/**
 * @file main.cpp
 * @brief Multithreaded Image Focusing Application for CSC 412.
 *
 * This version of the application, developed by Nicholas Faciano as of 12/3/23,
 * utilizes multithreading to process a stack of images and produce a single focused output image.
 * The program demonstrates advanced synchronization techniques and performance optimizations in a 
 * multithreaded environment. It employs pthreads for concurrent image processing, 
 * using multiple threads to process different regions of the image stack simultaneously.
 * This approach enhances the efficiency and speed of the image focusing process.
 *
 * Key features:
 * - Multithreaded processing of image stacks.
 * - Region-based locking for thread synchronization.
 * - Dynamic allocation of thread-specific data.
 * - Implementation of pthreads for concurrent processing.
 * - Utilization of atomic variables for thread-safe operations.
 * - Integration with a graphical interface using OpenGL/glut for visualization.
 *
 * @author Nicholas Faciano
 * @date 12/3/23
 */
#include <iostream>
#include <string>
#include <random>
#include <pthread.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <atomic>
#include "gl_frontEnd.h"
#include "ImageIO_TGA.h"

using namespace std;

/**
 * @brief Counter for the number of threads actively focusing parts of the image.
 */
std::atomic<unsigned int> numLiveFocusingThreads(0);

/**
 * @brief Maximum number of messages that can be displayed in the application's interface.
 */
const int MAX_NUM_MESSAGES = 8;

/**
 * @brief Maximum length of each message in the application's interface.
 */
const int MAX_LENGTH_MESSAGE = 32;

/**
 * @brief Array of messages for display in the application's interface.
 */
char** message;

/**
 * @brief Number of messages currently being used in the application's display.
 */
int numMessages;

/**
 * @brief Time at which the application was launched.
 */
time_t launchTime;

/**
 * @brief Pointer to the output image being composed in the application.
 */
RasterImage* imageOut;

/**
 * @brief Collection of input images to be processed.
 */
std::vector<RasterImage*> images;


/**
 * @brief Random device for generating random numbers.
 */
random_device myRandDev;

/**
 * @brief Random engine initialized with myRandDev, used for random number generation.
 */
default_random_engine myEngine(myRandDev());

/**
 * @brief Distribution for random color channel values.
 */
uniform_int_distribution<unsigned char> colorChannelDist;

/**
 * @brief Distribution for random row selection in images.
 */
uniform_int_distribution<unsigned int> rowDist;

/**
 * @brief Distribution for random column selection in images.
 */
uniform_int_distribution<unsigned int> colDist;

/**
 * @brief Path to the output image file.
 */
string outputPath;
/**
 * @brief Threads used for processing the image stack.
 * 
 * This vector stores POSIX thread identifiers for each thread created for image processing.
 */
std::vector<pthread_t> threads;


/**
 * @brief Global variable to store the number of threads used in the application.
 * 
 * This variable holds the number of threads that the application will create for processing
 * the image stack. It is set based on the command-line argument and used throughout the program
 * to manage thread operations.
 */

int numThreads;


/**
 * @brief Array of mutexes for the grid, used for region-based locking in image processing.
 */


pthread_mutex_t imageMutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Flag to control the ongoing operations of threads.
 */

std::atomic<bool> continue_going(true);  // Global variable


/**
 * @brief Displays the processed image.
 * 
 * @param scaleX Scaling factor in the x-direction.
 * @param scaleY Scaling factor in the y-direction.
 */
void displayImage(GLfloat scaleX, GLfloat scaleY)
{
	//==============================================
	//	This is OpenGL/glut magic.  Don't touch
	//==============================================
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPixelZoom(scaleX, scaleY);

	//--------------------------------------------------------
	//	stuff to replace or remove.
	//	Here, each time we render the image I assign a random
	//	color to a few random pixels
	//--------------------------------------------------------

	//==============================================
	//	This is OpenGL/glut magic.  Don't touch
	//==============================================
	glDrawPixels(imageOut->width, imageOut->height,
                  GL_RGBA, GL_UNSIGNED_BYTE, imageOut->raster);

}


/**
 * @brief Displays the state of the application.
 */


void displayState(void)
{
	//==============================================
	//	This is OpenGL/glut magic.  Don't touch
	//==============================================
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//--------------------------------------------------------

	time_t currentTime = time(NULL);
	numMessages = 4;
	sprintf(message[0], "System time: %ld", currentTime);
	sprintf(message[1], "Time since launch: %ld", currentTime-launchTime);
	sprintf(message[2], "I like cheese");
    sprintf(message[3], "Active threads: %u", numLiveFocusingThreads.load());

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//	You may have to synchronize this call if you run into
	//	problems, but really the OpenGL display is a hack for
	//	you to get a peek into what's happening.
	//---------------------------------------------------------
	drawState(numMessages, message);
}


/**
 * @brief Cleans up and exits the application.
 */

void cleanupAndQuit(void) {
    // Save the output image
    writeTGA(outputPath.c_str(), imageOut);

    // Free the message array
    for (int k = 0; k < MAX_NUM_MESSAGES; k++) {
        free(message[k]);
    }
    free(message);

    // Set the continue_going flag to false to signal all threads to terminate
    continue_going = false;

    // Join threads
    for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], NULL); // Using pthread_join to wait for thread completion
    }

    // Clean up images
    for (auto& img : images) {
        // Assuming img is dynamically allocated
        delete img;
    }
    images.clear();

   for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], NULL); // Using pthread_join
    }

    // No need to delete the threads vector

    pthread_mutex_destroy(&imageMutex);

    // Exit the program
    exit(0);
}

/**
 * @brief Handles keyboard events.
 *
 * This function is triggered by keyboard inputs in the GUI. It includes
 * functionalities like exiting the program.
 *
 * @param c The character pressed.
 * @param x The x-coordinate of the mouse.
 * @param y The y-coordinate of the mouse.
 */
void handleKeyboardEvent(unsigned char c, int x, int y)
{
	int ok = 0;
	
	switch (c)
	{
		//	'esc' to quit
		case 27:
			//	If you want to do some cleanup, here would be the time to do it.
			cleanupAndQuit();
			break;

		//	Feel free to add more keyboard input, but then please document that
		//	in the report.
		
		
		default:
			ok = 1;
			break;
	}
	if (!ok)
	{
		//	do something?
	}
}


/**
 * @brief Converts a pixel to grayscale.
 * 
 * @param image The image containing the pixel.
 * @param row The row of the pixel.
 * @param col The column of the pixel.
 * @return The grayscale value of the pixel.
 */
double convertToGrayscale(RasterImage* image, int row, int col) {
    if (image->type == RGBA32_RASTER) {
        unsigned char* pixel = static_cast<unsigned char*>(image->raster) + (row * image->width + col) * 4;
        // Using a weighted average for human perception: 0.21 R + 0.72 G + 0.07 B
        return 0.21 * pixel[0] + 0.72 * pixel[1] + 0.07 * pixel[2];
    }
    else if (image->type == GRAY_RASTER) {
        unsigned char* pixel = static_cast<unsigned char*>(image->raster) + (row * image->width + col);
        return *pixel;
    }
    return 0;
}

/**
 * @brief Calculates the contrast of a window around a pixel.
 * 
 * @param image The image containing the pixel.
 * @param row The row of the central pixel of the window.
 * @param col The column of the central pixel of the window.
 * @return The contrast value.
 */
double calculateWindowContrast(RasterImage* image, int row, int col, int windowSize){
    double minGray = 255.0, maxGray = 0.0;

    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            int neighborRow = row + i;
            int neighborCol = col + j;
            if (neighborRow >= 0 && neighborRow < image->height && neighborCol >= 0 && neighborCol < image->width) {
                double gray = convertToGrayscale(image, neighborRow, neighborCol);
                minGray = std::min(minGray, gray);
                maxGray = std::max(maxGray, gray);
            }
        }
    }

    return maxGray - minGray; // Contrast is the range of grayscale values
}

/**
/**
 * @brief Copies a pixel from one image to another.
 * 
 * @param srcImage Source image.
 * @param dstImage Destination image.
 * @param row Row of the pixel to copy.
 * @param col Column of the pixel to copy.
 */

void copyPixel(RasterImage* srcImage, RasterImage* dstImage, int row, int col) {
    unsigned char* srcPixel = static_cast<unsigned char*>(srcImage->raster) + (row * srcImage->width + col) * 4;
    unsigned char* dstPixel = static_cast<unsigned char*>(dstImage->raster) + (row * dstImage->width + col) * 4;

    memcpy(dstPixel, srcPixel, 4 * sizeof(unsigned char));  // Copy RGBA channels
}



struct ThreadData {
    std::vector<RasterImage*> images;
    RasterImage* outputImage;
};

void* processRegion(void* arg) {
    ThreadData* data = static_cast<ThreadData*>(arg);
    std::vector<RasterImage*>& imageStack = data->images;
    RasterImage* outputImage = data->outputImage;

    numLiveFocusingThreads++;

    std::default_random_engine generator(std::random_device{}());
    std::uniform_int_distribution<int> distributionRow(0, outputImage->height - 1);
    std::uniform_int_distribution<int> distributionCol(0, outputImage->width - 1);
    int windowSize = 11; // Or 13 as per your requirement

    while (continue_going) {
        int centerRow = distributionRow(generator);  // Random row
        int centerCol = distributionCol(generator);  // Random column

        double highestContrast = -1.0;
        RasterImage* bestImage = nullptr;

        // Calculate contrast and find best image
        for (auto& img : imageStack) {
            double contrast = calculateWindowContrast(img, centerRow, centerCol, windowSize);
            if (contrast > highestContrast) {
                highestContrast = contrast;
                bestImage = img;
            }
        }

        // Lock for writing to the output image
        if (bestImage) {
            pthread_mutex_lock(&imageMutex);
            // Write or blend pixels from the best image to the output image
            for (int i = -windowSize / 2; i <= windowSize / 2; ++i) {
                for (int j = -windowSize / 2; j <= windowSize / 2; ++j) {
                    unsigned int targetRow = centerRow + i;
                    unsigned int targetCol = centerCol + j;
                    if (targetRow >= 0 && targetRow < outputImage->height && targetCol >= 0 && targetCol < outputImage->width) {
                        unsigned char* dstPixel = static_cast<unsigned char*>(outputImage->raster) + (targetRow * outputImage->width + targetCol) * 4;
                        if (dstPixel[0] == 0 && dstPixel[1] == 0 && dstPixel[2] == 0) { // If pixel is black (0xFF000000)
                            copyPixel(bestImage, outputImage, targetRow, targetCol);
                        } else {
                            // Blend the colors (simple average)
                            unsigned char* srcPixel = static_cast<unsigned char*>(bestImage->raster) + (targetRow * bestImage->width + targetCol) * 4;
                            for (int k = 0; k < 3; ++k) { // For R, G, B channels
                                dstPixel[k] = static_cast<unsigned char>(0.5 * srcPixel[k] + 0.5 * dstPixel[k]);
                            }
                        }
                    }
                }
            }
            pthread_mutex_unlock(&imageMutex);
        }
    }

    numLiveFocusingThreads--;

    delete data; // Free the allocated memory for ThreadData
    return NULL;
}

/**
 * @brief Initializes the application with input images and output path.
 *
 * This function sets up the application by loading input images, allocating resources,
 * and setting the output path for the final processed image.
 *
 * @param inputPaths Vector containing paths to input images.
 * @param outPath Path where the output image will be saved.
 */
void initializeApplication(const vector<string>& inputPaths, const string& outPath)
{

	message = (char**) malloc(MAX_NUM_MESSAGES*sizeof(char*));
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		message[k] = (char*) malloc((MAX_LENGTH_MESSAGE+1)*sizeof(char));
	
	srand((unsigned int) time(NULL));

    // Load images
    for (const auto& path : inputPaths) {
        RasterImage* img = readTGA(path.c_str());
        if (img) {
            images.push_back(img);
        }
    }

     if (!images.empty()) {
        imageOut = new RasterImage(images[0]->width, images[0]->height, RGBA32_RASTER);
    }



    launchTime = time(NULL);
}


/**
 * @brief Main entry point for the Image Focusing application.
 *
 * Initializes the application, sets up image processing threads, and
 * starts the GLUT main loop for the graphical interface. This version
 * lays the groundwork for multithreaded image processing.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return Exit status of the program.
 */

int main(int argc, char** argv) {
    // Check if enough arguments are provided
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <numThreads> <outputPath> <inputPaths...>" << std::endl;
        return 1;
    }

    // Parse the number of threads
    numThreads = std::atoi(argv[1]);
    if (numThreads <= 0) {
        std::cerr << "Invalid number of threads: " << numThreads << std::endl;
        return 1;
    }

    // Store the output path
    outputPath = argv[2];

    // Store the input image paths
    std::vector<std::string> inputPaths;
    for (int i = 3; i < argc; ++i) {
        inputPaths.push_back(argv[i]);
    }


     // Initialize the application and load images
    initializeApplication(inputPaths, outputPath);

    // Initialize the front-end (GUI)
    initializeFrontEnd(argc, argv, imageOut);



    threads.resize(numThreads);
    for (int i = 0; i < numThreads; ++i) {
        ThreadData* data = new ThreadData{images, imageOut};
        pthread_create(&threads[i], NULL, processRegion, data);
    }

    glutMainLoop();


    return 0;
}