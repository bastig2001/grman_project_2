#pragma once

#include "config.h"
#include "internal_msg.h"
#include "pipe.h"


// runs the file operator and synchronization system
int run_file_operator(
    const Config&, 
    ReceivingPipe<InternalMsgWithOriginator>&
);
