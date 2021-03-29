#pragma once

#include "config.h"
#include "pipe.h"


int run_file_operator(
    const Config& config,
    ReceivingPipe* inbox,
    SendingPipe* server,
    SendingPipe* client
);
