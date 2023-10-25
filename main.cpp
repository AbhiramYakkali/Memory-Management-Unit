#include <iostream>
#include <bitset>
#include <fstream>
#include <cstdint>
#include <iomanip>
#include <unistd.h>

#include "PageTable.h"
#include "WSClock.h"
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
#define ADDRESS_LENGTH 32

int main(int argc, char** argv) {
    //default mode is summary
    int mode = MODE_SUMMARY;
    int ageRecentAccess = DEFAULT_AGE_RECENT_ACCESS;
    int availableFrames = DEFAULT_AVAILABLE_FRAMES;
    //Default: -1, means process all addresses in file
    int numAddressesToProcess = 50;
    //TODO: Hard coding for testing, remove later
    availableFrames = 20;
    ageRecentAccess = 5;

    WSClock clock(ageRecentAccess);

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

    //TODO: Values hard coded for testing, change later
    PageTable pageTable(new int[]{4, 6, 8}, 3);
    int totalPageTableBits = 18;

    int currentFrame = 0;
    int misses = 0, totalProcessed = 0;

    FILE* addressFile = fopen(argv[1], "r");
    std::ifstream readWriteFile(argv[2]);
    p2AddrTr address;
    int shiftAmountForAddressToVPN = ADDRESS_LENGTH - totalPageTableBits;

    char readWriteMode;
    //Iterate through the trace file until eof reached or number of processed lines (specified by cmd line argument) is reached
    while(totalProcessed < 50 && NextAddress(addressFile, &address) != EOF) {
        if(mode == MODE_OFFSET) {
            print_num_inHex(pageTable.getVPNFromVirtualAddress(address.addr, 3));
            continue;
        }

        readWriteFile.get(readWriteMode);

        //Find the frame this VPN is mapped to (if any)
        int frame = pageTable.findVPNtoPFNMapping(address.addr);

        if(frame == -1) {
            //VPN is not mapped to any frame
            misses++;

            if(currentFrame == availableFrames) {
                //No available frames, perform page replacement
                auto replacedPageInfo = clock.getFrameToBeReplaced(totalProcessed);
                int pageToBeReplaced = replacedPageInfo.first;
                //Assign address to replaced page in the page table
                pageTable.insertVPNtoPFNMapping(address.addr, pageToBeReplaced);
                //Set the old address to -1 in the page table (invalid page)
                pageTable.insertVPNtoPFNMapping(replacedPageInfo.second, -1);
                //Update address and age info for the replaced page
                clock.updateFrame(pageToBeReplaced, replacedPageInfo.second, totalProcessed);

                log_mapping(address.addr >> shiftAmountForAddressToVPN, pageToBeReplaced, replacedPageInfo.second >> shiftAmountForAddressToVPN, false);
            } else {
                //Available frames exist, no need to perform page replacement
                pageTable.insertVPNtoPFNMapping(address.addr, currentFrame);
                //Add new frame to WSClock
                //If frame was added from a write instruction, dirty flag should be set to true
                clock.addFrame(address.addr, totalProcessed, readWriteMode == WRITE);

                log_mapping(address.addr >> shiftAmountForAddressToVPN, currentFrame, -1, false);
                currentFrame++;
            }
        } else {
            //Address is already mapped to a frame, update age of the frame
            if(readWriteMode == READ) {
                clock.updateFrame(frame, totalProcessed + 1);
            } else {
                //Write mode, set dirty flag to true
                clock.setDirtyFlagForFrame(frame, totalProcessed + 1, true);
            }
            log_mapping(address.addr >> shiftAmountForAddressToVPN, frame, -1, true);
        }

        totalProcessed++;
    }

    readWriteFile.close();

    return NORMAL_EXIT;
}
