-----Copyright Vlad Balteanu 321CA--------
------Tema2 Pcom: Server client app-------

Pentru implementarea acestei teme am pornit de la rezolvarea
laboratorului 7 si am preluat functiile deja implementate 
din fisierul recv_send.cpp. Serverul porneste si se pune in
starea de asteptare pentru clientii TCP. Cand acestia se conecteaza,
pot da subscribe la anumite topicuri, iar cand clientii UDP trimit
mesaje pe acele topicuri, abonatii le vor primii.

--TCP_client--
Cand porneste clientul, acesta intializeaza o conexiune cu serverul
si ii trimite ID-ul cu care s-a conectat. Pentru a optimiza, trimit
mai intai un octet care reprezinta lungimea ID-ului apoi ID-ul in sine.
Clientul poate primi comenzile subscribe, unsubscribe si exit.
Pentru subscribe si unsubscribe, clientul trimite un mesaj cu comanda
primita, tot precedat de un mesaj ce contine lungimea ei. La exit
se inchide socketul folosit si serverul primeste mesajul si afiseaza
un mesaj corespunzator.

Cand primeste un mesaj de la server, il pune intr-o structura tcp_msg
si afiseaza la stdout mesajul in forma din cerinta.

--Server--
Cand serverul porneste, se creeaza un socket pentru comunicarea cu
clientii UDP si unul pasiv pe care serverul face listen, pentru
primirea de cereri de conectare de la clientii TCP.

Serverul foloseste un vector de client_info pentru a stoca informatii
despre fiecare client care s-a conectat la un moment dat la server.
Se retin ID-ul, socket-ul pe care s-a facut comunicarea cu clientul TCP,
starea de online/ofline si topicurile la care este abonat.

Intrucat mesajele de la clientii de UDP au o structura predefinita, la
primirea unui mesaj de la un astfel de client, serverul populeaza o
structura de tipul UDP_msg prin care avem acces la topic, tipul de date
si payload-ul. Analog, si mesajele pe care le trimite catre clientii TCP
au un format predefinit, asa ca serverul prelucreaza mesajul primit de la
UDP si populeaza o structura de tip tcp_msg pe care o trimite catre clientul
care trebuie sa primeasca mesajul.

Pentru comunicarea cu clientul de TCP, pentru mesaje de tip subscribe, unsubscribe
primeste intr-un string pe care prelucreaza si in functie de tipul de mesaj,
updateaza campul de subscriptions din structura clientului respectiv. Pentru exit din
partea clientului, acesta doar este marcat ca ofline si socket-ul pentru comunicare
este facut -1, pentru ca un client sa se poata reconecta odata ce a facut o cerere
de conectare prima oara. Cand un client incearca sa se conecteze cu un id existent,
serverul inchide conexiunea imediat.

Cand serverul primeste exit, acesta inchide conexiunea cu toti clientii si isi elibereaza
memoria.

PS: am avut 2 zile de sleepday, intrucat am fost plecat in vacanta de Paste, si am avut probleme
cu primirea pachetelor de udp, intrucat pe MACos rula corect programul, iar pe masina virtuala nu.
De aici si intarzierea pentru ca am trimis-o de pe MACos unde credeam ca merge. 
Tema a fost overall una decenta ca greutate, frumos de implementat, daca ma organizam mai bine cu
timpul nu aveam probleme.
