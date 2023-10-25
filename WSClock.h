#include <vector>

//Keeps track of each available frame
//Stores the address corresponding to the frame, its age of last access, and the dirty flag
typedef struct {
    unsigned int address;
    int ageOfLastAccess;
    bool dirty;
} Frame;

class WSClock {
private:
    int currentIndex;
    int ageConsideredRecent;
    std::vector<Frame>* frames;
public:
    WSClock(int recentAge);
    ~WSClock();

    void updateFrame(int frameNumber, int age);
    void updateFrame(int frameNumber, unsigned int address, int age);
    void setDirtyFlagForFrame(int frameNumber, int age, bool dirty);
    void addFrame(unsigned int address, int age, bool dirty);
    std::pair<int, int> getFrameToBeReplaced(int currentAge);
};
