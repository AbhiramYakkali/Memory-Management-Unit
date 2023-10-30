//
// Created by Abhiram on 10/24/2023.
//

#include <iostream>

#include "WSClock.h"

//Add new frame to the vector (used when there are available frames)
void WSClock::addFrame(unsigned int address, int age, bool dirty) {
    frames->push_back({address, age, dirty});
}
//Update age of an existing frame (used when frame is read)
void WSClock::updateFrame(int frameNumber, int age) {
    frames->at(frameNumber).ageOfLastAccess = age;
}
//Update age and address of an existing frame (used for page replacement)
void WSClock::updateFrame(int frameNumber, unsigned int address, int age) {
    frames->at(frameNumber).address = address;
    frames->at(frameNumber).ageOfLastAccess = age;
}
//Update age and dirty flag of an existing frame (used when frame is written to)
void WSClock::setDirtyFlagForFrame(int frameNumber, int age, bool dirty) {
    frames->at(frameNumber).dirty = dirty;
    frames->at(frameNumber).ageOfLastAccess = age;
}

//Returns a pair of ints: <page to be replaced, address corresponding to page to be replaced>
std::pair<int, int> WSClock::getFrameToBeReplaced(int currentAge) {
    //Iterates through the frames in the vector until a frame is found that meets conditions
    while(true) {
        Frame* currentFrame = &frames->at(currentIndex);

        //Check if frame was accessed recently
        if(currentAge - currentFrame->ageOfLastAccess > ageConsideredRecent) {
            if(!currentFrame->dirty) {
                //This frame meets conditions, return
                return std::make_pair(currentIndex, currentFrame->address);
            } else {
                //Set dirty flag to false to simulate writing to disk
                currentFrame->dirty = false;
            }
        }

        currentIndex++;
        if(currentIndex == frames->size()) currentIndex = 0;
    }
}

WSClock::WSClock(int recentAge) {
    frames = new std::vector<Frame>();
    ageConsideredRecent = recentAge;
    currentIndex = 0;
}

WSClock::~WSClock() {
    delete(frames);
}