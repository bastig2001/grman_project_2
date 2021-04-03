#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>


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
template<typename T>
class SendingPipe: public ClosablePipe {
  public:
    // returns true, when the Message was sent,
    //        false, when the Pipe is closed
    virtual bool send(const std::vector<T>&) = 0;
    virtual bool send(T) = 0;

    virtual ~SendingPipe() = default;
};


// The interface for the receiving end of a Pipe
template<typename T>
class ReceivingPipe: public ClosablePipe {
  public:
    // returns the Message, when the Message was received,
    //             nullopt, when the Pipe is closed
    virtual std::optional<T> receive() = 0;

    virtual ~ReceivingPipe() = default;
};


// The interface for a whole Pipe
template<typename T>
class Pipe: public SendingPipe<T>, public ReceivingPipe<T> {
  private:
    std::queue<T> msgs{};
    std::mutex pipe_mtx{};
    std::condition_variable receiving_finishable{};
    bool open{true};

  public:
    void close() override {
        std::lock_guard pipe_lck{pipe_mtx};
        open = false;
        receiving_finishable.notify_all();
    }

    bool is_open() const override { return open; }
    bool is_closed() const override { return !open; }

    bool is_empty() const override { return msgs.empty(); }
    bool is_not_empty() const override { return !is_empty(); }

    bool send(const std::vector<T>& new_msgs) override {
        std::lock_guard pipe_lck{pipe_mtx};
        if (is_open()) {
            for (auto msg: new_msgs) {
                msgs.push(std::move(msg));
            }
            receiving_finishable.notify_all();

            return true;
        }
        else {
            return false;
        }
    }

    bool send(T msg) override {
        std::lock_guard pipe_lck{pipe_mtx};
        if (is_open()) {
            msgs.push(std::move(msg));
            receiving_finishable.notify_one();

            return true;
        }
        else {
            return false;
        }  
    }

    std::optional<T> receive() override {
        std::unique_lock pipe_lck{pipe_mtx};
        if (is_open()) {
            receiving_finishable.wait(
                pipe_lck, 
                [this](){ return is_not_empty() || is_closed(); }
            );

            if (is_open()) {
                T msg{std::move(msgs.front())};
                msgs.pop();

                return msg;
            }
            else {
                return std::nullopt;
            }
        }
        else {
            return std::nullopt;
        }
    }

    ~Pipe() {
        close();
    }
};
