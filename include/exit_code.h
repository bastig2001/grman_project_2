#pragma once

enum ExitCode {
    Success = 0,

    // Server
    ServerException = 1,

    // Client
    ClientException = 33,
    ConnectionEstablishmentError,
    ConnectionError,

    // Other
    LogFileError = 65,

    // beginning with 100: CLI ExitCodes
};
