# tcp-chat-pthreads
You can use nc to test the chat server
To Start the server:
```
make
./server
```
On another terminal type
```
nc localhost 8080
```
to connect to the server.
After that

To register and login type 
```/register [username] [password]```
To login type
```/login [username] [password]```
To send a broadcast message
```/sendmsg "[message]"```
To send a private message to a user
```/sendmsgto [username] "[message]"```
