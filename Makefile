lab: server.c bot.c
	gcc -o server server.c
	gcc -o bot bot.c

clean:
	rm server bot