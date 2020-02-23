# Chat Program
## 1. Running
### i. Compilation
For server, use:
`gcc server.c -o server -lpthread`
    
    (Server uses lpthread library)
For client, use:
`gcc client.c -o client -lpthread -lncurses`
    
    (client.c uses ncurses for the terminal interface. Please install it for your platform first before compilation and running)

### ii. Running
First run server.
`./server <IP Address> <Port No>`

For client:
`./client <IP Address> <Port No>`

## 2. Usage
