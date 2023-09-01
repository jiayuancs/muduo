// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_SINGLETON_H
#define MUDUO_BASE_SINGLETON_H

#include <muduo/base/noncopyable.h>

#include <assert.h>
#include <pthread.h>
#include <stdlib.h> // atexit

namespace muduo
{

namespace detail
{
// This doesn't detect inherited member functions!
// http://stackoverflow.com/questions/1966362/sfinae-to-check-for-inherited-member-functions
// 这里使用 SFINAE 的特性检查一个类型是否具有 `no_destroy` 成员（该成员由我们自行定义，相当于为类打上一个tag）
template<typename T>
struct has_no_destroy
{
  template <typename C> static char test(decltype(&C::no_destroy));  // 参数是一个地址
  template <typename C> static int32_t test(...);  // 参数任意（省略符形参的优先级最低）
  const static bool value = sizeof(test<T>(0)) == 1;
};
}  // namespace detail

template<typename T>
class Singleton : noncopyable
{
 public:
  Singleton() = delete;
  ~Singleton() = delete;

  static T& instance()
  {
    // 确保 init() 只被执行一次
    pthread_once(&ponce_, &Singleton::init);
    assert(value_ != NULL);
    return *value_;
  }

 private:
  static void init()
  {
    value_ = new T();
    if (!detail::has_no_destroy<T>::value)
    {
      // 在整个程序结束的时候（执行exit函数，或main函数return），执行 destroy 函数
      ::atexit(destroy);
    }
  }

  static void destroy()
  {
    // 默认情况下，释放不完全类型的指针并不会报错。
    // 当类型 T 是不完全类型（声明之后，定义之前）时，下面的语句中的数组元素个数取 -1，
    // 从而导致编译错误，便于我们及时发现错误
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy; (void) dummy;

    delete value_;
    value_ = NULL;
  }

 private:
  static pthread_once_t ponce_;
  static T*             value_;
};

template<typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

template<typename T>
T* Singleton<T>::value_ = NULL;

}  // namespace muduo

#endif  // MUDUO_BASE_SINGLETON_H
