//
// Created by Abhiram on 10/22/2023.
//

#include "PageNode.h"

#include <vector>

PageNode::PageNode(int level, PageTable *table, bool leaf) {
    int entryCount = table->entryCounts[level];
    if(leaf) {
        frameMappings = new std::vector<int>(entryCount, -1);
        nextLevels = nullptr;
        table->byteCount += sizeof(frameMappings->at(0)) * entryCount;
    } else {
        nextLevels = new std::vector<PageNode*>(entryCount, nullptr);
        frameMappings = nullptr;
        table->byteCount += sizeof(this) * entryCount;
    }
}

PageNode::~PageNode() {
    delete(frameMappings);
    delete(nextLevels);
}
