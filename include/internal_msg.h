#pragma once

#include "pipe.h"
#include "messages/all.pb.h"


// Types of Messages for internal communication
enum class InternalMsgType {
    ServerWaits,
    ClientWaits,
    Sync,
    FileOperatorStarted,
    HandleMessage,
    SendMessage
};

// Message structure for internal communication between actors
struct InternalMsg {
    InternalMsgType type;
    Message msg;

    InternalMsg(
        InternalMsgType type, 
        Message msg = Message{}
    ): type{type},
       msg{msg}
    {}
};

// Internal Message structure with return address
struct InternalMsgWithOriginator {
    InternalMsgType type;
    SendingPipe<InternalMsg>& originator;
    Message msg;

    InternalMsgWithOriginator(
        InternalMsgType type, 
        SendingPipe<InternalMsg>& originator, 
        Message msg = Message{}
    ): type{type},
       originator{originator},
       msg{msg}
    {}
};


inline InternalMsg get_msg_to_originator(
    Message msg
) {
    return InternalMsg(
        InternalMsgType::SendMessage, 
        msg
    );
}

inline InternalMsgWithOriginator get_msg_to_file_operator(
    SendingPipe<InternalMsg>& originator, 
    Message msg
) {
    return InternalMsgWithOriginator(
        InternalMsgType::HandleMessage, 
        originator, 
        msg
    );
}
