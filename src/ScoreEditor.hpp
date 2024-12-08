#ifndef SCORE_EDITOR_H
#define SCORE_EDITOR_H

#include "ScoreFile.hpp"

#include "../external/SDL2/include/SDL.h"
#include "../external/SDL_ttf/include/SDL_ttf.h"

#include <vector>
#include <string>

struct ScoreEditor {
    ScoreEditor(ScoreFile& _scoreFile);
    ~ScoreEditor();
    
    int init(std::string windowTitle);
    int uninit();

    ScoreFile& scoreFile;
    
    int windowWidthInPixels = 800;
    int windowHeightInPixels = 800;
    SDL_Window* window;
	SDL_Renderer* renderer;
    TTF_Font* font;
    SDL_Event event;
    
    long last_ticks = 0;
    const float minElapsedSecondsBetweenFrames = 1.f/200.f;
    
    int color_back[4]{171,102,85,255};
    int color_menu[4]{255, 0, 0, 255};
    
    float rootPositionInPixels[2]{windowWidthInPixels*0.5f, 0};
    const float taktSizeInPixels_min = 10;
    float taktSizeInPixels = 96;
    float semitoneSizeInPixels = 12;
    
    inline static const int updateCode_ok = 0;
    inline static const int updateCode_skip = -1;
    inline static const int updateCode_abort = 1;
    
    int update();
    int draw();
    
    struct Node {
        Node(const ScoreEditor* _parent) : parent{_parent} {}
        const ScoreEditor* parent;
        void readScoreNode(const ScoreFile::Node& n);
        int taktsFromRoot;
        float semitonesFromRoot;
        float horizontalLeftPosition, verticalCenter, horizontalCenter;
        float x1, y1, x2, y2;
        int ratioFromParent[2];
        std::string ratioFromParentLabel;
    };
    int getNodeInWindowPosition(float x, float y);
    bool doesPositionOverlapWithSomeNode(int taktPositionFromRoot, float semitonePositionFromRoot) const;
    bool doesNodeRectangleOverlapWithSomeNode(int nodeId, int taktsFromRoot, float semitonesFromRoot, int durationInTakts) const;
    void readNodes();
    
    enum class EditMode { 
        ConsultNotes, ConsultRatios, AddNodes, DeleteNodes, AudioPlayback, HorizontalMovement, HorizontalScaling, ChangeRatio, ChangeParent, ChangeRoot
    };
    struct EditModeInfo {
        char code;
        std::string name;
    };
    inline static std::map<EditMode, EditModeInfo> editModeInfos{
        { EditMode::ConsultNotes, {'n', "consult notes"} },
        { EditMode::ConsultRatios, {'c', "consult ratios"} },
        { EditMode::AddNodes, {'a', "add notes"} },
        { EditMode::DeleteNodes, {'d', "delete notes"} },
        { EditMode::HorizontalMovement, {'m', "move notes"} },
        { EditMode::HorizontalScaling, {'s', "scale note durations"} },
        { EditMode::ChangeParent, {'p', "change parent of note"} },
        { EditMode::ChangeRatio, {'r', "change ratio of note"} },
        { EditMode::ChangeRoot, {'o', "change root"} },
        { EditMode::AudioPlayback, {'k', "audio playback"} },
    };
    
    struct MenuCollection {
        std::vector<discrete::Monzo> ratios;
        int centerItemId;
    };
    std::vector<MenuCollection> menuCollections;
    static std::vector<MenuCollection> calculatePossibleRatios(const int N);
    
    struct Menu {
        int itemWidthInPixels = 70;
        int itemHeightInPixels = 30;
        enum class State {
            Closed, Opened, ClosingProcess
        };
        State state = State::Closed;
        
        float navelTaktsFromRoot;
        float navelSemitonesFromRoot;
        
        float centerItemPosition[2];
        //float centerItemTaktsFromRoot;
        //float centerItemSemitonesFromRoot;
        int quantizedSemitonesFromParent;
        int parentNodeId, childNodeId;
        struct Item {
            float x1, y1, x2, y2;
            discrete::Monzo ratio;
            std::string label;
        };
        int centerItemId;
        std::vector<Item> items;
        
        int itemId = -1;
        
        bool hasGrowthArea = false;
    };
    void menuSetup(int _menuNodeId, float taktPositionFromRoot, float semitonePositionFromRoot, int quantizedStSep);
    void menuClose();
    
    //////////////////////////////////
    std::vector<Node> nodes;
    EditMode editMode = EditMode::ConsultRatios;
    bool hasEditModeChanged = false;
    
    int mouseX_pixels;
    float mouseX_taktsFromRoot = 10000; // // ?
    float mouseX_taktsFromRoot_prev;
    int mouseY_pixels;
    float mouseY_semitonesFromRoot;
    
    bool isMouseHeldDown;
    bool isMouseClick;
    bool isMouseUnclick;
    
    int idHeld = -1;
    int idHover = -1;
    int idUnheld = -1;
    int idHover_prev = -1;
    int idHeld_prev = -1;
    
    Menu menu;
    float AddNodes_navelTaktsFromRoot;
    float AddNodes_navelSemitonesFromRoot;
    int AddNodes_navelQuantizedSemitonesFromParent;

    int DeleteNodes_selectedId = -1;
    
    bool ChangeRatio_menuJustOpened;
    
    int HorizontalMovement_taktHeld;
    int HorizontalScaling_taktHeld;
    //////////////////////////////////
};

#endif