#include<bits/stdc++.h>
#include "raylib.h"

using namespace std;

int ran(int x, int y) {//AI
    static mt19937 gen(static_cast<uint32_t>(
        chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count()
    ));
    uniform_int_distribution<int> dist(x, y);
    return dist(gen);
}

struct notes{
    double x = 100;
    double y = -16;
    int lane = 1;
};
vector<notes> notev;

int main(){
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(512, 512, "gu gu ga ga!");
    SetTargetFPS(60);
    Texture2D notet = LoadTexture("assets/textures/note.png");
    Texture2D Dline = LoadTexture("assets/textures/Dline.png");

    long double noteset = 0.5;
    long double noted = 0.0019531;

    while(!WindowShouldClose()){

        long double dt = GetFrameTime();
        noteset -= dt;
        if(noteset <= 0){
            noteset += 0.5;
            notes p;
            p.lane = ran(1, 4);
            p.x = p.lane * 64 + 64;
            notev.push_back(p);
        }
        int dpx = 0;
        noted -= dt;
        while(noted <= 0){
            dpx++;
            noted += 0.0019531;
        }
        for(int i = 0;i < notev.size();i++){
            if(notev[i].y > 550){
                notev.erase(notev.begin() + i);
                i--;
            }else{
                notev[i].y += dpx;
            }
        }

        const int keyList[4] = { KEY_D, KEY_F, KEY_J, KEY_K };
        for(int lane = 1; lane <= 4; lane++){
            if(IsKeyPressed(keyList[lane - 1])){   // 仅按下瞬间触发，长按不会连续命中
                int best = -1;
                long double bestDt = 0.2;
                for(int i = 0;i < notev.size();i++){
                    if(notev[i].lane != lane) continue;
                    long double toLine = (448.0 - notev[i].y) * 0.0019531; // 剩余到线时间
                    long double dtAbs = fabsl(toLine);
                    if(dtAbs < bestDt){ bestDt = dtAbs; best = i; }
                }
                if(best >= 0) notev.erase(notev.begin() + best); // 每次按键至多删除一个note
            }
        }


        BeginDrawing();
        ClearBackground(BLACK);
        BeginBlendMode(BLEND_ALPHA);
        //---
        
        Color btmWHITE = ColorAlpha(WHITE, 0.5f);
        DrawLine(128, 0, 128, 512, btmWHITE);
        DrawLine(192, 0, 192, 512, btmWHITE);
        DrawLine(256, 0, 256, 512, btmWHITE);
        DrawLine(320, 0, 320, 512, btmWHITE);
        DrawLine(384, 0, 384, 512, btmWHITE);
        DrawTexture(Dline, 128, 448, WHITE);

        for(const auto& i : notev){
            DrawTexture(notet, i.x, i.y, WHITE);
        }

        //---
        EndBlendMode();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}