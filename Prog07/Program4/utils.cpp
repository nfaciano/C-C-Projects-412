//
//  utils.cpp
//  handout
//
//  Created by Jean-Yves Hervé on 2020-12-01.
//	Revised 2023-12-04

#include "dataTypes.h"

using namespace std;

std::string directionToString(Direction dir) {
    switch (dir) {
        case Direction::NORTH: return "NORTH";
        case Direction::SOUTH: return "SOUTH";
        case Direction::EAST: return "EAST";
        case Direction::WEST: return "WEST";
        default: return "UNKNOWN";
    }
}
string dirStr(const Direction& dir)
{
	string outStr;
	switch (dir)
	{
		case Direction::NORTH:
			outStr = "north";
			break;
		
		case Direction::WEST:
			outStr = "west";
			break;
		
		case Direction::SOUTH:
			outStr = "south";
			break;
		
		case Direction::EAST:
			outStr = "east";
			break;
		
		default:
			outStr = "";
			break;
	}

	return outStr;
}


string typeStr(const SquareType& type)
{
	string outStr;
	switch (type)
	{
		case SquareType::FREE_SQUARE:
			outStr = "free square";
			break;
		
		case SquareType::EXIT:
			outStr = "exit";
			break;
		
		case SquareType::WALL:
			outStr = "wall";
			break;
		
		case SquareType::VERTICAL_PARTITION:
			outStr = "vertical partition";
			break;
		
		case SquareType::HORIZONTAL_PARTITION:
			outStr = "horizontal partition";
			break;
		
		case SquareType::TRAVELER:
			outStr = "traveler";
			break;
		
		default:
			outStr = "";
			break;
	}

	return outStr;
}
