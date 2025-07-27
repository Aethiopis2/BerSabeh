# BerSabeh

BerSabeh is an SMS client/server application implementing the SMPP v3.4 protocol. It provides a web-based control interface and supports sending, receiving, and managing SMS messages, including integration with a database for message storage and billing.

## Features

- SMPP v3.4 protocol support for SMS communication
- TCP/IP client for connecting to SMSCs (Short Message Service Centers)
- Web-based dashboard for monitoring and control
- Database integration for message storage, billing, and auditing
- Configurable via a simple text-based configuration file
- Multi-threaded and asynchronous message handling
- Error handling and logging

## Project Structure

```
.
├── include/           # Header files
│   ├── basics.h
│   ├── errors.h
│   ├── utils.h
│   ├── db/
│   │   ├── iQE.h
│   │   └── messages.h
│   └── net/
│       ├── smpp-konstants.h
│       ├── sms.h
│       ├── tcp-base.h
│       └── tcp-client.h
├── src/               # Source files
│   ├── bersabeh.cpp
│   ├── errors.cpp
│   ├── utils.cpp
│   ├── db/
│   │   ├── iQE.cpp
│   │   └── messages.cpp
│   └── net/
│       ├── sms.cpp
│       ├── tcp-base.cpp
│       └── tcp-client.cpp
├── static/
│   └── dashboard.html # Web dashboard
├── test/
│   └── playground.cpp # Test driver
├── Makefile           # Build instructions
├── README.md          # Project documentation
└── .gitignore
```

## Getting Started

### Prerequisites

- Linux or Windows environment
- C++ compiler (e.g., g++)
- ODBC library (`libodbc`)
- pthreads library

### Building

To build the project, run:

```sh
make
```

The main executable will be created in the `bin/` directory.

### Configuration

Edit the `config.dat` file to set up database connections and SMSC providers. The configuration file uses key-value pairs separated by spaces.

### Running

Start the application:

```sh
./bin/bersabeh
```

The application will connect to the configured database and SMSC providers, and start the web dashboard.

### Web Dashboard

Open `static/dashboard.html` in your browser to access the web-based control interface.

## Testing

A test driver is available in [test/playground.cpp](test/playground.cpp).

## Authors

- Dr. Rediet Worku aka Aethiops ben Zahab

## License

Copyright (c) 2024

See source files for