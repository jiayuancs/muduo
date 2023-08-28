// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Types.h>

#include <deque>
#include <vector>

namespace muduo
{
// 线程池
// 该线程池的线程数量是固定的，不能动态变化
// TODO: 实现可以动态调整线程数量的线程池
class ThreadPool : noncopyable
{
 public:
  typedef std::function<void ()> Task;

  explicit ThreadPool(const string& nameArg = string("ThreadPool"));
  ~ThreadPool();

  // Must be called before start(). 设置任务队列的最大容量
  void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
  void setThreadInitCallback(const Task& cb)
  { threadInitCallback_ = cb; }

  void start(int numThreads);
  // 使所有空闲的线程退出，并等待未执行完任务的线程结束。
  void stop();

  const string& name() const
  { return name_; }

  size_t queueSize() const;

  // Could block if maxQueueSize > 0
  // 如果设置了 maxQueueSize > 0，则当任务队列中的任务数量达到 maxQueueSize 时会阻塞(生产者消费者模型：有界缓冲区)；
  // 如果设置 maxQueueSize = 0（默认），则相当于不限制任务队列长度，自然不会阻塞(生产者消费者模型：无界缓冲区)
  void run(Task f);

 private:
  bool isFull() const REQUIRES(mutex_);
  void runInThread();
  Task take();

  mutable MutexLock mutex_;
  Condition notEmpty_ GUARDED_BY(mutex_);
  Condition notFull_ GUARDED_BY(mutex_);
  string name_;   // 线程池名称
  Task threadInitCallback_;
  std::vector<std::unique_ptr<muduo::Thread>> threads_;
  std::deque<Task> queue_ GUARDED_BY(mutex_);  // 任务队列（生产者消费者模型）
  size_t maxQueueSize_;  // 任务队列的最大容量
  bool running_;  // 线程池是否处于运行状态
};

}  // namespace muduo

#endif  // MUDUO_BASE_THREADPOOL_H
