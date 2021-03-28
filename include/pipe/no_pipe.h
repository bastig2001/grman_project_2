#pragma once

#include "pipe.h"


// An implementation of Pipe which does nothing;
class NoPipe: public Pipe {
  public:
    void close() override {};

    bool is_open() const override { return false; };
    bool is_closed() const override { return true; };

    bool is_empty() const override { return true; };
    bool is_not_empty() const override { return false; };

    bool operator<<(Message&&) override { return false; };
    bool operator>>(Message&) override { return false; };
};
