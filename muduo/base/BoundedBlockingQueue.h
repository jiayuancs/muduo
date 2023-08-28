// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_BOUNDEDBLOCKINGQUEUE_H
#define MUDUO_BASE_BOUNDEDBLOCKINGQUEUE_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <deque>
#include <assert.h>

namespace muduo
{
// 生产者消费者模型：有界缓冲区
template<typename T>
class BoundedBlockingQueue : noncopyable
{
 public:
  explicit BoundedBlockingQueue(unsigned maxSize)
    : maxSize_(maxSize),
      mutex_(),
      notEmpty_(mutex_),
      notFull_(mutex_)
  {
  }

  void put(const T& x)
  {
    MutexLockGuard lock(mutex_);
    while (queue_.size() == maxSize_)
    {
      notFull_.wait();
    }
    assert(queue_.size() < maxSize_);
    queue_.push_back(x);
    notEmpty_.notify();
  }

  T take()
  {
    MutexLockGuard lock(mutex_);
    while (queue_.empty())
    {
      notEmpty_.wait();
    }
    assert(!queue_.empty());
    T front(queue_.front());
    queue_.pop_front();
    notFull_.notify();
    return front;
  }

  bool empty() const
  {
    MutexLockGuard lock(mutex_);
    return queue_.empty();
  }

  bool full() const
  {
    MutexLockGuard lock(mutex_);
    return queue_.size() == maxSize_;
  }

  size_t size() const
  {
    MutexLockGuard lock(mutex_);
    return queue_.size();
  }

  size_t capacity() const
  {
    MutexLockGuard lock(mutex_);
    return queue_.capacity();
  }

 private:
  const unsigned             maxSize_;
  mutable MutexLock          mutex_;
  Condition                  notEmpty_ GUARDED_BY(mutex_);
  Condition                  notFull_ GUARDED_BY(mutex_);
  // 使用 boost::circular_buffer 性能会更好
  // TODO: 自己编写一个环形环形缓冲区替换这里的 deque
  // https://www.zhihu.com/question/66760616/answer/247646426
  std::deque<T>              queue_ GUARDED_BY(mutex_);
};

}  // namespace muduo

#endif  // MUDUO_BASE_BOUNDEDBLOCKINGQUEUE_H
