#!/bin/bash

# Detect the platform
PLATFORM=$(uname)

# Define build commands for Linux and macOS
if [ "$PLATFORM" = "Linux" ]; then
    # Compile pthread versions
    g++ -Wall -std=c++20 ./../Programs/pthread/Version1/*.cpp -lGL -lglut -lpthread -o ./../focus_pthread_v1
    g++ -Wall -std=c++20 ./../Programs/pthread/Version2/*.cpp -lGL -lglut -lpthread -o ./../focus_pthread_v2
    g++ -Wall -std=c++20 ./../Programs/pthread/Version3/*.cpp -lGL -lglut -lpthread -o ./../focus_pthread_v3

    # Compile C++_thread versions
    g++ -Wall -std=c++20 ./../Programs/C++_thread/Version1/*.cpp -lGL -lglut -lpthread -o ./../focus_cpp_thread_v1
    g++ -Wall -std=c++20 ./../Programs/C++_thread/Version2/*.cpp -lGL -lglut -lpthread -o ./../focus_cpp_thread_v2
    g++ -Wall -std=c++20 ./../Programs/C++_thread/Version3/*.cpp -lGL -lglut -lpthread -o ./../focus_cpp_thread_v3
elif [ "$PLATFORM" = "Darwin" ]; then
    # macOS commands using clang++ and assuming GLUT is available
    # Compile pthread versions
    clang++ -Wall -Wno-deprecated -std=c++20 ./../Programs/pthread/Version1/*.cpp -framework OpenGL -framework GLUT -o ./../focus_pthread_v1
    clang++ -Wall -Wno-deprecated -std=c++20 ./../Programs/pthread/Version2/*.cpp -framework OpenGL -framework GLUT -o ./../focus_pthread_v2
    clang++ -Wall -Wno-deprecated -std=c++20 ./../Programs/pthread/Version3/*.cpp -framework OpenGL -framework GLUT -o ./../focus_pthread_v3

    # Compile C++_thread versions
    clang++ -Wall -Wno-deprecated -std=c++20 ./../Programs/C++_thread/Version1/*.cpp -framework OpenGL -framework GLUT -o ./../focus_cpp_thread_v1
    clang++ -Wall -Wno-deprecated -std=c++20 ./../Programs/C++_thread/Version2/*.cpp -framework OpenGL -framework GLUT -o ./../focus_cpp_thread_v2
    clang++ -Wall -Wno-deprecated -std=c++20 ./../Programs/C++_thread/Version3/*.cpp -framework OpenGL -framework GLUT -o ./../focus_cpp_thread_v3
else
    echo "Unsupported platform"
    exit 1
fi

echo "Build complete"

