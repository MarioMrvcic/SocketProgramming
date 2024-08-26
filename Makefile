lab: server.c bot.c CandC.c
	gcc -o server server.c
	gcc -o bot bot.c
	gcc -o CandC CandC.c

clean:
	rm server bot CandC