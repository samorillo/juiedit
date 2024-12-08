#include "ScoreFile.hpp"
#include "ScoreEditor.hpp"
#include "ScorePlayer.hpp"

#include <iostream>
#include <iomanip>
#include <string>  
#include <sys/stat.h> // // for fileExists
#include <thread>
#include <atomic>
#include <fstream>

ScoreFile scoreFile;
ScoreEditor scoreEditor{scoreFile};
std::string filePath;

std::vector<std::pair<std::string,float*>> allParams{
    //{"rev_decay", &ScorePlayer::ScoreSynth::rev.decay},
    { "rootPositionInPixels0", &scoreEditor.rootPositionInPixels[0] },
    //{ "rootPositionInPixels1", &scoreEditor.rootPositionInPixels[1] },
    { "taktSizeInPixels", &scoreEditor.taktSizeInPixels }
};

enum class CommunicationState {
    Idle,
    DataToBeWritten,
    PreparedToWrite,
    Writing
};

std::atomic<CommunicationState> communicationState; // // 0:none, 1:dataToBeWritten, 2:preparedToWrite, 3:writing
bool isWaitingInput = false;
bool doConsoleWork = true;

int changeAudioParam(std::stringstream& ss, std::string word, double& synthParam, double minValue, double maxValue) {
    std::string paramName = word;
    std::getline(ss, word, ' ');
    if (word == "get") {
        std::cout << ">> " << synthParam << '\n';
    }
    else {
        double param;
        try {
            param = std::stod(word);
        }
        catch(...) {
            return 1;
        }
        if (param < 0) param = 0;
        if (param > 1) param = 1;
        communicationState = CommunicationState::DataToBeWritten;
        while (communicationState != CommunicationState::PreparedToWrite) {}
        communicationState = CommunicationState::Writing;
        ScorePlayer::stop(true);
        while (ScorePlayer::ScoreSynth::state[0] != ScorePlayer::ScoreSynth::State::Idle) {}
        synthParam = param;
        communicationState = CommunicationState::Idle;
        std::cout << ">> " << paramName << " = " << param << '\n';
    }
    return 0;
}

std::string argumentExplanation = "The first argument must be either the word \"open\" or the word \"new\" (without quotes). The argument following the word \"open\" must be the path to an existing file in the computer. The word \"new\" must be followed by a feasible path where a new file can be created.";

void printHelp() {
    for (int i = 0; i < 60; ++i) std::cout << "-"; std::cout << '\n';
    std::cout << "This program allows you to create and listen to small snippets in just intonation. It's like a MIDI roll, but instead of using absolute pitches, each note is defined using another note (its parent) and a rational number (the ratio from its parent). What the program does is reading and writing files with json syntax. It reads or creates a file, and when terminated writes all the changes.\n";
    std::cout << '\n';
    std::cout << "The program must be executed from a console and 2 arguments must be provided. " << argumentExplanation << '\n';
    std::cout << '\n';
    std::cout << "This is how you interact with the editor:\n";
    std::cout << "    * Use left and right arrow keys to move around.\n";
    std::cout << "    * Press +/- keys to zoom in/out (for non-US keyboards, try pressing the = key if the + doesn't work).\n";
    std::cout << "    * These keys change the edit mode:\n";
    for (const auto& [state, info] : ScoreEditor::editModeInfos) {
        std::cout << "    "<<"    " << info.code << ": " << info.name << '\n';
    }
    std::cout << "    * Left-clicks. The action depends on the edit mode.\n";
    std::cout << '\n';
    std::cout << "You can write several commands in the console to make further changes:\n";
    std::cout << "    * limit n (where n is an integer and 15<n<100): n will be the greatest numerator or denominator (ignoring factors that are powers of 2) of the ratios that can be selected.\n";
    std::cout << "    * rev_decay x (where x is a decimal number, 0.0 <= x <= 1.0): the decay time of the reverberation. Smaller values mean dryer sound.\n";
    std::cout << '\n';
    std::cout << "It sometimes comes in handy to convert a ratio to semitones. Type\n";
    std::cout << "    * get_st m/n (where m and n are integers).\n";
    std::cout << '\n';
    std::cout << "Make sure that audio is working properly. You should hear an A2 (220 Hz) tone when typing the command \"play\". You can stop this test tone with the command \"stop\". The \"audio playback\", \"consult notes\" and \"consult ratios\" modes make sound.\n";
    for (int i = 0; i < 60; ++i) std::cout << "-"; std::cout << '\n';
}

void consoleWorker() {
    std::cout << ">> Type \"help\" for info.\n";
    std::string tmp;
    std::string word;
    while (doConsoleWork) {
        isWaitingInput = true;
        getline(std::cin, tmp);
        isWaitingInput = false;
        
        try {
            std::stringstream ss{tmp};
            std::getline(ss, word, ' ');
            
            if (word == "limit") {
                std::getline(ss, word, ' ');
                int N = std::stoi(word);
                if (N < 16 || 100 < N) {
                    std::cout << ">> ERROR: value must be between 16 and 100\n";
                }
                else {
                    auto menuCollections_tmp = ScoreEditor::calculatePossibleRatios(N);
                    communicationState = CommunicationState::DataToBeWritten;
                    while (communicationState != CommunicationState::PreparedToWrite) {}
                    communicationState = CommunicationState::Writing;
                    scoreEditor.menuCollections = menuCollections_tmp;
                    communicationState = CommunicationState::Idle;
                    std::cout << ">> limit has been changed\n";
                }
            }
            else if (word == "play") {
                std::vector<double> freqs{220};
                ScorePlayer::playFrequencies(freqs);
            }
            else if (word == "stop") {
                ScorePlayer::stop();
            }
            else if (word == "rev_decay") {
                if (changeAudioParam(ss, word, ScorePlayer::ScoreSynth::rev.decay, 0., 1.)) {
                    throw "err";
                }
            }
            else if (word == "help") {
                printHelp();
            }
            else if (word == "get_st") {
                std::string subword;
                std::getline(ss, word, ' ');
                std::stringstream ssWord{word};
                std::getline(ssWord, subword, '/');
                int m = std::stoi(subword);
                std::getline(ssWord, subword, '/');
                int n = std::stoi(subword);
                if (m < 0 || n <= 0) throw "err";
                long double r = (long double)m / (long double)n;
                long double st = std::log2l(r) * 12.0l;
                
                std::ios oldState(nullptr);
                oldState.copyfmt(std::cout);    
                const auto maxPrecision{std::numeric_limits<long double>::digits10 + 1};
                std::cout << std::fixed << std::setprecision(maxPrecision);
                std::cout << ">> " << st << '\n';
                std::cout.copyfmt(oldState);
            }
            else {
                throw "err";
            }
        }
        catch(...) {
            std::cout << ">> ERROR: not a valid command\n";
        }
    }
}

// // https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c
inline bool fileExists (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

int main(int argv, char** args) {
    std::string errorString = ">> ERROR: 2 arguments must be provided. " + argumentExplanation;
    if (argv != 3) {
        std::cerr << errorString << '\n';
        std::cout << '\n';
        std::cout << ">> Type anything to quit. Next time execute from a terminal console and provide two proper arguments.\n";
        std::string word;
        std::cin >> word;
        return 1;
    }
    filePath = args[2];
    std::string command(args[1]);
    if (command == "new") {
        if (fileExists(filePath)) {
            std::cerr << ">> ERROR: could not create file, " << filePath << " already exists" << '\n';
            return 1;
        }
        scoreFile.createBlank();
        if (0 != scoreFile.writeToDisk(filePath.c_str())) {
            std::cerr << ">> ERROR: could not create file " << filePath << '\n';
            return 1;
        }
    }
    else if (command == "open") {
        if (0 != scoreFile.readFromDisk(args[2])) {
            std::cerr << ">> ERROR: could not open file " << filePath << '\n';
            return 1;
        }
        
        std::ifstream f(filePath);
        nlohmann::json data = nlohmann::json::parse(f);
        auto it = data["params"].find("rev_decay");
        if (it != data["params"].end()) {
            ScorePlayer::ScoreSynth::rev.decay = *it;
        }
        
        for (auto& [name, pValue] : allParams) {
            auto it = data["params"].find(name);
            if (it != data["params"].end()) {
                *pValue = *it;
            }
        }
    }
    else {
        std::cerr << errorString << '\n';
        return 1;
    }

    std::thread consoleThread(consoleWorker);
    
    scoreEditor.init(filePath);
    
    ScorePlayer::init();

    std::vector<double> freqs;
    freqs.reserve(20); // // arbitrary size?
    while (1) {
        if (communicationState == CommunicationState::DataToBeWritten) {
            communicationState = CommunicationState::PreparedToWrite;
            while (communicationState != CommunicationState::Idle) {}
        }
        int updateResult = scoreEditor.update();
        if (updateResult == ScoreEditor::updateCode_skip) continue;
        if (updateResult == ScoreEditor::updateCode_abort) break;
        scoreEditor.draw();
        
        // // audio
        if (scoreEditor.hasEditModeChanged) {
            ScorePlayer::stop();
        }
        if (scoreEditor.editMode == ScoreEditor::EditMode::ConsultRatios) {
            if (scoreEditor.idHeld != scoreEditor.idHeld_prev || scoreEditor.idHover != scoreEditor.idHover_prev) {
                if (scoreEditor.idHeld == -1 || scoreEditor.idHover == -1 || scoreEditor.idHover == scoreEditor.idHeld) {
                    ScorePlayer::stop();
                }
                if (scoreEditor.idHover != scoreEditor.idHover_prev && scoreEditor.idHeld != -1 && scoreEditor.idHover != -1 && scoreEditor.idHover != scoreEditor.idHeld) {
                    freqs.resize(2);
                    freqs[0] = scoreFile.getFrequency(scoreEditor.idHeld);
                    freqs[1] = scoreFile.getFrequency(scoreEditor.idHover);
                    ScorePlayer::playFrequencies(freqs);
                }
            }
        }
        else if (scoreEditor.editMode == ScoreEditor::EditMode::ConsultNotes) {
            if (scoreEditor.idHeld != scoreEditor.idHeld_prev) {
                if (scoreEditor.idHeld == -1) {
                    ScorePlayer::stop();
                }
                else {
                    freqs.resize(1);
                    freqs[0] = scoreFile.getFrequency(scoreEditor.idHeld);
                    ScorePlayer::playFrequencies(freqs);
                }
            }
        }
        else if (scoreEditor.editMode == ScoreEditor::EditMode::AudioPlayback) {
            if (scoreEditor.isMouseUnclick) {
                ScorePlayer::stop();
            }
            int takts = (int)std::roundf(scoreEditor.mouseX_taktsFromRoot-0.5f);
            int takts_prev = (int)std::roundf(scoreEditor.mouseX_taktsFromRoot_prev-0.5f);
            if (scoreEditor.isMouseClick || (scoreEditor.isMouseHeldDown && takts != takts_prev)) {
                freqs.clear();
                for (int i = 0; i < scoreEditor.nodes.size(); ++i) {
                    int t = scoreEditor.nodes[i].taktsFromRoot;
                    if (takts < t || t+scoreFile.getDurationInTakts(i) <= takts) continue;
                    freqs.push_back(scoreFile.getFrequency(i));
                }
                if (freqs.empty()) {
                    ScorePlayer::stop();
                }
                else { 
                    ScorePlayer::playFrequencies(freqs);
                }
            }
        }
    }
    
    ScorePlayer::stop(true);
    
    if (0 != scoreFile.writeToDisk(filePath.c_str())) {
        std::cerr << ">> ERROR: could not save file " << filePath << ".\n";
    }
    else {
        std::ifstream fIn(filePath);
        nlohmann::json data = nlohmann::json::parse(fIn);
        fIn.close();
        
        for (const auto& [name,pValue] : allParams) {
            data["params"][name] = *pValue;
        }

        std::ofstream fOut(filePath);
        fOut << data << '\n';
        fOut.close();
    }
    
    std::cout << ">> file saved\n";
    
    while (ScorePlayer::ScoreSynth::state[0] != ScorePlayer::ScoreSynth::State::Idle) {}
    SDL_Delay(10 + ScorePlayer::audio_settings::PERIOD_SIZE_MS * ScorePlayer::audio_settings::NPERIODS);
    ScorePlayer::uninit();
    
    doConsoleWork = false;
    if (!isWaitingInput) {
        consoleThread.join();
    }
    
    scoreEditor.uninit();
    
    return 0;
}

#include "ScoreFile.cpp"
#include "ScoreEditor.cpp"
