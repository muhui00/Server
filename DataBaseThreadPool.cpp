#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>
#include <future>
#include <mariadb/conncpp.hpp>

using SqlConnPtr=std::unique_ptr<sql::Connection>;
class DataBaseThreadPool
{
    std::queue<std::function<void(SqlConnPtr&)>> taskQueue;
    std::condition_variable empty, full;
    std::vector<std::thread> threads;
    std::atomic_bool stopSignal;
    int maxNumberOfThreads;
    int maxQueueSize;
    std::mutex queueMutex;

public:
    DataBaseThreadPool(int maxNumberOfThreads, int maxQueueSize, sql::SQLString loginUserName,sql::SQLString password,sql::SQLString connUrl) :
        maxNumberOfThreads(maxNumberOfThreads),                                                                                                                                              
        maxQueueSize(maxQueueSize)
    {
        stopSignal = false;
        sql::Driver *driver = sql::mariadb::get_driver_instance();
        for (int i = 0; i < maxNumberOfThreads; i += 1)
        {
            threads.emplace_back(
                [this, driver,loginUserName,password,connUrl]()  mutable //should be mutable
                {
                    sql::Properties properties({{(sql::SQLString)"user", loginUserName},
                                    {(sql::SQLString)"password", password}});
                    SqlConnPtr dbConnectorPtr(driver->connect(connUrl, properties));
                    std::function<void(SqlConnPtr&)> curTask;
                    while (true)
                    {
                        {
                            std::unique_lock<std::mutex> ul(this->queueMutex);
                            empty.wait(ul, [this]()
                                       { return this->taskQueue.empty() == false or this->stopSignal; });
                            if (this->stopSignal)
                            {
                                break;
                            }
                            curTask = std::move(this->taskQueue.front());
                            this->taskQueue.pop();
                            this->full.notify_one();
                        }
                        curTask(dbConnectorPtr);
                    }
                });
        }
    }
    template <typename F, typename... Args>
    auto submit(F &&f, Args &&...args)
    {
        
        using ret_type = std::result_of_t<
            F(SqlConnPtr&,Args&&...)/* should be in correspondence with the protype of 'F'(the callable object)*/
        >;
        
        auto task = std::make_shared<std::packaged_task<ret_type(SqlConnPtr&/* how to pass unique_ptr through function*/)>>(
            std::bind( std::forward<F>(f),
                std::placeholders::_1 /*placeholder for database connetion pointer*/,
                std::forward<Args>(args).../* '...' are required*/));
        auto ret = task->get_future();
        std::unique_lock<std::mutex> ul(this->queueMutex);
        full.wait(ul, [&]()
                  { return this->taskQueue.size() < this->maxQueueSize or stopSignal; });
        if (stopSignal)
        {
            throw std::runtime_error("program termination");
        }
        // 'task' should be caputured through value, otherwise exceptions would be thrown;
        this->taskQueue.emplace([task](SqlConnPtr& conn)
                                { (*task)(conn); });
        empty.notify_one();
        return ret;
    }
    ~DataBaseThreadPool()
    {
        stopSignal = true;
        // notify all of the blocked threads to stop;
        full.notify_all();
        empty.notify_all();
        // wait for all the threads to finish its job, otherwise the program would not terminate normally.
        for (int i = 0; i < maxNumberOfThreads; i += 1)
        {
            threads[i].join();
        }
    }
};
