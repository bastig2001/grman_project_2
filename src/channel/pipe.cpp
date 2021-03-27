#include "channel/pipe.h"

#include <mutex>

using namespace std;


void Pipe::operator<<(Message&& msg) {
    std::lock_guard msgs_lck{msgs_mtx};
    msgs.push(move(msg));
    not_empty.notify_all();
}

void Pipe::operator>>(Message& msg) {
    std::unique_lock msgs_lck{msgs_mtx};
    not_empty.wait(msgs_lck, [this](){ return !is_empty(); });

    msg = msgs.front();
    msgs.pop();
}

bool Pipe::is_empty() const {
    return msgs.empty();
}
