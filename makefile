make: server.c db.c
	gcc -o server server.c db.c -I. -lpthread