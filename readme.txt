
===============================================================================


                    SALCIE IOAN-CRISTIAN, 323CDb
				Tema 2, Protocoale de Comunicatii, April 2020
                

===============================================================================


1. Structura temei:

- tema este alcatuita din 5 fisiere:
	-> Makefile        - compileaza, ruleaza si curata tema
                       - pentru a rula serverul se foloseste comanda
                            make run_server
                       - pentru a rula subscriber-ul se foloseste comanda
                            make ID=<id> run_subscriber

	-> server.h        - contine structuri, si functii necesare serverului.

	-> server.c 
	-> subscriber.c

    -> readme.txt


===============================================================================


2. Detalii implementare:
    Incep prin a preciza faptul ca am simtit absolut necesar sa adaug
urmatoarea functionalitate: Atunci cand un client s-a reconectat se va printa
in server: "Client <ID> reconnected from <IP>:<PORT>". Asa ca am lasat
printf-ul respectiv comentat, printand tot cum se cere "New client ...".

    Pt SF voi folosi fisiere pt fiecare subscriber de tipul
sub<Subscriber_ID>.SF in care se vor afla toate mesajele ce trebuie sa ajunga
la acesta atunci cand se reconecteaza.

    Am folosit setsockopt pentru a dezactiva algoritmul Neagle de la tcp.

    Aceste aspecte mi s-au parut mai importante de precizat, restul detaliilor
fiind scrise prin intermediul comentariilor pe parcursul codului.


===============================================================================
