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
        unsigned int owner; // 0 = this program; 1 = server; >=2 = a client

        static File from_proto(const ::File& file, unsigned int owner) {
            return File {
                file.name(),
                file.timestamp(),
                file.size(),
                file.signature(),
                owner
            };
        }

        ::File* to_proto() {
            return ::file(name, timestamp, size, signature);
        }
    };

    struct Block {
        unsigned long id;
        FileName file_name;
        Offset offset;
        BlockSize size;
        unsigned int owner; // 0 = this program; 1 = server; >=2 = a client
        std::optional<WeakSign> weak_signature;
        std::optional<StrongSign> strong_signature;
        std::optional<std::string> data;

        static Block from_proto(
            const ::Block& block, 
            unsigned int owner,
            std::optional<WeakSign> weak_signature = std::nullopt,
            std::optional<StrongSign> strong_signature = std::nullopt,
            std::optional<std::string> data = std::nullopt
        ) {
            return Block {
                0,
                block.file_name(),
                block.offset(),
                block.size(),
                owner,
                weak_signature,
                strong_signature,
                data
            };
        }

        ::Block* to_proto() {
            return ::block(file_name, offset, size);
        }
    };
}
