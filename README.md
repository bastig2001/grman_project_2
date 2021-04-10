# Sync

## A Synchronization Client and Server

**Sync** is a combined synchronization client and server for Linux
using a modification of the [rsync](https://en.wikipedia.org/wiki/Rsync) algorithm.


## Building

This project gets compiled with [Meson](https://mesonbuild.com/) and [ninja](https://ninja-build.org/).

The **dependencies** for this C++17 project are:
* [Asio](https://think-async.com/Asio/)
* [CLI11](https://github.com/CLIUtils/CLI11)
* [cpp-peglib](https://github.com/yhirose/cpp-peglib)
* [fmt](https://github.com/fmtlib/fmt)
* [JSON](https://github.com/nlohmann/json)
* [Protocol Buffers](https://developers.google.com/protocol-buffers/)
* [spdlog](https://github.com/gabime/spdlog)
* [SQLite ORM](https://github.com/fnc12/sqlite_orm)

For the **unit tests** you also need [doctest](https://github.com/onqtam/doctest).

Please make sure to get all dependencies and provide needed paths with `meson_options.txt`.

To build *Sync*, execute `meson build` and `ninja -C build sync` from the root folder of this repository.


## Usage

With the compiled binary `sync`, you can start a client, a server or both for synchronization.
Once started, you also get access to a command line to further interact with the system.

To synchronize a directory, start `sync` in this directory. 
`sync` will always take your current working directory as synchronization directory when you start it.
**Please make sure that `sync` has the rights to read and write to all files and subdirectories** in the synchronization directory.

The *server* has default values for everything and can be simply started with the command line argument `-s` or `--serve`.

To start the *client* you need to at least specify the address of the server to which to connect
with the command line argument `-a` or `--server-address`.

***Note: Sync does not yet change any files, it only sends synchronization messages between client and server!***

### Configuration

*Sync* can be configured via many different ways.
It can be configured with the CLI, environment variables or a JSON config file.

The concrete order in which the configurations are applied is the following:
1. There are **default values** for most parameters, those are overwritten by
2. set **environment variables**, which again are overwritten by
3. the **JSON config file**, if one is provided, and the final say has
4. the **command line interface (CLI)**

#### CLI and Environment Variables

The following table shows all command line parameters (CLI), their equivalent environment variables (EV),
their types and the default values together with a description:

| CLI                                    | EV                     | Type        | Default                   | Description |
| -------------------------------------- | ---------------------- |:-----------:|:-------------------------:| ----------- |
| `-h, --help`                           |                        | flag        |                           | Prints a help message and exits |
| `-c, --config`                         | `SYNC_CONFIG`          | path        |                           | JSON config file from which to load the configuration |
| `-a, --server-address`                 | `SYNC_SERVER_ADDRESS`  | address     |                           | The host to which to connect for syncing |
| `-p, --server-port`                    | `SYNC_SERVER_PORT`     | port number | `9876`                    | The port of the server to which to connect for syncing |
| `-s, --serve`                          | `SYNC_SERVE`           | flag        |                           | Enables the server |
| `    --bind-address`                   | `SYNC_BIND_ADDRESS`    | IP-address  | `0.0.0.0`                 | The IP-address to which to bind as server. `0.0.0.0` means *listen to all*. Also enables the server |
| `    --bind-port`                      | `SYNC_BIND_PORT`       | port number | `9876`                    | The port to which to bind as server. Also enables the server |
| `    --hidden`                         | `SYNC_HIDDEN`          | flag        |                           | Sync also hidden files |
| `-l, --log-to-console`                 | `SYNC_LOG_CONSOLE`     | flag        |                           | Enables logging to console |
| `-f, --log-file`                       | `SYNC_LOG_FILE`        | path        |                           | Enables logging to specified file |
| `    --log-level, --log-level-console` | `SYNC_LOG_LEVEL`       | log level   | `2` ... INFO              | Sets the visible logging level. Which number corresponds to which logging level is listed further down |
| `    --log-level-file`                 | `SYNC_LOG_LEVEL_FILE`  | log level   | the same as `--log-level` | Sets the visible logging level for the log file. By default it takes the logging level specified with `--log-level` or any equivalent |
| `    --log-size`                       | `SYNC_LOG_SIZE`        | size in KiB | 5 MiB                     | Maximum size of a log file in KiB |
| `    --log-file-number`                | `SYNC_LOG_FILE_NUMBER` | integer     | `2`                       | Number of log files between which the logger rotates |
| `    --log-date`                       | `SYNC_LOG_DATE`        | flag        |                           | Logs the date additionally to the time |
| `    --log-config`                     | `SYNC_LOG_CONFIG`      | flag        |                           | Log the used config as a DEBUG message |
| `    --no-color`                       | `SYNC_NO_COLOR`        | flag        |                           | Disables color output to console |

Logging levels and their corresponding integer values:

| Level  | Value |
|:------:|:-----:|
| Trace  | 0     |
| Debug  | 1     |
| Info   | 2     |
| Warn   | 3     |
| Error  | 4     |
|Critical| 5     |

##### Example

An invocation of `sync` might for example look as follows:

`sync -a myhost -l --log-level 2 --log-config -f test.log --log-level-file 1`

Given these arguments, *Sync* starts as a client connecting to `myhost:9876` 
which does not synchronize hidden files. Logging to console is enabled with level INFO 
and it also logs to the file `test.log` with level DEBUG. The log file will have a size of 5 MiB at most, 
then the logger will rotate to a second file. The used configuration is logged as a DEBUG message 
(so only visible in the log file).

#### JSON Config File

A JSON config file can be passed with the command line argument `-c` or `--config`,
or with the environment variable `SYNC_CONFIG` to *Sync*.

The following table shows all fields for the JSON config file, if they are required, their type
and the equivalent command line parameter (CLI) together with a description:

| Field                     | Type    | CLI                                | Description |
| ------------------------- |:-------:| ---------------------------------- | ----------- |
| `server`*                 | object  |                                    | The server to which the client shall connect |
| `server.address`          | string  | `-a, --server-address`             | The host to which to connect for syncing |
| `server.port`             | integer | `-p, --server-port`                | The port of the server to which to connect for syncing |
| `act_as_server`*          | object  |                                    | The address and port to which the server shall listen |
| `act_as_server.address`   | string  | `--bind-address`                   | The IP-address to which to bind as server. `0.0.0.0` means *listen to all* |
| `act_as_server.port`      | integer | `--bind-port`                      | The port to which to bind as server. |
| `sync.sync_hidden_files`* | boolean | `--hidden`                         | If to sync hidden files |
| `logger.log_to_console`*  | boolean | `-l, --log-to-console`             | If to log to the console |
| `logger.file`*            | string  | `-f, --log-file`                   | Logging to specified file |
| `logger.level_console`*   | integer | `--log-level, --log-level-console` | The visible logging level. Which number corresponds to which logging level is listed further up in the section *CLI and Environment Variables* |
| `logger.level_file`*      | integer | `--log-level-file`                 | The visible logging level for the log file |
| `logger.max_file_size`*   | integer | `--log-size`                       | Maximum size of a log file in KiB |
| `logger.number_of_files`* | integer | `--log-file-number`                | Number of log files between which the logger rotates |
| `logger.log_date`*        | boolean | `--log-date`                       | If to log the date additionally to the time |
| `logger.log_config`*      | boolean | `--log-config`                     | If to log the used config as a DEBUG message |
| `no_color`*               | boolean | `--no-color`                       | If to disable color output to console |

`field`* ... required field

A JSON config file has to contain all required fields, there are no defaults for any.

##### Example

A JSON config file might for example look as follows:

```json
{
    "server": {
        "address": "myhost",
        "port": 9876
    },
    "act_as_server": {},
    "sync": {
        "sync_hidden_files": false
    },
    "logger": {
        "log_to_console": true,
        "file": "test.log",
        "level_console": 2,
        "level_file": 1,
        "max_file_size": 5120,
        "number_of_files": 2,
        "log_date": false,
        "log_config": true
    },
    "no_color": false
}
```

Given this config file, *Sync* starts as a client connecting to `myhost:9876` 
which does not synchronize hidden files. Logging to console is enabled with level INFO 
and it also logs to the file `test.log` with level DEBUG. The log file will have a size of 5 MiB at most, 
then the logger will rotate to a second file. The used configuration is logged as a DEBUG message 
(so only visible in the log file).

### Command Line

Once *Sync* is started, you get access to a command line with which you can further interact with the system.
This command line supports all necessities you would expect from a basic command line.
You can walk through a history of your last 100 calls with the up and down arrow keys 
and execute any command again with the enter key once selected.
(Your history isn't saved between program invocations, so you always start with an empty history).
To exit the command line you can call the command `q`, `quit` or `exit`, or use the shortcut CTRL+D
(it might still be necessary for you to abort some parts of *Sync* with CTRL+C).

The following table shows all commands which are available in the *Sync* command line with a description:

| Command         | Description |
| --------------- | ----------- |
| `h, help`       | Outputs a help message |
| `ls, list`      | Lists all files which are to be synced |
| `ll, list long` | Lists all files which are to be synced with more information (their signature, size and time point of last change) |
| `q, quit, exit` | Quits and exits the program (at least the command line) |

## Algorithm

The algorithm used for synchronizing the data between a client and a server
is based on [rsync algorithm](https://en.wikipedia.org/wiki/Rsync#Algorithm)
as described in [PhD theses by Andrew Tridgell](https://www.samba.org/~tridge/phd_thesis.pdf).

The messages used for communication between client and server can be viewed in `messages/`
and the algorithm's implementation is in `src/file_operator/`.


## Unit Tests

This project has a few unit tests and test cases for a few implementations.
These use [doctest](https://github.com/onqtam/doctest) and can be executed with the command `unit_tests`.
