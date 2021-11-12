#pragma once
#include "Shifter.hpp"

class SpriteLineParser
{
public:
  SpriteLineParser( Shifter & shifter, bool literal, int bpp, int totalBits ) :
    mShifter{ shifter }, mPen{}, mBPP{ bpp }, mTotalBits{ totalBits }, mRLECount{-1}, mRLELiteral{}, mLiteral{ literal }
  {
  }

  int const* getPenIndex()
  {
    if ( mLiteral )
      return literalPen();
    else
      return rlePen();
  }

  int totalBits() const
  {
    return mTotalBits;
  }

private:

  void readPen()
  {
    mPen = mShifter.pull( mBPP );
    mTotalBits -= mBPP;
  }

  int const* literalPen()
  {
    if ( mTotalBits > mBPP )
    {
      readPen();
      return &mPen;
    }
    else
      return nullptr;
  }

  int const* rlePen()
  {
    if ( mRLECount < 0 )
    {
      if ( mTotalBits > 5 )
      {
        mRLELiteral = mShifter.pull<1>();
        mRLECount = mShifter.pull<4>();
        mTotalBits -= 5;
        if ( !mRLELiteral )
        {
          if ( mRLECount > 0 )
          {
            if ( mTotalBits > mBPP )
            {
              readPen();
            }
            else
            {
              return nullptr;
            }
          }
          else
          {
            return nullptr;
          }
        }
      }
      else
      {
        return nullptr;
      }
    }

    if ( mRLELiteral )
    {
      if ( mTotalBits > mBPP )
      {
        readPen();
      }
      else
      {
        return nullptr;
      }
    }

    mRLECount -= 1;

    return &mPen;
  }

private:
  Shifter & mShifter;
  int mPen;
  int mBPP;
  int mTotalBits;
  int mRLECount;
  int mRLELiteral;
  bool mLiteral;
};
