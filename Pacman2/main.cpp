/* Tomoko Sakisaka 咲坂智子  人工智能机器人
 * For Pacmen 2
 * Version: 1.0
 * Copyright XSXHY Soft(C) 2010-2016 All rights reserved
 * Author: Starlightroad2
 */


#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "jsoncpp/json.h"

using std::string;
using std::swap;
using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::runtime_error;
using std::stringstream;

/* 常数定义区 */
#define MAXHEIGHT 20
#define MAXWIDTH 20
#define MAXINTEGER 32700
#define MININTEGER -32700
#define MAXFLOAT 9999999
#define MINFLOAT -9999999
#define MAXPLAYERCOUNT 4
#define MAXDIRECTION 8
#define SPEECHCOUNT 11
#define MAXTURN 100
#define MAXSTEP 10
/* 常数定义区end */

/* Tomoko沉睡 */
bool GAME_BOARD_CONSTRUCTED;
/* Tomoko沉睡end */

/* 前端计算声明 */
template<typename T>
inline T operator |=(T &a, const T &b)
{
	return a = static_cast<T>(static_cast<int>(a) | static_cast<int>(b));
}
template<typename T>
inline T operator |(const T &a, const T &b)
{
	return static_cast<T>(static_cast<int>(a) | static_cast<int>(b));
}
template<typename T>
inline T operator &=(T &a, const T &b)
{
	return a = static_cast<T>(static_cast<int>(a) & static_cast<int>(b));
}
template<typename T>
inline T operator &(const T &a, const T &b)
{
	return static_cast<T>(static_cast<int>(a) & static_cast<int>(b));
}
template<typename T>
inline T operator -(const T &a, const T &b)
{
	return static_cast<T>(static_cast<int>(a) - static_cast<int>(b));
}
template<typename T>
inline T operator ++(T &a)
{
	return a = static_cast<T>(static_cast<int>(a) + 1);
}
template<typename T>
inline T operator ~(const T &a)
{
	return static_cast<T>(~static_cast<int>(a));
}
/* 前端计算声明end */



/* 数据类型处理器
 * Content:
 * 1 - 玩家1
 * 2 - 玩家2
 * 4 - 玩家3
 * 8 - 玩家4
 * 16 - 小豆
 * 32 - 大豆
 * Static:
 * 1 - 上方的墙
 * 2 - 右方的墙
 * 4 - 下方的墙
 * 8 - 左方的墙
 * 16 - 豆产生装置
 * ActionType:
 * -1 - 不动
 * 0 - 向上移动
 * 1 - 向右移动
 * 2 - 向下移动
 * 3 - 向左移动
 * 4 - 向上使用技能
 * 5 - 向右使用技能
 * 6 - 向下使用技能
 * 7 - 向左使用技能
 */
enum GridContentType
{
	empty = 0,
	player1 = 1,
	player2 = 2,
	player3 = 4,
	player4 = 8,
	playerMask = 1 | 2 | 4 | 8,  // 用于检查是否有玩家
	smallFruit = 16,
	bigFruit = 32
};
enum GridStaticType
{
	emptyWall = 0,
	wallNorth = 1,
	wallEast = 2,
	wallSouth = 4,
	wallWest = 8,
	generator = 16
};
enum ActionType
{
	stay = -1,
	up = 0,
	right = 1,
	down = 2,
	left = 3,
	shootUp = 4,
	shootRight = 5,
	shootDown = 6,
	shootLeft = 7
};
const GridContentType playerName[] = {player1, player2, player3, player4};
const int moveDirection[8][2] = {{-1,0},{0,1},{1,0},{0,-1},{-1,-1},{-1,1},{1,-1},{1,1}};
/* 数据类型处理器end */




// 游戏中所需要的常用工具
class GameTools
{
public:
	static void ReadInput(const char*);
	static void WriteOutput(ActionType, string, Json::Value&, Json::Value&, string);
	static void DebugPrint();
	static inline int RanInt(int, int);

};

struct par
{
	int row, column;
	par *nex;
	par();
	par(int r, int c);
};

struct ActionChain
{
	ActionType key;
	ActionChain *nex;
	int atMapType;
	ActionChain();
	ActionChain(ActionType, ActionChain*);
};

template <typename T>
struct CoefficientChain
{
	T key;
	CoefficientChain<T> *nex;
};

class Grid
{
public:
	GridContentType gridContent;
	GridStaticType gridStatic;
	Grid();
	Grid(const GridContentType&, const GridStaticType&);
};

class GameBoard
{
private:
	par *gene;
	inline bool ActionValid(int);

	/* Tomoko */
	int TomokoID, atRow, atColumn;

	// 计算成长价值
	double developmentValue[MAXHEIGHT][MAXWIDTH];
	void calculateDevelopmentValue();
	void calculateBestMoveForDevelopment();
	int generateValueDFS(int, int, int);
	int fruitValueDFS(int, int, int);
	int nearStrong(int, int);
	bool analyzePoint(int, int);
	int distanceToGene(int, int);
	CoefficientChain<double> *calculateDevelopmentValueBasedOnDistance, *fixValueChain,
			*eatableFruitNearChain;
	bool visited[MAXHEIGHT][MAXWIDTH];
	double _temp[MAXHEIGHT][MAXWIDTH];
	ActionType bestMoveForDevelopment;
	bool fruitCanEat[MAXHEIGHT][MAXWIDTH];
	int generateValue[MAXHEIGHT][MAXWIDTH];
	int fruitValue[MAXHEIGHT][MAXWIDTH];
	int fruitLeftTurn[MAXHEIGHT][MAXWIDTH];
	// 记忆
	Json::Value globalData, data;
	bool isFixed;
	int fixedRow, fixedColumn;
	bool generateValueMade;

	// 解析地图形状
	int mapType[MAXHEIGHT][MAXWIDTH];

	string debugSpeech;

	// 台词
	const string speeches[SPEECHCOUNT] =
	{
			"",
			"こんにちは～咲坂智子です~",
			"にっこにっこに~",
			"はは、ステキじゃない?",
			"ああ、おなかがいっぱい~",
			"リア充が死ねばいい！",
			"竜神の剣をくらえ！",
			"竜が我が敌を喰らう！",
			"Banishment This World!",
			"いやー、痛いよ(T^T)",
			"これから譲れないから！"
	};

	bool setSpeech9, setSpeech5, setSpeech10;

	int distance[MAXHEIGHT][MAXWIDTH][MAXHEIGHT][MAXWIDTH];
	int distanceForAttack[MAXHEIGHT][MAXWIDTH];
	ActionType actionToGoto[MAXHEIGHT][MAXWIDTH][MAXHEIGHT][MAXWIDTH];
	ActionType directGoto[MAXHEIGHT][MAXWIDTH][MAXHEIGHT][MAXWIDTH];
	void distanceBFS(int, int);
	ActionType findAttackPoint();
	void generateDistance();
	void generateMapType();
	bool chaseAfter(int, int, int, int, int);

	// 多项式求值
	template <typename T>
	T calculatePolynomial(CoefficientChain<T>*, T);


	/* Tomoko end */
public:
	GameBoard(int, int, int, int, int, int, int, const Json::Value&, const Json::Value&);
	void DataAnalyze(Json::Value&);
	void GlobalDataAnalyze(Json::Value&);
	void StartTurn();
	int gameTurn, generatorTurnLeft;
	int height, width, bigFruitDuration, bigFruitEnhancement,
	skillCost, generatorInterval, playerAlive;
	bool hasWall[4][MAXHEIGHT][MAXWIDTH];
	Grid *grids[MAXHEIGHT][MAXWIDTH];
	bool hasSmallFruit[MAXHEIGHT][MAXWIDTH];
	bool hasBigFruit[MAXHEIGHT][MAXWIDTH];
	bool canGenerateFruit[MAXHEIGHT][MAXWIDTH];
	void TomokoStart();
};

class Player
{
private:
	int playerID;
public:
	bool isDead;
	ActionChain *action;
	int atRow, atColumn, power, powerUpLeft;
	Player(int, int, int);
};


/* Tomoko驱动器 */
GameBoard *board;
/* Tomoko驱动器end */
/* 其他玩家监视器 */
Player *players[4];
/* 其他玩家监视器end */
/* 哨兵 */
par *nil;
ActionChain *aNil;
/* 哨兵end */


/* 主函数 */
int main()
{
	GAME_BOARD_CONSTRUCTED = false;
	GameTools::ReadInput("input.txt");
	board -> TomokoStart();
	GameTools::DebugPrint();
	return 0;
}
/* 主函数end */



/* 功能实现细节部分 */

/* GameTools类 */
// 读入Json数据
// @requires Json Data
// @returns Player ID
void GameTools::ReadInput(const char *fileName)
{
	string str, chunk;
	int i;

	// 读入方式预处理
#ifdef _BOTZONE_ONLINE
	std::ios::sync_with_stdio(false);
	getline(cin,str);
#else
	if(fileName)
	{
		std::ifstream fin(fileName);
		if(fin)
			while(getline(fin, chunk) && chunk != "")
				str += chunk;
		else
			while(getline(cin, chunk) && chunk != "")
				str += chunk;
	}
	else
		while(getline(cin, chunk) && chunk != "")
			str += chunk;
#endif

	// 开始读入
	Json::Reader reader;
	Json::Value input;
	reader.parse(str, input);
	int len = input["requests"].size();
	if(!GAME_BOARD_CONSTRUCTED)
	{

		Json::Value field = input["requests"][(Json::Value::UInt) 0],
		staticField = field["static"],
		contentField = field["content"];
		int _height = field["height"].asInt(),
			_width = field["width"].asInt(),
			_bigFruitDuration = field["LARGE_FRUIT_DURATION"].asInt(),
			_bigFruitEnhancement = field["LARGE_FRUIT_ENHANCEMENT"].asInt(),
			_skillCost = field["SKILL_COST"].asInt(),
			_generatorInterval = field["GENERATOR_INTERVAL"].asInt(),
			_tomokoid = field["id"].asInt();

		// 构造棋盘
		// @assumes 棋盘未被构造
		board = new GameBoard(_height, _width, _bigFruitDuration,
				_bigFruitEnhancement, _skillCost, _generatorInterval, _tomokoid,
				contentField, staticField);
		Json::Value _globalData = input["globaldata"];
		board -> GlobalDataAnalyze(_globalData);

	}
	Json::Value _data = input["data"];
	board -> DataAnalyze(_data);
	char ch[2];
	while(board -> gameTurn < len)
	{
		Json::Value req = input["requests"][board -> gameTurn];
		for(i = 0; i < MAXPLAYERCOUNT; i++)
			if(!players[i] -> isDead)
			{
				sprintf(ch, "%d", i);
				players[i] -> action = new ActionChain((ActionType)req[ch]["action"].asInt(), players[i] -> action);
			}
		board -> StartTurn();
	}

}

// 写出Json数据
void GameTools::WriteOutput(ActionType _action, string _message,
		Json::Value &_data, Json::Value &_globalData, string _debugMessage)
{

	Json::Value ret;
	ret["response"]["action"] = _action;
	ret["response"]["tauntText"] = _message;
	ret["data"] = _data;
	ret["globaldata"] = _globalData;
	ret["debug"] = _debugMessage;
#ifdef _BOTZONE_ONLINE
	Json::FastWriter writer;
#else
	Json::StyledWriter writer;
#endif
	cout << writer.write(ret) << endl;
}


void GameTools::DebugPrint()
{
	if(!GAME_BOARD_CONSTRUCTED)
		throw runtime_error("请先唤醒Tomoko");
	else
	{
#ifndef _BOTZONE_ONLINE
		int r, c;
		printf("回合号【%d】存活人数【%d】| 图例 产生器[G] 有玩家[0/1/2/3] 多个玩家[*] 大豆[o] 小豆[.]\n",
				board -> gameTurn, board -> playerAlive);
		for (int i = 0; i < MAXPLAYERCOUNT; i++)
		{
			printf("[玩家%d(%d, %d)|力量%d|加成剩余回合%d|%s]\n", i, players[i] -> atRow,
					players[i] -> atColumn, players[i] -> power, players[i] -> powerUpLeft,
					players[i] -> isDead ? "已阵亡" : "存活");
		}
		putchar(' ');
		putchar(' ');
		for (c = 0; c < board -> width; c++)
			printf("  %d ", c);
		putchar('\n');
		for (r = 0; r < board -> height; r++)
		{
			putchar(' ');
			putchar(' ');
			for (c = 0; c < board -> width; c++)
			{
				putchar(' ');
				printf((board -> hasWall[0][r][c]) ? "---" : "   ");
			}
			printf("\n%d ", r);
			for (c = 0; c < board -> width; c++)
			{
				putchar((board -> hasWall[3][r][c]) ? '|' : ' ');
				putchar(' ');
				int hasPlayer = -1;
				for (int i = 0; i < MAXPLAYERCOUNT; i++)
					if (players[i] -> atRow == r && players[i] -> atColumn == c && !players[i] -> isDead)
					{
						if (hasPlayer == -1)
							hasPlayer = i;
						else
							hasPlayer = 4;
					}
				if (hasPlayer == 4)
					putchar('*');
				else if (hasPlayer != -1)
					putchar('0' + hasPlayer);
				else if (board -> grids[r][c] -> gridStatic & generator)
					putchar('G');
				else if (board -> grids[r][c] -> gridContent & playerMask)
					putchar('*');
				else if (board -> hasSmallFruit[r][c])
					putchar('.');
				else if (board -> hasBigFruit[r][c])
					putchar('o');
				else
					putchar(' ');
				putchar(' ');
			}
			putchar((board -> hasWall[1][r][--c]) ? '|' : ' ');
			putchar('\n');
		}
		putchar(' ');
		putchar(' ');
		r--;
		for (c = 0; c < board -> width; c++)
		{
			putchar(' ');
			printf((board -> hasWall[2][r][c]) ? "---" : "   ");
		}
		putchar('\n');
		putchar('\n');

#endif
	}
}

inline int GameTools::RanInt(int low, int high)
{
	return low + rand() % (high - low + 1);
}



/* GameTools类end */



/* GameBoard类
 * 这是程序的核心类
 * 负责决策、全盘操作、演算和模拟
 */

// Tomoko启动
GameBoard::GameBoard(int _height, int _width, int _bigFruitDuration,
		int _bigFruitEnhancement, int _skillCost, int _generatorInterval, int _tomokoid,
		const Json::Value &_contentField, const Json::Value &_staticField)
{
	if(GAME_BOARD_CONSTRUCTED)
		throw runtime_error("Gameboard overconstructed.");
	else
	{
		GAME_BOARD_CONSTRUCTED = true;
		srand((unsigned int)time(NULL));
		gameTurn = 1;
		height = _height;
		width = _width;
		bigFruitDuration = _bigFruitDuration;
		bigFruitEnhancement = _bigFruitEnhancement;
		skillCost = _skillCost;
		generatorInterval = _generatorInterval;
		generatorTurnLeft = generatorInterval;
		TomokoID = _tomokoid;
		playerAlive = 0;
		setSpeech9 = false;
		setSpeech5 = false;
		setSpeech10 = false;
		nil = new par();
		gene = nil;
		int r, c, i, x, y;
		GridContentType _gridContent;
		GridStaticType _gridStatic;
		memset(canGenerateFruit,0,sizeof(canGenerateFruit));
		memset(actionToGoto,-1,sizeof(actionToGoto));

		for(r = 0; r < height; r++)
			for(c = 0; c < width; c++)
			{
				_gridContent = (GridContentType) _contentField[r][c].asInt();
				_gridStatic = (GridStaticType) _staticField[r][c].asInt();
				grids[r][c] = new Grid(_gridContent, _gridStatic);
				if(_gridStatic & generator)
				{
					par *p = new par(r,c);
					p -> nex = gene;
					gene = p;
				}
				if(_gridStatic & wallNorth)
					hasWall[0][r][c] = true;
				else
					hasWall[0][r][c] = false;
				if(_gridStatic & wallEast)
					hasWall[1][r][c] = true;
				else
					hasWall[1][r][c] = false;
				if(_gridStatic & wallSouth)
					hasWall[2][r][c] = true;
				else
					hasWall[2][r][c] = false;
				if(_gridStatic & wallWest)
					hasWall[3][r][c] = true;
				else
					hasWall[3][r][c] = false;
				if(_gridContent & smallFruit)
					hasSmallFruit[r][c] = true;
				else
					hasSmallFruit[r][c] = false;
				if(_gridContent & bigFruit)
					hasBigFruit[r][c] = true;
				else
					hasBigFruit[r][c] = false;
				for(i = 0; i < MAXPLAYERCOUNT; i++)
					if(_gridContent & playerName[i])
					{
						players[i] = new Player(i, r, c);
						playerAlive++;
					}
				if(_gridStatic & generator)
				{
					for(i = 0; i < MAXDIRECTION; i++)
					{
						x = (r + moveDirection[i][0] + height) % height;
						y = (c + moveDirection[i][1] + width) % width;
						if((_staticField[x][y].asInt() & generator) == 0)
							canGenerateFruit[x][y] = true;
					}
				}
			}
		generateDistance();
		generateMapType();
	}

}

// 判断一个玩家的行动是否合法
inline bool GameBoard::ActionValid(int _player)
{
	ActionType _action = players[_player] -> action -> key;
	if(_action == stay) return true;
	if(_action >= shootUp)
		return _action < 8 && players[_player] -> power > skillCost;
	return _action >= 0 && _action < 4 &&
			!(hasWall[_action][players[_player] -> atRow][players[_player] -> atColumn]);
}

// 演算一个回合发生的事件
void GameBoard::StartTurn()
{
	int i, j, x, y, k;
	ActionType a;
	gameTurn++;
	int tempData[MAXHEIGHT][MAXWIDTH], savedPower[MAXHEIGHT][MAXWIDTH];
	int playersOn[MAXHEIGHT][MAXWIDTH];
	setSpeech9 = false;

	// 对输入进行修改：杀死不合法输入以及计算所有恐吓
	for(i = 0; i < MAXPLAYERCOUNT; i++)
		if(!players[i] -> isDead)
		{
			a = players[i] -> action -> key;
			if(a == stay) continue;
			x = players[i] -> atRow;
			y = players[i] -> atColumn;
			players[i] -> action -> atMapType = mapType[x][y];
			if(!ActionValid(i))
			{
				players[i] -> isDead = true;
				players[i] -> power = 0;
				grids[x][y] -> gridContent &= ~playerName[i];
				playerAlive--;
			}
			else if(a < shootUp)
			{
				x = (x + moveDirection[a][0] + height) % height;
				y = (y + moveDirection[a][1] + width) % width;
				if(grids[x][y] -> gridContent & playerMask)
					for(j = 0; j < MAXPLAYERCOUNT; j++)
						if((grids[x][y] -> gridContent & playerName[j]) && players[j] -> power > players[i] -> power)
						{
							players[i] -> action -> key = stay;
						}
			}
		}


	// 移动阶段
	for(i = 0; i < MAXPLAYERCOUNT; i++)
	{
		if(players[i] -> isDead) continue;
		a = players[i] -> action -> key;
		if(a == stay || a >= shootUp) continue;
		x = players[i] -> atRow;
		y = players[i] -> atColumn;
		grids[x][y] -> gridContent &= ~playerName[i];
		x = (x + moveDirection[a][0] + height) % height;
		y = (y + moveDirection[a][1] + width) % width;
		players[i] -> atRow = x;
		players[i] -> atColumn = y;
		grids[x][y] -> gridContent |= playerName[i];
	}


	// 物理战斗阶段
	memset(tempData,0,sizeof(tempData));
	memset(savedPower,0,sizeof(savedPower));
	for(i = 0; i < MAXPLAYERCOUNT; i++)
		if(!players[i] -> isDead)
		{
			x = players[i] -> atRow;
			y = players[i] -> atColumn;
			if(tempData[x][y] == 0)
			{
				tempData[x][y] = players[i] -> power;
				playersOn[x][y] = 1;
			}
			else
			{
				if(players[i] -> power > tempData[x][y])
				{
					for(j = 0; j < i; j++)
						if(!players[j] -> isDead && players[j] -> atRow == x
								&& players[j] -> atColumn == y)
						{
							grids[x][y] -> gridContent &= ~playerName[j];
							players[j] -> isDead = true;
							savedPower[x][y] += players[j] -> power;
						}
					tempData[x][y] = players[i] -> power;
					playersOn[x][y] = 1;
				}
				else if(players[i] -> power < tempData[x][y])
				{
					grids[x][y] -> gridContent &= ~playerName[i];
					players[i] -> isDead = true;
					savedPower[x][y] += players[i] -> power;
				}
				else playersOn[x][y]++;
			}
		}
	for(i = 0; i < MAXPLAYERCOUNT; i++)
	{
		x = players[i] -> atRow;
		y = players[i] -> atColumn;
		if(!players[i] -> isDead && savedPower[x][y] > 0)
			players[i] -> power += savedPower[x][y] / (2 * playersOn[x][y]);
	}


	// 技能施放阶段
	for(i = 0; i < MAXPLAYERCOUNT; i++)
	{
		if(players[i] -> isDead || players[i] -> action -> key < shootUp) continue;
		x = players[i] -> atRow;
		y = players[i] -> atColumn;
		j = (int)(players[i] -> action -> key - shootUp);
		players[i] -> power -= skillCost;
		while(!hasWall[j][x][y])
		{
			x = (x + moveDirection[j][0] + height) % height;
			y = (y + moveDirection[j][1] + width) % width;
			if(x == players[i] -> atRow && y == players[i] -> atColumn) break;
			if(grids[x][y] -> gridContent & playerMask)
				for(k = 0; k < MAXPLAYERCOUNT; k++)
					if(grids[x][y] -> gridContent & playerName[k])
					{
						players[k] -> power -= skillCost * 1.5;
						players[i] -> power += skillCost * 1.5;
						if(k == TomokoID) setSpeech9 = true;
					}
		}
	}
	for(i = 0; i < MAXPLAYERCOUNT; i++)
		if(!players[i] -> isDead && players[i] -> power <= 0)
		{
			x = players[i] -> atRow;
			y = players[i] -> atColumn;
			grids[x][y] -> gridContent &= ~playerName[i];
			players[i] -> isDead = true;
			playerAlive--;
		}


	// 豆子产生阶段
	if(--generatorTurnLeft == 0)
	{
		generatorTurnLeft = generatorInterval;
		par *p = gene;
		while(p != nil)
		{
			for(i = 0; i < MAXDIRECTION; i++)
			{
				x = (p -> row + moveDirection[i][0] + height) % height;
				y = (p -> column + moveDirection[i][1] + width) % width;
				if((grids[x][y] -> gridContent & (smallFruit | bigFruit))
						|| (grids[x][y] -> gridStatic & generator)) continue;
				grids[x][y] -> gridContent |= smallFruit;
				hasSmallFruit[x][y] = true;
			}
			p = p -> nex;
		}
	}


	// 吃豆阶段
	for(i = 0; i < MAXPLAYERCOUNT; i++)
		if(!players[i] -> isDead)
		{
			x = players[i] -> atRow;
			y = players[i] -> atColumn;
			if(grids[x][y] -> gridContent & playerMask & ~playerName[i]) continue;
			if(grids[x][y] -> gridContent & smallFruit)
			{
				grids[x][y] -> gridContent &= ~smallFruit;
				hasSmallFruit[x][y] = false;
				players[i] -> power++;
			}
			else if(grids[x][y] -> gridContent & bigFruit)
			{
				if(players[i] -> powerUpLeft == 0)
					players[i] -> power += bigFruitEnhancement;
				players[i] -> powerUpLeft += bigFruitDuration;
				grids[x][y] -> gridContent &= ~bigFruit;
				hasBigFruit[x][y] = false;
			}
		}


	// 大豆回合减少
	for(i = 0; i < MAXPLAYERCOUNT; i++)
		if(!players[i] -> isDead && players[i] -> powerUpLeft > 0)
		{
			players[i] -> powerUpLeft--;
			if(players[i] -> powerUpLeft == 0)
				players[i] -> power -= bigFruitEnhancement;
		}
	for(i = 0; i < MAXPLAYERCOUNT; i++)
		if(!players[i] -> isDead && players[i] -> power <= 0)
		{
			x = players[i] -> atRow;
			y = players[i] -> atColumn;
			grids[x][y] -> gridContent &= ~playerName[i];
			players[i] -> isDead = true;
			playerAlive--;
		}
}

// 分析上一局
void GameBoard::DataAnalyze(Json::Value &_data)
{
	string str;
	try
	{
		str = _data.asString();
		data["turnTo5"] = false;
		data["turnTo10"] = false;
		data["turnTo20"] = false;
		data["turnTo30"] = false;
		data["fixed"] = false;
		data["fixedRow"] = 0;
		data["fixedColumn"] = 0;
		data["generateValueMade"] = false;
	}
	catch(std::runtime_error &_error)
	{
		data = _data;
	}
	isFixed = data["fixed"].asBool();
	fixedRow = data["fixedRow"].asInt();
	fixedColumn = data["fixedColumn"].asInt();
	generateValueMade = data["generateValueMade"].asBool();
	if(generateValueMade)
	{
		Json::Value field = data["generateValue"];
		for(int r = 0; r < height; r++)
			for(int c = 0; c < width; c++)
				generateValue[r][c] = field[(Json::Value::UInt)r][(Json::Value::UInt)c].asInt();
	}
}

// 取回记忆
void GameBoard::GlobalDataAnalyze(Json::Value &_globalData)
{
	string str;
	try
	{
		str = _globalData.asString();
		globalData["test"] = 1;
	}
	catch(std::runtime_error &_error)
	{
		globalData = _globalData;
	}
}

void GameBoard::distanceBFS(int x, int y)
{
	int que[MAXHEIGHT * MAXWIDTH + 6][2];
	memset(visited,0,sizeof(visited));
	int le = 0, ri = 1;
	que[1][0] = x;
	que[1][1] = y;
	distance[x][y][x][y] = 0;
	visited[x][y] = true;
	int r = 0, x0, y0, i, x1, y1;
	for(i = 0; i < 4; i++)
		if(!hasWall[i][x][y])
		{
			x0 = (x + moveDirection[i][0] + height) % height;
			y0 = (y + moveDirection[i][1] + width) % width;
			actionToGoto[x][y][x0][y0] = (ActionType)i;
		}
	while(le < ri)
	{
		x0 = que[++le][0];
		y0 = que[le][1];
		if(r < distance[x][y][x0][y0]) r++;
		for(i = 0; i < 4; i++)
			if(!hasWall[i][x0][y0])
			{
				x1 = (x0 + moveDirection[i][0] + height) % height;
				y1 = (y0 + moveDirection[i][1] + width) % width;
				if(visited[x1][y1]) continue;
				visited[x1][y1] = true;
				que[++ri][0] = x1;
				que[ri][1] = y1;
				distance[x][y][x1][y1] = r + 1;
				if(actionToGoto[x][y][x1][y1] == stay)
					actionToGoto[x][y][x1][y1] = actionToGoto[x][y][x0][y0];
			}
	}
}

void GameBoard::calculateDevelopmentValue()
{
	memset(fruitCanEat,0,sizeof(fruitCanEat));
	memset(fruitValue,0,sizeof(fruitValue));
	memset(fruitLeftTurn,-1,sizeof(fruitLeftTurn));
	int r, c, i, j, x, y, t;

	// 1.计算fruitCanEat 以及 fruitLeftTurn
	for(r = 0; r < height; r++)
		for(c = 0; c < width; c++)
			if(hasSmallFruit[r][c] || hasBigFruit[r][c])
			{
				x = distance[atRow][atColumn][r][c];
				y = MAXINTEGER;
				t = 0;
				for(i = 0; i < MAXPLAYERCOUNT; i++)
					if(!players[i] -> isDead && i != TomokoID)
					{
						if(distance[players[i] -> atRow][players[i] -> atColumn][r][c] <= y)
						{
							if(y == MAXINTEGER)
							{
								y = distance[players[i] -> atRow][players[i] -> atColumn][r][c];
								t = i;
							}
							else if(distance[players[i] -> atRow][players[i] -> atColumn][r][c] == y)
							{
								if(players[i] -> power > players[t] -> power) t = i;
							}
						}
					}
				if(x < y) fruitCanEat[r][c] = true;
				else if(x == y && players[TomokoID] -> power >= players[t] -> power)
					fruitCanEat[r][c] = true;
				fruitLeftTurn[r][c] = y;
			}

	// 2.对每一点的生成器价值进行分析（只在第一回合进行）
	int _generateValue[MAXHEIGHT][MAXWIDTH];
	if(!generateValueMade)
	{
		memset(generateValue,0,sizeof(generateValue));
		for(r = 0; r < height; r++)
			for(c = 0; c < width; c++)
				if((grids[r][c] -> gridStatic & generator) == 0)
				{
					memset(visited,0,sizeof(visited));
					_generateValue[r][c] = generateValueDFS(r,c,0);
					if(canGenerateFruit[r][c]) _generateValue[r][c] += 4;
					if(mapType[r][c] >= 3) _generateValue[r][c] -= 4;
				}
		for(r = 0; r < height; r++)
			for(c = 0; c < width; c++)
				if((grids[r][c] -> gridStatic & generator) == 0)
				{
					generateValue[r][c] = _generateValue[r][c];
					for(t = 0; t < 4; t++)
						if(!hasWall[t][r][c])
						{
							x = (r + moveDirection[t][0] + height) % height;
							y = (c + moveDirection[t][1] + width) % width;
							generateValue[r][c] += _generateValue[x][y];
						}
				}
		generateValueMade = true;
		data["generateValueMade"] = true;
		for(r = 0; r < height; r++)
			for(c = 0; c < width; c++)
				data["generateValue"][(Json::Value::UInt)r][(Json::Value::UInt)c] = generateValue[r][c];
	}

	// 3.对每个水果的连续价值进行分析
	memset(visited,0,sizeof(visited));
	for(r = 0; r < height; r++)
		for(c = 0; c < width; c++)
			if(fruitCanEat[r][c])
			{
				fruitValue[r][c] = fruitValueDFS(r,c,distance[atRow][atColumn][r][c]);
				if(distance[atRow][atColumn][r][c] == 1)
				{
					fruitValue[r][c] += 5;
				}
				else if(distance[atRow][atColumn][r][c] == 2)
				{
					fruitValue[r][c] += 2;
				}
			}

	// 4.计算每个点在几回合内可能被强的英雄接近
	for(r = 0; r < height; r++)
		for(c = 0; c < width; c++)
			distanceForAttack[r][c] = MAXINTEGER;
	for(r = 0; r < height; r++)
		for(c = 0; c < width; c++)
			if((grids[r][c] -> gridStatic & generator) == 0)
			{
				for(i = 0; i < MAXPLAYERCOUNT; i++)
					if(!players[i] -> isDead && i != TomokoID)
					{
						if(players[i] -> power > players[TomokoID] -> power)
							distanceForAttack[r][c] = std::min(distanceForAttack[r][c],
									distance[players[i] -> atRow][players[i] -> atColumn][r][c]);
					}
			}
}

int GameBoard::generateValueDFS(int r, int c, int used)
{
	visited[r][c] = true;
	int x, y, ans = 0;
	for(x = 0; x < height; x++)
		for(y = 0; y < width; y++)
			if(canGenerateFruit[x][y] && !visited[x][y] && used + distance[r][c][x][y] < 8)
			{
				ans = std::max(ans, generateValueDFS(x, y, used + distance[r][c][x][y]));
			}
	visited[r][c] = false;
	if(canGenerateFruit[r][c]) return ans + 1;
	else return ans;
}

int GameBoard::fruitValueDFS(int r, int c, int used)
{
	visited[r][c] = true;
	int x, y, ans = 0;
	for(x = 0; x < height; x++)
		for(y = 0; y < width; y++)
			if(fruitCanEat[x][y]
					&& !visited[x][y] && used + distance[r][c][x][y] < std::min(fruitLeftTurn[x][y],16))
			{
				ans = std::max(ans, fruitValueDFS(x, y, used + distance[r][c][x][y]));
			}
	visited[r][c] = false;
	if(fruitCanEat[r][c]) ans++;
	if(hasSmallFruit[r][c] || hasBigFruit[r][c]) return ans + 1;
	else return ans;
}



int GameBoard::nearStrong(int r, int c)
{
	int i, ans = MAXINTEGER, res = -1;
	for(i = 0; i < MAXPLAYERCOUNT; i++)
		if(!players[i] -> isDead && players[i] -> power > players[TomokoID] -> power)
		{
			if(players[i] -> powerUpLeft == 0 ||
				players[i] -> power > players[TomokoID] -> power + bigFruitEnhancement
				|| players[i] -> powerUpLeft >= distance[players[i] -> atRow][players[i] -> atColumn][r][c])
				if(distance[players[i] -> atRow][players[i] -> atColumn][r][c] < ans)
				{
					ans = distance[players[i] -> atRow][players[i] -> atColumn][r][c];
					res = i;
				}
		}
	return res;
}

// 追击战,如果一定可以追上返回true
bool GameBoard::chaseAfter(int xc, int yc, int xe, int ye, int lim)
{
	if(distance[xc][yc][xe][ye] == 0) return true;
	if(lim == 0) return false;
	int t, x, y, x0, y0;
	int u = (int)actionToGoto[xc][yc][xe][ye];
	x = (xc + moveDirection[u][0] + height) % height;
	y = (yc + moveDirection[u][1] + width) % width;
	for(t = 3; t >= 0; t--)
		if(!hasWall[t][xe][ye])
		{
			x0 = (xe + moveDirection[t][0] + height) % height;
			y0 = (ye + moveDirection[t][1] + width) % width;
			if(x0 == xc && y0 == yc) continue;
			if(chaseAfter(x,y,x0,y0,lim - 1) == false) return false;
		}
	return true;
}

// 对一个点是否可以进入进行预测
bool GameBoard::analyzePoint(int r, int c)
{
	int x, y, t, x0, y0, i;
	bool tf;
	for(i = 0; i < MAXPLAYERCOUNT; i++)
		if(!players[i] -> isDead && players[i] -> power > players[TomokoID] -> power)
		{
			x = atRow;
			y = atColumn;
			x0 = players[i] -> atRow;
			y0 = players[i] -> atColumn;
			if(distance[x0][y0][r][c] >= 4) continue;
			while((x != r || y != c) && (x0 != r || y0 != c))
			{
				t = (int)actionToGoto[x][y][r][c];
				x = (x + moveDirection[t][0] + height) % height;
				y = (y + moveDirection[t][1] + width) % width;
				t = (int)actionToGoto[x0][y0][r][c];
				x0 = (x0 + moveDirection[t][0] + height) % height;
				y0 = (y0 + moveDirection[t][1] + width) % width;
			}
			if(x0 == r && y0 == c) return false;

			// 如果会在追击战中失败则不可以去
			if(chaseAfter(x0,y0,x,y,3)) return false;

			// 距离为1或2时如果会被强制进入3或4也不可以去
			if(players[i] -> power > skillCost && distance[x0][y0][x][y] <= 2)
			{
				if(mapType[x][y] == 4) return false;
				if(distance[x0][y0][x][y] == 1 && mapType[x][y] == 3) return false;
				tf = true;
				for(t = 3; t >= 0; t--)
					if(!hasWall[t][x][y])
					{
						x = (x + moveDirection[t][0] + height) % height;
						y = (y + moveDirection[t][1] + width) % width;
						if(distance[x][y][x0][y0] <= 1) continue;
						if(mapType[x][y] <= 2)
						{
							tf = false;
							break;
						}
					}
				if(tf) return false;
			}
		}
	return true;
}

void GameBoard::calculateBestMoveForDevelopment()
{
	calculateDevelopmentValue();
	int fruitLeft = 0, r, c, x, y, i, j, t;
	for(r = 0; r < height; r++)
		for(c = 0; c < width; c++)
			if(fruitCanEat[r][c])
				fruitLeft++;

	// 1. 如果已经固定位置（包括重计算可行性）
	if(isFixed)
	{
		if(atRow == fixedRow && atColumn == fixedColumn)
		{
			isFixed = false;
			data["fixed"] = false;
		}
		else
		{
			if(analyzePoint(fixedRow, fixedColumn))
			{
				stringstream sstr;
				sstr << "fixed: " << fixedRow << "," << fixedColumn;
				debugSpeech = sstr.str();
				bestMoveForDevelopment = actionToGoto[atRow][atColumn][fixedRow][fixedColumn];
				return;
			}
		}
	}


	// 特判情况
	for(i = 0; i < MAXPLAYERCOUNT; i++)
		if(!players[i] -> isDead && i != TomokoID && players[i] -> atRow == atRow
				&& players[i] -> atColumn == atColumn && hasBigFruit[atRow][atColumn])
		{
			setSpeech10 = true;
			bestMoveForDevelopment = stay;
			return;
		}

	// 特判情况
	for(i = 0; i < MAXPLAYERCOUNT; i++)
		if(!players[i] -> isDead && i != TomokoID && players[i] -> atRow == atRow
				&& players[i] -> atColumn == atColumn)
		{
			t = MAXINTEGER;
			for(r = 0; r < height; r++)
				for(c = 0; c < width; c++)
					if(hasBigFruit[r][c] && fruitCanEat[r][c] && distance[atRow][atColumn][r][c] <= 4)
					{
						if(distance[atRow][atColumn][r][c] < t)
						{
							t = distance[atRow][atColumn][r][c];
							x = r;
							y = c;
						}
					}
			if(t != MAXINTEGER)
			{
				isFixed = true;
				data["fixed"] = true;
				data["fixedRow"] = x;
				data["fixedColumn"] = y;
				fixedRow = x;
				fixedColumn = y;
				bestMoveForDevelopment = actionToGoto[atRow][atColumn][x][y];
				setSpeech10 = true;
				return;
			}
		}

	// 2. 检查有没有可以吃的果实
	bool temp[MAXHEIGHT][MAXWIDTH], fs = false;
/*
	// 2.5 防傻
	for(t = 0; t < 4; t++)
		if(!hasWall[t][atRow][atColumn])
		{
			x = (atRow + moveDirection[t][0] + height) % height;
			y = (atColumn + moveDirection[t][1] + width) % width;
			if(fruitCanEat[x][y])
			{
				bestMoveForDevelopment = (ActionType)t;
				return;
			}
		}
*/
	memset(temp,0,sizeof(temp));
	for(r = 0; r < height; r++)
		for(c = 0; c < width; c++)
			if(fruitCanEat[r][c])
				temp[r][c] = true;
	while(fruitLeft > 0)
	{
		t = MININTEGER;
		for(r = 0; r < height; r++)
			for(c = 0; c < width; c++)
				if(temp[r][c])
				{
					if(fruitValue[r][c] > t ||
						(fruitValue[r][c] == t &&
						distance[atRow][atColumn][r][c] < distance[atRow][atColumn][x][y]))
					{
						t = fruitValue[r][c];
						x = r;
						y = c;
					}
				}
		temp[x][y] = false;
		fruitLeft--;
		if(analyzePoint(x,y))
		{
			r = atRow;
			c = atColumn;
			j = 0;
			while(!hasSmallFruit[r][c] && !hasBigFruit[r][c] && j <= 3)
			{
				t = (int)actionToGoto[r][c][x][y];
				r = (r + moveDirection[t][0] + height) % height;
				c = (c + moveDirection[t][1] + width) % width;
				j++;
			}
			isFixed = true;
			data["fixed"] = true;
			data["fixedRow"] = r;
			data["fixedColumn"] = c;
			fixedRow = r;
			fixedColumn = c;
			bestMoveForDevelopment = actionToGoto[atRow][atColumn][r][c];
			debugSpeech = "标准";
			fs = true;
			break;
		}
	}
	int x2 = x, y2 = y;

	// 3.前往生成器位置
	if(!fs || generatorTurnLeft < distance[atRow][atColumn][x2][y2])
	{
		x = atRow;
		y = atColumn;
		int v = MININTEGER;
		for(r = 0; r < height; r++)
			for(c = 0; c < width; c++)
				if(distance[atRow][atColumn][r][c] != 0)
				{
					j = nearStrong(r,c);
					if(j != -1 && distance[players[j] -> atRow][players[j] -> atColumn][r][c] <=
							distance[atRow][atColumn][r][c]) continue;
					if(generateValue[r][c] > v ||
						(generateValue[r][c] == v &&
						 distance[atRow][atColumn][r][c] < distance[atRow][atColumn][x][y]))
					{
						x = r;
						y = c;
						v = generateValue[r][c];
					}
				}
		if(v == MININTEGER)
		{
			for(r = 0; r < height; r++)
				for(c = 0; c < width; c++)
					if(distance[atRow][atColumn][r][c] != 0)
					{
						if(generateValue[r][c] > v ||
							(generateValue[r][c] == v &&
								distance[atRow][atColumn][r][c] < distance[atRow][atColumn][x][y]))
						{
							x = r;
							y = c;
							v = generateValue[r][c];
						}
					}
		}
		isFixed = false;
		data["fixed"] = false;
		bestMoveForDevelopment = actionToGoto[atRow][atColumn][x][y];
		debugSpeech = "生成器";
	}

}


ActionType GameBoard::findAttackPoint()
{
	int i, x, y, r, c, t, j;
	ActionChain *a;

	// 3.追杀英雄
	for(i = 0; i < MAXPLAYERCOUNT; i++)
		if(!players[i] -> isDead && players[i] -> power < players[TomokoID] -> power &&
				mapType[players[i] -> atRow][players[i] -> atColumn] == 4)
		{
			t = 0;
			while(hasWall[t][players[i] -> atRow][players[i] -> atColumn]) t++;
			x = (players[i] -> atRow + moveDirection[t][0] + height) % height;
			y = (players[i] -> atColumn + moveDirection[t][1] + width) % width;
			if(distance[atRow][atColumn][x][y] >= 2) continue;
			t = (int)actionToGoto[atRow][atColumn][players[i] -> atRow][players[i] -> atColumn];
			x = (atRow + moveDirection[t][0] + height) % height;
			y = (atColumn + moveDirection[t][1] + width) % width;
			t = nearStrong(x,y);
			if(t != -1 && distance[players[t] ->atRow][players[t] -> atColumn][x][y] <= 2) continue;
			debugSpeech = "attack 3";
			setSpeech5 = true;
			return actionToGoto[atRow][atColumn][players[i] -> atRow][players[i] -> atColumn];
		}
	if(players[TomokoID] -> power <= skillCost)
		return stay;

	int p = nearStrong(atRow,atColumn);
	// 1. 遇到在3或者4的直线上英雄
	for(i = 0; i < MAXPLAYERCOUNT; i++)
		if(!players[i] -> isDead && mapType[players[i] -> atRow][players[i] -> atColumn] >= 3
				&& i != TomokoID)
		{
			if(directGoto[atRow][atColumn][players[i] -> atRow][players[i] -> atColumn] != stay
				&& (p == -1 || distance[players[p] -> atRow][players[p] -> atColumn][atRow][atColumn] >= 2)
				&& (distance[atRow][atColumn][players[i] -> atRow][players[i] -> atColumn] > 1 ||
					players[i] -> power < players[TomokoID] -> power))
			{
				debugSpeech = "attack  1";
				return (ActionType)
			((int)directGoto[atRow][atColumn][players[i] -> atRow][players[i] -> atColumn] + 4);
			}
		}

	// 2.自己在3或4时直线上有英雄
	if(mapType[atRow][atColumn] >= 3)
	{
		for(i = 0; i < MAXPLAYERCOUNT; i++)
			if(!players[i] -> isDead && i != TomokoID &&
			directGoto[atRow][atColumn][players[i] -> atRow][players[i] -> atColumn] != stay)
			{
				if(players[i] -> power == players[TomokoID] -> power &&
					distance[players[i]->atRow][players[i]->atColumn][atRow][atColumn] == 1)
				{
					t = nearStrong(players[i] -> atRow, players[i] -> atColumn);
					if(t == -1 ||
			distance[players[t] -> atRow][players[t] -> atColumn][players[i] -> atRow][players[i] -> atColumn] >= 2)
					{
						debugSpeech = "attack 2.5";
						return actionToGoto[atRow][atColumn][players[i] -> atRow][players[i] -> atColumn];
					}
				}
				if(players[i] -> power > skillCost
					&& (p == -1 || distance[players[p] -> atRow][players[p] -> atColumn][atRow][atColumn] >= 2))
				{
					debugSpeech = "attack  2";
					return (ActionType)
					((int)directGoto[atRow][atColumn][players[i] -> atRow][players[i] -> atColumn] + 4);
				}
			}
	}


	// 4. 英雄准备从坑内出来
	for(i = 0; i < MAXPLAYERCOUNT; i++)
		if(!players[i] -> isDead && mapType[players[i] -> atRow][players[i] -> atColumn] == 4
				&& i != TomokoID)
		{
			t = 0;
			while(hasWall[t][players[i] -> atRow][players[i] -> atColumn]) t++;
			x = (players[i] -> atRow + moveDirection[t][0] + height) % height;
			y = (players[i] -> atColumn + moveDirection[t][1] + width) % width;
			if(directGoto[atRow][atColumn][x][y] != stay)
			{
				bool tf = true;
				a = players[i] -> action;
				while(a != aNil)
				{
					if(a -> key == stay && a -> atMapType == 4)
					{
						tf = false;
						break;
					}
					a = a -> nex;
				}
				if(tf && (p == -1 || distance[players[p] -> atRow][players[p] -> atColumn][atRow][atColumn] >= 2)
					&& (players[TomokoID] -> power > int(1.5 * skillCost) + bigFruitEnhancement
						|| (players[TomokoID] -> power > int(1.5 * skillCost) && players[TomokoID] -> powerUpLeft == 0)))
				{
					debugSpeech = "attack 4";
					return (ActionType)
				((int)directGoto[atRow][atColumn][x][y] + 4);
				}
			}
		}

	// 5.英雄不得不进入直线
	bool tt, ff;
	ActionType _ac = stay;
	if(players[TomokoID] -> power > int(1.5 * skillCost) + bigFruitEnhancement
			||(players[TomokoID] -> power > int(1.5 * skillCost) && players[TomokoID] -> powerUpLeft == 0))
	{
		for(i = 0; i < MAXPLAYERCOUNT; i++)
			if(!players[i] -> isDead && i != TomokoID)
			{
				tt = true;
				for(t = 3; t >= 0; t--)
					if(!hasWall[t][players[i] -> atRow][players[i] -> atColumn])
					{
						ff = false;
						r = (players[i] -> atRow + moveDirection[t][0] + height) % height;
						c = (players[i] -> atColumn + moveDirection[t][1] + width) % width;
						for(j = 0; j < MAXPLAYERCOUNT; j++)
							if(!players[j] -> isDead && j != i && players[j] -> atRow == r
									&& players[j] -> atColumn == c)
							{
								if(players[j] -> power > players[i] -> power) ff = true;
							}
						if(!ff)
						{
							if(directGoto[atRow][atColumn][r][c] != stay)
							{
								if(_ac == stay)
								{
									_ac = directGoto[atRow][atColumn][r][c];
									ff = true;
								}
							}
						}
						if(!ff) tt = false;
					}
				if(tt && _ac != stay
				&& (p == -1 || distance[players[p] -> atRow][players[p] -> atColumn][atRow][atColumn] >= 2))
				{
					debugSpeech = "attack  5";
					return (ActionType)(_ac + 4);
				}
			}
	}
/*
	// 6.对直线上英雄进行预测
	if(players[TomokoID] -> power > int(1.5 * skillCost) + bigFruitEnhancement
			||(players[TomokoID] -> power > int(1.5 * skillCost) && players[TomokoID] -> powerUpLeft == 0))
	{
		for(i = 0; i < MAXPLAYERCOUNT; i++)
			if(!players[i] -> isDead && i != TomokoID &&
					directGoto[atRow][atColumn][players[i] -> atRow][players[i] -> atColumn] != stay)
			{
				t = MAXINTEGER;
				for(r = 0; r < height; r++)
					for(c = 0; c < width; c++)
						if(hasSmallFruit[r][c] || hasBigFruit[r][c])
						{
							if(distance[players[i] -> atRow][players[i] -> atColumn][r][c] < t)
							{
								t = distance[players[i] -> atRow][players[i] -> atColumn][r][c];
								x = r;
								y = c;
							}
						}
				if(t == MAXINTEGER) continue;
				j = (int)actionToGoto[players[i] -> atRow][players[i] -> atColumn][x][y];
				x = (x + moveDirection[j][0] + height) % height;
				y = (y + moveDirection[j][1] + width) % width;
				if(directGoto[atRow][atColumn][x][y] ==
						directGoto[atRow][atColumn][players[i] -> atRow][players[i] -> atColumn])
				{
					debugSpeech = "attack 6";
					return (ActionType)((int)
						directGoto[atRow][atColumn][players[i] -> atRow][players[i] -> atColumn] + 4);
				}
			}
	}
*/
	return stay;
}


// 多项式计算
template <typename T>
T GameBoard::calculatePolynomial(CoefficientChain<T> *a, T x)
{
	T ans = 0;
	CoefficientChain<T> *p = a;
	while(p != NULL)
	{
		ans *= x;
		ans += p -> key;
		p = p -> nex;
	}
	return ans;
}


void GameBoard::generateDistance()
{
	int r, c, x, y, i;
	memset(distance,30,sizeof(distance));
	for(r = 0; r < height; r++)
		for(c = 0; c < width; c++)
			distanceBFS(r,c);
	for(r = 0; r < height; r++)
		for(c = 0; c < width; c++)
			for(x = 0; x < height; x++)
				for(y = 0; y < width; y++)
					directGoto[r][c][x][y] = stay;
	for(r = 0; r < height; r++)
		for(c = 0; c < width; c++)
		{
			for(i = 0; i < 4; i++)
				if(!hasWall[i][r][c])
				{
					x = (r + moveDirection[i][0] + height) % height;
					y = (c + moveDirection[i][1] + width) % width;
					while(x != r || y != c)
					{
						directGoto[r][c][x][y] = (ActionType)i;
						if(hasWall[i][x][y]) break;
						x = (x + moveDirection[i][0] + height) % height;
						y = (y + moveDirection[i][1] + width) % width;
					}
				}
		}
}

void GameBoard::generateMapType()
{
	int r, c, i, x;
	for(r = 0; r < height; r++)
		for(c = 0; c < width; c++)
		{
			if(grids[r][c] -> gridStatic & generator)
				mapType[r][c] = -1;
			else
			{
				x = 0;
				for(i = 0; i < 4; i++)
					if(hasWall[i][r][c])
						x++;
				switch(x)
				{
				case 0: mapType[r][c] = 0; break;
				case 1: mapType[r][c] = 1; break;
				case 2:
					if((hasWall[0][r][c] && hasWall[2][r][c]) || (hasWall[1][r][c] && hasWall[3][r][c]))
						mapType[r][c] = 3;
					else
						mapType[r][c] = 2;
					break;
				case 3: mapType[r][c] = 4; break;
				default: mapType[r][c] = -1;
				}
			}
		}
}



void GameBoard::TomokoStart()
{
	ActionType _ac;
	int p, t;
	atRow = players[TomokoID] -> atRow;
	atColumn = players[TomokoID] -> atColumn;

	calculateBestMoveForDevelopment();



#ifndef _BOTZONE_ONLINE
	int r, c;
	for(r = 0; r < height; r++)
	{
		for(c = 0; c < width; c++)
			cout << fruitValue[r][c] << "  ";
		cout << endl;
	}
#endif


	// 防守阶段
	if(bestMoveForDevelopment >= shootUp)
		_ac = stay;
	else
		_ac = bestMoveForDevelopment;
	int x= atRow, y = atColumn;
	if(_ac != stay)
	{
		x = (x + moveDirection[(int)_ac][0] + height) % height;
		y = (y + moveDirection[(int)_ac][1] + width) % width;
	}
	p = nearStrong(x,y);
	bool tf = false;
	if(p != -1)
	{
		switch(distance[players[p] -> atRow][players[p] -> atColumn][x][y])
		{
		case 0:
		case 1: tf = true; break;
		case 2:
			int x0 = players[p] -> atRow,
				y0 = players[p] -> atColumn;
			if(chaseAfter(x0,y0,x,y,3)) tf = true;
			break;
		}
		if(tf)
		{
			for(t = 3; t >= 0; t--)
				if(!hasWall[t][atRow][atColumn])
				{
					x = (atRow + moveDirection[t][0] + height) % height;
					y = (atColumn + moveDirection[t][1] + width) % width;
					if(distance[players[p] -> atRow][players[p] -> atColumn][x][y] >= 2
						&& mapType[x][y] != 4) break;
				}
			if(t != -1)
			{
				bestMoveForDevelopment =
						(ActionType)t;
				setSpeech10 = true;
				debugSpeech = "防守";
			}
		}
	}
	_ac = findAttackPoint();
	if(_ac != stay)
	{
		bestMoveForDevelopment = _ac;
	}

	// 特判会不会被吃
	if(bestMoveForDevelopment >= shootUp)
		_ac = stay;
	else
		_ac = bestMoveForDevelopment;
	x = atRow;
	y = atColumn;
	t = (int)_ac;
	if(t != -1)
	{
		x = (x + moveDirection[t][0] + height) % height;
		y = (y + moveDirection[t][1] + width) % width;
	}
	p = nearStrong(x,y);
	if(p != -1 && distance[players[p] -> atRow][players[p] -> atColumn][x][y] <= 1)
	{
		for(t = 3; t >= 0; t--)
			if(!hasWall[t][atRow][atColumn])
			{
				x = (atRow + moveDirection[t][0] + height) % height;
				y = (atColumn + moveDirection[t][1] + width) % width;
				if(distance[players[p] -> atRow][players[p] -> atColumn][x][y] >= 2
					&& mapType[x][y] != 4) break;
			}
		bestMoveForDevelopment = (ActionType)t;
	}


	// 安全检查
	if(bestMoveForDevelopment != stay)
	{
		if(bestMoveForDevelopment < shootUp)
		{
			t = (int)bestMoveForDevelopment;
			if(hasWall[t][atRow][atColumn])
			{
				setSpeech9 = true;
				bestMoveForDevelopment = stay;
			}
		}
		else
		{
			t = (int)bestMoveForDevelopment - 4;
			if(hasWall[t][atRow][atColumn] || players[TomokoID] -> power <= skillCost)
			{
				setSpeech9 = true;
				bestMoveForDevelopment = stay;
			}
		}
	}

	// 台词选择
	int _speech = 0;
	if(gameTurn <= 2)
	{
		_speech = 1;
	}
	else
	{
		if(bestMoveForDevelopment >= shootUp)
		{
			_speech = GameTools::RanInt(6, 8);
		}
		else if(setSpeech9)
		{
			_speech = 9;
		}
		else if(setSpeech5)
		{
			_speech = 5;
		}
		else if(setSpeech10)
		{
			_speech = 10;
		}
		else if(players[TomokoID] -> power >= 5 &&
				data["turnTo5"].asBool() == false)
		{
			_speech = 2;
			data["turnTo5"] = true;
		}
		else if(players[TomokoID] -> power >= 10 &&
				data["turnTo10"].asBool() == false)
		{
			_speech = 3;
			data["turnTo10"] = true;
		}
		else if(players[TomokoID] -> power >= 20 &&
				data["turnTo20"].asBool() == false)
		{
			_speech = 2;
			data["turnTo20"] = true;
		}
		else if(players[TomokoID] -> power >= 30 &&
				data["turnTo30"].asBool() == false)
		{
			_speech = 4;
			data["turnTo30"] = true;
		}
	}



	globalData["debugData"][gameTurn] = debugSpeech;

	GameTools::WriteOutput(bestMoveForDevelopment, speeches[_speech], data, globalData, "");
}

/* GameBoard类end */

/* par结构 */

par::par()
{
	row = column = 0;
	nex = nil;
}

par::par(int r, int c): row(r), column(c)
{
	nex = nil;
}

/* par结构end */

/* ActionChain结构 */

ActionChain::ActionChain()
{
	key = stay;
	nex = aNil;
}

ActionChain::ActionChain(ActionType _action, ActionChain *_nex):
		key(_action), nex(_nex)
{}

/* ActionChain结构end */


/* Grid类 */

Grid::Grid()
{
	gridContent = empty;
	gridStatic = emptyWall;
}

Grid::Grid(const GridContentType &_gridContent, const GridStaticType &_gridStatic)
{
	gridContent = _gridContent;
	gridStatic = _gridStatic;
}

/* Grid类end */


/* Player类 */

Player::Player(int _playerID, int _atRow, int _atColumn):
		playerID(_playerID), atRow(_atRow), atColumn(_atColumn)
{
	isDead = false;
	power = 1;
	powerUpLeft = 0;
	action = aNil;
}



/* Player类end */


/* 功能实现细节部分end */
