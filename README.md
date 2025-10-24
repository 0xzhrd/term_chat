# term_chat

NO ENCRYPTION ENABLED

would require port forwarding to work on a WAN,
    log into your router
    find the port forwarding section
    add rule: external port: 8080(any port > 1024)
    internal IP: [servers local IP]
    internal port: [same as the external port]
    protocol: TCP

insert guide on how to enable port forwarding 

to run as a server;
    ./chat --server [port]

to run as a client;
    ./chat --client [server IP] [port]

to send files;(limit currently 50MB)
    ::send filename / filepath

only handles 1 client connection
some issues in displaying messages more than terminal size, need to fix that