all : ./server ./client
./server : server_src.c
		gcc -o server server_src.c
./client : client_src.c
		gcc -o client client_src.c

