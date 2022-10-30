#include "Reactor.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <cstdio>
#include "log.h"


#define EPOLL_WAIT_MS 20

Reactor::Reactor() 
{
    this->epollfd = epoll_create1(0);
    this->numfd = 0;
}

Reactor::~Reactor() 
{
    //close(this->epollfd);
}

void Reactor::addWorker(Worker* w)
{
    this->workers.push_back(w);
    for (int fd : w->getFds())
    {
        this->workerFdMap[fd] = w;
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        epoll_ctl(this->epollfd, EPOLL_CTL_ADD, fd, &ev);
        this->numfd++;
    }
}

void Reactor::start()
{
    this->th = new std::thread([=](){
        this->stopFlag = false;
        while (!stopFlag){
            struct epoll_event events[this->numfd];
            int nfds = epoll_wait(epollfd, events, this->numfd, EPOLL_WAIT_MS);
            if (stopFlag) break;
            for (int i = 0; i < nfds; i++)
            {
                int fd = events[i].data.fd;

                if (!this->workerFdMap.count(fd))
                {
                    log("Invalid fd for event\n");
                    continue;
                }

                this->workerFdMap[fd]->handle_event(fd);
            }

            for (Worker* w : this->workers)
            {
                w->work();
            }
        }
        log("worker thread exiting\n");
    });
}

void Reactor::stop()
{
    this->stopFlag = true;
    this->th->join();
}
