//
// Created by Abhiram on 10/22/2023.
//

#include <cmath>
#include <iostream>
#include <cstdint>

#include "PageNode.h"
#include "log_helpers.h"

void PageTable::generateBitMasks(const int bitsPerLevel[], int numOfLevels) {
    int totalBits = 0;
    for(int i = 0; i < numOfLevels; i++) {
        int bitsInCurrentLevel = bitsPerLevel[i];

        unsigned int mask = pow(2, bitsInCurrentLevel) - 1;
        mask <<= 32 - totalBits - bitsInCurrentLevel;
        bitMasks[i] = mask;
        bitShifts[i] = 32 - totalBits - bitsInCurrentLevel;
        entryCounts[i] = pow(2, bitsInCurrentLevel);

        totalBits += bitsInCurrentLevel;
    }
    bitMasks[numOfLevels] = pow(2, 32 - totalBits) - 1;
    bitShifts[numOfLevels] = 0;
}

unsigned int PageTable::getVPNFromVirtualAddress(unsigned int virtualAddress, int level) {
    unsigned int VPN = virtualAddress;
    VPN &= bitMasks[level];
    VPN >>= bitShifts[level];
    return VPN;
}
int PageTable::findVPNtoPFNMapping(unsigned int vpn, bool log) {
    PageNode* currentNode = root;
    auto* vpns = new std::uint32_t[levelCount];
    int frame;
    for(int i = 0; i < levelCount; i++) {
        unsigned int currentIndex = getVPNFromVirtualAddress(vpn, i);
        vpns[i] = currentIndex;

        if(i == levelCount - 1) {
            frame = currentNode->frameMappings->at(currentIndex);
        } else {
            if(currentNode->nextLevels->at(currentIndex) != nullptr) {
                currentNode = currentNode->nextLevels->at(currentIndex);
            } else {
                frame = -1;
                break;
            }
        }
    }

    if(frame != -1 && log) log_vpns_pfn(levelCount, vpns, frame);

    return frame;
}
//Returns -1 if mapping was previously invalid, returns 0 otherwise
int PageTable::insertVPNtoPFNMapping(unsigned int vpn, int frame, bool log) {
    PageNode* currentNode = root;
    auto* vpns = new std::uint32_t[levelCount];
    bool invalid = false;
    for(int i = 0; i < levelCount; i++) {
        unsigned int currentIndex = getVPNFromVirtualAddress(vpn, i);
        vpns[i] = currentIndex;

        if(i == levelCount - 1) {
            if(currentNode->frameMappings->at(currentIndex) == -1) invalid = true;
            currentNode->frameMappings->at(currentIndex) = frame;
        } else {
            if(currentNode->nextLevels->at(currentIndex) != nullptr) {
                currentNode = currentNode->nextLevels->at(currentIndex);
                invalid = true;
            } else {
                auto *nextNode = new PageNode(i + 1, this, i == levelCount - 2);
                currentNode->nextLevels->at(currentIndex) = nextNode;
                currentNode = nextNode;
            }
        }
    }

    if(log) log_vpns_pfn(levelCount, vpns, frame);

    if(invalid) return -1;
    else return 0;
}

int PageTable::getBytesUsedInNode(PageNode *node) {
    int bytesUsed = 0;
    if(node->nextLevels != nullptr) {
        for(int i = 0; i < node->nextLevels->size(); i++) {
            PageNode* elem = node->nextLevels->at(i);
            bytesUsed += sizeof(*elem);
            if(elem != nullptr) bytesUsed += getBytesUsedInNode(elem);
        }
    } else {
        for(int i = 0; i < node->frameMappings->size(); i++) {
            bytesUsed += sizeof(node->frameMappings->at(i));
        }
    }
    return bytesUsed;
}
int PageTable::getBytesUsed() {
    return getBytesUsedInNode(root);
}

PageTable::PageTable(int bitsPerLevel[], int numOfLevels) {
    levelCount = numOfLevels;
    bitMasks = new unsigned int[levelCount + 1];
    bitShifts = new int[levelCount + 1];
    entryCounts = new int[levelCount];

    generateBitMasks(bitsPerLevel, numOfLevels);

    root = new PageNode(0, this, numOfLevels == 1);
}

PageTable::~PageTable() {
    delete(bitMasks);
    delete(bitShifts);
    delete(entryCounts);
    delete(root);
}