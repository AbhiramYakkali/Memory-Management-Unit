class PageNode;

class PageTable {
public:
    PageNode* root;

    int levelCount;
    unsigned int *bitMasks;
    int *bitShifts;
    int *entryCounts;

    PageTable(int bitsPerLevel[], int numOfLevels);
    ~PageTable();

    void generateBitMasks(const int bitsPerLevel[], int numOfLevel);
    unsigned int getVPNFromVirtualAddress(unsigned int virtualAddress, int level);
    int findVPNtoPFNMapping(unsigned int vpn);
    int insertVPNtoPFNMapping(unsigned int vpn, int frame);
};
