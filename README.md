# war-game-fork-semaphore-example-ncurses
semaphore example

use ./Makefile to complie
to download ncurses lib use "sudo apt-get install libncurses5-dev"

use make file to compile project or:
```
gcc client.c -lncurses -o client
gcc server.c -lm -o server
```

--------------------------------------
This is one of my first projects, so sorry for code style.

1. Server run first
2. Run 2 times client app
3. Enjoy game


--------------------------------------
Game commands:

1. build l10 - builds 10 light warrior
2. build h10 - build 10 heavy warrior
3. build c10 - build 10 caravan
4. attack l10 h10 c10 - sends warriors to attack
5. build w10 - build 10 workers - they produce more money
6. resign
