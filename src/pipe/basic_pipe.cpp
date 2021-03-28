#include "pipe/basic_pipe.h"

#include <mutex>

using namespace std;


void BasicPipe::close() {
    std::lock_guard pipe_lck{pipe_mtx};
    open = false;
    sending_finishable.notify_all();
}

bool BasicPipe::is_open() const {
    return open;
}

bool BasicPipe::is_closed() const {
    return !open;
}

bool BasicPipe::is_empty() const {
    return msgs.empty();
}

bool BasicPipe::is_not_empty() const {
    return !is_empty();
}

bool BasicPipe::operator<<(Message&& msg) {
    if (is_open()) {
        std::lock_guard pipe_lck{pipe_mtx};
        msgs.push(move(msg));
        sending_finishable.notify_one();

        return true;
    }
    else {
        return false;
    }  
}

bool BasicPipe::operator>>(Message& msg) {
    if (is_open()) {
        std::unique_lock pipe_lck{pipe_mtx};
        sending_finishable.wait(
            pipe_lck, 
            [this](){ return is_not_empty() || is_closed(); }
        );

        if (is_open()) {
            msg = msgs.front();
            msgs.pop();

            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}
