## Mbroker-TFS

Both of the projects from the 22/23 Operating Systems class in IST. (First project being the TFS file system and the second one being the mbroker server).

The Mbroker server runs on top of the TFS (Técnico File System) and connects publishers and subscribers to "boxes" which are files stored in TFS.
The server has 10 worker threads running on a producer-consumer queue that processes client requests sent to the server pipe (following a set wire protocol).

The publisher clients can send requests to the server through the server pipe in order to attach themselves to a box, after that, any messages sent by the user will be transmitted to the server which will then store them in the file system.

Subscriber clients can send requests to the server to subscribe to a box. After subscribing the client will receive all prior messages sent to the box as well as every new message that is published to that box (in real time).
