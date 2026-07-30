#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

///
/// A queue with blocking mechanism.
///
template <typename T>
class BlockingQueue {
public:
  BlockingQueue() = delete;
  BlockingQueue(const BlockingQueue&) = delete;
  BlockingQueue& operator=(const BlockingQueue&) = delete;

  ///
  /// Constructor.
  ///
  /// @param   max_size  the maximum possible number of elements in the queue.
  ///
  explicit BlockingQueue(size_t max_size)
    : max_size_(max_size),
      queue_(),
      mutex_(),
      non_empty_cond_(),
      non_full_cond_() {
  }

  ///
  /// Destructor.
  ///
  ~BlockingQueue() = default;

  ///
  /// Enqueue an element.
  ///
  /// @param   item  an element to enqueue.
  ///
  /// If the queue is full, `enqueue()` operation is blocked.
  /// When an element in the queue is taken by another thread, `enqueue()` returns.
  ///
  void enqueue(const T& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    non_full_cond_.wait(lock, [this]() { return queue_.size() < max_size_; });
    queue_.push(item);
    non_empty_cond_.notify_one();
  }

  ///
  /// Enqueue an element.
  ///
  /// @param   item  an element to enqueue.
  ///
  /// If the queue is full, `enqueue()` operation is blocked.
  /// When an element in the queue is taken by another thread, `enqueue()` returns.
  ///
  void enqueue(T&& item) {
    std::unique_lock<std::mutex> lock(mutex_);
    non_full_cond_.wait(lock, [this]() { return queue_.size() < max_size_; });
    queue_.push(std::move(item));
    non_empty_cond_.notify_one();
  }

  ///
  /// Dequeue an element.
  ///
  /// @return  a dequeued element.
  ///
  /// If the queue is empty, `dequeue()` operation is blocked.
  /// When an element is added by another thread, `dequeue()` returns the element.
  ///
  T dequeue() {
    std::unique_lock<std::mutex> lock(mutex_);
    non_empty_cond_.wait(lock, [this]() { return !queue_.empty(); });
    T item = queue_.front();
    queue_.pop();
    non_full_cond_.notify_one();
    return item;
  }

  ///
  /// Return the number of elements in the queue.
  ///
  /// @return  the number of elements.
  ///
  size_t size() {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

  ///
  /// Return true if the queue is empty.
  ///
  /// @return  true if the queue is empty.
  ///
  bool empty() {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  ///
  /// Return true if the queue is full.
  ///
  /// @return  true if the queue is full.
  ///
  bool full() {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size() >= max_size_;
  }

  ///
  /// Return the maximum possible number of elements in the queue.
  ///
  /// @return  the maximum possible number of elements.
  ///
  size_t max_size() const noexcept {
    return max_size_;
  }

private:
  size_t max_size_;
  std::queue<T> queue_;
  std::mutex mutex_;
  std::condition_variable non_empty_cond_;
  std::condition_variable non_full_cond_;
}; // class BlockingQueue
