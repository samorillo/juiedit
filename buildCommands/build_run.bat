::g++ -std=c++2a -m64 "src/main.cpp" -o _builds/program.exe
g++ -std=c++2a -m64 "src/main.cpp" -o _builds/juiedit.exe -Lexternal\SDL2\lib\x64 -lSDL2main -lSDL2 -Lexternal\SDL_ttf\lib -lSDL2_ttf -Iexternal\SDL2\include
@echo off
if %errorlevel%==0 (
    "_builds/juiedit.exe" open demos\demo1.json
) else (
    echo "COMPILATION FAILED"
)
@echo on