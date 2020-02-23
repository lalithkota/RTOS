# Chat Program
##  1. Running
###         i. Compilation
For server, use:

`gcc server.c -o server -lpthread`
Server uses lpthread library

For client, use:

`gcc client.c -o client -lpthread -lncurses`

client.c uses ncurses for the terminal interface. Please install it for your platform first before compilation and running

### ii. Running
First run server.

`./server <IP Address> <Port No>`

For client:

`./client <IP Address> <Port No>`

## 2. Usage

First type in the name when prompted.
Now on, any message that you type will be treated as ones, that you want to send to your chosen receiver.
Except messages starting with '-'. (Say, they are more like messages to server.)
Now here is how to use them.

- `-q` to exit the server.
- `-l` to list all the users and groups.
- `-s <receiver name>` to select who you want to talk to.
- `-sc` to clear selection.
- `-c` to change your name.
- `-g <group name>` to select the group you want to talk to.
- `-gn <new group name>` to create a group, with the name given.
- `-gr` to leave the group.

