#include "ScoreFile.hpp"

#include <fstream>
#include <list>

int ScoreFile::createBlank() {
    rootId = 0;
    rootFrequency = 261.625565301;
    taktDurationInSeconds = 1.;
    
    nodes.clear();
    nodes.emplace_back();
    Node& n = nodes.back();
    n.id = 0;
    n.parentId = Node::NULL_ID;
    n.ratioFromParent = discrete::Monzo(1);
    n.positionInTaktsFromParent = 0;
    n.durationInTakts = 1;
    return 0;
}
int ScoreFile::readFromDisk(const char* path) {
    std::ifstream f(path);
    if (!f) return 1;
    nlohmann::json data = nlohmann::json::parse(f);
    rootId = data["rootId"];
    rootFrequency = data["rootFrequency"];
    taktDurationInSeconds = data["taktDurationInSeconds"];
    nodes.clear();
    nodes.reserve(data["score"].size());
    for (int i = 0; i < data["score"].size(); ++i) {
        nlohmann::json nodeData = data["score"][i];
        nodes.emplace_back();
        Node& n = nodes.back();
        n.id = nodeData["id"];
        n.parentId = nodeData["parentId"];
        n.ratioFromParent = discrete::Monzo((int)nodeData["ratioFromParent"][0]) / discrete::Monzo((int)nodeData["ratioFromParent"][1]);
        n.positionInTaktsFromParent = nodeData["positionInTaktsFromParent"];
        n.durationInTakts = nodeData["durationInTakts"];
    }
    f.close();
    return 0;
}

int ScoreFile::writeToDisk(const char* path) const {
    nlohmann::json data;
    data["rootId"] = rootId;
    data["rootFrequency"] = rootFrequency;
    data["taktDurationInSeconds"] = taktDurationInSeconds;
    data["score"] = {};
    for (int i = 0; i < nodes.size(); ++i) {
        const auto& n = nodes[i];
        auto& d = data["score"][i];
        d["id"] = n.id;
        d["parentId"] = n.parentId;
        d["ratioFromParent"] = {(int)n.ratioFromParent.numerator(), (int)n.ratioFromParent.denominator()};
        d["positionInTaktsFromParent"] = n.positionInTaktsFromParent;
        d["durationInTakts"] = n.durationInTakts;
    }
    std::ofstream f(path);
    if (!f) return 1;
    f << data << std::endl;
    f.close();
    return 0;
}

int ScoreFile::getNumberOfNodes() const { return nodes.size(); }
int ScoreFile::getRootId() const { return rootId; }
double ScoreFile::getRootFrequency() const { return rootFrequency; }
double ScoreFile::getTaktDurationInSeconds() const { return taktDurationInSeconds; }
int ScoreFile::getParentId(int id) const {
    if (id >= nodes.size()) throw "err"; /////////////////////////////////////////////
    return nodes[id].parentId;
}
discrete::Monzo ScoreFile::getRatioFromParent(int id) const {
    if (id >= nodes.size()) throw "err"; /////////////////////////////////////////////
    return nodes[id].ratioFromParent;
}
int ScoreFile::getPositionInTaktsFromParent(int id) const {
    if (id >= nodes.size()) throw "err"; /////////////////////////////////////////////
    return nodes[id].positionInTaktsFromParent;
}
int ScoreFile::getDurationInTakts(int id) const {
    if (id >= nodes.size()) throw "err"; /////////////////////////////////////////////
    return nodes[id].durationInTakts;
}
discrete::Monzo ScoreFile::getRelativeRatio(int fromId, int toId) const {
    if (fromId >= nodes.size() || toId >= nodes.size()) throw "err"; ///////////////////////////////////////////
    discrete::Monzo m(1);
    for (int i = toId; i != rootId; i = nodes[i].parentId) {
        m *= nodes[i].ratioFromParent;
    }
    for (int i = fromId; i != rootId; i = nodes[i].parentId) {
        m /= nodes[i].ratioFromParent;
    }
    return m;
}    
int ScoreFile::getRelativePositionInTakts(int fromId, int toId) const {
    if (fromId >= nodes.size() || toId >= nodes.size()) throw "err"; ///////////////////////////////////////////
    int dur = 0;
    for (int i = toId; i != rootId; i = nodes[i].parentId) {
        dur += nodes[i].positionInTaktsFromParent;
    }
    for (int i = fromId; i != rootId; i = nodes[i].parentId) {
        dur -= nodes[i].positionInTaktsFromParent;
    }
    return dur;
}
double ScoreFile::getFrequency(int id) const {
    return rootFrequency * (double)getRelativeRatio(rootId, id);
}

const ScoreFile::Node& ScoreFile::getNode(int id) const { 
    if (id >= nodes.size()) throw "err"; /////////////////////////////////////////////
    return nodes[id];
}

// // MODIFY
int ScoreFile::changeRoot(int newRootId) {
    if (newRootId >= nodes.size()) return 1;
    if (newRootId == rootId) return 0;

    nodes[rootId].parentId = newRootId;
    nodes[rootId].ratioFromParent = getRelativeRatio(newRootId, rootId);
    nodes[rootId].positionInTaktsFromParent = getRelativePositionInTakts(newRootId, rootId);
    
    nodes[newRootId].parentId = Node::NULL_ID;
    nodes[newRootId].ratioFromParent = discrete::Monzo(1);
    nodes[newRootId].positionInTaktsFromParent = 0;
    
    rootFrequency *= (double)(discrete::Monzo(1)/nodes[rootId].ratioFromParent);
    rootId = newRootId;
    return 0;
}
int ScoreFile::changeRootFrequency(double newRootFrequency) {
    rootFrequency = newRootFrequency;
    return 0;
}
int ScoreFile::changeTaktDuration(double newTaktDuration) {
    taktDurationInSeconds = newTaktDuration;
    return 0;
}
int ScoreFile::changeParent(int id, int newParentId) {
    if (id >= nodes.size() || newParentId >= nodes.size()) return 1;
    if (id == rootId) return 1;
    if (id == newParentId) return 1;
    if (newParentId == nodes[id].parentId) return 0;
    
    // // abort if the change would form a loop
    for (int i = newParentId; i != rootId; i = nodes[i].parentId) {
        if (i == id) return 1;
    }
    
    nodes[id].ratioFromParent = getRelativeRatio(newParentId, id);
    nodes[id].positionInTaktsFromParent = getRelativePositionInTakts(newParentId, id);
    nodes[id].parentId = newParentId;
    return 0;
}
int ScoreFile::changeRatio(int id, discrete::Monzo newRatio) {
    if (id >= nodes.size()) return 1;
    nodes[id].ratioFromParent = newRatio;
    return 0;
}
int ScoreFile::incrementPositionInTaktsFromParent(int id, int incrementInTakts) {
    if (id >= nodes.size()) return 1;
    for (Node& n : nodes) {
        if (n.parentId == id) n.positionInTaktsFromParent -= incrementInTakts;
    }
    if (id != rootId) nodes[id].positionInTaktsFromParent += incrementInTakts;
    return 0;
}
int ScoreFile::changeNodeDuration(int id, int newDuration) {
    if (id >= nodes.size()) return 1;
    nodes[id].durationInTakts = newDuration;
    return 0;
}

// // CREATE/DELETE
int ScoreFile::createNode(int parentId, discrete::Monzo ratioFromParent, int positionInTaktsFromParent, int durationInTakts) {
    if (parentId >= nodes.size()) return 1;
    int id = nodes.size();
    nodes.emplace_back();
    Node& n = nodes.back();
    n.id = id;
    n.parentId = parentId;
    n.ratioFromParent = ratioFromParent;
    n.positionInTaktsFromParent = positionInTaktsFromParent;
    n.durationInTakts = durationInTakts;
    return 0;
}
int ScoreFile::deleteNode(int id) {
    if (id >= nodes.size()) return 1;
    if (id == rootId) return 1;
    int newParentId = nodes[id].parentId;
    for (Node& n : nodes) {
        if (n.parentId == id) changeParent(n.id, newParentId);
    }
    
    int backId = nodes.back().id;
    for (Node& n : nodes) {
        if (n.parentId == backId) n.parentId = id;
    }
    if (rootId == backId) rootId = id;
    nodes[backId].id = id;
    nodes[id] = nodes.back();
    nodes.pop_back();
    return 0;
}

std::vector<int> ScoreFile::getOrderedNodeIds() const {
    std::vector<int> result;
    result.reserve(nodes.size());
    std::list<int> oldQue, newQue;
    oldQue.push_back(rootId);
    while (!oldQue.empty()) {
        for (int i : oldQue) result.push_back(i);
        for (int j = 0; j < nodes.size(); ++j) {
            for (int i : oldQue) {
                if (nodes[j].parentId == i) {
                    newQue.push_back(j);
                    break;
                }
            }
        }
        oldQue = newQue;
        newQue.clear();
    }
    return result;
}