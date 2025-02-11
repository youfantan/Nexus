# Nexus - High Performance and Light Weight Http(s) Server

## What is its current condition?
Nexus just start its develop process. We plan to make it Cross-Platform and have better performance in concurrency.  
Now Nexus running on Windows platform and not support IOCP yet (it use winselect for instead).  

## How it works
It using OpenSSL to implement TLS feature and using select() to implement IO multiplexing.When the main thread accept the connection, it will first register events and then create a Http(s)Connection.Each of connections is considered a state machine, it executes events in its state machine function.When main thread received events(for connection which status is HANDSHAKE, READ or WRITE) or finished io events(for connection which status is EXECUTING or FINISHED), it will post a task to the thread pool to drive connections.

## Application & Deployment
For demo, visit [my personal website](https://glous.work) to have a view.  
We use clang-19.1.4 on msys2 as toolchain to compile the project. OpenSSL is installed through vcpkg.

## About Author
The author is a high school student from China, facing the college entrance examination, so he doesn't have much free time. If you have any questions, please send me an email.