#pragma once 

#include "config.h"
#include "pipe.h"


int run_client(const Config&, ReceivingPipe*, SendingPipe*);
