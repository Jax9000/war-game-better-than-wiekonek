/* Na start serwer wysyła dwie wiadomości (mtype = 5) z różnymi
* wartościami w polu data[X * * * * *] (X = 1,2)
* Każdy z klientów odbiera jedną z wyżej opisanych wiadomości,
* która przypisuje mu jego ID. Następnie każdy z klientów
* wysła komunikaty o mtype=ID (gracz pierszy mtype = 1, gracz drugi mtype = 2),
* natomiast serwer wysyła wiadomości do różnych klientów
* o mtype=ID+2 (do gracza pierwszego mtype=3, dla gracza drugiego mtype=4).
*
* Przykładowa wiadomość gracza:
* mtype = 1; (mtype = 1,2)
* data = {2 0 X X X X}; (gra jest kontynuowana (2 na miejscu 0),
* a X oznaczają ilość jednostek do wyprodukowania)
*
* Przykładowa wiadomość z serwera:
* mtype = 3; (mtype = 3,4)
* data = {2 2344 4 5 6 7 } (2 na pierwszym miejscu oznacza, że w grze dalej jest 2 graczy,
* nastepne wartosci opisuja ilosc surowców i jednostek)
*
* Nietypowe wiadomości od gracza:
* pierwsza wiadomość [0 0 0 0 0 0]
* rezygnacja [-1 0 0 0 0 0] (już w czasie gry)
* walka [3 0 0 0 0 0]
*
* Nietypowe wiadomości z serwera:
* wiadomości startowe
* przerwanie gry [-1 0 0 0 0 0]
* wygrales walke [3 X X X X X]
* przegrales walke [4 X X X X X]
* wygrales [5 X X X X X]
* przegrales [6 X X X X X]
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <error.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <math.h>

#define KEY_MSG 666

#define PRICE 0
#define ATTACK 1
#define DEFENCE 2
#define SPEED 3

#define RESOURCES 1
#define WORKER 2
#define LIGHT 3
#define HEAVY 4
#define CAVALRY 5

// **************************** STATYCZNE ZMIENNE ********************************* //
int semid[5]; //1 semafor zmiany bufora gracza 1; 2 sem zmiany buf gracz 2; 3 semafor budowy gracza 1;
struct sembuf p,v;
int item_attribute[6][4]; // 2 - worker 3 - light 4 - heavy 5 - cavalry
// 0 price 1 attack 2 defence 3 production time
// item_attribute[4][2] - worker defence
int k; // do msgsnd(k,...);
/////////////////////////////////////////////////////////////////////////////////////

typedef struct msgplayerbuf {
long mtype; // 1 - player 1, 2 - player 2;
int data[6];
// standar
// 0 - number of players; (3 - war)
// 1 - resources;
// 2 - workers;
// 3 - light_soldiers;
// 4 - heavy_soldiers;
// 5 - cavalry;
} msgplayerbuf;

typedef struct Shared_variable{

    int working;
    int player_item[3][6]; // 1 resources 2 worker 3 light soldier 4 heavy soldier 5 cavalry
    int player_req_item[3][6]; //2 worker 3 light soldier 4 heavy sold 5 cavalry
    int win_count[3];
    //player_req_item[1][2] = 6// player 1 requesting worker count 6

} Shared_variable;
//wszystkie tablice maja mniej wiecej taka sama strukture
//generalnie by nie bylo klopotow z indeksowaniem tablice sa o jeden rzad wieksze

Shared_variable *shared;

void build(int player, int choice, int number)
{
    if(fork()==0)
    {
        semop(semid[player], &p, 1);
        if(shared->player_item[player][RESOURCES] > item_attribute[choice][PRICE] * number)
        {
            shared->player_req_item[player][choice] += number; //dodaj ilosc "do wyprodukowania"
            shared->player_item[player][RESOURCES] -= item_attribute[choice][PRICE] * number; //pobierz surowce
        }
        semop(semid[player], &v, 1);

        semop(semid[player+2], &p, 1);//semafor budowy
        while(shared->player_req_item[player][choice]>0 && shared->working)
        {
            sleep(item_attribute[choice][SPEED]);

            semop(semid[player], &p, 1); //P();

            shared->player_item[player][choice]++; //stworz jednostke
            shared->player_req_item[player][choice]--; //zmniejsz ilosc "do wyprodukowania"

            semop(semid[player], &v, 1); //V();

        }
        semop(semid[player+2], &v, 1);//koniec budowy

        exit(0);
    }

}

void confrontation(int defender, int attacker, int light, int heavy, int cavalry)
{
    int sum_a = light * item_attribute[LIGHT][ATTACK];
    sum_a += heavy * item_attribute[HEAVY][ATTACK] + cavalry * item_attribute[CAVALRY][ATTACK];

    int i, sum_b = 0;
    for(i = 3; i<6; i++)
        sum_b += shared->player_item[defender][i] * item_attribute[i][DEFENCE];

    printf("sum_a %d sum_b %d\n", sum_a, sum_b);
    int tab[3][6]; // tab[player][item]

    semop(semid[defender], &p, 1); //dodany semafor defendera
    if(sum_a - sum_b > 0) //powinno byc ok
    {
        for(i = 3; i<6; i++)
            tab[defender][i] = 0;

        msgplayerbuf msg[3];
        //3 4 to wysylanie
        //1 + 2 i 2 + 2
        msg[attacker].mtype = attacker + 2;
        for(i=0; i<6; i++)
            msg[attacker].data[i] = 0;
        msg[attacker].data[0] = 3;
        msgsnd(k, &msg[attacker], sizeof(msgplayerbuf)-sizeof(long), 0);

        msg[defender].mtype = defender + 2;
        for(i=0; i<6; i++)
            msg[defender].data[i] = 0;
        msg[defender].data[0] = 4;
        msgsnd(k, &msg[defender], sizeof(msgplayerbuf)-sizeof(long), 0);

        semop(semid[0], &p, 1);
        shared->win_count[attacker]++;
        semop(semid[0], &v, 1);
    }
    else
    {
        for(i = 3; i<6; i++)
            tab[defender][i] = shared->player_item[defender][i] - floor((double)shared->player_item[defender][i] * (double)(sum_a*1.0 / sum_b));
        printf("else suma-sumb\n");
        printf("obronca : l%d h%d c%d\n", tab[defender][LIGHT], tab[defender][HEAVY], tab[defender][CAVALRY]);
    } //

/////////////////////////////////////////////////////////////////////////

    sum_b = 0;
    for(i = 3; i<6; i++)
        sum_b += shared->player_item[defender][i] * item_attribute[i][ATTACK];

    sum_a = light * item_attribute[LIGHT][DEFENCE];
    sum_a += heavy * item_attribute[HEAVY][DEFENCE] + cavalry * item_attribute[CAVALRY][DEFENCE];
    printf("sum_b %d sum_a %d\n", sum_b, sum_a);

    if(sum_b - sum_a > 0)
    {
        for(i=3; i<6; i++)
            tab[attacker][i] = 0;
    }
    else
    {
        tab[attacker][LIGHT] = light - floor((double)light * 1.0 * (double)(sum_b*1.0/sum_a));
        tab[attacker][HEAVY] = heavy - floor((double)heavy * 1.0 * (double)(sum_b*1.0/sum_a));
        tab[attacker][CAVALRY] = cavalry - floor((double)cavalry * 1.0 * (double)(sum_b*1.0/sum_a));
        //tab[attacker][CAVALRY] = cavalry * (sum_b*1.0/sum_a);
        printf("else sumb-suma\n");
        int x = (floor((double)light * 1.0 * (sum_b*1.0/sum_a)));
        int y = (sum_b*1.0/sum_a);
        printf("light %d floor %d sumb_suma %d\n", light, x, y);
        printf("atakujacy : l%d h%d c%d\n", tab[attacker][LIGHT], tab[attacker][HEAVY], tab[attacker][CAVALRY]);
    }

    //semop(semid[defender], &p, 1); // ten wyrzucic
    for(i=3; i<6; i++)
        shared->player_item[defender][i] = tab[defender][i];
    semop(semid[defender], &v, 1); // a ten zostawiac

    sleep(5);

    semop(semid[attacker], &p, 1); //wojska atakujacego, ktore przetrwaly niech wroca do domu
    for(i=3; i<6; i++)
        shared->player_item[attacker][i] += tab[attacker][i];
    semop(semid[attacker], &v, 1);


}

void communication(int client_num)
{
    //client 1 or 2
    // 1 - wiadomosc nadawana przez gracza 1
    // 3 - wiadomosc nadawana przez serwer do gracza 1
	if(fork()==0)
	{
        msgplayerbuf player;
        int j = 0, choice = 0;
        int oponent = 0;
        if(client_num == 1)
            oponent = 2;
        else if (client_num == 2)
            oponent = 1;
        else
            printf("?\n");

        while(shared->working)
        {
            	int msg_size = msgrcv(k, &player, sizeof(msgplayerbuf)-sizeof(long), client_num, IPC_NOWAIT);
            	if( msg_size == -1 ) usleep(100000); //jesli kolejka wiadomosc od gracza pusta to czekaj 0.1s
                else if (player.data[0] == 2 && player.data[1] == 0) //rozkaz budowy
            	{
                    choice = 0;
                    for(j=2; j<6; j++)
                    {
                        if(player.data[j] > 0)
                        {
                            choice = j;
                            break;
                        }
                    }
                    if(player.data[choice] > 0)
                    {
                        build(client_num, choice, player.data[choice]);
                        printf("budujemy!\n");

                        for(j=0; j<6; j++)
                            player.data[j] = 0;
                    }
            	}
            	else if (player.data[0] == -1) // rozkaz rezygnacji
            	{
                    shared->working = 0;
                    //jesli 1 to 4 jesli 2 to 3
                    //if play num == 1 then mtype 4 else if play num == 2 then mtype 3
                    if ( client_num == 1 )
                        player.mtype = 4;
                    else if( client_num == 2 )
                        player.mtype = 3;

                    player.data[0] = -1;
                    msgsnd(k, &player, sizeof(msgplayerbuf)-sizeof(long), 0);

                    if ( client_num == 1 )
                        player.mtype = 3;
                    else if( client_num == 2 )
                        player.mtype = 4;
                    else
                        player.mtype = 10;

                    msgsnd(k, &player, sizeof(msgplayerbuf)-sizeof(long), 0);
                    printf("rezygnacja!\n");
                    exit(0);
            	}
            	else if (player.data[0] == 3) //rozkaz ataku
            	{
                    if(player.data[3] == player.data[4] && player.data[4] == player.data[5] && player.data[5] == 0){}
                    else{
                    semop(client_num, &p, 1);
                    if(player.data[3] <= shared->player_item[client_num][3] && player.data[4] <= shared->player_item[client_num][4] &&
                    player.data[5] <= shared->player_item[client_num][5])
                    {
                        // zmniejsz liczbe wojsk w shared
                        for(j=3; j<6; j++)
                            shared->player_item[client_num][j] -= player.data[j];

                        semop(client_num, &v, 1);

                        printf("walka!\n");
                        printf("gracz%d z lekka %d ciezka %d jazda %d vs gracz%d z l. %d c. %d j. %d \n", client_num, player.data[LIGHT], player.data[HEAVY], player.data[CAVALRY],
                        oponent, shared->player_item[oponent][LIGHT], shared->player_item[oponent][HEAVY], shared->player_item[oponent][CAVALRY]);

                        confrontation(oponent, client_num, player.data[LIGHT], player.data[HEAVY], player.data[CAVALRY]);
                    }
                    else
                    {
                    semop(client_num, &v, 1);
                    // podnies semafor
                    }}
            	for(j=0; j<6; j++)
            	player.data[j] = 0;
            	}
            }
        exit(0);
	}



}


int main()
{
	key_t kl = KEY_MSG;
	k = msgget(kl,IPC_CREAT|0600);
	msgplayerbuf start_message;
	start_message.mtype = 5;
	int i=0;

	for(i; i<6; i++)
    start_message.data[i]=0;

	start_message.data[0]= 1;
    msgsnd(k,&start_message, sizeof(msgplayerbuf)-sizeof(long), 0);

	start_message.data[0]= 2;
    msgsnd(k,&start_message, sizeof(msgplayerbuf)-sizeof(long), 0);

	msgrcv(k, &start_message, sizeof(msgplayerbuf)-sizeof(long), 1, 0);
	msgrcv(k, &start_message, sizeof(msgplayerbuf)-sizeof(long), 2, 0);

	printf("Connected!\n");



///////////////////////////////////////////////////////////////////////////////////////
//*************************** PAMIEC WSPOLDZIELONA **********************************//

	int shmid;
	shmid = shmget(601, sizeof(Shared_variable), IPC_CREAT|0600);
	if (shmid == -1)
        printf("blad utworzenia\n");

	shared = shmat(shmid, 0, 0);
	if (shared == NULL)
	{
        printf("blad przydzielenia pamieci itemsy puste\n");
	}

	for(i=1; i<3; i++)
	{
		shared->working = 1;
		shared->player_item[i][RESOURCES] = 300 * 1;
		shared->player_item[i][WORKER] = 0;
		shared->player_item[i][LIGHT] = 0;
		shared->player_item[i][HEAVY] = 0;
		shared->player_item[i][CAVALRY] = 0;
	}
///////////////////////////////////////////////////////////////////////////////////////
//********************************* SEMAFORY ****************************************//

    semid[0] = semget(0, 1, 0600|IPC_CREAT); // semafor zwycieztwa
    semid[1] = semget(1, 1, 0600|IPC_CREAT); // przydzial edycji bufora gracza 1
    semid[2] = semget(2, 1, 0600|IPC_CREAT); // przydzial edycji bufora gracza 2
    semid[3] = semget(3, 1, 0600|IPC_CREAT); // przydzial budowy jednostek gracza 1
    semid[4] = semget(4, 1, 0600|IPC_CREAT); // przydzial budowy jednostek gracza 2

    if((semid[1]==-1)||(semid[4]==-1)||(semid[2]==-1)||(semid[3]==-1))
    {
        printf("problem z semaforami");
    }

    semctl(semid[0],0,SETVAL,1);
    semctl(semid[1],0,SETVAL,1);
    semctl(semid[2],0,SETVAL,1);
    semctl(semid[3],0,SETVAL,1);
    semctl(semid[4],0,SETVAL,1);

    p.sem_num=0; p.sem_flg=0; p.sem_op=-1; //P()
    v.sem_num=0; v.sem_flg=0; v.sem_op=1;  //V()

///////////////////////////////////////////////////////////////////////////////////////
//**************************** POCZATKOWE DANE **************************************//

    item_attribute[WORKER][PRICE] = 150; //workers
    item_attribute[LIGHT][PRICE] = 100; //light
    item_attribute[HEAVY][PRICE] = 250; //heavy
    item_attribute[CAVALRY][PRICE] = 550; //cavalry
    //UWAGA ATTAKI i OBRONA POMNOZONA x10 !!!
    //int rlz
    item_attribute[2][ATTACK] = 0; //workers
    item_attribute[3][ATTACK] = 10; //light
    item_attribute[4][ATTACK] = 15; //heavy
    item_attribute[5][ATTACK] = 35; //cavalry

    item_attribute[2][DEFENCE] = 0; //workers
    item_attribute[3][DEFENCE] = 12; //light
    item_attribute[4][DEFENCE] = 30; //heavy
    item_attribute[5][DEFENCE] = 12; //cavalry

    item_attribute[2][SPEED] = 2; //workers
    item_attribute[3][SPEED] = 2; //light
    item_attribute[4][SPEED] = 3; //heavy
    item_attribute[5][SPEED] = 5; //cavalr

    shared->win_count[1] = 0;
    shared->win_count[2] = 0;

///////////////////////////////////////////////////////////////////////////////////////
//****************************** PRZYROST SUROWCOW **********************************//
	if(fork()==0)
	{
        while(shared->working)
        {
            sleep(1);

            semop(semid[1],&p,1);
            shared->player_item[1][RESOURCES] += 50 + (5 * shared->player_item[1][WORKER]);
            semop(semid[1],&v,1);

            semop(semid[2],&p,1);
            shared->player_item[2][RESOURCES] += 50 + (5 * shared->player_item[2][WORKER]);
            semop(semid[2],&v,1);
        }
        exit(0);
	}

/////////////////////////////////////////////////////////////////////////////////////////
// ****************************** WYSYLANIE WIADOMOSCI ********************************//

// 1 - wiadomosc odbierana przez serwer od gracza 1
// 2 - wiadomosc odbierana przez serwer od gracza 2
// 3 - wiadomosc wysylana do gracza 1
// 4 - wiadomosc wysylana do gracza 2

    if(fork()==0) //cyklicznie wysylaj wiadomosci o stanie gry
    {
        msgplayerbuf player1, player2;
        player1.mtype = 3;
        player2.mtype = 4;
        player1.data[0] = 2;
        player2.data[0] = 2;

        while(shared->working)
        {
            sleep(1);

            for(i = 1; i<6; i++)
            {
                player1.data[i] = shared->player_item[1][i];
                player2.data[i] = shared->player_item[2][i];
            }
            msgsnd(k, &player1, sizeof(msgplayerbuf)-sizeof(long), 0);
            msgsnd(k, &player2, sizeof(msgplayerbuf)-sizeof(long), 0);
        }//while
        exit(0);
    }//fork


    //Odbieranie wiadomosci od gracza
    communication(1);
    communication(2);

    if(fork()==0)
    {
        while(shared->working)
        {
            sleep(5);
            printf("p1 %d p2 %d\n", shared->win_count[1], shared->win_count[2]);

        }
        exit(0);

    }

	while(shared->working)
	{
        if(shared->win_count[1] >= 5)
        {
            shared->working = 0;
            msgplayerbuf msg;
            msg.mtype = 3;
            msg.data[0] = 5;
            msgsnd(k,&msg,sizeof(msgplayerbuf)-sizeof(long), 0);
            perror("1");
            msg.mtype = 4;
            msg.data[0] = 6;
            msgsnd(k,&msg,sizeof(msgplayerbuf)-sizeof(long), 0);
            perror("1");
        } else if (shared->win_count[2] >= 5)
        {
            shared->working = 0;
            msgplayerbuf msg;
            msg.mtype = 3;
            msg.data[0] = 6;
            msgsnd(k,&msg,sizeof(msgplayerbuf)-sizeof(long), 0);
            perror("2");
            msg.mtype = 4;
            msg.data[0] = 5;
            msgsnd(k,&msg,sizeof(msgplayerbuf)-sizeof(long), 0);
            perror("2");
        }
	}

    sleep(5);
	msgctl(k, IPC_RMID, 0);

	return 0;

}
