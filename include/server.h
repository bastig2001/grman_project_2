# pragma once

#include "config.h"
#include "internal_msg.h"
#include "pipe.h"


int run_server(const Config&, SendingPipe<InternalMsg>*);
