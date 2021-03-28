#pragma once

#include "pipe.h"
#include "messages/all.pb.h"

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>


class BasicPipe: public Pipe {
  private:
    std::queue<Message> msgs{};
    std::mutex pipe_mtx{};
    std::condition_variable sending_finishable{};
    bool open{true};

  public:
    void close() override;

    bool is_open() const override;
    bool is_closed() const override;

    bool is_empty() const override;
    bool is_not_empty() const override;

    bool operator<<(Message&&) override;
    bool operator>>(Message&) override;
};
