#pragma once
#include "Worker.h"
#include <thread>
#include <vector>
#include <map>

class Reactor
{
private:
    int epollfd;
    int numfd;
    std::thread* th;
    volatile bool stopFlag;
    std::vector<Worker*> workers;
    std::map<int,Worker*> workerFdMap;

public:
    Reactor();
    ~Reactor();

    void addWorker(Worker* w);
    void start();
    void stop();
};

