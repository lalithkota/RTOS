# MessMe

MessMe is a messaging app. Now built for android. (It is an extension of [Assignment 1](https://github.com/lalithkota/RTOS/edit/master/Assignment1) of RTOS, where a client-server chat program was built using C and sockets)

## 1. How to Use
- To run the server:

Compile it with pthread library
`cc server.c -o client -lpthread`

- For the android client:

Install and use the version of the app given.

- For the CLI client:

Compile with ncurses, pthread, pulse and pulse-simple libraries (It needs the libraries installed first. See, [Assignment 1](https://github.com/lalithkota/RTOS/edit/master/Assignment1) usage first.)
`cc client.c -o client -lpthread -lncurses -lpulse -lpulse-simple`

Happy messaging.

## 2. Some Notes

For all the previous notes. See [Assignment 1](https://github.com/lalithkota/RTOS/edit/master/Assignment1).

- Call feature is implemented. Pulseaudio is used to implement it in C. Even in android, the audio encoding, bit depth, no of channels, are same for both the pulseaudio-one and the android-one. So seamless calling can be made from the CLI client to the android client.
Pickup/Hangup feature is also implemented. But the CLI version (and only the CLI one) is not quitting "gracefully". Meaning, soon as client-1 quits, client-2 crashes. Here are the details:
  - Call are maitained on a different socket than the usual messaging one. So on each client, atleast two threads are running for the call. One which records and sends to server, and one which read from server and plays out.
  - If a client wants to quit the call, it basically closes its end of the socket. Server now realises that and closes its end of the sockets too.
  - But server has to close two sockets, one for each user (both the sockets; considering only two people in the call; also that group call is not implemented yet). Now when the second user reads from the closed server socket, it gets EOF; and then realises the other user quit the call. So now this ends/closes its sockets and quits too.
  - Now why is it crashing? I think during a read/write process in C, if it gets EOF, it returns properly with a zero. But now if you further do another read/write it causes the program to crash. That is what is happening in my program, because of the two-thread nature.
  - On the server side (also being multithreaded for each user's call handling) atleast some measures are taken so that such a crash like the CLI one doesnt occur. But a rare chance is there, for that a more robust method is needed for handling the call-finish process.
- The application, which once employed a centralised user-base system, (meaning the user-related info of other users was stored only in the server)
now became decentralised. Meaning every other client gets all the info about your joinings, name-changes (yes there is that too), the user-quits, etc.
So that it can manage its own info and gui. (CLI doesn't do anything with that info for now) (Idea being; each client maintains its own database of users)
- Previously, a user's identity was solely based on his name, which felt foolish,
and a new aspect of Username is added which is unique for each user.
So that the name can be whatever it wants, and that there can also be same named clients.
All the previous operations that are done using the name, are changed so that they are done using the username.
- Previously when the sender sends a message, the server used to append the name of the sender at the beginning and then send the message over to the receiver.
But now it is changed to use the username, assuming that every client has a database of the user-info and can take care of it.
The android client does that already, but the CLI doesn't (yet). It simply displays the message raw on screen.
  - When a group is created, its username is set to the name itself, mostly. (When such name already exists a char is appended and tried again, retaining the idea that username is unique)
- Once the username concept is involved, a password concept is also implemented, to create a login interface. So that a user can log back into his own username the next time.
- On android, unread messages feature is also implemented. A simple scroll lock is also implemented so that once the user scrolls up (the scroll lock is lifted) all new messages received, go unread. While the user gets such a message the scroll is NOT disturbed back to bottom.
But if the user scrolls down or if the scroll is at the end (the scroll is locked), and all new messages go read directly, and a new message bottom-fits itself.
