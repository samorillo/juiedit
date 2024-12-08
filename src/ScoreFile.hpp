#ifndef SCORE_FILE_H
#define SCORE_FILE_H

#include <vector>

#include "../external/json/single_include/nlohmann/json.hpp"
#include "../external/discrete/primes.hpp"

struct ScoreFile {
    struct Node {
        inline static constexpr int NULL_ID = -1;
        int id;
        int parentId;
        discrete::Monzo ratioFromParent;
        int positionInTaktsFromParent;
        int durationInTakts;
    };
    
    int createBlank();
    int readFromDisk(const char* path);
    int writeToDisk(const char* path) const;
    
    // // GET
    int getNumberOfNodes() const;
    int getRootId() const;
    double getRootFrequency() const;
    double getTaktDurationInSeconds() const;
    int getParentId(int id) const;
    discrete::Monzo getRatioFromParent(int id) const;
    int getPositionInTaktsFromParent(int id) const;
    int getDurationInTakts(int id) const;
    discrete::Monzo getRelativeRatio(int fromId, int toId) const;
    int getRelativePositionInTakts(int fromId, int toId) const;
    double getFrequency(int id) const;
    const ScoreFile::Node& getNode(int id) const;
    
    // // MODIFY
    int changeRoot(int newRootId);
    int changeRootFrequency(double newRootFrequency);
    int changeTaktDuration(double newTaktDuration);
    int changeParent(int id, int newParentId);
    int changeRatio(int id, discrete::Monzo newRatio);
    int incrementPositionInTaktsFromParent(int id, int newPosition);
    int changeNodeDuration(int id, int newDuration);
    
    // // CREATE/DELETE
    int createNode(int parentId, discrete::Monzo ratioFromParent, int positionInTaktsFromParent, int durationInTakts);
    int deleteNode(int id);
    
    std::vector<int> getOrderedNodeIds() const;
    
private:
    int rootId;
    double rootFrequency;
    double taktDurationInSeconds;
    std::vector<Node> nodes;
};

#endif /* end of include guard: SCORE_FILE_H */
