#include "pipe.h"
#include "sync.p/messages/all.pb.h"
#include "unit_tests/doctest_utils.h"

#include <algorithm>
#include <doctest.h>
#include <thread>
#include <vector>

using namespace std;

// sleep is needed since multiple threads are used
#define sleep() this_thread::sleep_for(chrono::milliseconds(25))


TEST_SUITE("pipe") {
    TEST_CASE("pipe") {
        Pipe<Message> pipe;

        REQUIRE(pipe.is_open());
        REQUIRE(pipe.is_empty());

        SUBCASE("is_empty holds true when the pipe has no message assigned") {
            REQUIRE(pipe.is_empty());
            REQUIRE_FALSE(pipe.is_not_empty());

            bool sent{pipe.send(Message{})};
            REQUIRE(sent);

            CHECK(pipe.is_not_empty());
            CHECK_FALSE(pipe.is_empty());

            bool received{pipe.receive()};
            REQUIRE(received);

            CHECK(pipe.is_empty());
            CHECK_FALSE(pipe.is_not_empty());
        }

        SUBCASE("messages can't be taken before they haven't been assigned first") {
            bool msg_assigned{false};

            thread t{[&](){
                sleep();
                msg_assigned = true;
                pipe.send(Message{});
            }};

            bool received{pipe.receive()};
            REQUIRE(received);

            CHECK(msg_assigned);

            t.join();
        }

        SUBCASE("messages don't change; assigned message == taken message") {
            vector<string> file_names{
                "A", "test_file", "test.log", "src/unit_tests/pipe.cpp", "z"
            };
            string file_name;

            DOCTEST_VALUE_PARAMETERIZED_DATA(file_name, file_names);

            auto file{new File};
            file->set_name(file_name);
            auto file_request{new FileRequest};
            file_request->set_allocated_file(file);
            Message msg{};
            msg.set_allocated_file_request(file_request);

            bool sent{pipe.send(move(msg))};
            REQUIRE(sent);

            auto received_msg{pipe.receive()};
            REQUIRE(received_msg);

            CHECK(received_msg.value().file_request().file().name() 
                    == 
                  file_name);
        }

        SUBCASE("multiple sent messages are received in the sent order") {
            vector<vector<string>> file_name_vectors{
                {"A", "test_file", "test.log", "src/unit_tests/pipe.cpp", "z"},
                {"b", "file_test", "log.test", "src/pipe/basic_pipe.cpp"},
                {"1", "2", "3", "4", "XYZ", "ABC"}
            };
            vector<string> file_names;

            DOCTEST_VALUE_PARAMETERIZED_DATA(file_names, file_name_vectors);

            for (auto file_name: file_names) {
                auto file{new File};
                file->set_name(file_name);
                auto file_request{new FileRequest};
                file_request->set_allocated_file(file);
                Message msg{};
                msg.set_allocated_file_request(file_request);

                bool sent{pipe.send(move(msg))};
                REQUIRE(sent);
            }
            
            for (auto file_name: file_names) {
                auto msg{pipe.receive()};
                REQUIRE(msg);

                CHECK(msg.value().file_request().file().name() 
                        == 
                      file_name);
            }
        }

        SUBCASE("messages sent as a vector are received in the initial order") {
            vector<vector<string>> file_name_vectors{
                {"A", "test_file", "test.log", "src/unit_tests/pipe.cpp", "z"},
                {"b", "file_test", "log.test", "src/pipe/basic_pipe.cpp"},
                {"1", "2", "3", "4", "XYZ", "ABC"}
            };
            vector<string> file_names;

            DOCTEST_VALUE_PARAMETERIZED_DATA(file_names, file_name_vectors);

            vector<Message> msgs{};
            msgs.reserve(file_names.size());

            for (auto file_name: file_names) {
                auto file{new File};
                file->set_name(file_name);
                auto file_request{new FileRequest};
                file_request->set_allocated_file(file);
                Message msg{};
                msg.set_allocated_file_request(file_request);

                msgs.push_back(move(msg));
            }

            bool sent{pipe.send(msgs)};
            REQUIRE(sent);
            
            for (auto file_name: file_names) {
                auto msg{pipe.receive()};
                REQUIRE(msg);

                CHECK(msg.value().file_request().file().name() 
                        == 
                      file_name);
            }
        }

        SUBCASE("pipe closes immediately and does not try to receive any values anymore") {
            REQUIRE(pipe.is_open());
            REQUIRE_FALSE(pipe.is_closed());

            bool closed{false};

            thread t{[&](){
                sleep();
                closed = true;
                pipe.close();
            }};

            bool received{pipe.receive()};
            REQUIRE_FALSE(received);

            CHECK(closed);
            CHECK(pipe.is_closed());
            CHECK_FALSE(pipe.is_open());

            bool sent{pipe.send(Message{})};
            CHECK_FALSE(sent);

            received = pipe.receive().has_value();
            CHECK_FALSE(received);

            t.join();
        }
    }

    TEST_CASE("pipe as receiving and sending end") {
        Pipe<Message> pipe;

        REQUIRE(pipe.is_open());
        REQUIRE(pipe.is_empty());

        ReceivingPipe<Message>* receiving{&pipe};
        SendingPipe<Message>* sending{&pipe};

        REQUIRE(receiving->is_open());
        REQUIRE(receiving->is_empty());

        REQUIRE(sending->is_open());
        REQUIRE(sending->is_empty());

        bool sent{sending->send(Message{})};
        REQUIRE(sent);

        CHECK(pipe.is_not_empty());
        CHECK(receiving->is_not_empty());
        CHECK(sending->is_not_empty());

        bool received{receiving->receive()};
        REQUIRE(received);

        CHECK(pipe.is_empty());
        CHECK(receiving->is_empty());
        CHECK(sending->is_empty());

        receiving->close();

        CHECK(pipe.is_closed());
        CHECK(receiving->is_closed());
        CHECK(sending->is_closed()); 
    }
}
