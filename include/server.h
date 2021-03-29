# pragma once

#include "config.h"
#include "pipe.h"


int run_server(const Config&, ReceivingPipe*, SendingPipe*);
