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
    long double step = 0.0019531;  // 每下落1像素所需秒数（文件中的"下落速度"，默认与原版一致）
    long double spawnMs = 0;       // 文件模式：顶部出现时间（ms，由“落到判定线的时间”换算，可为负）
};
vector<notes> notev;
bool playS = 1;

vector<notes> levelv;              // 文件模式：启动时一次性读入的全部note
bool fileMode = false;             // 是否使用 level.cgame 谱面
bool over = false;                 // 谱面是否已播放完毕
long double overT = 0;             // OVER 画面持续时间

struct tapeff{
    bool active = false;   // 该轨道是否正在显示特效
    long double life = 0;  // 剩余显示时间
};
tapeff tapev[4];           // 每个轨道至多一个特效

string levelSelect(const vector<string>& files);   // 关卡选择界面（在 re.cpp 中实现）

int main(){
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(512, 512, "gu gu ga ga!");
    SetTargetFPS(60);
    Texture2D notet = LoadTexture("assets/textures/note.png");
    Texture2D Dline = LoadTexture("assets/textures/Dline.png");
    Texture2D taped = LoadTexture("assets/textures/taped.png");

    InitAudioDevice();
    Sound ding = { 0 };
    if(FileExists("assets/sounds/ding.wav")) ding = LoadSound("assets/sounds/ding.wav");

    long double noteset = 0.5;
    long double gameTime = 0;      // 自程序开始运行起经过的时间（秒）

    Music music = { 0 };
    bool hasMusic = false;

    // 扫描 assets/level 下的所有 .cgame 谱面
    vector<string> chartFiles;
    FilePathList fl = LoadDirectoryFilesEx("assets/level", ".cgame", false);
    for(unsigned int i = 0;i < fl.count;i++) chartFiles.push_back(fl.paths[i]);
    UnloadDirectoryFiles(fl);
    sort(chartFiles.begin(), chartFiles.end());

    fileMode = !chartFiles.empty();
    string chartPath;
    if(chartFiles.size() == 1){
        chartPath = chartFiles[0];                        // 只有一个谱面：直接播放
    }else if(chartFiles.size() > 1){
        chartPath = levelSelect(chartFiles);              // 多个谱面：显示关卡选择界面
        if(chartPath.empty()){                            // 选择界面被关闭
            CloseAudioDevice();
            CloseWindow();
            return 0;
        }
    }
    if(fileMode){                  // 有谱面文件：一次性读入全部note并按下落时间排序
        ifstream fin(chartPath);
        string line;
        while(getline(fin, line)){
            stringstream ss(line);
            string first;
            ss >> first;
            if(first == "music"){                       // 音乐声明行：music <名称>（与谱面同目录）
                string mname;
                ss >> mname;
                string mpath;
                if(mname.find('.') == string::npos) mpath = "assets/level/" + mname + ".wav";
                else mpath = "assets/level/" + mname;
                if(FileExists(mpath.c_str())){
                    music = LoadMusicStream(mpath.c_str());
                    hasMusic = true;
                }
            }else{                                      // note行：轨道 判定时间(ms，落到判定线的时间) 下落速度
                int lane;
                long double tMs, st;
                ss.str(line);
                ss.clear();
                if(ss >> lane >> tMs >> st && lane >= 1 && lane <= 4){   // 跳过非法行
                    notes p;
                    p.lane = lane;
                    p.x = lane * 64 + 64;
                    p.step = st;
                    p.spawnMs = tMs - 464.0 * p.step * 1000.0;  // 换算成顶部出现时间（可为负=开局已在下落）
                    levelv.push_back(p);
                }
            }
        }
        sort(levelv.begin(), levelv.end(), [](const notes& a, const notes& b){
            return a.spawnMs < b.spawnMs;
        });
        if(hasMusic) PlayMusicStream(music);   // 音乐与note时间同一基准，程序开始即播放（不循环）
    }

    while(!WindowShouldClose()){

        long double dt = GetFrameTime();
        gameTime += dt;
        if(hasMusic) UpdateMusicStream(music);   // 音乐流每帧更新（放完自动停止，不循环）

        if(!fileMode){             // 没有谱面文件：维持原随机生成逻辑
            noteset -= dt;
            if(noteset <= 0){
                noteset += 0.5;
                notes p;
                p.lane = ran(1, 4);
                p.x = p.lane * 64 + 64;
                notev.push_back(p);
            }
        }else if(!over){           // 文件模式：到达顶部出现时间后把note放进notev
            while(!levelv.empty() && gameTime * 1000 >= levelv.front().spawnMs){
                notes p = levelv.front();
                p.y = -16.0 + (gameTime * 1000.0 - p.spawnMs) / 1000.0 / p.step;  // 出现时间早于开局则按已下落距离摆放
                notev.push_back(p);
                levelv.erase(levelv.begin());
            }
        }

        for(int i = 0;i < notev.size();i++){
            if(notev[i].y > 550){
                notev.erase(notev.begin() + i);
                i--;
            }else{
                notev[i].y += dt / notev[i].step;   // 每个note按自己的速度下落
            }
        }

        const int keyList[4] = { KEY_D, KEY_F, KEY_J, KEY_K };
        if(IsKeyPressed(KEY_S)) playS = !playS;   // 按 S 键切换音效开关
        for(int lane = 1; lane <= 4; lane++){
            if(IsKeyPressed(keyList[lane - 1])){   // 仅按下瞬间触发，长按不会连续命中
                int best = -1;
                long double bestDt = 0.2;
                for(int i = 0;i < notev.size();i++){
                    if(notev[i].lane != lane) continue;
                    long double toLine = (448.0 - notev[i].y) * notev[i].step; // 剩余到线时间
                    long double dtAbs = fabsl(toLine);
                    if(dtAbs < bestDt){ bestDt = dtAbs; best = i; }
                }
                if(best >= 0){
                    notev.erase(notev.begin() + best); // 每次按键至多删除一个note
                    tapev[lane - 1].active = true;     // 显示击中特效，若该轨道已有则覆盖（相当于删除原来那个）
                    tapev[lane - 1].life = 0.5;
                    if(playS) PlaySound(ding);         // 显示特效的同时，playS 为 1 则播放音效
                }
            }
        }


        for(int i = 0;i < 4;i++){          // 特效剩余时间递减，归零后删除
            if(tapev[i].active){
                tapev[i].life -= dt;
                if(tapev[i].life <= 0) tapev[i].active = false;
            }
        }

        // 谱面播放完毕且音乐也结束（或没有音乐）才进入OVER：音乐先完就静音等谱面，反之亦然
        if(fileMode && !over && levelv.empty() && notev.empty() &&
           (!hasMusic || !IsMusicStreamPlaying(music))){
            over = true;
            overT = 0;
        }
        if(over) overT += dt;

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

        for(int i = 0;i < 4;i++){          // 绘制击中特效：中心对齐判定线行的中间，透明度由1均匀降到0
            if(tapev[i].active){
                long double alpha = tapev[i].life / 0.5;
                if(alpha > 1) alpha = 1;
                if(alpha < 0) alpha = 0;
                Color tapC = ColorAlpha(WHITE, (float)alpha);
                int laneX = (i + 1) * 64 + 64;
                DrawTexture(taped,
                    laneX + notet.width / 2 - taped.width / 2,
                    448 + Dline.height / 2 - taped.height / 2,
                    tapC);
            }
        }

        if(over){                          // 结束：先用0.2秒渐渐变暗，再闪烁三次OVER，之后常亮
            long double fade = overT / 0.2;
            if(fade > 1) fade = 1;
            if(fade < 1){
                DrawRectangle(0, 0, 512, 512, ColorAlpha(BLACK, (float)fade));  // 黑幕渐显
            }else{
                long double bt = overT - 0.2;                                   // 闪烁阶段
                bool vis = true;
                if(bt < 1.5) vis = (fmodl(bt, 0.5) < 0.25);                     // 0.25s亮/0.25s灭，共三次（2倍速）
                if(vis) DrawText("OVER", 256 - MeasureText("OVER", 64) / 2, 256 - 32, 64, WHITE);
            }
        }

        //---
        EndBlendMode();
        EndDrawing();
    }
    if(hasMusic) UnloadMusicStream(music);
    UnloadSound(ding);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}