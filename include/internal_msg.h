#pragma once

#include "pipe.h"
#include "messages/all.pb.h"


enum class InternalMsgType {
    ServerWaits,
    ClientWaits,
    FileOperatorStarted,
    HandleMessage,
    SendMessage
};


struct InternalMsg {
    InternalMsgType type;
    SendingPipe<InternalMsg>* originator;
    Message msg;

    InternalMsg() {}

    InternalMsg(const InternalMsg& other): 
        InternalMsg(other.type, other.originator, other.msg)
    {}

    InternalMsg(InternalMsg&& other): 
        InternalMsg(other.type, other.originator, std::move(other.msg))
    {}

    InternalMsg(
        InternalMsgType type, 
        SendingPipe<InternalMsg>* originator, 
        Message msg = Message{}
    ): type{type},
       originator{originator},
       msg{msg}
    {}

    static InternalMsg to_file_operator(
        SendingPipe<InternalMsg>* originator, 
        Message msg
    ) {
        return InternalMsg(InternalMsgType::HandleMessage, originator, msg);
    }

    static InternalMsg to_originator(
        SendingPipe<InternalMsg>* originator, 
        Message msg
    ) {
        return InternalMsg(InternalMsgType::SendMessage, originator, msg);
    }

    void operator=(const InternalMsg& other) {
        type = other.type;
        originator = other.originator;
        msg = other.msg;
    }

    void operator=(InternalMsg&& other) {
        type = other.type;
        originator = other.originator;
        msg = std::move(other.msg);
    }
};
