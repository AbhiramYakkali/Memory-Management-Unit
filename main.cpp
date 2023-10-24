#include <iostream>
#include <bitset>
#include <fstream>
#include <cstdint>
#include <iomanip>
#include <unistd.h>

#include "PageTable.h"
#include "vaddr_tracereader.h"
#include "log_helpers.h"

#define NORMAL_EXIT 0
#define BAD_ARGUMENT 1

#define READ '0'
#define WRITE '1'

#define MODE_BITMASKS 0
#define MODE_VA2PA 1
#define MODE_VPNS_PFN 2
#define MODE_VPN2PFN_PR 3
#define MODE_OFFSET 4
#define MODE_SUMMARY 5

#define DEFAULT_AVAILABLE_FRAMES 999999
#define DEFAULT_AGE_RECENT_ACCESS 10
#define MAXIMUM_TOTAL_BITS 28

int main(int argc, char** argv) {
    //default mode is summary
    int mode = MODE_SUMMARY;
    int ageRecentAccess = DEFAULT_AGE_RECENT_ACCESS;
    int availableFrames = DEFAULT_AVAILABLE_FRAMES;
    //Default: -1, means process all addresses in file
    int numAddressesToProcess = -1;

    //Process options
    int option;
    while((option = getopt(argc, argv, "n:f:a:l:")) != -1) {
        switch(option) {
            case 'n': {
                int num = atoi(optarg);
                numAddressesToProcess = num;
                break;
            }
            case 'f': {
                int num = atoi(optarg);
                availableFrames = num;
                break;
            }
            case 'a': {
                int num = atoi(optarg);
                ageRecentAccess = num;
                break;
            }
            case 'l': {
                if(strcmp(optarg, "bitmasks") == 0) mode = MODE_BITMASKS;
                else if(strcmp(optarg, "va2pa") == 0) mode = MODE_VA2PA;
                else if(strcmp(optarg, "vpns_pfn") == 0) mode = MODE_VPNS_PFN;
                else if(strcmp(optarg, "vpn2pfn_pr") == 0) mode = MODE_VPN2PFN_PR;
                else if(strcmp(optarg, "offset") == 0) mode = MODE_OFFSET;
                else if(strcmp(optarg, "summary") == 0) mode = MODE_SUMMARY;
                else {
                    std::cout << "Argument: " << optarg << "is not a valid mode for this program." << std::endl;
                    exit(BAD_ARGUMENT);
                }
                break;
            }
            default: {
                std::cout << "Error parsing arguments, invalid arguments found." << std::endl;
                exit(BAD_ARGUMENT);
            }
        }
    }

    PageTable pageTable(new int[]{4, 6, 8}, 3);
    int currentFrame = 0;
    int misses = 0, totalProcessed = 0;

    FILE* addressFile = fopen(argv[1], "r");
    std::ifstream readWriteFile(argv[2]);
    p2AddrTr address;

    char readWriteMode;
    while(totalProcessed < 50 && NextAddress(addressFile, &address) != EOF) {
        //std::cout << std::hex << std::setw(8) << std::setfill('0') << address.addr << std::endl;
        //std::cout << std::hex << std::setw(8) << std::setfill('0') << pageTable.getVPNFromVirtualAddress(address.addr, 3) << std::endl;
        if(mode == MODE_OFFSET) {
            print_num_inHex(pageTable.getVPNFromVirtualAddress(address.addr, 3));
            continue;
        }
        totalProcessed++;
        readWriteFile.get(readWriteMode);

        if(readWriteMode == READ) {
            //std::cout << "read ";
            int frame = pageTable.findVPNtoPFNMapping(address.addr);
            if(frame == -1) {
                misses++;
                pageTable.insertVPNtoPFNMapping(address.addr, currentFrame);
                currentFrame++;
            }
        } else {
            //std::cout << "write ";
            int frame = pageTable.findVPNtoPFNMapping(address.addr);

            if(frame == -1) {
                pageTable.insertVPNtoPFNMapping(address.addr, currentFrame);
                currentFrame++;
            }
        }

        //std::cout << "total: " << totalProcessed << " misses: " << misses << std::endl;
    }

    readWriteFile.close();

    return 0;
}
