#pragma once

// Exit Codes for the program which can be used as a Bitmask Type
enum ExitCode {
    Success = 0,

    // Server
    ServerException = 1,

    // Client
    ClientException = 2,
    ConnectionEstablishmentError = 4,
    ConnectionError = 6,

    // File Operator
    FileOperatorException = 8,
    DatabaseNotWorking = 16,

    // Other/All
    LogFileError = 32,
    FileOperatorNotStarted = 64,

    // beginning with 100: CLI ExitCodes
};
