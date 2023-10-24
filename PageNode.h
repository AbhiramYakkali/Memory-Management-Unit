//
// Created by Abhiram on 10/22/2023.
//

#ifndef CS480_PROJECT_3_PAGENODE_H
#define CS480_PROJECT_3_PAGENODE_H

#include <vector>
#include "PageTable.h"

class PageNode {
public:
    PageNode(int level, PageTable* table, bool leaf);
    ~PageNode();

    std::vector<PageNode*>* nextLevels;
    std::vector<int>* frameMappings;
};


#endif //CS480_PROJECT_3_PAGENODE_H
