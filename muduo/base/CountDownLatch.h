// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

namespace muduo
{

// 计时器门闩
// 既可用于所有子线程等待主线程发起“起跑”，也可用于主线程
// 等待子线程初始化完毕后才开始工作
class CountDownLatch : noncopyable
{
 public:

  explicit CountDownLatch(int count);

  // 等待计数为0，如大于0则阻塞
  void wait();

  // 计数器减1，如果减到0，则通知所有阻塞在该条件变量上的线程
  void countDown();

  int getCount() const;

 private:
  mutable MutexLock mutex_;
  Condition condition_ GUARDED_BY(mutex_);
  int count_ GUARDED_BY(mutex_);
};

}  // namespace muduo
#endif  // MUDUO_BASE_COUNTDOWNLATCH_H
