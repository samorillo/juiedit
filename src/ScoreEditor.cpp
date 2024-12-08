#include "ScoreEditor.hpp"
#include "utilities.hpp"

#include <set>

ScoreEditor::ScoreEditor(ScoreFile& _scoreFile) : scoreFile{_scoreFile} {
    menuCollections = calculatePossibleRatios(23);
}
ScoreEditor::~ScoreEditor() {}

int ScoreEditor::init(std::string windowTitle) {
    if (0 != SDL_InitSubSystem(SDL_INIT_EVERYTHING))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL_INIT_ERROR", 
			SDL_GetError(), NULL);

		return -1;
	}
    
	window = SDL_CreateWindow(windowTitle.c_str(), SDL_WINDOWPOS_CENTERED, 
		SDL_WINDOWPOS_CENTERED, windowWidthInPixels, windowHeightInPixels, SDL_WINDOW_SHOWN);// | SDL_WINDOW_RESIZABLE);

	if (NULL == window)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL_WINDOW_ERROR", 
			SDL_GetError(), NULL);

		return -1;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (NULL == renderer)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL_RENDERER_ERROR", 
			SDL_GetError(), NULL);

		return -1;
	}

    if (0 != TTF_Init()) return -777;
    
    font = TTF_OpenFont("C:/terminal_graphics/jui_editor/data/fonts/arial.ttf", 20);
    if (!font) {
        return 888;
    }
    
    rootPositionInPixels[1] = windowHeightInPixels/2 - 12.f*std::log2f(scoreFile.getRootFrequency() / 261.625565301)*semitoneSizeInPixels/2;

	return 0;
}

int ScoreEditor::uninit() {
    SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
    return 0;
}

int ScoreEditor::update() {
    float elapsedTimeInSeconds = (SDL_GetTicks() - last_ticks) * 0.001f;
    if (elapsedTimeInSeconds < minElapsedSecondsBetweenFrames) return updateCode_skip;
    last_ticks = SDL_GetTicks();
    
    //////////////////////////////////////
    mouseX_taktsFromRoot_prev = mouseX_taktsFromRoot;
    SDL_GetMouseState(&mouseX_pixels, &mouseY_pixels);
    mouseX_taktsFromRoot = (mouseX_pixels - rootPositionInPixels[0]) / taktSizeInPixels;
    mouseY_semitonesFromRoot = -(mouseY_pixels - rootPositionInPixels[1]) / semitoneSizeInPixels;
    hasEditModeChanged = false;
    isMouseClick = false;
    isMouseUnclick = false;
    
    static bool isLeftArrowHeld = false;
    static bool isRightArrowHeld = false;
    static bool isPlusKeyHeld = false;
    static bool isMinusKeyHeld = false;
    //////////////////////////////////////
    
    while (SDL_PollEvent(&event)) {
        const bool isCtrlPressed{ SDL_GetModState() == KMOD_LCTRL || SDL_GetModState() == KMOD_RCTRL};
        if (event.type == SDL_KEYDOWN && isCtrlPressed) {
            switch ((char)event.key.keysym.sym) {
                case 'q': // // quit
                return updateCode_abort;
            }
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
            return updateCode_abort;
        }
        
        EditMode oldEditMode = editMode;
        if (event.type == SDL_KEYDOWN) {
            bool b = true;
            char sym = (char)event.key.keysym.sym;
            if (sym == editModeInfos[EditMode::ConsultNotes].code) {
                editMode = EditMode::ConsultNotes;
            }
            else if (sym == editModeInfos[EditMode::ConsultRatios].code) {
                editMode = EditMode::ConsultRatios;
            }
            else if (sym == editModeInfos[EditMode::HorizontalMovement].code) {
                editMode = EditMode::HorizontalMovement;
            }
            else if (sym == editModeInfos[EditMode::HorizontalScaling].code) {
                editMode = EditMode::HorizontalScaling;
            }
            else if (sym == editModeInfos[EditMode::AddNodes].code) {
                editMode = EditMode::AddNodes;
            }
            else if (sym == editModeInfos[EditMode::DeleteNodes].code) {
                editMode = EditMode::DeleteNodes;
                DeleteNodes_selectedId = -1;
            }
            else if (sym == editModeInfos[EditMode::ChangeParent].code) {
                editMode = EditMode::ChangeParent;
            }
            else if (sym == editModeInfos[EditMode::ChangeRatio].code) {
                editMode = EditMode::ChangeRatio;
            }
            else if (sym == editModeInfos[EditMode::ChangeRoot].code) {
                editMode = EditMode::ChangeRoot;
            }
            else if (sym == editModeInfos[EditMode::AudioPlayback].code) {
                editMode = EditMode::AudioPlayback;
            }
            else {
                b = false;
            }
            if (b) {
                hasEditModeChanged = true;
            }
        }
        
        switch(event.key.keysym.sym) {
            case SDLK_LEFT: 
                isLeftArrowHeld = (event.type == SDL_KEYDOWN);
                break;
            case SDLK_RIGHT: 
                isRightArrowHeld = (event.type == SDL_KEYDOWN);
                break;
            case SDLK_EQUALS:
                isPlusKeyHeld = (event.type == SDL_KEYDOWN);
                break;
            case SDLK_MINUS:
                isMinusKeyHeld = (event.type == SDL_KEYDOWN);
                break;
        }
        
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            isMouseClick = true;
            isMouseHeldDown = true;
        }
        if (event.type == SDL_MOUSEBUTTONUP) {
            isMouseUnclick = true;
            isMouseHeldDown = false;
        }
    }
    
    if (hasEditModeChanged) {
        idHeld = -1;
        idHover = -1;
        idUnheld = -1;
        idHover_prev = -1;
        idHeld_prev = -1;
        menu.state = Menu::State::Closed;
    }
    
    // // zoom, position
    const float arrowPixelsPerSecond = 200.f;
    if (isLeftArrowHeld && isRightArrowHeld) ; // // do nothing
    else if (isLeftArrowHeld) {
        rootPositionInPixels[0] += arrowPixelsPerSecond*elapsedTimeInSeconds;
    }
    else if (isRightArrowHeld) {
        rootPositionInPixels[0] -= arrowPixelsPerSecond*elapsedTimeInSeconds;
    }
    const float zoomPixelsPerSecond = 40.f;
    if (isPlusKeyHeld || isMinusKeyHeld) {    
        float taktSizeInPixels_prev = taktSizeInPixels;
        taktSizeInPixels += (isPlusKeyHeld ? 1 : -1) * zoomPixelsPerSecond*elapsedTimeInSeconds;
        taktSizeInPixels = std::max(taktSizeInPixels, taktSizeInPixels_min);
        float factor = taktSizeInPixels / taktSizeInPixels_prev;
        rootPositionInPixels[0] = mouseX_pixels - (mouseX_pixels - rootPositionInPixels[0])*factor;
    }
    // // // //
    
    readNodes();
    
    idHover_prev = idHover;
    idHover = getNodeInWindowPosition(mouseX_pixels, mouseY_pixels);
    idHeld_prev = idHeld;
    if (isMouseClick) {
        idHeld = idHover;
    }
    idUnheld = -1;
    if (isMouseUnclick) {
        idUnheld = idHeld;
        idHeld = -1;
    }
    
    if (editMode == EditMode::AddNodes) {
        if (menu.state == Menu::State::Opened) {
            // // do nothing, handle later, after menu data is computed
        }
        else if (menu.state == Menu::State::Closed) {
            if (idHeld != -1 || idUnheld != -1) {
                int id = idHeld == -1 ? idUnheld : idHeld;
                AddNodes_navelTaktsFromRoot = roundint(mouseX_taktsFromRoot-0.5f)+0.5f;
                AddNodes_navelQuantizedSemitonesFromParent = roundint(mouseY_semitonesFromRoot - nodes[id].semitonesFromRoot);
                AddNodes_navelSemitonesFromRoot = std::round(mouseY_semitonesFromRoot - nodes[id].semitonesFromRoot) + nodes[id].semitonesFromRoot;
            }
            if (idUnheld != -1 && isMouseUnclick) { 
                if (!doesPositionOverlapWithSomeNode((int)std::floor(mouseX_taktsFromRoot), mouseY_semitonesFromRoot)) {
                    int quantizedStSep = roundint(mouseY_semitonesFromRoot - nodes[idUnheld].semitonesFromRoot); 
                    menu.parentNodeId = idUnheld;
                    menu.childNodeId = -1;
                    menu.state = Menu::State::Opened;
                    menu.quantizedSemitonesFromParent = quantizedStSep;
                    int menuCollectionId = quantizedStSep;
                    discrete::Monzo octaveCompensation(1);
                    while (menuCollectionId <= -12) {
                        menuCollectionId += 12;
                        octaveCompensation /= discrete::Monzo(2);
                    }
                    while (menuCollectionId >= 12) {
                        menuCollectionId -= 12;
                        octaveCompensation *= discrete::Monzo(2);
                    }
                    int sgn = menuCollectionId < 0 ? -1 : 1;
                    if (sgn == -1) menuCollectionId = -menuCollectionId;
                    const auto& ratios = menuCollections[menuCollectionId].ratios;
                    menu.items.resize(ratios.size());    
                    if (sgn == 1) {
                        for (int i = 0; i < ratios.size(); ++i) {
                            menu.items[i].ratio = ratios[i] * octaveCompensation;
                        }
                        menu.centerItemId = menuCollections[menuCollectionId].centerItemId;
                    }
                    else {
                        for (int i = 0; i < ratios.size(); ++i) {
                            menu.items[i].ratio = discrete::Monzo(1)/ratios[ratios.size()-1-i] * octaveCompensation;
                        }
                        menu.centerItemId = ratios.size()-1 - menuCollections[menuCollectionId].centerItemId;
                    }
                    //menu.centerItemTaktsFromRoot = roundint(mouseX_taktsFromRoot-0.5f)+0.5f;
                    //menu.centerItemSemitonesFromRoot = std::round(mouseY_semitonesFromRoot - nodes[menu.parentNodeId].semitonesFromRoot) + nodes[menu.parentNodeId].semitonesFromRoot;
                    menu.navelTaktsFromRoot = AddNodes_navelTaktsFromRoot;
                    menu.navelSemitonesFromRoot = AddNodes_navelSemitonesFromRoot;//std::round(mouseY_semitonesFromRoot - nodes[menu.parentNodeId].semitonesFromRoot) + nodes[menu.parentNodeId].semitonesFromRoot;
                    for (int i = 0; i < menu.items.size(); ++i) { 
                        menu.items[i].label = std::to_string((int)menu.items[i].ratio.numerator())+":"+std::to_string((int)menu.items[i].ratio.denominator());
                    }
                }
            }
        }
        else if (menu.state == Menu::State::ClosingProcess) {
            if (isMouseUnclick) menu.state = Menu::State::Closed;
        }
    }
    else if (editMode == EditMode::DeleteNodes) {
        if (isMouseClick) {
            if (DeleteNodes_selectedId != -1 && DeleteNodes_selectedId != idHover) {
                DeleteNodes_selectedId = -1;
            }
            else if (idHover != -1 && DeleteNodes_selectedId == idHover) {
                if (0 == scoreFile.deleteNode(idHover)) {
                    readNodes();
                }
                else {
                    std::cout << ">> this node cannot be deleted\n";
                }
                DeleteNodes_selectedId = -1;
            }
            else {
                DeleteNodes_selectedId = idHover;
            }
        }
    }
    else if (editMode == EditMode::ChangeRatio) {
        ChangeRatio_menuJustOpened = false;
        if (isMouseClick && idHover != -1 && menu.state == Menu::State::Closed && idHover != scoreFile.getRootId()) {
            int id = idHover;
            int parentId = scoreFile.getParentId(id);
            discrete::Monzo ratio = scoreFile.getRelativeRatio(parentId, id);
            int quantizedStSep = roundint(12. * std::log2((double)ratio));

            menu.parentNodeId = parentId;
            menu.childNodeId = id;
            menu.state = Menu::State::Opened; ChangeRatio_menuJustOpened = true;
            menu.quantizedSemitonesFromParent = quantizedStSep;
            int menuCollectionId = quantizedStSep;
            discrete::Monzo octaveCompensation(1);
            while (menuCollectionId <= -12) {
                menuCollectionId += 12;
                octaveCompensation /= discrete::Monzo(2);
            }
            while (menuCollectionId >= 12) {
                menuCollectionId -= 12;
                octaveCompensation *= discrete::Monzo(2);
            }
            int sgn = menuCollectionId < 0 ? -1 : 1;
            if (sgn == -1) menuCollectionId = -menuCollectionId;
            const auto& ratios = menuCollections[menuCollectionId].ratios;
            menu.items.resize(ratios.size());    
            if (sgn == 1) {
                for (int i = 0; i < ratios.size(); ++i) {
                    menu.items[i].ratio = ratios[i] * octaveCompensation;
                }
                menu.centerItemId = menuCollections[menuCollectionId].centerItemId;
            }
            else {
                for (int i = 0; i < ratios.size(); ++i) {
                    menu.items[i].ratio = discrete::Monzo(1)/ratios[ratios.size()-1-i] * octaveCompensation;
                }
                menu.centerItemId = ratios.size()-1 - menuCollections[menuCollectionId].centerItemId;
            }
            //menu.centerItemTaktsFromRoot = roundint(mouseX_taktsFromRoot-0.5f)+0.5f;
            //menu.centerItemSemitonesFromRoot = std::round(mouseY_semitonesFromRoot - nodes[menu.parentNodeId].semitonesFromRoot) + nodes[menu.parentNodeId].semitonesFromRoot;
            menu.navelTaktsFromRoot = nodes[id].taktsFromRoot + 0.5f;
            menu.navelSemitonesFromRoot = nodes[parentId].semitonesFromRoot + menu.quantizedSemitonesFromParent;//std::round(mouseY_semitonesFromRoot - nodes[menu.parentNodeId].semitonesFromRoot) + nodes[menu.parentNodeId].semitonesFromRoot;
            for (int i = 0; i < menu.items.size(); ++i) { 
                menu.items[i].label = std::to_string((int)menu.items[i].ratio.numerator())+":"+std::to_string((int)menu.items[i].ratio.denominator());
            }
        }
    }
    else if (editMode == EditMode::ChangeParent) {
        int idFrom = idHover, idTo = idUnheld;
        if (isMouseUnclick && idUnheld != -1 && idFrom != -1 && idTo != -1 && idFrom != idTo && idFrom != scoreFile.getParentId(idTo)) {
            if (0 != scoreFile.changeParent(idTo, idFrom)) {
                std::cout << ">> parent could not be changed\n";
            }
        }
    }
    else if (editMode == EditMode::ChangeRoot) {
        if (isMouseClick && idHeld != -1 && idHeld != scoreFile.getRootId()) {
            rootPositionInPixels[0] = nodes[idHeld].x1;
            rootPositionInPixels[1] = nodes[idHeld].verticalCenter;
            scoreFile.changeRoot(idHeld);
            readNodes();
            std::cout << ">> changed root\n";
        }
    }
    else if (editMode == EditMode::HorizontalMovement) {
        if (isMouseClick) {
            HorizontalMovement_taktHeld = (int)std::floor(mouseX_taktsFromRoot);
        }
        if (idHeld != -1) {
            int mp = (int)std::floor(mouseX_taktsFromRoot);
            int tInc = mp - HorizontalMovement_taktHeld;
            bool doesOverlap = doesNodeRectangleOverlapWithSomeNode(
                idHeld,
                nodes[idHeld].taktsFromRoot + tInc, 
                nodes[idHeld].semitonesFromRoot,
                scoreFile.getDurationInTakts(idHeld)
            );
            if (tInc != 0 && !doesOverlap) {
                scoreFile.incrementPositionInTaktsFromParent(idHeld, tInc);
                HorizontalMovement_taktHeld = mp;
                readNodes();
            }
        }
    }
    else if (editMode == EditMode::HorizontalScaling) {
        if (isMouseClick) {
            HorizontalScaling_taktHeld = (int)std::floor(mouseX_taktsFromRoot);
        }
        if (idHeld != -1) {
            int mp = (int)std::floor(mouseX_taktsFromRoot);
            int tInc = mp - HorizontalScaling_taktHeld;
            int prevDur = scoreFile.getDurationInTakts(idHeld);
            int newDur = std::max(1, prevDur + tInc);
            if (newDur != prevDur) {
                bool doesOverlap = doesNodeRectangleOverlapWithSomeNode(
                    idHeld,
                    nodes[idHeld].taktsFromRoot, 
                    nodes[idHeld].semitonesFromRoot,
                    newDur
                );
                if (!doesOverlap) {
                    scoreFile.changeNodeDuration(idHeld, newDur);
                    HorizontalScaling_taktHeld = mp;
                    readNodes();
                }
            }
        }
    }
    
    if (menu.state == Menu::State::Opened) {
        menu.itemId = -1;
        menu.centerItemPosition[0] = rootPositionInPixels[0] + menu.navelTaktsFromRoot * taktSizeInPixels;
        menu.centerItemPosition[1] = rootPositionInPixels[1] - menu.navelSemitonesFromRoot * semitoneSizeInPixels;
        float yUp = menu.centerItemPosition[1] - ((int)menu.items.size()-menu.centerItemId-0.5f)*menu.itemHeightInPixels;
        float yDown = menu.centerItemPosition[1] + (menu.centerItemId+0.5f)*menu.itemHeightInPixels;
        menu.hasGrowthArea = false;
        if (yUp < 0 && yDown > windowHeightInPixels) {
            menu.hasGrowthArea = true;
            std::cerr << "handle this!!!! menu too big\n";
            throw "err";
        }
        else if (yUp < 0) {
            menu.centerItemPosition[1] -= yUp;
        }
        else if (yDown > windowHeightInPixels) {
            menu.centerItemPosition[1] -= (yDown - windowHeightInPixels);
        }
        for (int i = 0; i < menu.items.size(); ++i) {
            menu.items[i].x1 = menu.centerItemPosition[0] - menu.itemWidthInPixels/2;
            menu.items[i].y1 = menu.centerItemPosition[1] - menu.itemHeightInPixels/2 - (i-menu.centerItemId)*menu.itemHeightInPixels;
            menu.items[i].x2 = menu.items[i].x1 + menu.itemWidthInPixels;
            menu.items[i].y2 = menu.items[i].y1 + menu.itemHeightInPixels;
            if (menu.items[i].x1 <= mouseX_pixels && mouseX_pixels <= menu.items[i].x2 && menu.items[i].y1 <= mouseY_pixels && mouseY_pixels < menu.items[i].y2) {
                menu.itemId = i;
            }
        }
        
        if (isMouseClick && !(editMode == EditMode::ChangeRatio && ChangeRatio_menuJustOpened)) {
            menu.state = Menu::State::ClosingProcess;
            if (menu.itemId != -1) {
                if (editMode == EditMode::AddNodes) {
                    scoreFile.createNode(menu.parentNodeId, menu.items[menu.itemId].ratio, (int)std::floor(menu.navelTaktsFromRoot)-nodes[menu.parentNodeId].taktsFromRoot, 1);
                    readNodes();
                }
                else if (editMode == EditMode::ChangeRatio) {
                    scoreFile.changeRatio(menu.childNodeId, menu.items[menu.itemId].ratio);
                    readNodes();
                }
            }
        }
        
    }
    
    if (menu.state == Menu::State::ClosingProcess) {
        if (isMouseUnclick) menu.state = Menu::State::Closed;
    }
    
    return 0;
}

int ScoreEditor::draw() {
    SDL_SetRenderDrawColor(renderer, color_back[0], color_back[1], color_back[2], color_back[3]);
    SDL_RenderClear(renderer);
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    // // horizontal reference line
    int h = windowHeightInPixels/2;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255/4);
    SDL_RenderDrawLine(renderer, 0, h, windowWidthInPixels, h);
    
    // // takt grid
    float windowHorizontalCenter = rootPositionInPixels[1];
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255/10);
    
    float hInit = windowHorizontalCenter = rootPositionInPixels[0];
    while (hInit > windowWidthInPixels) hInit -= taktSizeInPixels;
    while (hInit < 0) hInit += taktSizeInPixels;
    
    SDL_RenderDrawLine(renderer, roundint(hInit), 0, roundint(hInit), windowHeightInPixels);
    
    for (int dir = -1; dir <= 1; dir += 2) {
        float h = hInit + taktSizeInPixels*dir;
        while (0 <= h && h <= windowWidthInPixels) {
            SDL_RenderDrawLine(renderer, roundint(h), 0, roundint(h), windowHeightInPixels);
            h += dir*taktSizeInPixels;
        }
    }
    
    // // nodes
    for (const Node& n : nodes) {
        int x1 = roundint(n.x1), y1 = roundint(n.y1);
        int x2 = roundint(n.x2), y2 = roundint(n.y2);
        SDL_Rect rect = {x1, y1, x2-x1, y2-y1};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
        drawRectangleContour(renderer, rect.x, rect.y, rect.x+rect.w, rect.y+rect.h);
    }
    for (int id = 0; id < nodes.size(); ++id) {
        int parentId = scoreFile.getParentId(id);
        if (parentId == ScoreFile::Node::NULL_ID) continue;
        const Node& m = nodes[parentId];
        Node& n = nodes[id];
        SDL_SetRenderDrawColor(renderer, 0,0,0,255);
        drawArrow(renderer, m.horizontalCenter, m.verticalCenter, n.horizontalCenter, n.verticalCenter);
    }
    
    if (editMode == EditMode::ConsultRatios) {
        if (idHeld != -1) {
            drawArrow(renderer, nodes[idHeld].horizontalCenter, nodes[idHeld].verticalCenter, mouseX_pixels, mouseY_pixels);
            
            if (idHover != -1) {
                int idFrom = idHeld, idTo = idHover;
                const Node& m = nodes[idFrom];
                const Node& n = nodes[idTo];
                
                std::string label;
                if (scoreFile.getParentId(idTo) == idFrom) {
                    label = nodes[idTo].ratioFromParentLabel;
                }
                else {
                    discrete::Monzo ratio = scoreFile.getRelativeRatio(idFrom, idTo);
                    label = std::to_string((int)ratio.numerator())+":"+std::to_string((int)ratio.denominator());
                }
                
                float x1 = m.horizontalCenter;
                float y1 = m.verticalCenter;
                float x2 = n.horizontalCenter;
                float y2 = n.verticalCenter;
                SDL_Color textColor = {255, 255, 255}; // White color
                SDL_Surface* textSurface = TTF_RenderText_Solid(font, label.c_str(), textColor);
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                
                float x = (x1 + x2) * 0.5f, y = (y1 + y2) * 0.5f;
                
                int textWidth, textHeight;
                TTF_SizeText(font, label.c_str(), &textWidth, &textHeight);

                SDL_Rect destinationRect;
                destinationRect.x = x - textWidth/2;
                destinationRect.y = y - textHeight/2;
                destinationRect.w = textWidth;
                destinationRect.h = textHeight;
                
                SDL_SetRenderDrawColor(renderer, 210,102,205,255);
                SDL_RenderFillRect(renderer, &destinationRect);
                
                renderTextCentered(renderer, font, label.c_str(), destinationRect);
            }
        }
    }
    if (editMode == EditMode::ConsultNotes) {
        if (idHeld != -1) {
            double f = scoreFile.getFrequency(idHeld);
            double m = dsp::ftom(f); //////////////////////////////
            double n[2]{m-60., 0.};
            while (n[0] >= 12) {
                n[0] -= 12;
                n[1] += 1;
            }
            while (n[0] < 0) {
                n[0] += 12;
                n[1] -= 1;
            }
            int nOct = (int)n[1];
            int n0i = roundint(n[0]);
            
            double n0f = n[0] - n0i;
            
            if (n0i == 12) {
                n0i = 0;
                nOct += 1;
            }
            
            int cents = roundint(n0f*100.);
            if (cents == -100) {
                cents = 0;
                n0i -= 1;
                if (n0i < 0) {
                    n0i += 12;
                    nOct -= 1;
                }
            }
            else if (cents == 100) {
                cents = 0;
                n0i += 1;
                if (n0i >= 12) {
                    n0i -= 12;
                    nOct += 1;
                }
            }
            int centsAbs = (cents >= 0) ? cents : -cents;
            
            std::string centsStr = std::to_string(centsAbs);
            if (centsStr.length() < 2) centsStr = "0"+centsStr;
            centsStr = (n0f >= 0 ? "+" : "-") + centsStr;
            static std::vector<std::string> noteNames{"C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B"};
            
            // // draw it
            std::string label = noteNames[n0i] + std::to_string(4+nOct) + " " + centsStr;
            
            SDL_Color textColor = {255, 255, 255}; // White color
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, label.c_str(), textColor);
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            
            int textWidth, textHeight;
            TTF_SizeText(font, label.c_str(), &textWidth, &textHeight);
            
            const Node& node = nodes[idHeld];
            SDL_Rect destinationRect;
            destinationRect.x = node.horizontalCenter - textWidth/2;
            destinationRect.y = node.verticalCenter - textHeight/2;
            destinationRect.w = textWidth;
            destinationRect.h = textHeight;
            
            SDL_SetRenderDrawColor(renderer, 210,102,205,255);
            SDL_RenderFillRect(renderer, &destinationRect);
            
            SDL_RenderCopy(renderer, textTexture, NULL, &destinationRect);
            SDL_FreeSurface(textSurface);
            SDL_DestroyTexture(textTexture);
        }
    }
    else if (editMode == EditMode::AddNodes) {
        if (menu.state == Menu::State::Closed && idHeld != -1) {
            float cx = rootPositionInPixels[0] + AddNodes_navelTaktsFromRoot*taktSizeInPixels;
            float cy = rootPositionInPixels[1] - AddNodes_navelSemitonesFromRoot*semitoneSizeInPixels;
            drawRectangleContour(renderer, roundint(cx-taktSizeInPixels*0.5f), roundint(cy-semitoneSizeInPixels), roundint(cx+taktSizeInPixels*0.5f), roundint(cy+semitoneSizeInPixels));
        }
        if (menu.state != Menu::State::ClosingProcess && (menu.state == Menu::State::Opened || idHeld != -1)) {
            const Node& n = nodes[idHeld != -1 ? idHeld : menu.parentNodeId];
            float a1 = n.horizontalCenter;
            float b1 = n.verticalCenter;
            float a2 = rootPositionInPixels[0]+AddNodes_navelTaktsFromRoot*taktSizeInPixels;//(x1+x2)*0.5f;
            float b2 = rootPositionInPixels[1]-AddNodes_navelSemitonesFromRoot*semitoneSizeInPixels;//(y1+y2)*0.5f;
            drawArrow(renderer, a1, b1, a2, b2);
            if (a1 > a2) std::swap(a1, a2);
            if (b1 > b2) std::swap(b1, b2);
            int st = (menu.state == Menu::State::Opened ? menu.quantizedSemitonesFromParent : AddNodes_navelQuantizedSemitonesFromParent);
            renderTextCentered(renderer, font, std::to_string(st).c_str(), SDL_Rect{roundint(a1), roundint(b1), roundint(a2-a1), roundint(b2-b1)});
        }
    }
    else if (editMode == EditMode::DeleteNodes) {
        if (DeleteNodes_selectedId != -1) {
            const Node& n = nodes[DeleteNodes_selectedId];
            SDL_Rect rect = {
                roundint(n.x1), roundint(n.y1), 
                roundint(n.x2-n.x1), roundint(n.y2-n.y1)
            };
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 200);
            SDL_RenderFillRect(renderer, &rect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }
    else if (editMode == EditMode::ChangeParent) {
        if (idHeld != -1 && idHeld != scoreFile.getRootId()) {
            int idTo = idHeld;
            const Node& m = nodes[scoreFile.getParentId(idTo)];
            const Node& n = nodes[idTo];
            float a1 = m.horizontalCenter, b1 = m.verticalCenter, a2 = n.horizontalCenter, b2 = n.verticalCenter;
            SDL_SetRenderDrawColor(renderer, 255,0,0,255);
            drawArrow(renderer, a1, b1, a2, b2);
            SDL_SetRenderDrawColor(renderer, 0,0,0,255);
            float c1 = mouseX_pixels, d1 = mouseY_pixels, c2 = nodes[idTo].horizontalCenter, d2 = nodes[idTo].verticalCenter;
            if (scoreFile.getParentId(idTo) != idHover) {
                drawArrow(renderer, c1, d1, c2, d2);
            }
            if (idHover != -1 && idHover != idTo) {
                int idFrom = idHover;
                int st = roundint(nodes[idTo].semitonesFromRoot - nodes[idFrom].semitonesFromRoot);
                renderTextCentered(renderer, font, std::to_string(st).c_str(), SDL_Rect{roundint(c1), roundint(d1), roundint(c2-c1), roundint(d2-d1)});
            }
        }
    }
    else if (editMode == EditMode::AudioPlayback) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 40);
        SDL_Rect rect;
        rect.x = roundint(rootPositionInPixels[0] + std::floor(mouseX_taktsFromRoot)*taktSizeInPixels);
        rect.y = 0;
        rect.w = roundint(taktSizeInPixels);
        rect.h = windowHeightInPixels;
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    
    if (menu.state == Menu::State::Opened) {
        for (int i = 0; i < menu.items.size(); ++i) {
            int x1 = roundint(menu.items[i].x1), x2 = roundint(menu.items[i].x2);
            int y1 = roundint(menu.items[i].y1), y2 = roundint(menu.items[i].y2);
            
            SDL_Rect rectangle = {
                x1, y1, menu.itemWidthInPixels, menu.itemHeightInPixels
            };
            SDL_SetRenderDrawColor(renderer, color_menu[0], color_menu[1], color_menu[2], color_menu[3]);
            SDL_RenderFillRect(renderer, &rectangle);
            if (i == menu.centerItemId) {
                drawRectangleContour(renderer, x1, y1, x2, y2);
            }
            //renderTextCentered(renderer, font, menuItems[i].label.c_str(), rectangle);
            if (menu.itemId == i) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 40);
                SDL_RenderFillRect(renderer, &rectangle);
            }
            
            renderTextCentered(renderer, font, menu.items[i].label.c_str(), rectangle);
        }
    }
    
    SDL_RenderPresent(renderer);
    return 0;
}

void ScoreEditor::Node::readScoreNode(const ScoreFile::Node& n) {
    taktsFromRoot = parent->scoreFile.getRelativePositionInTakts(parent->scoreFile.getRootId(), n.id);
    discrete::Monzo ratio = parent->scoreFile.getRelativeRatio(parent->scoreFile.getRootId(), n.id);
    
    semitonesFromRoot = (float)(12.0 * std::log2((double)ratio));
    horizontalLeftPosition = parent->rootPositionInPixels[0] + taktsFromRoot * parent->taktSizeInPixels;
    verticalCenter = parent->rootPositionInPixels[1] - semitonesFromRoot*parent->semitoneSizeInPixels;
    x1 = horizontalLeftPosition;
    x2 = horizontalLeftPosition + parent->taktSizeInPixels * n.durationInTakts;
    y1 = verticalCenter - parent->semitoneSizeInPixels;
    y2 = verticalCenter + parent->semitoneSizeInPixels;
    horizontalCenter = (x1 + x2) * 0.5f;
    
    if (n.id != parent->scoreFile.getRootId()) {
        discrete::Monzo ratioFP = n.ratioFromParent;
        ratioFromParent[0] = ratioFP.numerator();
        ratioFromParent[1] = ratioFP.denominator();
        ratioFromParentLabel = std::to_string((int)ratioFromParent[0])+":"+std::to_string((int)ratioFromParent[1]);
    }
}

void ScoreEditor::readNodes() {
    const int nNodes = scoreFile.getNumberOfNodes();
    if (nodes.size() > nNodes) nodes.erase(nodes.begin()+nNodes, nodes.end());
    else if (nodes.size() < nNodes) {
        nodes.reserve(nNodes);
        for (int i = nodes.size(); i < nNodes; ++i) {
            nodes.emplace_back(this);
        }
    }
    for (int i = 0; i < nodes.size(); ++i) {
        nodes[i].readScoreNode(scoreFile.getNode(i));
    }
}

int ScoreEditor::getNodeInWindowPosition(float x, float y) {
    int selectedNodeId = -1;
    for (int i = 0; i < nodes.size(); ++i) {
        const Node& n = nodes[i];
        if (n.x1 <= x && x <= n.x2 && n.y1 <= y && y <= n.y2) {
            if (selectedNodeId != -1) { // // if click touches two rectangles, do as if it touched neither
                selectedNodeId = -1;
                break;
            }
            selectedNodeId = i;
        }
    }
    return selectedNodeId;
}


/*void ScoreEditor::menuSetup(int _menuNodeId, float taktPositionFromRoot, float semitonePositionFromRoot, int quantizedStSep) {
    menu.parentNodeId = _menuNodeId;
    menu.isOpened = true;
    menu.quantizedSemitonesFromParent = quantizedStSep;
    int menuCollectionId = quantizedStSep;
    discrete::Monzo octaveCompensation(1);
    while (menuCollectionId <= -12) {
        menuCollectionId += 12;
        octaveCompensation /= discrete::Monzo(2);
    }
    while (menuCollectionId >= 12) {
        menuCollectionId -= 12;
        octaveCompensation *= discrete::Monzo(2);
    }
    int sgn = menuCollectionId < 0 ? -1 : 1;
    if (sgn == -1) menuCollectionId = -menuCollectionId;
    const auto& ratios = menuCollections[menuCollectionId].ratios;
    menu.items.resize(ratios.size());    
    if (sgn == 1) {
        for (int i = 0; i < ratios.size(); ++i) {
            menu.items[i].ratio = ratios[i] * octaveCompensation;
        }
        menu.centerItemId = menuCollections[menuCollectionId].centerItemId;
    }
    else {
        for (int i = 0; i < ratios.size(); ++i) {
            menu.items[i].ratio = discrete::Monzo(1)/ratios[ratios.size()-1-i] * octaveCompensation;
        }
        menu.centerItemId = ratios.size()-1 - menuCollections[menuCollectionId].centerItemId;
    }
    menu.centerItemTaktsFromRoot = (taktPositionFromRoot+0.5f);
    menu.centerItemSemitonesFromRoot = - semitonePositionFromRoot;
    menu.taktsFromItsNode = taktPositionFromRoot - nodes[menu.parentNodeId].taktsFromRoot;
    for (int i = 0; i < menu.items.size(); ++i) { 
        menu.items[i].label = std::to_string((int)menu.items[i].ratio.numerator())+":"+std::to_string((int)menu.items[i].ratio.denominator());
    }
}*/

std::vector<ScoreEditor::MenuCollection> ScoreEditor::calculatePossibleRatios(const int N) {
    std::set<discrete::Monzo> ratios;
    for (int n = 3; n <= N; ++n) {
        for (int d = n/2+1; d <= n; ++d) {
            ratios.insert(discrete::Monzo(n) / discrete::Monzo(d));
        }
    }
    
    std::vector<MenuCollection> menuCollections;
    menuCollections.resize(12);
    for (auto r : ratios) {
        double st = 12. * std::log2((double)r);
        int stRound = roundint(st);
        if (stRound == 12) {
            stRound = 0;
            r /= discrete::Monzo(2);
        }
        menuCollections[stRound].ratios.push_back(r);
    }
    std::sort(menuCollections[0].ratios.begin(), menuCollections[0].ratios.end());
    
    using Monzo = discrete::Monzo;
    std::vector<discrete::Monzo> centers {
        Monzo(1),
        Monzo(16)/Monzo(15),
        Monzo(9)/Monzo(8),
        Monzo(6)/Monzo(5),
        Monzo(5)/Monzo(4),
        Monzo(4)/Monzo(3),
        Monzo(10)/Monzo(7), ////////////////////////////////////////
        Monzo(3)/Monzo(2),
        Monzo(8)/Monzo(5),
        Monzo(5)/Monzo(3),
        Monzo(9)/Monzo(5),
        Monzo(15)/Monzo(8)
    };
    
    for (int i = 0; i < 12; ++i) {
        const auto& v = menuCollections[i].ratios;
        menuCollections[i].centerItemId = std::distance(v.begin(), std::find(v.begin(), v.end(), centers[i]));
        if (menuCollections[i].centerItemId == v.size()) {
            std::cerr << "WTF has happened with centers in ScoreEditor::ScoreEditor\n";
            throw "err";
        }
    }
    
    return menuCollections;
}

bool ScoreEditor::doesPositionOverlapWithSomeNode(int taktsFromRoot, float semitonesFromRoot) const {
    bool overlapsWithAnother = false;
    for (int id = 0; id < nodes.size(); ++id) {
        int taktSep = taktsFromRoot - nodes[id].taktsFromRoot;
        int stSep = roundint(semitonesFromRoot - nodes[id].semitonesFromRoot);
        if (stSep == 0 && 0 <= taktSep && taktSep < scoreFile.getDurationInTakts(id)) {
            overlapsWithAnother = true;
            break;
        }
    }
    return overlapsWithAnother;
}

bool ScoreEditor::doesNodeRectangleOverlapWithSomeNode(int nodeId, int taktsFromRoot, float semitonesFromRoot, int durationInTakts) const {
    int this_min = taktsFromRoot;
    int this_max = this_min + durationInTakts;
    bool overlapsWithAnother = false;
    for (int id = 0; id < nodes.size(); ++id) {
        if (id == nodeId) continue;
        int stSep = roundint(semitonesFromRoot - nodes[id].semitonesFromRoot);
        int other_min = nodes[id].taktsFromRoot;
        int other_max = other_min + scoreFile.getDurationInTakts(id);
        if (stSep == 0 && (other_min < this_max && this_min < other_max)) {
            overlapsWithAnother = true;
            break;
        }
    }
    return overlapsWithAnother;
}

