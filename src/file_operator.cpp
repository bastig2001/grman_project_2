#include "file_operator.h"
#include "exit_code.h"
#include "unit_tests.p/messages/all.pb.h"

using namespace std;


int run_file_operator(
    const Config&,
    ReceivingPipe* inbox,
    SendingPipe* server,
    SendingPipe* client
) {
    Message msg;
    while (*inbox >> msg) {

    }

    inbox->close();
    server->close();
    client->close();

    return Success;
}

