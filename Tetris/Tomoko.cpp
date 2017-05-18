/*
*	Tomoko_Chan
*	Botzone游戏Tetris智能Bot解决方案
*	版本号05141933
*/

#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <vector>
#include <queue>
#include <random>
#include <chrono>
#include <memory>
using namespace std;

//#define DEBUG
//#define DEBUG_4

#define MAPWIDTH 10
#define MAPHEIGHT 20
#define MAXI 2100000000

// 我所在队伍的颜色（0为红，1为蓝，仅表示队伍，不分先后）
int currBotColor;
int enemyColor;

// 先y后x，记录地图状态，0为空，1为以前放置，2为刚刚放置，负数为越界
// （2用于在清行后将最后一步撤销再送给对方）
int gridInfo[2][MAPHEIGHT + 2][MAPWIDTH + 2] = { 0 };

// 代表分别向对方转移的行
int trans[2][4][MAPWIDTH + 2] = { 0 };

// 转移行数
int transCount[2] = { 0 };

// 运行eliminate后的当前高度
int maxHeight[2] = { 0 };

// 总消去行数的分数之和
int elimTotal[2] = { 0 };

// 一次性消去行数对应分数
const int elimBonus[] = { 0, 1, 3, 5, 7 };

// 给对应玩家的各类块的数目总计
int typeCountForColor[2][7] = { 0 };

const int blockShape[7][4][8] = {
	{ { 0,0,1,0,-1,0,-1,-1 },{ 0,0,0,1,0,-1,1,-1 },{ 0,0,-1,0,1,0,1,1 },{ 0,0,0,-1,0,1,-1,1 } },
	{ { 0,0,-1,0,1,0,1,-1 },{ 0,0,0,-1,0,1,1,1 },{ 0,0,1,0,-1,0,-1,1 },{ 0,0,0,1,0,-1,-1,-1 } },
	{ { 0,0,1,0,0,-1,-1,-1 },{ 0,0,0,1,1,0,1,-1 },{ 0,0,-1,0,0,1,1,1 },{ 0,0,0,-1,-1,0,-1,1 } },
	{ { 0,0,-1,0,0,-1,1,-1 },{ 0,0,0,-1,1,0,1,1 },{ 0,0,1,0,0,1,-1,1 },{ 0,0,0,1,-1,0,-1,-1 } },
	{ { 0,0,-1,0,0,1,1,0 },{ 0,0,0,-1,-1,0,0,1 },{ 0,0,1,0,0,-1,-1,0 },{ 0,0,0,1,1,0,0,-1 } },
	{ { 0,0,0,-1,0,1,0,2 },{ 0,0,1,0,-1,0,-2,0 },{ 0,0,0,1,0,-1,0,-2 },{ 0,0,-1,0,1,0,2,0 } },
	{ { 0,0,0,1,-1,0,-1,1 },{ 0,0,-1,0,0,-1,-1,-1 },{ 0,0,0,-1,1,-0,1,-1 },{ 0,0,1,0,0,1,1,1 } }
};// 7种形状(长L| 短L| 反z| 正z| T| 直一| 田格)，4种朝向(上左下右)，8:每相邻的两个分别为x，y

  // 判断当前位置是否合法
inline bool is_valid(const void* _grid, int blockType, int x, int y, int o) {
	if (o < 0 || o > 3)
		return false;
	int *int_grid = (int*)_grid;
	int tmpX, tmpY;
	for (int i = 0; i < 4; i++) {
		tmpX = x + blockShape[blockType][o][2 * i + 1];
		tmpY = y + blockShape[blockType][o][2 * i];
		if (tmpX < 1 || tmpX > MAPHEIGHT ||
			tmpY < 1 || tmpY > MAPWIDTH ||
			*(int_grid + tmpX * (MAPWIDTH + 2) + tmpY) != 0)
			return false;
	}
	return true;
}

inline bool check_drop(const void* _grid, int blockType, int x, int y, int o) {
	const int *def = blockShape[blockType][o];
	int *int_grid = (int*)_grid;
	for (; x <= MAPHEIGHT; ++x)
		for (int i = 0; i < 4; ++i) {
			int _x = def[i * 2 + 1] + x;
			int _y = def[i * 2] + y;
			if (_x > MAPHEIGHT)
				continue;
			if (_x < 1 || _y < 1 || _y > MAPWIDTH || *(int_grid + _x * (MAPWIDTH + 2) + _y) != 0)
				return false;
		}
	return true;
}

inline bool on_ground(const void *_grid, int block_type, int x, int y, int o) {
	return !is_valid(_grid, block_type, x - 1, y, o);
}

inline void clear_rows(const void *_grid) {
	int *int_grid = (int*)_grid;
	for (int i = 1; i <= MAPHEIGHT; ++i) {
		bool p = true;
		for (int j = 1; j <= MAPWIDTH; ++j)
			if (*(int_grid + i * (MAPWIDTH + 2) + j) == 0)
				p = false;
		if (p) {
			for (int k = i; k < MAPHEIGHT; ++k)
				for (int j = 1; j <= MAPWIDTH; ++j)
					*(int_grid + k * (MAPWIDTH + 2) + j) = *(int_grid + (k + 1) * (MAPWIDTH + 2) + j);
			for (int j = 1; j <= MAPWIDTH; ++j)
				*(int_grid + MAPHEIGHT * (MAPWIDTH + 2) + j) = 0;
		}
	}
}

class Tetris {
public:
	int blockType;   // 标记方块类型的序号 0~6
	int blockX;            // 旋转中心的x轴坐标
	int blockY;            // 旋转中心的y轴坐标
	int orientation;       // 标记方块的朝向 0~3

	int color;

	Tetris(int t, int color) : blockType(t), color(color) {}
	Tetris(int t) : blockType(t), color(-1) {}
	Tetris(const Tetris &t)
		: blockType(t.blockType), blockX(t.blockX), blockY(t.blockY),
		orientation(t.orientation), color(t.color) {}

	inline Tetris &set(int x = -1, int y = -1, int o = -1) {
		blockX = x == -1 ? blockX : x;
		blockY = y == -1 ? blockY : y;
		orientation = o == -1 ? orientation : o;
		return *this;
	}

	inline bool isValid(int x = -1, int y = -1, int o = -1) {
		x = (x == -1) ? blockY : x;
		y = (y == -1) ? blockX : y;
		o = (o == -1) ? orientation : o;
		return is_valid(gridInfo[color], blockType, y, x, o);
	}

	// 判断是否落地
	inline bool onGround() {
		if (isValid() && !isValid(-1, blockY - 1))
			return true;
		return false;
	}

	// 将方块放置在场地上
	inline bool place() {

		int i, tmpX, tmpY;
		for (i = 0; i < 4; i++) {
			tmpX = blockX + blockShape[blockType][orientation][2 * i];
			tmpY = blockY + blockShape[blockType][orientation][2 * i + 1];
			gridInfo[color][tmpY][tmpX] = 2;
		}
		return true;
	}

	// 检查能否逆时针旋转自己到o
	inline bool rotation(int o) {
		if (o < 0 || o > 3)
			return false;

		if (orientation == o)
			return true;

		int fromO = orientation;
		while (true) {
			if (!isValid(-1, -1, fromO))
				return false;

			if (fromO == o)
				break;

			fromO = (fromO + 1) % 4;
		}
		return true;
	}
};

// 围一圈护城河
void init() {
	int i;
	for (i = 0; i < MAPHEIGHT + 2; i++) {
		gridInfo[1][i][0] = gridInfo[1][i][MAPWIDTH + 1] = -2;
		gridInfo[0][i][0] = gridInfo[0][i][MAPWIDTH + 1] = -2;
	}
	for (i = 0; i < MAPWIDTH + 2; i++) {
		gridInfo[1][0][i] = gridInfo[1][MAPHEIGHT + 1][i] = -2;
		gridInfo[0][0][i] = gridInfo[0][MAPHEIGHT + 1][i] = -2;
	}
}

namespace Util {

	// 检查能否从场地顶端直接落到当前位置
	inline bool checkDirectDropTo(int color, int blockType, int x, int y, int o) {
		return check_drop(gridInfo[color], blockType, y, x, o);
	}

	// 消去行
	void eliminate(int color) {
		int &count = transCount[color] = 0;
		int i, j, emptyFlag, fullFlag;
		maxHeight[color] = MAPHEIGHT;
		for (i = 1; i <= MAPHEIGHT; i++) {
			emptyFlag = 1;
			fullFlag = 1;
			for (j = 1; j <= MAPWIDTH; j++) {
				if (gridInfo[color][i][j] == 0)
					fullFlag = 0;
				else
					emptyFlag = 0;
			}
			if (fullFlag) {
				for (j = 1; j <= MAPWIDTH; j++) {
					// 注意这里只转移以前的块，不包括最后一次落下的块（“撤销最后一步”）
					trans[color][count][j] = gridInfo[color][i][j] == 1 ? 1 : 0;
					gridInfo[color][i][j] = 0;
				}
				count++;
			} else if (emptyFlag) {
				maxHeight[color] = i - 1;
				break;
			} else
				for (j = 1; j <= MAPWIDTH; j++) {
					gridInfo[color][i - count][j] =
						gridInfo[color][i][j] > 0 ? 1 : gridInfo[color][i][j];
					if (count)
						gridInfo[color][i][j] = 0;
				}
		}
		maxHeight[color] -= count;
		elimTotal[color] += elimBonus[count];
	}

	// 转移双方消去的行，返回-1表示继续，否则返回输者
	int transfer() {
		int color1 = 0, color2 = 1;
		if (transCount[color1] == 0 && transCount[color2] == 0)
			return -1;
		if (transCount[color1] == 0 || transCount[color2] == 0) {
			if (transCount[color1] == 0 && transCount[color2] > 0)
				swap(color1, color2);
			int h2;
			maxHeight[color2] = h2 = maxHeight[color2] + transCount[color1];
			if (h2 > MAPHEIGHT)
				return color2;
			int i, j;

			for (i = h2; i > transCount[color1]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color2][i][j] = gridInfo[color2][i - transCount[color1]][j];

			for (i = transCount[color1]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color2][i][j] = trans[color1][i - 1][j];
			return -1;
		} else {
			int h1, h2;
			maxHeight[color1] = h1 = maxHeight[color1] + transCount[color2];//从color1处移动count1去color2
			maxHeight[color2] = h2 = maxHeight[color2] + transCount[color1];

			if (h1 > MAPHEIGHT) return color1;
			if (h2 > MAPHEIGHT) return color2;

			int i, j;
			for (i = h2; i > transCount[color1]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color2][i][j] = gridInfo[color2][i - transCount[color1]][j];

			for (i = transCount[color1]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color2][i][j] = trans[color1][i - 1][j];

			for (i = h1; i > transCount[color2]; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color1][i][j] = gridInfo[color1][i - transCount[color2]][j];

			for (i = transCount[color2]; i > 0; i--)
				for (j = 1; j <= MAPWIDTH; j++)
					gridInfo[color1][i][j] = trans[color2][i - 1][j];

			return -1;
		}
	}

	// 颜色方还能否继续游戏
	inline bool canPut(int color, int blockType) {
		Tetris t(blockType, color);
		for (int y = MAPHEIGHT; y >= 1; y--)
			for (int x = 1; x <= MAPWIDTH; x++)
				for (int o = 0; o < 4; o++) {
					t.set(x, y, o);
					if (t.isValid() && checkDirectDropTo(color, blockType, x, y, o))
						return true;
				}
		return false;
	}

	// 打印场地用于调试
	inline void printField(const void *a, const void *b) {
		static const char *i2s[] = {
			"~~",
			"~~",
			"  ",
			"[]",
			"##"
		};
		int *a1 = (int*)a;
		int *b1 = (int*)b;
		cout << "~~：墙，[]：块，##：新块" << endl;
		for (int y = MAPHEIGHT + 1; y >= 0; y--) {
			for (int x = 0; x <= MAPWIDTH + 1; x++)
				cout << i2s[*(a1 + y * (MAPWIDTH + 2) + x) + 2];
			for (int x = 0; x <= MAPWIDTH + 1; x++)
				cout << i2s[*(b1 + y * (MAPWIDTH + 2) + x) + 2];
			cout << endl;
		}
	}

	inline void printField() {
		printField(gridInfo[0], gridInfo[1]);
	}
}

// 全局变量
int blockForEnemy, finalX, finalY, finalO;
int nextTypeForColor[2];

/* Tomoko Start */
namespace Tomoko {
	/* 注意，用x来表示高度，用y来表示宽度 */

	class my_block_work_end_exception : public exception {};
	class enemy_block_work_end_exception : public exception {};
	class uncatched_exception : public exception {};

	/* 参数设置区，比赛过程中需要调整参数时，只需要调整这一块 */

	const int c_e_point[6][6] = {
		{ 0,0,0,0,0,0 },
		{ 0,0,0,0,0,0 },
		{ 0,30,5,0,0,0 },
		{ 0,80,10,5,0,0 },
		{ 0,120,20,10,5,0 },
		{ 0,300,50,30,15,5 },
	};

	inline int continuous_empty_point(int height, int num) {
		if (num == -2)
			num = 1;
		if (num >= 6)
			return 0;
		if (num >= 1 && num <= 5) {
			if (height <= 4)
				return c_e_point[height][num];
			return c_e_point[5][num];
		}
		if (num == -1)
			return 0;
		throw uncatched_exception();
	}

	const int h_diff_point[8] = {
		0, 0, 0, 5, 25, 100, 300, 500
	};

	inline int height_diff_point(int height) {
		if (height < 0)
			return 0;
		if (height <= 6)
			return h_diff_point[height];
		return h_diff_point[7];
	}

	// 方块高度的影响
	inline int block_height_point(int height) {
		return 20 * height;
	}

	// 被封闭的空格造成的影响值
	const int SEALED_POINT = 500;
	int CAN_BE_REGARDED_AS_SEALED;

	inline void generate_can_be_sealed(const int *type_count) {
		int min_count = MAXI;
		for (int i = 0; i < 7; ++i)
			if (type_count[i] < min_count)
				min_count = type_count[i];
		int diff[7];
		for (int i = 0; i < 7; ++i)
			diff[i] = type_count[i] - min_count;
		if (diff[2] + diff[3] + diff[4] + diff[6] <= 6)
			CAN_BE_REGARDED_AS_SEALED = 1;
		else CAN_BE_REGARDED_AS_SEALED = 2;
	}

	// 消除一行的奖励
	const int CLEAR_ROW_POINT = -700;

	// 剪枝上限
	const int MAX_BRANCH = 3;

	// 搜索深度
	const int MAX_DEPTH = 2;

	// 分数递减系数
	const double DEPTH_COEFFICIENT = 0.6;

	/* 参数设置区End */

	// 每行的块数
	int row_block_num[MAPHEIGHT + 2];

	// 当前方块类型
	int my_block_type;

	// 焦点行
	int focus_row;

	// 方块的状态，-2代表已有方块，-1代表没有方块但上方有方块，否则表示左右的较高高度
	int block_status[MAPHEIGHT + 2][MAPWIDTH + 2];

	// 左右高度，0左1右
	int abut_height[2][MAPHEIGHT + 2][MAPWIDTH + 2];

	// 连续的空格
	int continuous_empty[MAPHEIGHT + 2][MAPWIDTH + 2];
	int has_top[MAPHEIGHT + 2][MAPWIDTH + 2];

	// 敌方方块计数
	int maxCount = 0, minCount = 99;

	struct triple {
		int score;
		int score_2;
		int id;
		triple(int _score, int _score_2, int _id)
			:score(_score), score_2(_score_2), id(_id) {}
		triple(const pair<int, int> &scores, int _id)
			:score(scores.first), score_2(scores.second), id(_id) {}
		bool operator < (const triple &t) {
			if (score != t.score)
				return score < t.score;
			return score_2 < t.score_2;
		}
	};

	class my_heap {
	private:
		typedef shared_ptr<triple> _type;
		int size;
		int branch;
		vector<_type> vec;
		void heapify(int z) {
			while (((z << 1) | 1) < vec.size()) {
				if (*vec[(z << 1) | 1] < *vec[z])
					break;
				_type temp = vec[z];
				vec[z] = vec[(z << 1) | 1];
				vec[(z << 1) | 1] = temp;
				z = (z << 1) | 1;
			}
		}
	public:
		my_heap(int _branch)
			: size(0), branch(_branch) {}
		void pop() {
			_type temp = vec[0];
			vec[0] = vec[vec.size() - 1];
			vec[vec.size() - 1] = temp;
			vec.resize(vec.size() - 1);
			heapify(0);
		}

		void push(_type element) {
			vec.push_back(element);
			int i = vec.size() - 1;
			while (i > 0) {
				if (*element < *(vec[(i - 1) >> 1]))
					break;
				_type temp = vec[i];
				vec[i] = vec[(i - 1) >> 1];
				vec[(i - 1) >> 1] = temp;
				i = (i - 1) >> 1;
			}
			if (vec.size() > branch)
				pop();
		}

		_type top() {
			return vec[0];
		}

		shared_ptr<vector<_type> > toVector() {
			return shared_ptr<vector<_type> >(new vector<_type>(vec));
		}
	};

	// 判断当前答案是否合法
	bool isCorrect(const void* _grid, int block_type, int x, int y, int o, int pre) {
		if (x > MAPHEIGHT)
			return false;
		if (!is_valid(_grid, block_type, x, y, o))
			return false;
		if (check_drop(_grid, block_type, x, y, o))
			return true;
		if (pre == 0)
			return (isCorrect(_grid, block_type, x + 1, y, o, 0)
				|| isCorrect(_grid, block_type, x, y - 1, o, -1)
				|| isCorrect(_grid, block_type, x, y + 1, o, 1));
		if (pre == 1 && y < MAPWIDTH)
			return (isCorrect(_grid, block_type, x + 1, y, o, 0)
				|| isCorrect(_grid, block_type, x, y + 1, o, 1));
		if (pre == -1 && y > 0)
			return (isCorrect(_grid, block_type, x + 1, y, o, 0)
				|| isCorrect(_grid, block_type, x, y - 1, o, -1));
		return false;
	}

	// 判断当前答案是否合法
	bool isCorrect(const void* _grid, int block_type, int x, int y, int o) {
		if (x > MAPHEIGHT)
			return false;
		if (!on_ground(_grid, block_type, x, y, o))
			return false;
		if (!is_valid(_grid, block_type, x, y, o))
			return false;
		return isCorrect(_grid, block_type, x, y, o, 0);
	}

	// 判断当前答案是否合法
	bool isCorrect(int _my_block_type, int _curr_bot_color, int x, int y, int o) {
		if (x > MAPHEIGHT)
			return false;
		Tetris t(_my_block_type, _curr_bot_color);
		t.blockX = y;
		t.blockY = x;
		t.orientation = o;
		if (!t.onGround())
			return false;
		return isCorrect(gridInfo[_curr_bot_color], _my_block_type, x, y, o, 0);
	}

	// 输入答案
	void block_enter(int x, int y, int o) {
		finalY = x;
		finalX = y;
		finalO = o;
		throw my_block_work_end_exception();
	}

	// 输入答案
	void block_enter(shared_ptr<Tetris>t) {
		block_enter(t->blockY, t->blockX, t->orientation);
	}

	// 输入答案
	void block_enter(int block_for_enemy) {
		if (maxCount - minCount == 2 && typeCountForColor[1 - currBotColor][block_for_enemy] == maxCount)
			return;
		blockForEnemy = block_for_enemy;
		throw enemy_block_work_end_exception();
	}

	void block_init() {
		for (int i = 0; i < 7; i++) {
			if (typeCountForColor[enemyColor][i] > maxCount)
				maxCount = typeCountForColor[enemyColor][i];
			if (typeCountForColor[enemyColor][i] < minCount)
				minCount = typeCountForColor[enemyColor][i];
		}
		memset(row_block_num, 0, sizeof(row_block_num));
		for (int i = MAPHEIGHT; i >= 1; --i) {
			for (int j = 1; j <= MAPWIDTH; ++j)
				if (gridInfo[currBotColor][i][j] != 0) {
					if (gridInfo[currBotColor][i][j] > 0)
						++row_block_num[i];
					block_status[i][j] = -2;
				} else if (gridInfo[currBotColor][i + 1][j] > 0) {
					block_status[i][j] = -1;
				} else {
					int k = MAPHEIGHT;
					while (gridInfo[k][j - 1] == 0)
						--k;
					abut_height[0][i][j] = (k - i + 1 > 0) ? (k - i + 1) : 0;
					k = MAPHEIGHT;
					while (gridInfo[k][j + 1] == 0)
						--k;
					abut_height[1][i][j] = (k - i + 1 > 0) ? (k - i + 1) : 0;
					block_status[i][j] = max(gridInfo[0][i][j], gridInfo[1][i][j]);
				}
		}

		my_block_type = nextTypeForColor[currBotColor];
	}

	pair<int, int> generate_score(const void* _grid, const int *type_count) {
		generate_can_be_sealed(type_count);
		int grids[MAPHEIGHT + 2][MAPWIDTH + 2];
		memcpy(grids, _grid, sizeof(grids));
#ifdef DEBUG_4
		Util::printField(grids, grids);
#endif
		int ans = 0;
		for (int i = 1; i <= MAPHEIGHT; ) {
			bool p = true;
			for (int j = 1; j <= MAPWIDTH; ++j)
				if (grids[i][j] == 0)
					p = false;
			if (p) {
				for (int k = i; k < MAPHEIGHT; ++k)
					for (int j = 1; j <= MAPWIDTH; ++j)
						grids[k][j] = grids[k + 1][j];
				ans += CLEAR_ROW_POINT;
			} else {
				++i;
			}
		}
		int max_height = 0;
		int row_max_height[MAPWIDTH + 2];
		memset(row_max_height, 0, sizeof(row_max_height));
		for (int i = 1; i <= MAPHEIGHT; ++i)
			for (int j = 1; j <= MAPWIDTH; ++j)
				if (grids[i][j] > 0) {
					row_max_height[j] = i;
					max_height = i;
					ans += block_height_point(i);
				}

		bool vis[MAPHEIGHT + 2][MAPWIDTH + 2];
		int sealed[MAPHEIGHT + 2][MAPWIDTH + 2];
		bool save_sealed[250];
		int save_sealed_num = 0;
		save_sealed[0] = false;
		memset(sealed, 0, sizeof(sealed));
		memset(vis, 0, sizeof(vis));
		{
			for (int x = 1; x <= MAPHEIGHT; ++x)
				for (int y = 1; y <= MAPWIDTH; ++y)
					if (grids[x][y] == 0 && !vis[x][y]) {
						int this_num = ++save_sealed_num;
						save_sealed[this_num] = true;
						int left = 0, right = 1;
						int que[250][2];
						que[1][0] = x;
						que[1][1] = y;
						vis[x][y] = true;
						while (left < right) {
							++left;
							int xi = que[left][0];
							int yi = que[left][1];
							sealed[xi][yi] = this_num;
							if (xi == MAPHEIGHT)
								save_sealed[this_num] = false;
							if (grids[xi + 1][yi] == 0 && !vis[xi + 1][yi]) {
								que[++right][0] = xi + 1;
								que[right][1] = yi;
								vis[xi + 1][yi] = true;
							}
							if (grids[xi - 1][yi] == 0 && !vis[xi - 1][yi]) {
								que[++right][0] = xi - 1;
								que[right][1] = yi;
								vis[xi - 1][yi] = true;
							}
							if (grids[xi][yi + 1] == 0 && !vis[xi][yi + 1]) {
								que[++right][0] = xi;
								que[right][1] = yi + 1;
								vis[xi][yi + 1] = true;
							}
							if (grids[xi][yi - 1] == 0 && !vis[xi][yi - 1]) {
								que[++right][0] = xi;
								que[right][1] = yi - 1;
								vis[xi][yi - 1] = true;
							}
						}
					}
		}

		save_sealed[249] = true;

		for (int i = MAPHEIGHT - 1; i > 0; --i)
			for (int j = 1; j <= MAPWIDTH; ++j)
				if (grids[i][j] == 0 && grids[i + 1][j] == 0 &&
					save_sealed[sealed[i + 1][j]])
					sealed[i][j] = 249;

		memset(has_top, 0, sizeof(has_top));
		for (int i = MAPHEIGHT; i > 0; --i)
			for (int j = 1; j <= MAPWIDTH; ++j) {
				if (has_top[i + 1][j] == 0) {
					if (grids[i][j] > 0)
						has_top[i][j] = 1;
				} else {
					has_top[i][j] = has_top[i + 1][j] + 1;
				}
			}
		
		for (int i = 1; i <= MAPHEIGHT; ++i)
			for (int j = 2; j <= MAPWIDTH; ++j)
				if (has_top[i][j] > 0 && grids[i][j] == 0 &&
					grids[i][j + 1] != 0 &&
					(grids[i][j - 2] != 0 || (j >= 3 && grids[i][j - 3] != 0)))
					sealed[i][j] = 249;
		for (int i = 1; i <= MAPHEIGHT; ++i)
			for (int j = 1; j <= MAPWIDTH - 1; ++j)
				if (has_top[i][j] > 0 && grids[i][j] == 0 &&
					grids[i][j - 1] != 0 &&
					(grids[i][j + 2] != 0 || (j + 3 < MAPWIDTH + 2 && grids[i][j + 3] != 0)))
					sealed[i][j] = 249;

		for (int i = 1; i <= MAPHEIGHT; ++i)
			for (int j = 1; j <= MAPWIDTH; ++j) {
				int left = j;
				int right = j;
				while (left > 0 && !has_top[i][left])
					--left;
				while (right <= MAPWIDTH && !has_top[i][right])
					++right;
				continuous_empty[i][j] = right - left - 1;
				if (continuous_empty[i][j] == -1 && grids[i][j] == 0)
					continuous_empty[i][j] = -2; 
			}

		for (int i = 1; i <= MAPHEIGHT; ++i) {
			continuous_empty[i][0] = -3;
			continuous_empty[i][MAPWIDTH + 1] = -3;
		}

		bool row_sealed[MAPHEIGHT + 2];
		row_max_height[0] = MAXI;
		row_max_height[MAPWIDTH + 1] = MAXI;
		for (int i = 1; i <= MAPHEIGHT; ++i) {
			row_sealed[i] = false;
			for (int j = 1; j <= MAPWIDTH; ++j)
				if (grids[i][j] == 0 && (save_sealed[sealed[i][j]] ||
					min(row_max_height[j - 1], row_max_height[j + 1]) - i >= CAN_BE_REGARDED_AS_SEALED))
					row_sealed[i] = true;
		}

		for (int i = 1; i <= MAPHEIGHT; ++i) {
			if (row_sealed[i])
				continue;
			for (int j = 1; j <= MAPWIDTH; ) {
				int k = j, l = 0;
				while (continuous_empty[i][j] == continuous_empty[i][j + 1]) {
					l = max(l, has_top[i][j]);
					++j;
				}
				l = max(l, has_top[i][j]);
				ans += continuous_empty_point(max(min(has_top[i][j + 1], has_top[i][k - 1]), l),
					continuous_empty[i][j]);
				++j;
			}
		}

		for (int i = 1; i <= MAPHEIGHT; ++i) {
			if (row_sealed[i])
				continue;
			for (int j = 1; j <= MAPWIDTH; ++j)
				if (has_top[i][j] == 0)
					ans += height_diff_point(max_height - i);
		}


		for (int i = 1; i <= MAPHEIGHT; ++i)
			if (row_sealed[i])
				ans += SEALED_POINT;

		int ans2 = 0;
		for (int i = 1; i <= MAPHEIGHT; ++i)
			for (int j = 1; j < MAPWIDTH; ++j)
				if (grids[i][j] == 0 && grids[i][j + 1] == 0
					&& !save_sealed[sealed[i][j]] && !save_sealed[sealed[i][j + 1]])
					ans2 -= 100;

		return make_pair(ans, ans2);
	}

	void add_possible_solution(vector<Tetris> &vec, const void* _grid, int block_type,
		int x, int y, int o, bool on_ground_required) {
		if (block_type == 6 && o != 0)
			return;
		if ((block_type == 2 || block_type == 3 || block_type == 5) && o >= 2)
			return;
		if (on_ground_required && !isCorrect(_grid, block_type, x, y, o))
			return;
		if (!on_ground_required && !isCorrect(_grid, block_type, x, y, o, 0))
			return;
		Tetris t(block_type);
		t.blockY = x;
		t.blockX = y;
		t.orientation = o;
		vec.push_back(t);
	}

	// 0层的情形
	shared_ptr<vector<shared_ptr<triple> > > try_all_possible_solution(const vector<Tetris> &vec, const int* _type_count,
		const void* _grid, bool is_zero, bool is_enemy = false) {
		if (vec.size() == 0)
			throw uncatched_exception();
		int branch = (is_zero) ? 1 : MAX_BRANCH;
		if (is_enemy)
			branch = 10;
		my_heap p(branch);
		int grids[MAPHEIGHT + 2][MAPWIDTH + 2];
		memcpy(grids, _grid, (MAPHEIGHT + 2) * (MAPWIDTH + 2) * sizeof(int));
		for (int i = 0; i < vec.size(); ++i) {
			int temp_grid[MAPHEIGHT + 2][MAPWIDTH + 2];
			memcpy(temp_grid, grids, (MAPHEIGHT + 2) * (MAPWIDTH + 2) * sizeof(int));
			for (int j = 0; j < 4; ++j) {
				int tmpX = vec[i].blockX + blockShape[vec[i].blockType][vec[i].orientation][2 * j];
				int tmpY = vec[i].blockY + blockShape[vec[i].blockType][vec[i].orientation][2 * j + 1];
				temp_grid[tmpY][tmpX] = 2;
			}
#ifdef DEBUG_2
			Util::printField(temp_grid, gridInfo[currBotColor]);
#endif
			pair<int, int> pr = generate_score(temp_grid, _type_count);
			p.push(shared_ptr<triple>(new triple(pr, i)));
		}
		return p.toVector();
	}

	shared_ptr<pair<triple, shared_ptr<Tetris> > > get_solution(const void* _grid, const int* _type_count,
		int block_type, int depth);

	// a-b剪枝
	shared_ptr<triple> try_all_possible_solution(const vector<Tetris> &vec, const int* _type_count,
		const void* _grid, int depth) {
		if (vec.size() == 0)
			throw uncatched_exception();
		int my_max_count = 0;
		int my_min_count = 99;
		int type_count[7];
		bool type_possible[7];
		for (int i = 0; i < 7; ++i)
			type_count[i] = _type_count[i];
		for (int i = 0; i < 7; ++i) {
			if (type_count[i] > my_max_count)
				my_max_count = type_count[i];
			if (type_count[i] < my_min_count)
				my_min_count = type_count[i];
		}
		for (int i = 0; i < 7; ++i)
			type_possible[i] = true;
		if (my_max_count - my_min_count == 2) {
			for (int i = 0; i < 7; ++i)
				if (typeCountForColor[currBotColor][i] == my_max_count)
					type_possible[i] = false;
		}
		auto temp_ans = try_all_possible_solution(vec, _type_count, _grid, (depth == 0));
		vector<shared_ptr<triple> > temp_vec(*temp_ans);
		if (depth == 0) {
			shared_ptr<triple> ans_ptr(new triple(*temp_vec[temp_vec.size() - 1]));
			return ans_ptr;
		}
		int grids[MAPHEIGHT + 2][MAPWIDTH + 2];
		int ans_id = -1;
		int min_score = MAXI;
		int min_score_2 = MAXI;
		memcpy(grids, _grid, (MAPHEIGHT + 2) * (MAPWIDTH + 2) * sizeof(int));
		for (int i = 0; i < temp_vec.size(); ++i) {
			int _id = temp_vec[i]->id;
			int mscore = -MAXI;
			int mscore_2 = -MAXI;
			int temp_grid[MAPHEIGHT + 2][MAPWIDTH + 2];
			memcpy(temp_grid, grids, (MAPHEIGHT + 2) * (MAPWIDTH + 2) * sizeof(int));
			for (int j = 0; j < 4; ++j) {
				int tmpX = vec[_id].blockX + blockShape[vec[_id].blockType][vec[_id].orientation][2 * j];
				int tmpY = vec[_id].blockY + blockShape[vec[_id].blockType][vec[_id].orientation][2 * j + 1];
				temp_grid[tmpY][tmpX] = 2;
			}
			clear_rows(temp_grid);
			int max_score = -MAXI;
			int max_score_2 = -MAXI;
			int chosen_type = -1;
			for (int _type = 0; _type < 7; ++_type)
				if (type_possible[_type]) {
					++type_count[_type];
					//			cout << "depth = " << depth << ", index = " << i << ", type = " << _type << endl;
					triple temp_triple = get_solution(_grid, type_count, _type, 0)->first;
					if (temp_triple.score > max_score ||
						(temp_triple.score == max_score && temp_triple.score_2 > max_score_2)) {
						max_score = temp_triple.score;
						max_score_2 = temp_triple.score_2;
						chosen_type = _type;
					}
					--type_count[_type];
				}
			++type_count[chosen_type];
			triple temp_triple = get_solution(_grid, type_count, chosen_type, depth - 1)->first;
			temp_triple.score *= DEPTH_COEFFICIENT;
			temp_triple.score += temp_vec[i]->score;
			temp_triple.score_2 *= DEPTH_COEFFICIENT;
			temp_triple.score_2 += temp_vec[i]->score_2;
			--type_count[chosen_type];
			if (temp_triple.score < min_score ||
				(temp_triple.score == min_score && temp_triple.score_2 < min_score_2)) {
				min_score = temp_triple.score;
				min_score_2 = temp_triple.score_2;
				ans_id = _id;
			}
		}
		for (int j = 0; j < 4; ++j) {
			int tmpX = vec[ans_id].blockX + blockShape[vec[ans_id].blockType][vec[ans_id].orientation][2 * j];
			int tmpY = vec[ans_id].blockY + blockShape[vec[ans_id].blockType][vec[ans_id].orientation][2 * j + 1];
			grids[tmpY][tmpX] = 2;
		}
		++type_count[vec[ans_id].blockType];
		auto pr = generate_score(grids, type_count);
		shared_ptr<triple> ans_ptr(new triple(pr, ans_id));
		return ans_ptr;
	}

	shared_ptr<pair<triple, shared_ptr<Tetris> > > get_solution(const void* _grid, const int* _type_count,
		int block_type, int depth) {
		vector<Tetris> possible_solution;
		for (int i = 1; i <= MAPHEIGHT; ++i)
			for (int j = 1; j <= MAPWIDTH; ++j)
				for (int k = 0; k < 4; ++k)
					add_possible_solution(possible_solution, _grid, block_type, i, j, k, true);
		auto temp_triple = try_all_possible_solution(possible_solution, _type_count, _grid, depth);
		return shared_ptr<pair<triple, shared_ptr<Tetris> > >(
			new pair<triple, shared_ptr<Tetris> >(make_pair(*temp_triple,
				shared_ptr<Tetris>(new Tetris(possible_solution[temp_triple->id])))));
	}

	shared_ptr<vector<Tetris> > get_enemy_solution(const void* _grid, const int* _type_count, int block_type) {
		vector<Tetris> possible_solution;
		for (int i = 1; i <= MAPHEIGHT; ++i)
			for (int j = 1; j <= MAPWIDTH; ++j)
				for (int k = 0; k < 4; ++k)
					add_possible_solution(possible_solution, _grid, block_type, i, j, k, true);
		auto temp_triple = try_all_possible_solution(possible_solution, _type_count, _grid, false, true);
		shared_ptr<vector<Tetris> > ans(new vector<Tetris>());
		for (auto ite = temp_triple->begin(); ite != temp_triple->end(); ++ite) {
			int x = ite->get()->id;
			ans->push_back(possible_solution[x]);
		}
		return ans;
	}

	// 先处理自己的方块的放置
	void my_block_work() {

		// 1.特殊阶段，对一些特例做特别处理  TODO
		/*
		switch (my_block_type) {
		case 0:
		{
		for (int i = 1; i <= MAPHEIGHT; ++i)
		for (int j = 1; j <= MAPWIDTH; ++j)
		if (block_status[i][j] == -1) {
		if (block_status[i][j + 1] == -1)
		block_enter(i, j + 1, 2);
		}
		for (int i = 1; i < MAPHEIGHT; ++i)
		for (int j = 1; j <= MAPWIDTH; ++j)
		if (continuous_empty[1][i][j] == 0 && continuous_empty[1][i + 1][j] == 2)
		block_enter(i + 1, j + 1, 0);
		}
		break;

		case 1:
		{

		}
		break;

		case 2:
		{

		}
		break;

		case 3:
		{

		}
		break;

		case 4:
		{

		}
		break;

		case 5:
		{

		}
		break;

		case 6:
		{

		}
		}
		*/
		// 2.通常计算阶段

		int type_count[7];
		for (int i = 0; i < 7; ++i)
			type_count[i] = typeCountForColor[currBotColor][i];
		shared_ptr<Tetris> t =
			get_solution(gridInfo[currBotColor], type_count, my_block_type, MAX_DEPTH)->second;
		t->color = currBotColor;
		block_enter(t);

	}

	// 再给对手方块
	void enemy_block_work() {
		try {
			int a[7];
			pair<int, int> ans[7];
			for (int i = 0; i < 7; ++i)
				ans[i] = make_pair(MAXI, MAXI);
			for (int i = 0; i < 7; ++i)
				a[i] = i;
			int type_count[7];
			for (int i = 0; i < 7; ++i)
				type_count[i] = typeCountForColor[1 - currBotColor][i];
			int enemy_block_type = nextTypeForColor[1 - currBotColor];
			auto enemy_solution = get_enemy_solution(gridInfo[1 - currBotColor], type_count,
				enemy_block_type);
			for (auto ite = enemy_solution->begin(); ite != enemy_solution->end(); ++ite) {
				int temp_grid[MAPHEIGHT + 2][MAPWIDTH + 2];
				memcpy(temp_grid, gridInfo[1 - currBotColor], (MAPHEIGHT + 2) * (MAPWIDTH + 2) * sizeof(int));
				for (int j = 0; j < 4; ++j) {
					int tmpX = ite->blockX + blockShape[ite->blockType][ite->orientation][2 * j];
					int tmpY = ite->blockY + blockShape[ite->blockType][ite->orientation][2 * j + 1];
					temp_grid[tmpY][tmpX] = 2;
				}
				clear_rows(temp_grid);

#ifdef DEBUG_3
				cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$" << endl;
				Util::printField(temp_grid, gridInfo[1 - currBotColor]);
				cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$" << endl;
#endif

				for (int t = 0; t < 7; ++t) {
					if (maxCount - minCount == 2 && typeCountForColor[1 - currBotColor][t] == maxCount) {
						ans[t] = make_pair(-MAXI, -MAXI);
						continue;
					}
					++type_count[t];
					auto _solution = get_solution(temp_grid, type_count, t, 0);
					--type_count[t];
					if (_solution->first.score < ans[t].first ||
						(_solution->first.score == ans[t].first && _solution->first.score_2 < ans[t].second))
						ans[t] = make_pair(_solution->first.score, _solution->first.score_2);
				}
			}
			sort(a, a + 7, [&ans](int x, int y)->bool {
				if (ans[x].first != ans[y].first)
					return ans[x].first > ans[y].first;
				return ans[x].second > ans[y].second;
			});
			for (int i = 0; i < 7; ++i)
				block_enter(a[i]);
		} catch (uncatched_exception &e) {
			int a[7];
			for (int i = 0; i < 7; ++i)
				a[i] = i;
			random_shuffle(a, a + 7);
			for (int i = 0; i < 7; ++i)
				block_enter(a[i]);
		}
	}

	// Tomoko接口
	void start() {
		block_init();
#ifdef DEBUG
		Util::printField();
		cout << endl;
#endif

		try {
			my_block_work();
		} catch (my_block_work_end_exception &e) {
			try {
				enemy_block_work();
			} catch (enemy_block_work_end_exception &e1) {}
		}
	}

}
/* Tomoko End */

int main() {

	srand((unsigned int)time(NULL));
	init();

	int turnID, blockType;
	cin >> turnID;

	// 先读入第一回合，得到自己的颜色
	// 双方的第一块肯定是一样的
	cin >> blockType >> currBotColor;
	enemyColor = 1 - currBotColor;
	nextTypeForColor[0] = blockType;
	nextTypeForColor[1] = blockType;
	typeCountForColor[0][blockType]++;
	typeCountForColor[1][blockType]++;

	// 然后分析以前每回合的输入输出，并恢复状态
	// 循环中，color 表示当前这一行是 color 的行为
	// 平台保证所有输入都是合法输入
	for (int i = 1; i < turnID; i++) {
		int currTypeForColor[2] = { nextTypeForColor[0], nextTypeForColor[1] };
		int x, y, o;
		// 根据这些输入输出逐渐恢复状态到当前回合

		// 先读自己的输出，也就是自己的行为
		// 自己的输出是自己的最后一步
		// 然后模拟最后一步放置块
		cin >> blockType >> x >> y >> o;

		// 我当时把上一块落到了 x y o！
		Tetris myBlock(currTypeForColor[currBotColor], currBotColor);
		myBlock.set(x, y, o).place();

		// 我给对方什么块来着？
		typeCountForColor[enemyColor][blockType]++;
		nextTypeForColor[enemyColor] = blockType;

		// 然后读自己的输入，也就是对方的行为
		// 裁判给自己的输入是对方的最后一步
		cin >> blockType >> x >> y >> o;

		// 对方当时把上一块落到了 x y o！
		Tetris enemyBlock(currTypeForColor[enemyColor], enemyColor);
		enemyBlock.set(x, y, o).place();

		// 对方给我什么块来着？
		typeCountForColor[currBotColor][blockType]++;
		nextTypeForColor[currBotColor] = blockType;

		// 检查消去
		Util::eliminate(0);
		Util::eliminate(1);

		// 进行转移
		Util::transfer();
	}

	Tomoko::start();

	cout << blockForEnemy << " " << finalX << " " << finalY << " " << finalO;
#ifdef DEBUG
	cout << endl;
	Tetris t1(nextTypeForColor[currBotColor], currBotColor);
	t1.blockX = finalX;
	t1.blockY = finalY;
	t1.orientation = finalO;
	t1.place();
	Util::printField();
	system("pause");
#endif

	return 0;
}