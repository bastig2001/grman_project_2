#pragma once

#include "message_utils.h"
#include "type/definitions.h"
#include "messages/basic.pb.h"

#include <optional>


namespace msg {
    struct File {
        FileName name;
        Timestamp timestamp;
        size_t size;
        StrongSign signature;

        static File from_proto(const ::File& file) {
            return File {
                file.name(),
                file.timestamp(),
                file.size(),
                file.signature()
            };
        }

        ::File* to_proto() {
            return ::file(name, timestamp, size, signature);
        }
    };

    struct Removed {
        FileName name;
        Timestamp timestamp;

        Removed() {}

        Removed(
            FileName name, 
            Timestamp timestamp
        ): name{name},
           timestamp{timestamp}
        {}

        Removed(File file): name{file.name}, timestamp{file.timestamp} {}
    };
}
