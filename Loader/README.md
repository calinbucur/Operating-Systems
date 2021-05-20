Calin-Andrei Bucur 332CB
Tema 2 SO

In so_init_loader asignez noul handler pt SIGSEGV
Mai intai aflu dimensiunea unei pagini
Declar o structura de tipul sigaction
Initializez masca de semnale, setez flagul SA_SIGINFO pt a avea acces la adresa page fault-ului
Prin apelul sigaction setez noul handler dar pastrez intr-o variabila si vechiul handler (cel default)

In so_execute, pe langa apelurile functiilor din schelet:
Deschid un file descriptor pt fisierul executabil
De la acest fd voi scrie in paginile de memorie virtuala
Dupa ce parsez executabilul parcurg fiecare segment
Calculez cate pagini ocupa
Aloc un vector de int cu acea dimensiune
Fac data sa pointeze la aceasta zona de memorie
Practic am un int pt fiecare pagina
0 = pagina e inca nemapata
1 = pagina e mapata

In so_handler:
Parcurg segmentele si verific daca page fault-ul se afla in acel segment (intre vaddr si mem_size)
La sfarsitul functiei daca page fault-ul nu apartinea niciunui segment rulez handler-ul default.
Daca apartine unui segment, verific daca fault-ul este intr-o pagina deja mapata.
Daca da, rulez handler-ul default.
Daca nu, verific daca mem_size depaseste file_size
Daca sunt egale verific daca am suficiente date cat sa umplu pagina si calculez cati bytes trebuie copiati din exec
Apoi mapez pagina respectiva si copiez datele din exec
Daca am si zone ce trebuie zeroizate am 3 cazuri:
- Pagina trebuie umpluta complet cu date, caz in care fac maparea corespunzatoare
- Pagina trebuie umpluta partial si restul zeroizata caz in care mapez pagina si copiez datele din executabil apoi zeroizez restul paginii cu un memset
- Pagina trebuie complet zeroizata, caz in care fac maparea folosind flagul MAP_ANON pentru a umple cu zerouri
In cele din urma actualizez vectorul data al segmentului pentru a indica faptul ca pagina este acum mapata

Tema decenta. Greut de inceput, a durat ceva pana m-am prins ce vrea de la mine dar dupa merge repede. Debugging cam dubios, dar am reusit pana la urma cu printuri :)))))