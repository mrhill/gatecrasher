#ifndef GATECRASHER_GC_H
#define GATECRASHER_GC_H

#include <cstdint>

enum DIR { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

struct Pos
{
	int32_t x, y;
	Pos(int32_t x_=0, int32_t y_=0) : x(x_), y(y_) {}
	bool operator==(const Pos& o) { return x==o.x && y==o.y; }
	Pos next(DIR dir) const;
};

#define TILETYPESHIFT 6
#define TILEMASK 0x3F

enum TILETYPE
{
	TILETYPE_BG = 0,
	TILETYPE_WALL,
	TILETYPE_GATE,
	TILETYPE_HOLE
};

enum TILE
{
	TILE_BG = TILETYPE_BG << TILETYPESHIFT,

	TILE_WALL_T = TILETYPE_WALL << TILETYPESHIFT,
	TILE_WALL_B,
	TILE_WALL_L,
	TILE_WALL_R,
	TILE_WALL_TL,
	TILE_WALL_TR,
	TILE_WALL_BL,
	TILE_WALL_BR,
	TILE_WALL_HOLE_L,
	TILE_WALL_HOLE_R,
	TILE_WALL_MID,

	TILE_GATE = TILETYPE_GATE << TILETYPESHIFT,
	TILE_HOLE = TILETYPE_HOLE << TILETYPESHIFT
};

enum BALLTYPE {	BALLTYPE_NORMAL, BALLTYPECOUNT };
enum BALLSTATE { BALLSTATE_FRESH, BALLSTATE_LATCHED, BALLSTATE_ACTIVE, BALLSTATE_GOAL, BALLSTATE_BUST };
struct Ball
{
	Pos       mPos;
	BALLTYPE  mType;
	BALLSTATE mState;
	DIR       mDir;
	uint32_t  mSubPos = 0;

	Ball(const Pos& pos, BALLSTATE state = BALLSTATE_FRESH, BALLTYPE type = BALLTYPE_NORMAL) : mPos(pos), mType(type), mState(state), mDir(DIR_DOWN) {}
	inline Pos pos() const { return mPos; }
};

enum CARTYPE { CARTYPE_NORMAL, CARTYPECOUNT };
struct Car
{
	Pos mPos;
	CARTYPE  mType = CARTYPE_NORMAL;
	inline void setPos(const Pos& pos) { mPos = pos; }
	inline Pos pos() const { return mPos; }
	inline int32_t x() const { return mPos.x; }
	inline int32_t y() const { return mPos.y; }
};

struct IField
{
	uint32_t mTop;	// start y pos of map
	uint32_t mWidth;
	uint32_t mHeight;
	uint8_t* mpMap = nullptr;
};

enum GCKEY
{
	GCKEY_NONE  = 0,
	GCKEY_LEFT  = 0x01,
	GCKEY_RIGHT = 0x02,
	GCKEY_UP    = 0x04,
	GCKEY_DOWN  = 0x08,
	GCKEY_FIRE  = 0x10
};

struct IGame
{
	uint32_t mLevel = 0;
	uint32_t mPoints = 0;
	static IGame& instance();
	virtual IField& field() = 0;
	virtual Car& car() = 0;
	virtual void initLevel(uint32_t level) = 0;
	virtual void poll(uint32_t keymask) = 0;
	virtual uint32_t ballCount() = 0;
	virtual Ball& ball(uint32_t idx) = 0;
};

#endif
