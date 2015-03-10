//klienta polecam odpalac w terminalu linuxa (najlepiej ubuntu dla rozdzielczosci 1600x900 - 17,3cala)
//serwer musi zostac uruchomiony przed uruchomieniem klienta
#include <ncurses.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <error.h>
#include <stdlib.h>

#define KEY_MSG 666
#define TICKRATE 100



void draw_borders(WINDOW *screen) {
    int x, y, i;

    getmaxyx(screen, y, x);

//1-3
//| |
//2-4

// 4 corners
    mvwprintw(screen, 0, 0, "+");
    mvwprintw(screen, y - 1, 0, "+");
    mvwprintw(screen, 0, x - 1, "+");
    mvwprintw(screen, y - 1, x - 1, "+");

// sides
    for (i = 1; i < (y - 1); i++)
    {
        mvwprintw(screen, i, 0, "|");
        mvwprintw(screen, i, x - 1, "|");
    }

// top and bottom
    for (i = 1; i < (x - 1); i++)
    {
        mvwprintw(screen, 0, i, "-");
        mvwprintw(screen, y - 1, i, "-");
    }
}


typedef struct Message
{
	long mtyp;
	int number;

} Message;

typedef struct msgplayerbuf {
long mtype; // 0 - player 1, 1 - player 2;
int data[6];
// standar
// 0 - number of players; (3 - war)
// 1 - resources;
// 2 - workers;
// 3 - light_soldiers;
// 4 - heavy_soldiers;
// 5 - cavalry;
} msgplayerbuf;

int main()
{
    //start
    initscr();
    printw( "Connecting to server" );
    noecho();
    timeout(TICKRATE);
    refresh();
    curs_set(FALSE);

    int k;
	key_t kl = KEY_MSG;
	k = msgget(kl,0600);
    msgplayerbuf start_game;
    int y=0;
    for(y; y<6; y++)
    start_game.data[y]=-1;

    msgrcv(k,&start_game,sizeof(msgplayerbuf)-sizeof(long), 5, 0); //odb wiadomosc startowa

    if(start_game.data[0]==1)
    {
        start_game.mtype = 1;
    }
    else if(start_game.data[0]==2)
    {
        start_game.mtype = 2;
    }
    else
    {
        printw("ERR!");
        refresh();
    }
    printw("%d %d %d %d %d %d", start_game.data[0], start_game.data[1], start_game.data[2], start_game.data[3], start_game.data[4], start_game.data[5]);
    refresh();
    msgsnd(k, &start_game, sizeof(msgplayerbuf)-sizeof(long), 0); //potwierdz dolaczenie do gry

    clear();

    int recive = start_game.mtype + 2; //kolejka odbierania
    int send = start_game.mtype; //kolejka wysylania
    int znak;
    int liczba = 0;
    char command[100] = "";
    int i = 0;

    msgplayerbuf msg, msg_to_send;
    msg_to_send.mtype = send;

    for(i = 0; i<6; i++)
    {
        msg.data[i] = 0;
        msg_to_send.data[i] = 0;
    }
    i = 0;
    bool last_cmnd_err = 0;


    WINDOW *field = newwin(24 - 3, 80, 0, 0); //deklaracja okienek
    WINDOW *score = newwin(3, 80, 24 - 3, 0);
    WINDOW *resourcebox = newwin(4,16,0,2);
    WINDOW *workerbox = newwin(4,15,0,18);
    WINDOW *lightbox = newwin(4,15,0,33);
    WINDOW *heavybox = newwin(4,15,0,48);
    WINDOW *cavalrybox = newwin(4,15,0,63);
    WINDOW *moneyicon = newwin(10, 20,4,1);
    WINDOW *cavalryicon = newwin(14, 22,4,60-3);
    WINDOW *lighticon = newwin(10, 10, 4, 37);
    WINDOW *heavyicon = newwin(10, 17, 4, 47);
    WINDOW *workericon = newwin(10, 12, 4, 20);
    WINDOW *info = newwin(7, 50, 11, 0);
    WINDOW *last_cmm = newwin(3, 80, 18, 0);

    mvwprintw(moneyicon, 0, 0, "      _______ \n    .'_/_|_\\_'. \n    \\`\\  |  /`/ \n     `\\\\ | //' \n       `\\|/` \n         `");
    mvwprintw(workericon, 0, 0, " \n    O <=> \n   /|\\/ \n    | \n    /\\ \n   /  \\ ");
    mvwprintw(cavalryicon, 0, 0, " \n                \\ \n                 \\ \n                ~ \\) \n         _,,   {] |` \n        \"-=\\;   %/  \n         _ \\;_/%% \n         _\\| \\_%% \n         \\  \\/\\  \n             ( )~~~ \n             | \\ \n            /  / ");
    mvwprintw(heavyicon, 0, 0, "\n              / \n       ,~~   / \n   _  <=)  _/_ \n  /I\\.=\"==.{> \n  \\I/-\\T/-\' \n      /_\\ \n     // \\\\_ \n    _I    /");
    mvwprintw(lighticon, 0, 0, "\n      k \n   O  | \n ()Y==o\n  /_\\ | \n  _W_ |");
    mvwprintw(info, 1, 1, "Buduj za pomoca komendy build TYP*liczba\n atakuj za pomoca komendy attack TYP*liczba\n zawsze mozesz zrezygnowac komenda resign np:\n build w10 - zbuduj 10 roboli \n attack l10 h10 c10 ");

    draw_borders(cavalrybox); //rysowanie ramek
    draw_borders(heavybox);
    draw_borders(lightbox);
    draw_borders(workerbox);
    draw_borders(resourcebox);
    draw_borders(field);
    draw_borders(score);
    draw_borders(info);
    mvwprintw(info, 0, 4, "INFO");
    draw_borders(last_cmm);
    mvwprintw(last_cmm, 0, 4, "Zdarzenia");

    wrefresh(field); //odswiezanie okienek
    wrefresh(score);
    wrefresh(resourcebox);
    wrefresh(workerbox);
    wrefresh(lightbox);
    wrefresh(heavybox);
    wrefresh(cavalrybox);
    wrefresh(moneyicon);
    wrefresh(cavalryicon);
    wrefresh(lighticon);
    wrefresh(heavyicon);
    wrefresh(workericon);
    wrefresh(info);
    wrefresh(last_cmm);
    bool working = 1; // zmienia sie na 0 gdy otrzyma wiadomosc ze gra sie zakonczyla


    while ((znak = getch()) && working == 1)
    {
        usleep(30000); // odswiezanie mniej wiecej 30FPSow
        // w konsoli raczej nie widac odswiezania

        int new;
        new = msgrcv(k, &msg, sizeof(msgplayerbuf)-sizeof(long), recive, IPC_NOWAIT);

        if(msg.data[0] != 2)
        {
            wclear(last_cmm);
            switch(msg.data[0])
            {
                case -1: working = 0; mvwprintw(last_cmm, 1, 1, "gra przerwana!"); break;
                case 3: mvwprintw(last_cmm, 1, 1, "wygrales ostatnia walke!"); break;
                case 4: mvwprintw(last_cmm, 1, 1, "przegrales ostatnia walke!"); break;
                case 5: working = 0; mvwprintw(last_cmm, 1, 1, "jestes zwyciezca! - koniec gry"); break;
                case 6: working = 0; mvwprintw(last_cmm, 1, 1, "przegrales - gra zakonczona"); break;
            }
            wrefresh(last_cmm);
        }
        else if(working)
        {
            wclear(resourcebox);
            wclear(workerbox);
            wclear(lightbox);
            wclear(heavybox);
            wclear(cavalrybox);
            wclear(score);
            mvwprintw(resourcebox, 1, 1, " Zloto\n   %d",  msg.data[1]);
            mvwprintw(workerbox, 1, 1, " Robotnicy[w]\n   %d", msg.data[2]);
            mvwprintw(lightbox, 1, 1, " Lekka[l]\n   %d", msg.data[3]);
            mvwprintw(heavybox, 1, 1, " Ciezka[h]\n   %d", msg.data[4]);
            mvwprintw(cavalrybox, 1, 1, " Jazda[c]\n   %d", msg.data[5]);
            mvwprintw(score, 1, 1, command);

        }


        if (last_cmnd_err == 1 )
        {
            wclear(last_cmm);
            mvwprintw(last_cmm, 1, 1, "bledna komenda");
            wrefresh(last_cmm);
            last_cmnd_err = 0;
        }


        mvwprintw(moneyicon, 0, 0, "      _______ \n    .'_/_|_\\_'. \n    \\`\\  |  /`/ \n     `\\\\ | //' \n       `\\|/` \n         `");
        mvwprintw(workericon, 0, 0, " \n    O <=> \n   /|\\/ \n    | \n    /\\ \n   /  \\ ");
        mvwprintw(cavalryicon, 0, 0, " \n                \\ \n                 \\ \n                ~ \\) \n         _,,   {] |` \n        \"-=\\;   %/  \n         _ \\;_/%% \n         _\\| \\_%% \n         \\  \\/\\  \n             ( )~~~ \n             | \\ \n            /  / ");
        mvwprintw(heavyicon, 0, 0, "\n              / \n       ,~~   / \n   _  <=)  _/_ \n  /I\\.=\"==.{> \n  \\I/-\\T/-\' \n      /_\\ \n     // \\\\_ \n    _I    /");
        mvwprintw(lighticon, 0, 0, "\n      k \n   O  | \n ()Y==o\n  /_\\ | \n  _W_ |");
        mvwprintw(info, 1, 1, "Buduj za pomoca komendy build TYP*liczba\n atakuj za pomoca komendy attack TYP*liczba\n zawsze mozesz zrezygnowac komenda resign np:\n build w10 - zbuduj 10 roboli \n attack l10 h10 c10 ");

        draw_borders(cavalrybox);
        draw_borders(heavybox);
        draw_borders(lightbox);
        draw_borders(workerbox);
        draw_borders(resourcebox);
        draw_borders(field);
        draw_borders(score);
        draw_borders(info);
        mvwprintw(info, 0, 4, "INFO");
        draw_borders(last_cmm);
        mvwprintw(last_cmm, 0, 4, "Zdarzenia");

        wrefresh(field);
        wrefresh(score);
        wrefresh(resourcebox);
        wrefresh(workerbox);
        wrefresh(lightbox);
        wrefresh(heavybox);
        wrefresh(cavalrybox);
        wrefresh(moneyicon);
        wrefresh(cavalryicon);
        wrefresh(lighticon);
        wrefresh(heavyicon);
        wrefresh(workericon);
        wrefresh(info);
        wrefresh(last_cmm);
        //sleep(1);

        refresh();

        if(znak != ERR) //odczytywanie komendy z klawiatury, ponownie zalecam ubuntu 14.10, a przynajmniej terminal backspace odczytuje jako liczbe 127
        {
            if(znak == 10 )  //enter
            {
                last_cmnd_err = 0;

                int j=0;
                    while(!( (command[j]>='a' && command[j]<='z') || (command[j]>='0' && command[j]<='9') ) )
                        j++;

                    if(command[j]=='a' && command[j+1] == 't' && command[j+2] == 't' && command[j+3]=='a' && command[j+4]=='c' && command[j+5]=='k')
                    {//attack comand
                        //ATAK
                        int x;
                        for(x=0; x<6; x++)
                        msg_to_send.data[x] = 0;
                        int next = 13;

                        msg_to_send.data[0] = 3; //deklaracja wojny
                        j = 6; //odczytano attack

                        while(command[j]==' ')
                        j++;
                        //j powinno byc 7
                        int choice = 0;
                        while((command[j]>='a' && command[j]<='z') || (command[j]>='0' && command[j]<='9') || command[j]==' ')
                        {
                            choice = 0;
                            while(command[j]==' ')
                            j++;
                            next++;

                            if(command[j]=='l')
                            choice = 3;
                            else if(command[j]=='h')
                            choice = 4;
                            else if(command[j]=='c')
                            choice = 5;

                            j++;
                            //tutaj nie moze byc spacji miedzy typem a iloscia

                            int number_from_string = 0;
                            if(choice > 0)
                            {
                                while(command[j]>='0' && command[j]<='9')
                                {
                                    number_from_string *= 10;
                                    number_from_string += command[j] - '0';
                                    j++;
                                }
                                msg_to_send.data[choice] = number_from_string;
                            }
                        }
                        msg_to_send.mtype = send;
                        msgsnd(k,&msg_to_send,sizeof(msgplayerbuf)-sizeof(long),0);
                    }
                    else if (command[j]=='b' && command[j+1]=='u' && command[j+2]=='i' && command[j+3] =='l' && command[j+4] =='d')
                    {//BUDUJ
                        int choice = 0;

                        j += 4; //literka d
                        j++; //spacja
                        j++; //typ

                        if(command[j]=='w')
                        choice = 2;
                        else if(command[j]=='l')
                        choice = 3;
                        else if(command[j]=='h')
                        choice = 4;
                        else if(command[j]=='c')
                        choice = 5;
                        else
                        last_cmnd_err = 1;

                        j++;                            //nastepny znak powinien byc liczba!
                        int number_from_string = 0;

                        if(choice > 0)
                        {
                            while(command[j]>='0' && command[j]<='9')
                            {
                                number_from_string *= 10;
                                number_from_string += command[j] - '0';
                                j++;
                            }
                            //tu liczba powinna sie skonczyc
                            //nastepne znaki beda ignorowane
                            //innymi slowy, budowana bedzie pierwsza podana jednostka
                            //serwer nie przyjmuje wiecej niz jednego typu jednostke do budowy
                            int x;
                            for(x=0; x<6; x++)
                            msg_to_send.data[x] = 0;

                            msg_to_send.mtype = send;
                            msg_to_send.data[0] = 2;
                            msg_to_send.data[choice] = number_from_string;
                            msgsnd(k, &msg_to_send, sizeof(msgplayerbuf)-sizeof(long), 0);
                        }//pobieranie i wysylanie żądania
                    }
                    else if (command[j]=='r' && command[j+1]=='e' && command[j+2]=='s' && command [j+3] == 'i' &&command[j+4]=='g' && command[j+5]=='n')
                    {
                        //REZYGNUJ :<
                        msg_to_send.mtype = send;
                        msg_to_send.data[0] = -1;
                        msgsnd(k, &msg_to_send, sizeof(msgplayerbuf)-sizeof(long), 0);
                    }
                    else
                    {
                        last_cmnd_err = 1;
                    }

                for(j=0; j<i; j++)
                command[j] = '\0';
                i = 0;
                wclear(score);
                draw_borders(score);
                mvwprintw(score, 1, 1,command);
                wrefresh(score);
                //refresh();
            }
            else if(znak == 127) //backspace
            {
                if(i>0)
                {
                    i--;
                    command[i]='\0';
                    wclear(score);
                    draw_borders(score);
                    mvwprintw(score, 1, 1, command);
                    wrefresh(score);
                    //refresh();
                }
            }
            else if((znak>='a' && znak<='z') || (znak>='0' && znak<='9') || znak==' ') //odczytywanie zwyklych znakow do komendy
            {
                char word[1];
                word[0]=znak;
                command[i]=word[0];
                i++;
                wrefresh(score);
                //refresh();
            }
        }
    }
    sleep(10);

    getch();
    endwin();
}
