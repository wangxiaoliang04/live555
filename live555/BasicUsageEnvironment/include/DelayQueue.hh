/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
 // Copyright (c) 1996-2014, Live Networks, Inc.  All rights reserved
// Delay queue
// C++ header

#ifndef _DELAY_QUEUE_HH
#define _DELAY_QUEUE_HH

#ifndef _NET_COMMON_H
#include "NetCommon.h"
#endif

#ifdef TIME_BASE
typedef TIME_BASE time_base_seconds;
#else
typedef long time_base_seconds;
#endif

///// A "Timeval" can be either an absolute time, or a time interval /////

class Timeval {
public:
  time_base_seconds seconds() const {
    return fTv.tv_sec;
  }
  time_base_seconds seconds() {
    return fTv.tv_sec;
  }
  time_base_seconds useconds() const {
    return fTv.tv_usec;
  }
  time_base_seconds useconds() {
    return fTv.tv_usec;
  }

  int operator>=(Timeval const& arg2) const;
  int operator<=(Timeval const& arg2) const {
    return arg2 >= *this;
  }
  int operator<(Timeval const& arg2) const {
    return !(*this >= arg2);
  }
  int operator>(Timeval const& arg2) const {
    return arg2 < *this;
  }
  int operator==(Timeval const& arg2) const {
    return *this >= arg2 && arg2 >= *this;
  }
  int operator!=(Timeval const& arg2) const {
    return !(*this == arg2);
  }

  void operator+=(class DelayInterval const& arg2);
  void operator-=(class DelayInterval const& arg2);
  // returns ZERO iff arg2 >= arg1

protected:
  Timeval(time_base_seconds seconds, time_base_seconds useconds) {
    fTv.tv_sec = seconds; fTv.tv_usec = useconds;
  }

private:
  time_base_seconds& secs() {
    return (time_base_seconds&)fTv.tv_sec;
  }
  time_base_seconds& usecs() {
    return (time_base_seconds&)fTv.tv_usec;
  }

  struct timeval fTv;
};

#ifndef max
inline Timeval max(Timeval const& arg1, Timeval const& arg2) {
  return arg1 >= arg2 ? arg1 : arg2;
}
#endif
#ifndef min
inline Timeval min(Timeval const& arg1, Timeval const& arg2) {
  return arg1 <= arg2 ? arg1 : arg2;
}
#endif

class DelayInterval operator-(Timeval const& arg1, Timeval const& arg2);
// returns ZERO iff arg2 >= arg1


///// DelayInterval /////

class DelayInterval: public Timeval {
public:
  DelayInterval(time_base_seconds seconds, time_base_seconds useconds)
    : Timeval(seconds, useconds) {}
};

DelayInterval operator*(short arg1, DelayInterval const& arg2);

extern DelayInterval const DELAY_ZERO;
extern DelayInterval const DELAY_SECOND;
extern DelayInterval const DELAY_MINUTE;
extern DelayInterval const DELAY_HOUR;
extern DelayInterval const DELAY_DAY;

///// EventTime /////

class EventTime: public Timeval {
public:
  EventTime(unsigned secondsSinceEpoch = 0,
	    unsigned usecondsSinceEpoch = 0)
    // We use the Unix standard epoch: January 1, 1970
    : Timeval(secondsSinceEpoch, usecondsSinceEpoch) {}
};

EventTime TimeNow();

extern EventTime const THE_END_OF_TIME;


///// DelayQueueEntry /////
//整个延时队列是用DelayQueue这个类实现的，而它的基类DelayQueueEntry就是用来描述每个事件节点的。
//在DelayQueueEntry中的主要成员有以下几个：fDelayTimeRemaining表示的就是与前一事件之间的时间差；
//fNext和fPrev就是指向时间轴上的下一个事件和前一个事件的指针；ftoken表示当前节点的标识；handleTimeout就是事件超时后的处理方法。
class DelayQueueEntry {
public:
  virtual ~DelayQueueEntry();

  intptr_t token() {
    return fToken;
  }

protected: // abstract base class
  DelayQueueEntry(DelayInterval delay);

  virtual void handleTimeout();

private:
  friend class DelayQueue;
  DelayQueueEntry* fNext;
  DelayQueueEntry* fPrev;
  DelayInterval fDeltaTimeRemaining;

  intptr_t fToken;
  static intptr_t tokenCounter;
};

///// DelayQueue /////
//而DelayQueue类里描述的则是具体的实现方法。首先是一些对这个队列进行的基本操作：
//addEntry实现的是在队列中增加事件节点；removeEntry实现的是在队列中删除某事件节点；
//updateEntry实现的则是更新某事件的触发时间；而findEntryByToken则是根据节点的标识查找相应的事件。
//在此类中最常用的方法应该是synchronize，它实现的就是将整个事件队列和当前系统时间进行同步，
//检测有无事件已经被触发，如果触发并调用handleAlarm方法对相应事件进行处理。
//而属性fLastSyncTime表示的就是上次同步的系统时间，其实一般情况下，
//方法synchronize的实现方法其实就是简单地把队列上第一个事件节点存储的时间差减去当前系统时间和上次同步时间的差。
class DelayQueue: public DelayQueueEntry {
public:
  DelayQueue();
  virtual ~DelayQueue();

  void addEntry(DelayQueueEntry* newEntry); // returns a token for the entry
  void updateEntry(DelayQueueEntry* entry, DelayInterval newDelay);
  void updateEntry(intptr_t tokenToFind, DelayInterval newDelay);
  void removeEntry(DelayQueueEntry* entry); // but doesn't delete it
  DelayQueueEntry* removeEntry(intptr_t tokenToFind); // but doesn't delete it

  DelayInterval const& timeToNextAlarm();
  void handleAlarm();

private:
  DelayQueueEntry* head() { return fNext; }
  DelayQueueEntry* findEntryByToken(intptr_t token);
  void synchronize(); // bring the 'time remaining' fields up-to-date

  EventTime fLastSyncTime;
};

#endif
