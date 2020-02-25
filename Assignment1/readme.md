# Chat Program
Run one server on a network. Now multiple clients on the same network can join the server using the IP.

Then start messaging.
## 1. Running
### 1a. Prerequisites
Server uses _pthread_ library.
Client uses _pthread_ and _ncurses_ libraries.
### 1b. Compilation
This is a pretty basic program. So most of the C compilers should work. While compiling make sure to link the pthread library for server, and pthread and ncurses library for client. Server, sample compile:

`gcc server.c -o server -lpthread`

For client:

`gcc client.c -o client -lpthread -lncurses`

### 1c. Running
First run server. Sample run:

`./server <IP Address> <Port No>`

For client:

`./client <IP Address> <Port No>`

## 2. Usage

- First type in the name when prompted.
- Now on, any message that you type will be treated as one that you want to send to your chosen receiver.
- Except messages starting with '-'. (Say, they are more like messages to server.)
- Now here is how to use them.
  - `-q` to exit the server.
  - `-l` to list all the users and groups.
  - `-s <receiver name>` to select who you want to talk to.
  - `-sr` to clear selection.
  - `-o` to see who you are talking to.
  - `-c` to change your name.
  - `-g <group name>` to select the group you want to talk to.
  - `-gn <new group name>` to create a group, with the name given.
  - `-ga <person name>` to add that person to the selected group.
  - `-gl` to list the members for the selected group.
  - `-gc <new group name>` to change the name of the selected group.
  - `-gr` to leave the selected group.
  - `-n` to shift the terminal 2 lines up.
  - `-n<x>` to shift the terminal x lines up (-n1 to shift one line up. -n2 to shift 2 lines up, etc).
  - `-r` to clear the whole page on terminal.
  
Some side notes
- The program, on some level, treats groups and users alike. So group names have a ':' appended before them to distinguish them from normal users. (Use -l after creating a group to better understand this.)
- So group can be members of other groups themselves. Though, as of now, nothing recursive is programmed to happen when messaged.
- You can list all the groups present on the server. But can only select (-g) the ones that you are present in.
- New Group creation will already add you as the first member in the group. It will also clear your previous selection of talking. Group or person. (-sc will also clear the current selection of talking. Group or person.)
