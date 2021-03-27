#pragma once

#include "messages/all.pb.h"

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>


class Pipe {
  private:
    std::queue<Message> msgs{};
    std::mutex msgs_mtx{};
    std::condition_variable not_empty{};

  public:
    void operator<<(Message&&);
    void operator>>(Message&);

    bool is_empty() const;
};
