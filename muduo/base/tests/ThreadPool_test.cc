#include <muduo/base/ThreadPool.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Logging.h>

#include <stdio.h>
#include <unistd.h>  // usleep

void print()
{
  printf("tid=%d\n", muduo::CurrentThread::tid());
}

void printString(const std::string& str)
{
  LOG_INFO << str;
  usleep(100*1000);
}

void test(int maxSize)
{
  LOG_WARN << "Test ThreadPool with max queue size = " << maxSize;
  muduo::ThreadPool pool("MainThreadPool");
  pool.setMaxQueueSize(maxSize);
  pool.start(5);

  LOG_WARN << "Adding";
  pool.run(print);
  pool.run(print);
  for (int i = 0; i < 100; ++i)
  {
    char buf[32];
    snprintf(buf, sizeof buf, "task %d", i);
    pool.run(std::bind(printString, std::string(buf)));
  }
  LOG_WARN << "Done";

  // Note: 这里妙啊，当 latch.wait() 返回时，说明任务队列已经为空，
  // 所有任务都已被线程池中的线程取走，因此可以执行 pool.stop()。
  // stop 函数会使所有空闲的线程退出，并等待未执行完任务的线程结束。
  muduo::CountDownLatch latch(1);
  pool.run(std::bind(&muduo::CountDownLatch::countDown, &latch));
  latch.wait();
  pool.stop();
}

int main()
{
  test(0);  // 不限制任务队列的长度（run 函数不会发生阻塞）
  test(1);  // 任务队列的最大长度为1
  test(5);
  test(10);
  test(50);
}
