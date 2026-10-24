#include<bits/stdc++.h>
#include "raylib.h"

using namespace std;

// 关卡选择界面：整块滚动栏是一个向右倾斜的大平行四边形，内部 7 格无缝隙铺满，
// 选中项居中高亮，上下循环、斜向滚动（缓动动画）
// 返回选中的完整路径；用户关闭窗口时返回空串
string levelSelect(const vector<string>& files){
    if(files.empty()) return "";
    const int N = (int)files.size();
    const int VISIBLE = 7;                                   // 竖向可见格数
    const float REPEAT_INTERVAL = 1.0f / 7.0f;               // 长按连滚最快 7 格/秒
    const float H = 512.0f / VISIBLE;                        // 每格高度：7 格正好铺满整个窗口高度
    const float W = 232.0f;                                  // 每格宽度
    const float K = 80.0f;                                   // 整块面板的水平倾斜量
    const float XB = 40.0f;                                  // 面板底边左端 x
    const float panelBottom = 512.0f;                        // 面板下沿 = 窗口底部（上沿 = 0）
    const float s = K / (VISIBLE * H);                       // 倾斜斜率（每像素横向偏移）

    auto xLeft = [&](float y){ return XB + (panelBottom - y) * s; };   // 某高度处面板左边界

    int sel = 0;
    float pos = 0.0f;      // 连续滚动位置（渲染用，可为任意实数）
    float vel = 0.0f;      // 弹簧速度（丝滑滚动用）
    float repeatT = -1.0f; // 长按连滚计时：<0 表示当前没有按住

    auto levelName = [](string s2) -> string {
        string::size_type p = s2.find_last_of("/\\");
        if(p != string::npos) s2 = s2.substr(p + 1);
        if(s2.size() > 6 && s2.substr(s2.size() - 6) == ".cgame") s2 = s2.substr(0, s2.size() - 6);
        return s2;
    };

    // 收集需要渲染的字符：ASCII 全部 + 文件名里的所有字符（支持中文文件名）
    set<int> cpSet;
    for(int c = 32; c <= 126; c++) cpSet.insert(c);
    for(const auto& f : files){
        const unsigned char* p = (const unsigned char*)f.c_str();
        while(*p){
            int cp;
            if(*p < 0x80) { cp = *p; p += 1; }
            else if((*p >> 5) == 0x06) { cp = ((*p & 0x1F) << 6) | (p[1] & 0x3F); p += 2; }
            else if((*p >> 4) == 0x0E) { cp = ((*p & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); p += 3; }
            else if((*p >> 3) == 0x1E) { cp = ((*p & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F); p += 4; }
            else { p += 1; continue; }
            cpSet.insert(cp);
        }
    }
    vector<int> cps(cpSet.begin(), cpSet.end());
    Font font = LoadFontEx("C:/Windows/Fonts/simhei.ttf", 32, cps.data(), (int)cps.size());
    bool customFont = IsFontValid(font);
    if(!customFont) font = GetFontDefault();   // 加载失败则退回默认字体（退出时不能卸载它）

    while(!WindowShouldClose()){
        float dt = GetFrameTime();
        if(dt > 0.05f) dt = 0.05f;   // 卡顿时限制步长，避免弹簧失稳

        // 长按连滚：按键瞬间走一格，之后停 0.35s，再每 1/7 秒滚一格（最快 7 格/秒）
        bool heldDown = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
        bool heldUp   = IsKeyDown(KEY_UP)   || IsKeyDown(KEY_W);
        if(IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)){
            sel = (sel - 1 + N) % N;
            repeatT = 0.35f;
        }else if(IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)){
            sel = (sel + 1) % N;
            repeatT = 0.35f;
        }else if(heldDown || heldUp){
            if(repeatT >= 0.0f){
                repeatT -= dt;
                if(repeatT <= 0.0f){
                    repeatT += REPEAT_INTERVAL;
                    if(heldDown) sel = (sel + 1) % N;
                    else sel = (sel - 1 + N) % N;
                }
            }
        }else{
            repeatT = -1.0f;
        }

        if(IsKeyPressed(KEY_ENTER)){
            if(customFont) UnloadFont(font);
            return files[sel];
        }

        // 丝滑滚动：临界阻尼弹簧，朝目标平滑滑动（加速→减速→柔和吸附），循环边界走最短方向
        float diff = (float)sel - pos;
        while(diff > N / 2.0f) diff -= (float)N;
        while(diff < -N / 2.0f) diff += (float)N;
        const float w = 15.0f;                             // 弹簧角频率（越大越跟手）
        vel += (-2.0f * w * vel + w * w * diff) * dt;
        pos += vel * dt;
        if(fabsf(diff) < 0.0025f && fabsf(vel) < 0.02f){   // 收敛后吸附对齐
            pos = (float)sel;
            vel = 0.0f;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        int basePos = (int)floorf(pos);

        // 先找出离屏幕中心最近的“选中项”实例（循环列表里可能出现多次）
        int bestK = 0;
        float bestAbs = 1e9f;
        for(int k = basePos - 5; k <= basePos + 5; k++){
            if((((k % N) + N) % N) != sel) continue;
            float off = (float)k - pos;
            if(fabsf(off) < bestAbs){ bestAbs = fabsf(off); bestK = k; }
        }

        // 滚动栏底色：整块大平行四边形
        Vector2 pBL = { xLeft(panelBottom), panelBottom };
        Vector2 pBR = { pBL.x + W, panelBottom };
        Vector2 pTR = { xLeft(panelBottom - VISIBLE * H) + W, panelBottom - VISIBLE * H };
        Vector2 pTL = { pTR.x - W, pTR.y };
        DrawTriangle(pBL, pBR, pTR, ColorAlpha(WHITE, 0.03f));
        DrawTriangle(pBL, pTR, pTL, ColorAlpha(WHITE, 0.03f));

        // 裁剪到面板范围，内部 7 格无缝隙铺满，随滚动斜向滑动
        BeginScissorMode((int)XB, (int)(panelBottom - VISIBLE * H), (int)(W + K), (int)(VISIBLE * H));
        for(int k = basePos - 5; k <= basePos + 5; k++){
            float off = (float)k - pos;
            if(fabsf(off) > 4.5f) continue;
            int idx = ((k % N) + N) % N;
            float yc = 256.0f + off * H;
            if(yc + H / 2 < panelBottom - VISIBLE * H || yc - H / 2 > panelBottom) continue;

            float y0 = yc - H / 2, y1 = yc + H / 2;
            float xl0 = xLeft(y0), xl1 = xLeft(y1);
            Vector2 BL = { xl1, y1 }, BR = { xl1 + W, y1 }, TR = { xl0 + W, y0 }, TL = { xl0, y0 };
            bool chosen = (k == bestK);
            Color fillC = chosen ? ColorAlpha(SKYBLUE, 0.25f) : ColorAlpha(WHITE, 0.05f);
            Color lineC = chosen ? SKYBLUE : ColorAlpha(WHITE, 0.3f);
            float thick = chosen ? 3.0f : 2.0f;

            DrawTriangle(BL, BR, TR, fillC);
            DrawTriangle(BL, TR, TL, fillC);
            DrawLineEx(BL, BR, thick, lineC);
            DrawLineEx(BR, TR, thick, lineC);
            DrawLineEx(TR, TL, thick, lineC);
            DrawLineEx(TL, BL, thick, lineC);

            // 文件名（不含 .cgame），按宽度自动缩小
            string nmC = levelName(files[idx]);
            float fsC = 26.0f;
            Vector2 tm = MeasureTextEx(font, nmC.c_str(), fsC, 1);
            if(tm.x > W - 30){ fsC *= (W - 30) / tm.x; tm = MeasureTextEx(font, nmC.c_str(), fsC, 1); }
            DrawTextEx(font, nmC.c_str(), { xLeft(yc) + W / 2 - tm.x / 2, yc - tm.y / 2 }, fsC, 1,
                       chosen ? SKYBLUE : WHITE);
        }
        EndScissorMode();

        // 面板外框
        DrawLineEx(pBL, pBR, 2.0f, ColorAlpha(WHITE, 0.45f));
        DrawLineEx(pBR, pTR, 2.0f, ColorAlpha(WHITE, 0.45f));
        DrawLineEx(pTR, pTL, 2.0f, ColorAlpha(WHITE, 0.45f));
        DrawLineEx(pTL, pBL, 2.0f, ColorAlpha(WHITE, 0.45f));

        // 右侧上方：当前选中关卡的文件名（不含 .cgame）
        string nm = levelName(files[sel]);
        float fsL = 26.0f;
        Vector2 lm = MeasureTextEx(font, nm.c_str(), fsL, 1);
        if(lm.x > 144.0f){ fsL *= 144.0f / lm.x; lm = MeasureTextEx(font, nm.c_str(), fsL, 1); }
        DrawTextEx(font, nm.c_str(), { 360, 60 - lm.y / 2 }, fsL, 1, SKYBLUE);

        // 右侧中间：播放三角 + START
        DrawTriangle({ 340, 200 }, { 340, 285 }, { 400, 242 }, SKYBLUE);
        const char* start = "START";
        Vector2 sm = MeasureTextEx(font, start, 24, 1);
        DrawTextEx(font, start, { 512 - sm.x - 12, 242 - sm.y / 2 }, 24, 1, SKYBLUE);

        EndDrawing();
    }
    if(customFont) UnloadFont(font);
    return "";
}
