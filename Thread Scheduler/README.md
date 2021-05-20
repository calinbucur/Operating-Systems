Tema 4
Calin-Andrei Bucur
332CB

Scheduler-ul este reprezentat de o structura ce contine:
	- Cuanta
	- Numarul de evenimente IO
	- Thread-ul running
	- Cozi pentru thread-urile waiting, ready si finished
	- Semafor ce verifica daca toate threadurile au terminat

Thread-urile sunt reprezentate de o structura(in queue.h) ce contine:
	- ID-ul thread-ului
	- Prioritatea thread-ului
	- Cuanta de timp ramasa pt acel thread
	- "Varsta" folosita pt a garanta FIFO deoarece am implementat coada de prioritati cu heap(care la prioritati egale nu ar respecta FIFO fara "varsta")
	- Functia ce trebuie rulata de thread
	- Semafor care este folosit pt a bloca/porni executia thread-ului

Folosesc structura queue pentru a implementa
atat o coada normala pentru cozile de waiting si finished(dar care nu respecta FIFO pt ca nu e nevoie)
cat si o coada de prioritati(actually a heap) folosita pt coada ready

Pentru sincronizarea thread-urilor, fiecare structura corespunzatoare unui thread are un semafor initializat cu 0 pe care se face wait (deci threadul e blocat)
Cand un thread trebuie sa ruleze se ia structura din coada corespunzatoare si se face post pe semafor pt a proni executia.
Cand un thread este preemptat se face un wait pe semaforul respectiv, blocand thread-ul pana la urmatorul post.  

Implementarea efectiva:

Functia schedule() decide ce thread va rula si face preemptia
-Mai intai verifica daca ruleaza un thread
-Daca nici un thread nu ruleaza si nici un thread nu e ready, ii semnaleaza schedulerului ca toate threadurile au terminat
-Daca nici un thread nu ruleaza dar sunt threaduri ready, extrage din heap threadul cu max priority, il pune in running si face post pe semaforul lui pt a il porni.
-Daca exista thread care ruleaza, ii scade cuanta
-Daca in coada ready exista un thread cu prioritatea mai mare sau cuanta este 0, preempteaza astfel:
--Reseteaza cuanta si insereaza threadul in coada ready, apoi scoate maximul din ready si il pune in running
--Porneste thread-ul printr-un post pe semafor si il opreste pe anteriorul cu un wait pe semaforul lui.

Functia so_init:
-Verifica argumentele si daca scheduler-ul a fost deja instantiat
-Aloca scheduler-ul si ii initializeaza campurile (inclusiv cozile)

Functia start_thread:
-Rulata de toate threadurile
-Folosita pt sincronizare
-Primeste structura thread-ului ca param
-Face wait pe semafor deoarece initial thread-ul trebuie sa fie blocat
-Ruleaza handler-ul corespunzator
-Cand a terminat elibereaza campul running, insereaza thread-ul in coada finished si apeleaza schedule pt a pune in executie urmatorul thread

Functia so_fork:
-Varifica argumentele
-Indica scheduler-ului ca un prim fork a avut loc deci trebuie sa astepte toate thread-urile
-Aloca structura thread
-Creeaza thread-ul efectiv
-Il insereaza in coada ready
-Apeleaza scheduler-ul in caz ca noul thread are prioritate maxima

Functia so_wait:
-Verifica parametrul
-Insereaza thread-ul curent in coada de waiting corespunzatoare evenimentului
(In caz ca nu am mentionat, pt waiting am un array ce contine o coada pt fiecare IO event posibil)
-Apeleaza schedule pt a pune in executie urmatorul thread
-Blocheaza vechiul thread cu un apel de wait pe semafor

Functia so_signal:
-Verifica parametrul
-Scoate toate thread-urile din coada de waiting corespunzatoare evenimentului si le introduce in coada ready
Apeleaza schedulerul
Returneaza numarul de thread-uri mutate

Functia so_exec pur si simplu apeleaza schedule

Functia so_end:
Verifica daca scheduler-ul este instantiat ca sa aiba ce elibera :)
Asteapta terminarea tuturor thread-urilor, le face join si le elibereaza structura
Elibereaza cozile
Elibereaza schedulerul si il seteaza pe null


O tema destul de interesanta, mai ales ca am mai facut o simulare a ei in anul 1 la SD. Totusi putin frustranta deoarece mi se pare ca testarea e weird si e destul de greu de facut debugging, iar enuntul din nou cam greu de inteles. Am stat cateva ore bune doar sa inteleg ce se vrea de la mine inainte sa ma pot apuca :))) dar dupa, implemetarea a mers chiar ok. Also, ar fi mers un C++, mi-e sila sa tot trebuiasca sa implementez cozi in C