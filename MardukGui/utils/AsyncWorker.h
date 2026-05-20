#pragma once

#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <functional>
#include <memory>

/**
 * @brief 异步任务工具类
 *
 * 封装 QtConcurrent::run，在后台线程执行耗时操作，
 * 完成后自动回到 GUI 线程调用回调。
 *
 * 用法：
 *   AsyncWorker::run<LoginResult>(
 *       []() { return someBlockingCall(); },
 *       [this](const LoginResult& result) { updateUi(result); }
 *   );
 */
class AsyncWorker
{
public:
    /**
     * @brief 在后台线程执行 task，完成后在 GUI 线程调用 callback
     *
     * @tparam T        任务返回值类型
     * @param task      要在后台执行的函数
     * @param callback  完成后的回调（在 GUI 线程）
     */
    template<typename T>
    static void run(std::function<T()> task,
                    std::function<void(const T&)> callback)
    {
        // QFutureWatcher 需要在堆上分配，回调中 deleteLater
        auto* watcher = new QFutureWatcher<T>();

        QObject::connect(watcher, &QFutureWatcher<T>::finished, [watcher, callback]() {
            T result = watcher->result();
            callback(result);
            watcher->deleteLater();
        });

        QFuture<T> future = QtConcurrent::run(task);
        watcher->setFuture(future);
    }

    /**
     * @brief 无返回值版本
     */
    static void run(std::function<void()> task,
                    std::function<void()> callback = nullptr)
    {
        auto* watcher = new QFutureWatcher<void>();

        QObject::connect(watcher, &QFutureWatcher<void>::finished, [watcher, callback]() {
            if (callback) callback();
            watcher->deleteLater();
        });

        QFuture<void> future = QtConcurrent::run(task);
        watcher->setFuture(future);
    }

private:
    AsyncWorker() = default;
};
