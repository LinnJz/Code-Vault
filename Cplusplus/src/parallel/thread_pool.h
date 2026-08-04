#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <print>
#include <queue>
#include <thread>
#include <vector>

/*
私有构造函数，使用友元 friend class std::optional<ThreadPool>;无法生效
为什么友元走不通（clang-cl + MSVC STL）：
1. optional 的 in_place 构造函数带 enable_if_t<is_constructible_v<...>>，这个默认模板实参的访问检查发生在模板定义上下文（namespace std），友元 std::optional<ThreadPool> 只覆盖特化自身的成员函数，管不到 → SFINAE 移除候选。
2. 想友元内部类 _Optional_destruct_base 也不行：友元声明在类的 } 前，ThreadPool 尚不完整，而它的默认模板实参 is_trivially_destructible_v<ThreadPool> 需要完整类型 → 直接报错。
*/
class ThreadPool
{
  // passkey 惯用法
  struct ctor_key
  {
  }; // private tag: outsiders cannot name it, so construction stays unreachable

public:
  static std::optional<ThreadPool> create(size_t threads)
  {
    if (threads == 0) return std::nullopt;

    return std::make_optional<ThreadPool>(ctor_key {}, threads);
  }

  ThreadPool() noexcept                      = delete;
  ThreadPool(ThreadPool const &)             = delete;
  ThreadPool &operator= (ThreadPool const &) = delete;

  ~ThreadPool() { shutdown(); }

  template<typename F, typename... Args>
  auto submit(F &&f, Args &&...args)
      -> std::optional<std::future<typename std::invoke_result_t<F, Args...>>>
  {
    using return_type = typename std::invoke_result_t<F, Args...>;
    try {
      auto task = std::make_unique<std::packaged_task<return_type()>>(
          [f = std::forward<F>(f), ... args = std::forward<Args>(args)]() mutable
      { return std::invoke(std::move(f), std::move(args)...); });

      auto result = task->get_future();
      {
        std::unique_lock<std::mutex> ulock { queue_mutex_ };
        if (stop_.load(std::memory_order_relaxed) || tasks_.size() > max_queue_size)
          return std::nullopt;
        tasks_.emplace([task = std::move(task)]() { task->operator() (); });
        cv_.notify_one();
      }
      return std::move(result);
    } catch (...) {
      return std::nullopt;
    }
  }

  void shutdown()
  {
    stop_.store(true, std::memory_order_release);
    cv_.notify_all();
  }

  explicit ThreadPool(ctor_key, size_t threads)
      : ThreadPool { threads }
  {
  }

private:
  explicit ThreadPool(size_t threads)
  {
    workers_.reserve(threads);

    for (size_t i = 0; i < threads; ++i) {
      workers_.emplace_back(&ThreadPool::worker_loop, this);
    }
  }

  void worker_loop()
  {
    for (;;) {
      std::move_only_function<void()> task;
      {
        std::unique_lock<std::mutex> ulock { queue_mutex_ };
        cv_.wait(ulock, [this] {
          return stop_.load(std::memory_order_relaxed) || !tasks_.empty();
        });

        if (stop_.load(std::memory_order_relaxed) && tasks_.empty()) return;

        task = std::move(tasks_.front());
        tasks_.pop();
      }
      task(); // 执行任务，异常会由 packaged_task 捕获并存储到 future
    }
  }

  std::vector<std::jthread> workers_;
  std::queue<std::move_only_function<void()>> tasks_;
  std::mutex queue_mutex_;
  std::condition_variable cv_;
  std::atomic_bool stop_ { false };
  static constexpr size_t max_queue_size { 1024 };
};
