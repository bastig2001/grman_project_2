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

    // Other/All
    LogFileError = 16,
    FileOperatorNotStarted = 32,

    // beginning with 100: CLI ExitCodes
};
