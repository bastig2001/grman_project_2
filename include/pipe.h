#pragma once

#include "messages/all.pb.h"


// The interface for a closable Pipe
class ClosablePipe {
  public:
    virtual void close() = 0;

    virtual bool is_open() const = 0;
    virtual bool is_closed() const = 0;

    virtual bool is_empty() const = 0;
    virtual bool is_not_empty() const = 0;

    virtual ~ClosablePipe() = default;
};


// The interface for the sending end of a Pipe
class SendingPipe: public ClosablePipe {
  public:
    // returns true, when the Message was sent,
    //        false, when the Pipe is closed
    virtual bool operator<<(Message&&) = 0;

    virtual ~SendingPipe() = default;
};


// The interface for the receiving end of a Pipe
class ReceivingPipe: public ClosablePipe {
  public:
    // returns true, when the Message was received,
    //        false, when the Pipe is closed
    virtual bool operator>>(Message&) = 0;

    virtual ~ReceivingPipe() = default;
};


// The interface for a whole Pipe
class Pipe: public SendingPipe, public ReceivingPipe {
  public:
    virtual ~Pipe() = default;
};
