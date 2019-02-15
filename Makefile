all:ringmaster player

ringmaster:ringmaster.c
	gcc -o ringmaster ringmaster.c

player:player.c
	gcc -o player player.c
