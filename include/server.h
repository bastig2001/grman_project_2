# pragma once

#include "config.h"
#include "internal_msg.h"
#include "pipe.h"


// runs the synchronization server
int run_server(const ServerData&, SendingPipe<InternalMsgWithOriginator>&);
