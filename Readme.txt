//////////////////////////////////////////
// war game Jacek Jusianiec I2.2        //
//////////////////////////////////////////
kompilacja ./Makefile

lub
	
gcc client.c -lncurses -o client


gcc server.c -lm -o server

--------------------------------------

1. server uruchamiamy zawsze pierwszy!
2. nastepnie uruchamiamy 2x client
3. gre mozemy zakonczyc wysylajac wiadomosc resign lub wygrywajac
nagle zatrzymanie aplikacji moze spowodowac nie usuniecie kolejki

