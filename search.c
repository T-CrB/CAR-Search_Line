#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "camera.c" // 确保这里包含的是定义了所有图像数组的文件

#define ROWS 60
#define COLS 120

// 声明当前正在处理的图像指针
int (*CurrentImage)[120];

// OutputImage: 1边线, 2中线, 0其他
int OutputImage[ROWS][COLS];

// 点定义
typedef struct POINT {
    int x;
    int y;
} POINT;

// 边缘点
typedef struct EDGE {
    POINT pot[ROWS * 3];
    int endpos;
} EDGE;

POINT findPot;
EDGE LeftEdge, RightEdge;

// 图像颜色判断
bool POINT_BLACK(int y, int x) {
    if (x < 0 || x >= COLS || y < 0 || y >= ROWS)
        return true; // 越界视为黑
    return CurrentImage[y][x] == 0;
}

bool POINT_WHITE(int y, int x) {
    if (x < 0 || x >= COLS || y < 0 || y >= ROWS)
        return false;
    return CurrentImage[y][x] == 1;
}

// 判断是否在图像范围内
bool _iinc(int x, int left, int right) {
    return (x >= left && x < right);
}

// 搜起点函数，找到返回true
bool SEARCH_START(int *start_x, int *start_y, int dir) {
    int IMG_LEFT = 0;
    int IMG_RIGHT = COLS - 1;
    int IMG_BOTTOM = ROWS - 1;
    int stop = ROWS / 2;

    // 先搜左边第一列
    if (dir == 1) {
        for (int r = IMG_BOTTOM - 1; r > stop; --r) {
            if (POINT_BLACK(r, IMG_LEFT) && POINT_BLACK(r - 1, IMG_LEFT) && POINT_BLACK(r - 2, IMG_LEFT) &&
                POINT_BLACK(r - 4, IMG_LEFT) && POINT_BLACK(r - 6, IMG_LEFT)) {
                break;
            }
            if (POINT_WHITE(r, IMG_LEFT) && POINT_BLACK(r - 1, IMG_LEFT) && POINT_BLACK(r - 2, IMG_LEFT) &&
                POINT_BLACK(r - 4, IMG_LEFT) && POINT_BLACK(r - 6, IMG_LEFT)) {
                *start_x = IMG_LEFT;
                *start_y = r;
                return true;
            }
        }
        // 再搜底部最后一列
        for (int c = IMG_LEFT; c < COLS; ++c) {
            if (POINT_WHITE(IMG_BOTTOM, c) && POINT_WHITE(IMG_BOTTOM, c + 1) && POINT_WHITE(IMG_BOTTOM, c + 2) &&
                POINT_WHITE(IMG_BOTTOM, c + 4) && POINT_WHITE(IMG_BOTTOM, c + 6)) {
                break;
            }
            if (POINT_BLACK(IMG_BOTTOM, c) && POINT_WHITE(IMG_BOTTOM, c + 1) && POINT_WHITE(IMG_BOTTOM, c + 2) &&
                POINT_WHITE(IMG_BOTTOM, c + 4) && POINT_WHITE(IMG_BOTTOM, c + 6)) {
                *start_x = c;
                *start_y = IMG_BOTTOM;
                return true;
            }
        }
    }
    // 搜右起点
    else if (dir == -1) {
        // 先搜右边最后一列 (从下往上)
        for (int r = IMG_BOTTOM - 1; r > stop; --r) {
            if (POINT_BLACK(r, IMG_RIGHT) && POINT_BLACK(r - 1, IMG_RIGHT) && POINT_BLACK(r - 2, IMG_RIGHT) &&
                POINT_BLACK(r - 4, IMG_RIGHT) && POINT_BLACK(r - 6, IMG_RIGHT)) {
                break;
            }
            if (POINT_WHITE(r, IMG_RIGHT) && POINT_BLACK(r - 1, IMG_RIGHT) && POINT_BLACK(r - 2, IMG_RIGHT) &&
                POINT_BLACK(r - 4, IMG_RIGHT) && POINT_BLACK(r - 6, IMG_RIGHT)) {
                *start_x = IMG_RIGHT;
                *start_y = r;
                return true;
            }
        }
        // 再搜底部 (从右往左)
        for (int c = IMG_RIGHT; c > COLS / 2; --c) {
            if (POINT_WHITE(IMG_BOTTOM, c) && POINT_WHITE(IMG_BOTTOM, c - 1) && POINT_WHITE(IMG_BOTTOM, c - 2) &&
                POINT_WHITE(IMG_BOTTOM, c - 4) && POINT_WHITE(IMG_BOTTOM, c - 6)) {
                break;
            }
            if (POINT_BLACK(IMG_BOTTOM, c) && POINT_WHITE(IMG_BOTTOM, c - 1) && POINT_WHITE(IMG_BOTTOM, c - 2) &&
                POINT_WHITE(IMG_BOTTOM, c - 4) && POINT_WHITE(IMG_BOTTOM, c - 6)) {
                *start_x = c;
                *start_y = IMG_BOTTOM;
                return true;
            }
        }
    }
    return false;
}

// 九宫格坐标变换
POINT NINESQURED[9] = {
    {.x = 0, .y = 0},  {.x = 0, .y = 1},   {.x = 1, .y = 1},  {.x = 1, .y = 0},  {.x = 1, .y = -1},
    {.x = 0, .y = -1}, {.x = -1, .y = -1}, {.x = -1, .y = 0}, {.x = -1, .y = 1},
};

// 序号迭代
// int NumberIteration[9] = {0, 5, 6, 7, 8, 1, 2, 3, 4}; // 不再使用

void SEARCH_LINE(EDGE *edge, POINT startpot, char dir) {
    int quantity = 300;                  
    int rollpotnum = 1; // 初始方向索引 1-8

    edge->pot[0] = startpot;
    edge->endpos = 1;

    // 1. 初始化起始搜索方向
    // NINESQURED 定义: 1(S), 2(SE), 3(E), 4(NE), 5(N), 6(NW), 7(W), 8(SW)
    if (startpot.y >= ROWS - 2) {
        // 底部起点
        // 左边线：赛道在右，墙在左。我们需要从“墙”开始顺时针扫向“路”。
        // 墙在左(7)或左下(8)。扫向路(上5/右上4)。
        // 初始方向设为 左(7)，然后顺时针扫(7->6->5...)
        if (dir == 'L') rollpotnum = 7; 
        
        // 右边线：赛道在左，墙在右。
        // 墙在右(3)或右下(2)。扫向路(上5/左上6)。
        // 初始方向设为 右(3)，然后逆时针扫(3->4->5...)
        else rollpotnum = 3;
    } else {
        // 侧边起点
        if (dir == 'L') {  // 左侧边起点 (x=0)
            // 墙在左(7). 顺时针扫: 7->6->5...
            rollpotnum = 7;
        } else {           // 右侧边起点 (x=119)
            // 墙在右(3). 逆时针扫: 3->4->5...
            rollpotnum = 3;
        }
    }

    // 2. 循环搜线
    POINT current_pt = startpot;
    
    while (quantity > 0) {
        if (current_pt.y < 5) break; // 到顶了

        bool found_next = false;
        
        // 最多旋转8次找下一个点
        for (int i = 0; i < 8; i++) {
            // 计算探测坐标
            int tx = current_pt.x + NINESQURED[rollpotnum].x;
            int ty = current_pt.y + NINESQURED[rollpotnum].y;
            
            // 检查颜色: 越界视为黑(墙壁)
            bool is_black = !_iinc(tx, 0, COLS) || !_iinc(ty, 0, ROWS) || POINT_BLACK(ty, tx);
            
            if (is_black) {
                // 是墙壁，继续旋转方向寻找路
                // 左边线：顺时针旋转 (索引减小: 5->4->3...)
                // 右边线：逆时针旋转 (索引增大: 5->6->7...)
                if (dir == 'L') {
                    rollpotnum--; 
                    if (rollpotnum < 1) rollpotnum = 8;
                } else {
                    rollpotnum++;
                    if (rollpotnum > 8) rollpotnum = 1;
                }
            } 
            else {
                // 是白点(路)！找到新的边缘点
                edge->pot[edge->endpos].x = tx;
                edge->pot[edge->endpos].y = ty;
                edge->endpos++;
                
                current_pt.x = tx;
                current_pt.y = ty;
                quantity--;
                found_next = true;
                
                // 【关键回弹】：为了贴墙走，必须把方向旋转回墙壁一侧
                // 左边线(顺时针搜): 此时在路，需逆时针转回墙壁方向 (idx增加)
                // 回转幅度：2格(90度) 到 3格(135度)。针对直角弯推荐3格。
                if (dir == 'L') {
                    rollpotnum += 3; 
                    while(rollpotnum > 8) rollpotnum -= 8;
                } else {
                    // 右边线(逆时针搜): 此时在路，需顺时针转回墙壁方向 (idx减小)
                    rollpotnum -= 3;
                    while(rollpotnum < 1) rollpotnum += 8;
                }
                
                break; // 找到这步的点后，跳出内层旋转循环，去搜下一步
            }
        }
        
        if (!found_next) {
            // 死路或孤岛
            break;
        }
    }
}

// 简单的线性插值计算每行的理论赛道半宽
int GetTrackHalfWidth(int row) {
    // 基础透视模型：底部宽，顶部窄
    float k = (45.0f - 15.0f) / ((float)ROWS - 1.0f);
    return (int)(15.0f + k * row); 
}

void RENDER_OUTPUT() {
    // 1. 初始化OutputImage
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            OutputImage[i][j] = 0;

    int lineL[ROWS];
    int lineR[ROWS];
    // 初始化为特殊值 (-1 和 COLS)
    for (int i = 0; i < ROWS; i++) {
        lineL[i] = -1;
        lineR[i] = COLS;
    }

    // 2. 填充边线 并 记录坐标
    // 过滤底部噪点，并取最内侧点
    for (int i = 0; i < LeftEdge.endpos; i++) {
        int y = LeftEdge.pot[i].y;
        int x = LeftEdge.pot[i].x;
        if (y >= 0 && y < ROWS && x >= 0 && x < COLS) {
            if (y == ROWS - 1 && i != 0) continue;
            OutputImage[y][x] = 1;
            if (x > lineL[y]) lineL[y] = x;
        }
    }
    for (int i = 0; i < RightEdge.endpos; i++) {
        int y = RightEdge.pot[i].y;
        int x = RightEdge.pot[i].x;
        if (y >= 0 && y < ROWS && x >= 0 && x < COLS) {
            if (y == ROWS - 1 && i != 0) continue;
            OutputImage[y][x] = 1;
            if (x < lineR[y]) lineR[y] = x;
        }
    }

    // 3. 计算中线 - 回归基础稳健版
    // 核心逻辑：
    // 1. 既然是直道，绝大部分时候都有左右边线。这种情况直接 (L+R)/2，不要搞花哨的。
    // 2. 遇到单边丢失，才用"半宽"补。
    // 3. 只有遇到真正的突然断裂，才用斜率补。

    float current_center = COLS / 2.0f;
    int start_row = -1;

    // 3.1 寻找最底部的稳固起点
    // 我们找连续两行都存在的点，确保稳定
    for (int i = ROWS - 1; i >= 1; i--) {
        if (lineL[i] != -1 && lineR[i] != COLS && 
            lineL[i-1] != -1 && lineR[i-1] != COLS) {
            
            start_row = i;
            current_center = (float)(lineL[i] + lineR[i]) / 2.0f;
            break; 
        }
    }
    
    // 降级：如果找不到连续两行，找任意一行
    if (start_row == -1) {
         for (int i = ROWS - 1; i >= 0; i--) {
            if (lineL[i] != -1 || lineR[i] != COLS) {
                start_row = i;
                int hw = GetTrackHalfWidth(i);
                if (lineL[i] != -1 && lineR[i] != COLS) current_center = (lineL[i] + lineR[i]) / 2.0f;
                else if (lineL[i] != -1) current_center = lineL[i] + hw;
                else current_center = lineR[i] - hw;
                break;
            }
         }
    }

    float slope = 0.0f; // 初始斜率设为0 (默认直行)

    // 3.2 循环计算
    for (int i = ROWS - 1; i >= 0; i--) {
        // 底部回填
        if (i > start_row) {
            int fill = (int)current_center;
            if(OutputImage[i][fill] != 1) OutputImage[i][fill] = 2;
            continue;
        }

        int hw = GetTrackHalfWidth(i);
        float measured_center = -1.0f;
        
        bool valid_L = (lineL[i] != -1);
        bool valid_R = (lineR[i] != COLS);
        
        bool is_stable = false; // 标记这一行是否"稳"

        if (valid_L && valid_R) {
            // 双边都在
            int width = lineR[i] - lineL[i];
            
            // 简单判断宽度合理性 (20 ~ 100)
            if (width > 20 && width < 110) {
                measured_center = (lineL[i] + lineR[i]) / 2.0f;
                is_stable = true; // 双边都在且宽度正常，这是最稳的
                
                // 环岛特判：如果宽度突然比上一行大很多(>15)，可能进环岛了
                // 但对于直道，不会触发这个
            } else {
                // 宽度异常，取离 current_center 近的那边
                float cL = lineL[i] + hw;
                float cR = lineR[i] - hw;
                if (fabs(cL - current_center) < fabs(cR - current_center)) 
                    measured_center = cL;
                else 
                    measured_center = cR;
            }
        } 
        else if (valid_L) {
            measured_center = lineL[i] + hw;
        } 
        else if (valid_R) {
            measured_center = lineR[i] - hw;
        } 
        else {
            // 全丢，纯盲跑
            measured_center = current_center + slope;
        }

        // 滤波融合
        // 如果是 is_stable (双边直道)，我们要重重地信任它 (0.8 ~ 0.9)，把之前的错误斜率纠正回来
        // 如果是单边或异常，我们稍微多信任一点惯性 (0.3 ~ 0.5)
        
        float alpha = is_stable ? 0.9f : 0.4f;
        
        // 预测位置
        float predicted = current_center + slope;
        
        // 更新位置
        float new_center = alpha * measured_center + (1.0f - alpha) * predicted;
        
        // 更新斜率
        // 在直道上，斜率更新要快，这样才能迅速归中
        float new_slope = 0.6f * slope + 0.4f * (new_center - current_center);
        
        // 限制斜率 (防止发散)
        if (new_slope > 2.0f) new_slope = 2.0f;
        if (new_slope < -2.0f) new_slope = -2.0f;
        
        current_center = new_center;
        slope = new_slope;

        // 绘制
        int draw_x = (int)current_center;
        if (draw_x < 0) draw_x = 0; 
        if (draw_x >= COLS) draw_x = COLS - 1;
        
        if (OutputImage[i][draw_x] != 1) {
            OutputImage[i][draw_x] = 2;
        }
    }
}

// 单次处理并写入函数
void ProcessAndWrite(const char *name, int (*img)[120], FILE *fp) {
    // 设置当前图像
    CurrentImage = img;

    // 重置全局变量
    LeftEdge.endpos = 0;
    RightEdge.endpos = 0;

    // 1. 搜左边线
    POINT startL;
    if (SEARCH_START(&startL.x, &startL.y, 1)) {
        SEARCH_LINE(&LeftEdge, startL, 'L'); 
    }

    // 2. 搜右边线
    POINT startR;
    if (SEARCH_START(&startR.x, &startR.y, -1)) {
        SEARCH_LINE(&RightEdge, startR, 'R'); 
    }

    // 3. 生成结果
    RENDER_OUTPUT();

    // 4. 写入文件
    fprintf(fp, "=== Processing %s ===\n", name);
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            int mark = OutputImage[i][j];
            bool is_track_white = POINT_WHITE(i, j);

            if (mark == 2) {
                fprintf(fp, "2 ");
            } else if (mark == 1) {
                fprintf(fp, "1 ");
            } else if (is_track_white) {
                fprintf(fp, "  ");
            } else {
                fprintf(fp, "0 ");
            }
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n\n");
}

int main() {
    FILE *fp = fopen("output.txt", "w");
    if (fp == NULL) {
        printf("Error opening output.txt!\n");
        return 1;
    }

    ProcessAndWrite("Image1", Image1, fp);
    ProcessAndWrite("Image2", Image2, fp);
    ProcessAndWrite("Image3", Image3, fp);
    ProcessAndWrite("VIDEO1", VIDEO1_120_60_458, fp);
    ProcessAndWrite("Island_1", Island_1, fp);
    ProcessAndWrite("Island_2", Island_2, fp);
    ProcessAndWrite("Island_3", Island_3, fp);

    fclose(fp);
    printf("\nAll data processed and saved to output.txt\n");
    return 0;
}
