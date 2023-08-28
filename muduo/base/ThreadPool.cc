// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/ThreadPool.h>

#include <muduo/base/Exception.h>

#include <assert.h>
#include <stdio.h>

using namespace muduo;

ThreadPool::ThreadPool(const string& nameArg)
  : mutex_(),
    notEmpty_(mutex_),
    notFull_(mutex_),
    name_(nameArg),
    maxQueueSize_(0),
    running_(false)
{
}

ThreadPool::~ThreadPool()
{
  if (running_)
  {
    stop();
  }
}

void ThreadPool::start(int numThreads)
{
  assert(threads_.empty());
  running_ = true;
  threads_.reserve(numThreads);
  for (int i = 0; i < numThreads; ++i)
  {
    char id[32];
    snprintf(id, sizeof id, "%d", i+1);
    threads_.emplace_back(new muduo::Thread(
          std::bind(&ThreadPool::runInThread, this), name_+id));
    threads_[i]->start();
  }
  // 如果numThreads为0，则会再调用run函数时直接执行任务，不会进入runInThread函数，
  // 因此需要再这里执行 threadInitCallback_
  if (numThreads == 0 && threadInitCallback_)
  {
    threadInitCallback_();
  }
}

// 任务未执行完毕的线程仍会继续执行，
// 已执行完毕的线程立即退出
void ThreadPool::stop()
{
  {
  MutexLockGuard lock(mutex_);
  running_ = false;
  // 欺骗线程池中阻塞的线程，让其运行并死亡
  notEmpty_.notifyAll();
  }
  for (auto& thr : threads_)
  {
    thr->join();
  }
}

size_t ThreadPool::queueSize() const
{
  MutexLockGuard lock(mutex_);
  return queue_.size();
}

void ThreadPool::run(Task task)
{
  if (threads_.empty())
  {
    task();
  }
  else  // 生产者
  {
    MutexLockGuard lock(mutex_);
    while (isFull())
    {
      notFull_.wait();
    }
    assert(!isFull());

    queue_.push_back(std::move(task));
    notEmpty_.notify();
  }
}

// 消费者
ThreadPool::Task ThreadPool::take()
{
  MutexLockGuard lock(mutex_);
  // always use a while-loop, due to spurious wakeup
  // 这里等待的条件有两个：
  // 1. 任务队列非空
  // 2. 停止运行（执行了stop()函数）
  while (queue_.empty() && running_)
  {
    notEmpty_.wait();
  }
  Task task;
  if (!queue_.empty())
  {
    task = queue_.front();
    queue_.pop_front();
    // 当 maxQueueSize == 0 时（即任务队列的长度无上限），即具有无界缓冲区的生产者消费者模型，
    // 此时只需要一个条件变量即可，具体见 muduo/base/BlockingQueue.h 的实现思路
    if (maxQueueSize_ > 0)
    {
      notFull_.notify();
    }
  }
  return task;
}

bool ThreadPool::isFull() const
{
  mutex_.assertLocked();
  return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
  try
  {
    if (threadInitCallback_)
    {
      threadInitCallback_();
    }
    while (running_)
    {
      Task task(take());
      if (task)
      {
        task();
      }
    }
  }
  catch (const Exception& ex)
  {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
    abort();
  }
  catch (const std::exception& ex)
  {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    abort();
  }
  catch (...)
  {
    fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
    throw; // rethrow
  }
}

