#pragma once

#include "config.h"
#include "internal_msg.h"
#include "pipe.h"


int run_file_operator(
    const SyncConfig&, 
    ReceivingPipe<InternalMsgWithOriginator>&
);
