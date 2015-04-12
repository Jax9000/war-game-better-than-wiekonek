# war-game-fork-semaphore-example-ncurses
semaphore example

use ./Makefile to complie
to download ncurses lib use "sudo apt-get install libncurses5-dev" in terminal

kompilacja ./Makefile

lub
	
gcc client.c -lncurses -o client
gcc server.c -lm -o server


--------------------------------------

1. server uruchamiamy zawsze pierwszy!
2. nastepnie uruchamiamy 2x client
3. gre mozemy zakonczyc wysylajac wiadomosc resign lub wygrywajac
nagle zatrzymanie aplikacji moze spowodowac nie usuniecie kolejki
co spowoduje problemy przy ponownym uruchomieniu programu.

