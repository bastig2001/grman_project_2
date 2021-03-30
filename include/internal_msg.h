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
    SendingPipe<InternalMsgType>* originator;
    Message msg;

    InternalMsg() {}

    InternalMsg(
        InternalMsgType type, 
        SendingPipe<InternalMsgType>* originator, 
        Message msg = Message{}
    ): type{type},
       originator{originator},
       msg{msg}
    {}

    static InternalMsg to_file_operator(
        SendingPipe<InternalMsgType>* originator, 
        Message msg
    ) {
        return InternalMsg(InternalMsgType::HandleMessage, originator, msg);
    }

    static InternalMsg to_originator(
        SendingPipe<InternalMsgType>* originator, 
        Message msg
    ) {
        return InternalMsg(InternalMsgType::SendMessage, originator, msg);
    }
};
