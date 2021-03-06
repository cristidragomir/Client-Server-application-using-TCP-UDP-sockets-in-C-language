324CB - Dragomir Constantin-Cristian

Explicare implementare TEMA 2 - Protocoale de Comunicatii

I. Partea de server (SURSA: server.c)

Acesta trebuie sa fie capabil sa primeasca atat mesaje UDP cat si TCP.
Pentru aceasta trebuie sa se configureze un socket pentru ambele protocoale.
In functiile configure_server_tcp() si configure_server_udp() se realizeaza
acest lucru. Intrucat se procedeaza aproximativ la fel in cazul ambelor
initializari, a fost creata functia server_common_config() in care se tine
cont de pasii suplimentari pentru initializarea socket-ului TCP.

Dupa initializarea acestor socketi, se pune la punct partea de 
multiplexare I/O. Se creeaza o multime de descriptori de citire (input)
formata, initial, din cei 2 socketi si intrarea standard - 0.
Ne vom ajuta de o structura numita TCPClientsDB pentru a retine
detaliile despre clientii ce vor urma sa intre pe server (numarul curent
de clienti, tabloul de structuri in care se retin informatiile despre clienti,
struct Clients_info).

Pentru abonari/dezabonari se va utiliza o tabela de dispersie. Lista principala
va contine numele topic-urilor. In structura specifica (struct topic_cls)
se retine numele topic-ului si o lista ce contine structuri de tipul
struct Subscription ce are drept campuri: un pointeri catre un struct 
Client_info si un intreg prin care se retine daca un client, pe acel abonament,
doreste sa aiba capacitatea de a citi mesajele ce au fost trimise cat timp el a
fost offline (facilitatea store-and-forward).

In bucla continua, se face o copie a descriptorilor de fisiere initiali, 
intrucat functia select() va modifica aceasta multime. Verificam daca exista
input pe intrarea standard si ulterior pe ceilalti socketi (de mentionat ca in
cazul TCP numarul de socketi va creste cu fiecare client conectat, 
fata de UDP pentru care exista numai un socket).

Daca pe un anumit socket exista un input se face apelul functiei
control_inp_srcs() care manipuleaza tot ceea ce primeste serverul.

Daca socket-ul serverului exista input, atunci inseamna ca se realizeaza
o noua conexiune. Prin apelul functiei accept() se creeaza un nou socket
care va fi utilizat in comunicarea server-client specific. Acest nou socket
se adauga la lista de descriptori de input si se actualizeaza rangul celui
mare descriptor(max_input_rank). In baza de date a clientilor se adauga
informatiile. De mentionat ca in aceasta etapa, clientul nou creat nu poseda
un anumit ID. Clientul este configurat astfel incat, dupa ce conexiunea cu 
serverul este realizata, sa trimita un mesaj TCP (struct TCPmsg) de tip '1' 
care contine ID-ul clientului. Serverul primeste un astfel de mesaj si
se apeleaza functia recv_cl_id(). In acest punct al executiei se cauta
clientul in baza de date in functie de socket-ul pe care a venit mesajul.
Se verifica daca nu cumva in baza de date exista deja un client conectat
cu acelasi ID. In caz afirmativ, clientul va primi un mesaj de tip 'E'
care nu contine altceva, si in urma caruia conexiunea cu acel client
va fi inchisa, iar la nivelul clientului se inchide programul. (Acest caz
este tratat de server in functia duplicate_id() care afiseaza la ecran
si o atentionare conform careia clientul este deja conectat). In cazul
in care clientul figureaza ca fiind deconectat, se considera ca un client
s-a conectat prima data pe server, s-a deconectat, iar acum doreste
sa refaca conexiunea cu alte detalii de conectare (IP si PORT schimbate).
Daca clientul este chiar nou-venit, atunci se incrementeaza numarul
de clienti ai serverului si i se completeaza campul ID.

Daca clientul trimite o cerere de subscribe, adica un mesaj TCP de tip '2',
se acceseaza functia add_subscription(). In aceasta functie se cauta indexul
clientului din baza de date dupa socket-ul pe care a venit cererea de 
subscribe. Ulterior, se face o cautare in lista de topic-uri (cheiele tabelei
de dispersie), dupa topicul la care se vrea abonarea (sau se creeaza o 
noua cheie) - lucru realizat in functia find_topic() -. Dupa ce topic-ul a
fost gasit, se verifica daca exista un abonament mai vechi al clientului si 
se reinnoieste. In caz contrar, se creeaza un nou abonament (o noua intrare 
la o anumita cheie din tabela de dispersie).

Structura buffer-ului este: 1 byte pentru a desemna tipul mesajului TCP = '2',
51 de bytes care contin numele topicului + '\0' + 1 byte pentru '0' sau '1',
pentru a dezactiva/activa facilitatea de store-and-forward.

Daca clientul trimite o cerere de unsubscribe, un mesaj TCP de tip '3', se 
acceseaza functia remove_subscription(). Ca si la add_subscribe, se cauta 
clientul in functie de socket-ul pe care a venit mesajul. Se cauta topicul
de la care se doreste dezabonarea, cu mentiunea ca atunci cand topicul
de la care se doreste dezabonarea este inexistent, atunci nu se intampla nimic
(in aceasta implementare). Ulterior se incearca o cautare simpla in lista
de abonamente a clientilor pentru a-l elimina pe cel dorit (in caz ca exista).

Structura buffer-ului este: 1 byte pentru a desemna tipul mesajului TCP = '3',
51 de bytes care contin numele topicului + '\0'.

Clientul se poate deconecta. Acest lucru este semnalat de functia recv() care
returneaza valoarea 0. In aceasta situatie se marcheza in baza de date a
clientilor, la acel client, cu '0' campul is_connected si socket devine -1.
Se elimina din multimea descriptorilor acel socket, iar la randul sau
socket-ul este inchis. Totodata, se afiseaza si un mesaj la consola server-ului
cum ca acel client s-a deconectat.

Cand serverul primeste un mesaj UDP se apeleaza functia handle_udp_msg().
In aceasta functie, se creeza un buffer ce va putea fi decodificat
intr-un mesaj TCP de tip '4'. Buffer-ul respectiv contine pe primul byte
tipul mesajului, urmat de un alt buffer de dimensiunea mesajului UDP. Ultimii
6 bytes sunt ocupati de: 4 bytes - IP-ul clientului UDP, PORTUL clientului UDP.

In share_udp_msg() acest buffer este trimis clientilor. In caz ca sunt deja
conectati, ei vor primi imediat mesajul de pe acel topic. In caz ca sunt
offline, si au activata optiunea de store-and-forward, mesajul se va salva
intr-o coada specifica acelui client (campul prev_msgs din Client_info).

Este important de mentionat ca, atunci cand un client se reconecteaza, 
si in coada specifica lui sunt mesaje mai vechi, acesta le va primi in 
momentul in care se reconecteaza. (Acest lucru se realizeaza in recv_cl_id()).

Daca serverul preia de la intrarea standard comanda "exit" atunci se apeleaza
functia disconnect_all_cls() care va realiza deconectarea si inchiderea tuturor
clientilor. Acestia primesc un mesaj TCP cu tipul 'E'.
Ulterior se iese din bucla infinita si se elibereaza memoria.

II. Partea de client (tcp_client.c)

Se initializeaza socket-ul si conexiunea cu serverul. Se initializeaza
multimea de descriptori de input, formata din intrarea standard si socket-ul
TCP specific clientului, pe care va primi informatii de la server.
Dupa conectare, clientul trimite automat un mesaj de tip '1', cu ID-ul
pe care sau, lucru realizat in functia send_client_id().

Intr-o bucla infinita se verifica pe care dintre descriptori s-au primit
informatii. Daca de la intrarea standard se primeste input, se apeleaza functia 
read_stdin(). Aici, in functie de comanda data de utilizator, se poate returna
un EXIT_CODE - ceea ce determina inchiderea clientului. Daca se trimite 
o comanda de SUBSCRIBE valida, se apeleaza functia send_subscribe_req().
Daca se trimite o comanda de UNSUBSCRIBE valida, se apeleaza functia 
send_unsub_req(). In fiecare din aceste functii se completeaza o structura
de tip TCP care va fi ulterior asezata intr-un buffer ce va fi trimis catre
server. Trecerea de la reprezentarea in structura intr-un buffer se realizeaza
in functia send_tcp_msg().

In cazul in care descriptorul de socket "este setat" (FD_ISSET), atunci se
apeleaza functia read_server(). Aici poate primi un mesaj de tipul 'E', care
da semnal clientului sa se deconecteze de la server si sa inchida. Pe de alta
parte, el poate primi un mesaj UDP care trebuie afisat la ecran.

Incadrarea buffer-ului UDP primit intr-o structura de tip UDP (struct UDPmsg)
este realizata de functia parse_message_udp() din fisierul sursa 
common_funcs.c. Afisarea efectiva a continutului UDP este realizata de 
functia display_udp_msg() din acelasi fisier sursa mentionat.

In header-ul tools.h s-au definit anumite macro-uri, structurile de date
mentionate si antetul anumitor functii.

Pentru structura de date Lista si Coada s-au folosit fisierele sursa:
list.c, list.h, queue.c, queue.h din scheletul Temei 1.
