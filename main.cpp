#include <iostream>
#include <bitset>
#include <fstream>
#include <cstdint>
#include <iomanip>
#include <unistd.h>
#include <cstring>
#include <math.h>

#include "PageTable.h"
#include "WSClock.h"
#include "vaddr_tracereader.h"
#include "log_helpers.h"

#define READ '0'
#define WRITE '1'

#define NORMAL_EXIT 0
#define BAD_EXIT 1

using namespace std;

enum LogMode {MODE_BITMASKS, MODE_VA2PA, MODE_VPNS_PFN,
    MODE_VPN2PFN_PR, MODE_OFFSET, MODE_SUMMARY};

#define MIN_ACCESSES 1
#define MIN_FRAMES 1
#define MIN_AGE 1
#define MIN_MANDATORY_ARGS 2
#define DEFAULT_AVAILABLE_FRAMES 999999
#define DEFAULT_AGE_RECENT_ACCESS 10
#define MIN_BITS 1
#define MAXIMUM_TOTAL_BITS 28
#define ADDRESS_LENGTH 32

unsigned int getOffset(PageTable* table, unsigned int address) {
    return table->getVPNFromVirtualAddress(address, table->levelCount);
}

//Handles logging for modes vpn2pfn_pr and va2pa
//Combined into one function as both logging modes print at the same time in execution
void logAddress(unsigned int vpn, int frame, int replaced, int shiftAmountToVPN, unsigned int offset, bool hit, LogMode mode) {
    if(mode == MODE_VPN2PFN_PR) {
        if(replaced == -1) {
            log_mapping(vpn >> shiftAmountToVPN, frame, replaced, hit);
        } else {
            log_mapping(vpn >> shiftAmountToVPN, frame, replaced >> shiftAmountToVPN, hit);
        }
    }
    if(mode == MODE_VA2PA) {
        log_va2pa(vpn, (frame << shiftAmountToVPN) + offset);
    }
}

int main(int argc, char** argv) {
    //default mode is summary
    LogMode log_mode = MODE_SUMMARY;
    int ageRecentAccess = DEFAULT_AGE_RECENT_ACCESS;
    int availableFrames = DEFAULT_AVAILABLE_FRAMES;
    //Default: -1, means process all addresses in file
    int numAddressesToProcess = -1;

    int option;

    while ((option = getopt(argc, argv, "n:f:a:l:")) != -1) {
        switch (option) {
            case 'n': {
                int num = atoi(optarg);
                if (num < MIN_ACCESSES) {
                    cout << "Number of memory accesses must be a number, greater than 0." << endl;
                    exit(BAD_EXIT);
                }
                numAddressesToProcess = num;

                break;
            }
            case 'f': {
                int num = atoi(optarg);
                if (num < MIN_FRAMES) {
                    cout << "Number of available frames must be a number, greater than 0." << endl;
                    exit(BAD_EXIT);
                }

                availableFrames = num;
                break;
            }
            case 'a': {
                int num = atoi(optarg);
                if (num < MIN_AGE) {
                    cout << "Age of last access considered recent must be a number, greater than 0." << endl;
                    exit(BAD_EXIT);
                }

                ageRecentAccess = num;
                break;
            }
            case 'l': {
                char* mode = optarg;
                if(strcmp(mode, "summary") == 0) {
                    log_mode = MODE_SUMMARY;
                    break;
                }
                if(strcmp(mode, "bitmasks") == 0) {
                    log_mode = MODE_BITMASKS;
                    break;
                }
                if(strcmp(mode, "va2pa") == 0) {
                    log_mode = MODE_VA2PA;
                    break;
                }
                if(strcmp(mode, "vpns_pfn") == 0) {
                    log_mode = MODE_VPNS_PFN;
                    break;
                }
                if(strcmp(mode, "vpn2pfn_pr") == 0) {
                    log_mode = MODE_VPN2PFN_PR;
                    break;
                }
                if(strcmp(mode, "offset") == 0) {
                    log_mode = MODE_OFFSET;
                    break;
                }
                if(strcmp(mode, "summary") == 0) {
                    log_mode = MODE_SUMMARY;
                    break;
                }

                cout << "Unknown Log Mode" << endl;
                exit(BAD_EXIT);
            }
            default:
                cout << "error parsing optional arguments" << endl;
                exit(BAD_EXIT);
        }
    }

    // optind should contain the index of the first mandatory argument
    if (argc - optind < MIN_MANDATORY_ARGS) {
        cout << "Not enough mandatory arguments" << endl;
        exit(BAD_EXIT);
    }

    char* Trace_File = argv[optind ++];
    char* Readswrites_File = argv[optind ++];

    WSClock clock(ageRecentAccess);

    int num_levels = argc - optind;
    int bits[num_levels];
    int totalPageTableBits = 0;
    int i = 0;

    if (optind >= argc){
        cout << "Level 0 page table must be at least 1 bit" << endl;
        exit(BAD_EXIT);
    }


    while (optind < argc) {
        int num_bits = atoi(argv[optind ++]);

        if (num_bits < MIN_BITS){
            cout << "Level " << i << " page table must be at least 1 bit" << endl;
            exit(BAD_EXIT);
        }

        bits[i] = num_bits;
        totalPageTableBits += num_bits;
        i++;

    }

    if(totalPageTableBits > MAXIMUM_TOTAL_BITS) {
        cout << "Too many bits used in page tables";
        exit(BAD_EXIT);
    }

    // check if trace file exists
    ifstream trace_stream(Trace_File);
    if (!trace_stream.good()) {
        cout << "Unable to open <<" << Trace_File << ">>" << endl;
        exit(BAD_EXIT);
    }
    trace_stream.close();

    // check if readwrites file exists
    ifstream readwrites_stream(Readswrites_File);
    if (!readwrites_stream.good()) {
        cout << "Unable to open <<" << Readswrites_File << ">>" << endl;
        exit(BAD_EXIT);
    }
    readwrites_stream.close();

    FILE* addressFile = fopen(Trace_File, "r");
    ifstream readWriteFile(Readswrites_File);
    p2AddrTr address;

    PageTable pageTable(bits, num_levels);

    if(log_mode == MODE_BITMASKS) {
        cout << "Bitmasks" << endl;
        for(int maskNumber = 0; maskNumber < num_levels; maskNumber++) {
            std::cout << "level " << maskNumber << " mask ";
            print_num_inHex(pageTable.bitMasks[maskNumber]);
        }

        exit(NORMAL_EXIT);
    }

    int currentFrame = 0;
    int misses = 0, totalProcessed = 0, pageReplacements = 0;

    int shiftAmountForAddressToVPN = ADDRESS_LENGTH - totalPageTableBits;

    char readWriteMode;
    //Iterate through the trace file until eof reached or number of processed lines (specified by cmd line argument) is reached
    while((numAddressesToProcess == -1 || totalProcessed < numAddressesToProcess) && NextAddress(addressFile, &address) != 0) {
        totalProcessed++;
        if(log_mode == MODE_OFFSET) {
            print_num_inHex(pageTable.getVPNFromVirtualAddress(address.addr, num_levels));
            continue;
        }

        readWriteFile.get(readWriteMode);

        //Find the frame this VPN is mapped to (if any)
        int frame = pageTable.findVPNtoPFNMapping(address.addr, log_mode == MODE_VPNS_PFN);

        if(frame == -1) {
            //VPN is not mapped to any frame
            misses++;

            if(currentFrame == availableFrames) {
                //No available frames, perform page replacement
                auto replacedPageInfo = clock.getFrameToBeReplaced(totalProcessed);
                int pageToBeReplaced = replacedPageInfo.first;
                unsigned int replacedAddress = replacedPageInfo.second;
                //Assign address to replaced page in the page table
                pageTable.insertVPNtoPFNMapping(address.addr, pageToBeReplaced, log_mode == MODE_VPNS_PFN);
                //Set the old address to -1 in the page table (invalid page)
                pageTable.insertVPNtoPFNMapping(replacedAddress, -1, false);
                //Update address and age info for the replaced page
                clock.updateFrame(pageToBeReplaced, address.addr, totalProcessed);

                logAddress(address.addr, pageToBeReplaced, replacedAddress, shiftAmountForAddressToVPN,
                           getOffset(&pageTable, address.addr), false, log_mode);

                pageReplacements++;
            } else {
                //Available frames exist, no need to perform page replacement
                pageTable.insertVPNtoPFNMapping(address.addr, currentFrame, log_mode == MODE_VPNS_PFN);
                //Add new frame to WSClock
                //If frame was added from a write instruction, dirty flag should be set to true
                clock.addFrame(address.addr, totalProcessed, readWriteMode == WRITE);

                logAddress(address.addr, currentFrame, -1, shiftAmountForAddressToVPN,
                           getOffset(&pageTable, address.addr), false, log_mode);
                currentFrame++;
            }
        } else {
            //Address is already mapped to a frame, update age of the frame
            if(readWriteMode == READ) {
                clock.updateFrame(frame, totalProcessed);
            } else {
                //Write mode, set dirty flag to true
                clock.setDirtyFlagForFrame(frame, totalProcessed, true);
            }
            logAddress(address.addr, frame, -1, shiftAmountForAddressToVPN, getOffset(&pageTable, address.addr), true,
                       log_mode);
        }
    }

    if(log_mode == MODE_SUMMARY) {
        log_summary(pow(2, ADDRESS_LENGTH - totalPageTableBits), pageReplacements, totalProcessed - misses, totalProcessed, currentFrame, pageTable.getBytesUsed());
    }

    readWriteFile.close();

    return NORMAL_EXIT;
}