#include "gc.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <stdexcept>

using namespace std;

Pos Pos::next(DIR dir) const
{
	Pos p {x, y};
	switch(dir) {
		case DIR_UP: p.y--; break;
		case DIR_DOWN: p.y++; break;
		case DIR_LEFT: p.x--; break;
		case DIR_RIGHT: p.x++; break;
	}
	return p;
}

struct Field : IField
{
	uint32_t mScrollPos;
	uint32_t mScrollLines;
	uint32_t mScrollWidth;
	uint32_t mScrollHeight;
	uint8_t* mpScrollMap = nullptr;

	enum {
		MARGINTOP = 3,       // minimum margin at top for car
		WALLWIDTH = 1,       // left and right wall widht
		HOLEAREAHEIGHT = 4,  // 1 brim, 2 hole, 1 bottom wall
		TOPHOLEHEIGHT = 2,   // 1 brim, 1 space
		HOLEDIST = 3,        // distance between holes
		SCROLLLINEHEIGHT = 3 // 2 wall 1 space
	};

	void resize(uint32_t w, uint32_t h)
	{
		uint32_t ws = w - (WALLWIDTH<<1);
		uint32_t hs = h - HOLEAREAHEIGHT - MARGINTOP - TOPHOLEHEIGHT;
		mScrollLines = hs / SCROLLLINEHEIGHT;
		hs = mScrollLines * SCROLLLINEHEIGHT;
		mTop = h - HOLEAREAHEIGHT - hs - TOPHOLEHEIGHT;

		uint8_t* m = (uint8_t*)::realloc(mpMap, w*h);
		uint8_t* sm = (uint8_t*)::realloc(mpScrollMap, ws*hs);
		if (!m || !sm)
			throw runtime_error("out of mem");

		mpMap = m;
		mWidth = w;
		mHeight = h;
		mpScrollMap = sm;
		mScrollWidth = ws;
		mScrollHeight = hs;
		mScrollPos = 0;
	}

	Field(uint32_t w = 29, uint32_t h = 24) { ::srand(::time(nullptr)); resize(w, h);  }
	~Field() { ::free(mpScrollMap); ::free(mpMap); }

	struct Hole
	{
		Pos pos;
		int ball;
		Hole(const Pos& p) : pos(p), ball(-1) {}
	};
	vector<Hole> mHoles;

	int fillHole(Pos p, int ball) // returns old ball if hole was filled
	{
		for (Hole& h : mHoles)
			if (h.pos == p) {
				if (h.ball == -1) {
					h.ball = ball;
					return -1;
				} else {
					ball = h.ball;
					h.ball = -1;
					return ball;
				}
			}
		return -1;
	}

	bool testHolesFilled() const
	{
		for (const Hole& h : mHoles)
			if (h.ball == -1)
				return false;
		return true;
	}

	inline Pos startPos() const
	{
		uint32_t const holes = (mWidth-2) / HOLEDIST;
		return Pos(2 + (holes>>1)*HOLEDIST, mTop-1);
	}

	uint8_t tile(Pos p) const
	{
		if (((uint32_t)p.x < mWidth) && ((uint32_t)p.y < mHeight))
			return mpMap[p.y*mWidth+p.x];
		return TILE_BG;
	}

	void scrollUp()
	{
		mScrollPos = mScrollPos + SCROLLLINEHEIGHT;
		if (mScrollPos >= mScrollHeight)
			mScrollPos -= mScrollHeight;
	}

	void scrollDown()
	{
		mScrollPos = mScrollPos - SCROLLLINEHEIGHT;
		if ((int)mScrollPos < 0)
			mScrollPos += mScrollHeight;
	}

	void initScrollField(uint32_t level)
	{
		for (uint32_t y = 0; y < mScrollLines; y++)
		{
			uint8_t* m = mpScrollMap + mScrollWidth * y * SCROLLLINEHEIGHT;
			memset(m, TILE_BG, mScrollWidth*SCROLLLINEHEIGHT);

			for (uint32_t x = 0; x < mScrollWidth;) {
				uint32_t w = (::rand() & 7) + 2;
				if (x + w > mScrollWidth)
					w = mScrollWidth - x;
				for (uint32_t i=0; i<w; i++) {
					m[x+i]              = i==0 && x ? TILE_WALL_TL : i<(w-1) || (x+i)==(mScrollWidth-1) ? TILE_WALL_T : TILE_WALL_TR;
					m[x+i+mScrollWidth] = i==0 && x ? TILE_WALL_BL : i<(w-1) || (x+i)==(mScrollWidth-1) ? TILE_WALL_B : TILE_WALL_BR;
				}
				x += w + 1;
			}
		}
	}

	void copyScrollField()
	{
		uint32_t y = mScrollPos;
		uint8_t* d = mpMap + mWidth * (mTop + TOPHOLEHEIGHT) + WALLWIDTH;
		const uint8_t* s = mpScrollMap + mScrollWidth * y;
		const uint8_t* e = mpScrollMap + mScrollWidth * mScrollHeight;

		for (y = 0; y < mScrollHeight; y++)
		{
			if (s >= e)
				s -= mScrollWidth * mScrollHeight;
			memcpy(d, s, mScrollWidth);
			d += mWidth;
			s += mScrollWidth;
		}		
	}

	void initRandon(uint32_t level)
	{
		uint8_t* const m = mpMap;
		uint32_t const w = mWidth;
		uint32_t const h = mHeight;
		uint32_t x, y, i;

		memset(m, TILE_BG, w*h);
		mHoles.clear();

		// left and right wall -- |   |
		for (y = mTop; y < h-1; y++) {
			m[y*w] = TILE_WALL_R;
			m[y*w + w-1] = TILE_WALL_L;
		}

		// bottom wall -- M___M
		memset(m + (h-1)*w, TILE_WALL_T, w);
		m[(h-1)*w] = m[(h-1)*w + w-1] = TILE_WALL_MID;

		// bottom holes -- M| || || |M
		uint32_t holes = (w-2) / HOLEDIST;
		y = h - HOLEAREAHEIGHT;
		for (i = 0; i<HOLEAREAHEIGHT-1; i++) {
			memset(m + (y+i)*w + 1, TILE_WALL_MID, w-2);
			for (x = 0; x < holes; x++) {
				static const uint8_t holeTiles[] = {
					TILE_WALL_TR,     TILE_BG,   TILE_WALL_TL,
					TILE_WALL_HOLE_L, TILE_BG,   TILE_WALL_HOLE_R,
					TILE_WALL_HOLE_L, TILE_HOLE, TILE_WALL_HOLE_R
				};
				m[(y+i)*w + x*HOLEDIST + 1] = holeTiles[i*3 + 0];
				m[(y+i)*w + x*HOLEDIST + 2] = holeTiles[i*3 + 1];
				m[(y+i)*w + x*HOLEDIST + 3] = holeTiles[i*3 + 2];
				if (i == HOLEAREAHEIGHT-2)
					mHoles.push_back(Hole(Pos(x*HOLEDIST + 2, y+i)));
			}
		}

		// top holes -- Ml rl rl rM
		y = mTop;
		memset(m + y*w, TILE_WALL_MID, w);
		for (x = 0; x < holes; x++) {
			m[y*w + x*3 + 1] = TILE_WALL_BL;
			m[y*w + x*3 + 2] = TILE_BG;
			m[y*w + x*3 + 3] = TILE_WALL_BR;
		}

		mScrollPos = 0;
		initScrollField(level);
		copyScrollField();
	}
};

struct Game : IGame
{
	uint32_t	 mKeymaskOld = 0;
	Field 		 mField;
	Car   		 mCar;
	vector<Ball> mBalls;
	uint32_t     mNextBall = 0;

	enum {
		BALLSPEED = 4, // No of frames for moving between tiles
	};

	IField& field() override { return mField; }
	Car& car() override { return mCar; }
	uint32_t ballCount() override { return mBalls.size(); }
	Ball& ball(uint32_t idx) override { return mBalls[idx]; }

	void handleKeys(uint32_t keymask)
	{
		if (mNextBall < mBalls.size()) {
			mBalls[mNextBall].mPos = mCar.pos();
			mBalls[mNextBall].mState = BALLSTATE_LATCHED;
		}

		if (keymask != mKeymaskOld) {

			if ((keymask & GCKEY_LEFT) && (mCar.x() > 1 + Field::HOLEDIST))
				mCar.mPos.x -= Field::HOLEDIST;

			if ((keymask & GCKEY_RIGHT) && (mCar.x() < mField.mWidth - 2 - Field::HOLEDIST))
				mCar.mPos.x += Field::HOLEDIST;

			if (keymask & GCKEY_FIRE) {
				if (mNextBall < mBalls.size()) {
					Ball& b = mBalls[mNextBall++];
					b.mPos = mCar.pos();
					b.mState = BALLSTATE_ACTIVE;
					b.mDir = DIR_DOWN;
				}
			}

			if ((keymask & (GCKEY_UP|GCKEY_DOWN)) && !testBallsActive()) {
				if (keymask & GCKEY_UP) mField.scrollUp();
				if (keymask & GCKEY_DOWN) mField.scrollDown();
				mField.copyScrollField();
			}

			mKeymaskOld = keymask;
		}
	}

	void handleBalls()
	{
		for (uint32_t i = 0; i < mBalls.size(); i++)
		{
			Ball& ball = mBalls[i];
			if (ball.mState == BALLSTATE_ACTIVE)
			{
				if (++ball.mSubPos >= BALLSPEED)
				{
					ball.mSubPos = 0;
					ball.mPos = ball.mPos.next(ball.mDir);
					switch (mField.tile(ball.mPos))
					{
					case TILE_HOLE:
						ball.mState = BALLSTATE_GOAL;
						int otherBall = mField.fillHole(ball.mPos, i);
						if (otherBall != -1)
							mBalls[otherBall].mState = ball.mState = BALLSTATE_BUST;
						else
							mPoints += 10;
						break;
					}
				}
			}
		}
	}

	bool testBallsActive()
	{
		for (const Ball& b : mBalls)
			if (b.mState == BALLSTATE_ACTIVE)
				return true;
		return false;
	}

	bool testGameOver()
	{
		for (const Ball& b : mBalls)
			if (b.mState == BALLSTATE_FRESH || b.mState == BALLSTATE_LATCHED || b.mState == BALLSTATE_ACTIVE)
				return false;
		return true;
	}

	void poll(uint32_t keymask) override
	{
		handleBalls();
		if (mField.testHolesFilled())
			initLevel(mLevel + 1);
		if (testGameOver())
			initLevel(0);
		handleKeys(keymask);
		mField.mpMap[0]++;
	}

	void initLevel(uint32_t level) override
	{
		mLevel = level;
		mNextBall = 0;
		mBalls.clear();
		mBalls.resize(20, Ball(Pos(0,0)));
		mField.initRandon(level);
		mCar.setPos(mField.startPos());
	}
};

IGame& IGame::instance()
{
	static Game game;
	return game;
}
