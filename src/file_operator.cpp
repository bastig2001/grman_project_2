#include "file_operator.h"
#include "exit_code.h"
#include "internal_msg.h"
#include "unit_tests.p/messages/all.pb.h"

using namespace std;


int run_file_operator(
    const Config&,
    ReceivingPipe<InternalMsg>* inbox
) {
    InternalMsg msg;
    while (*inbox >> msg) {

    }

    inbox->close();

    return Success;
}

