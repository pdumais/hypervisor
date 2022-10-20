#pragma once
#include <vector>

class Worker
{
public:
    virtual void work() = 0;
    virtual void handle_event(int fd) = 0;
    virtual std::vector<int> getFds() = 0;

};