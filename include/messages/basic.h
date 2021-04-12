#pragma once

#include "message_utils.h"
#include "type/definitions.h"
#include "messages/basic.pb.h"

#include <optional>


// Messages to represent the same or similar types as with protobuf 
// internally and for the database
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

        Removed(const File& file): name{file.name}, timestamp{file.timestamp} {}
    };

    struct Data {
        FileName file_name;
        Offset offset;
        BlockSize size;
        std::string data;

        Data() {}

        Data(
            FileName file_name,
            Offset offset,
            BlockSize size,
            std::string data
        ): file_name{file_name},
           offset{offset},
           size{size},
           data{std::move(data)}
        {}

        Data(const Correction& correction)
        : file_name{correction.block().file_name()},
          offset{correction.block().offset()},
          size{correction.block().size()},
          data{correction.data()}
        {}
    };
}
