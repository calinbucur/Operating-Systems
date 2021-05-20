Tema 2 SO
Calin Bucur
332CB

Structura SO_FILE retine urmatoarele informatii:
 - file descriptor
 - flaguri pt permisiunea de scriere/citire
 - cursoare ce indica pozitia curenta in buffer
 - flaguri care indica daca s-a atins end of file sau s-a intalnit o eroare
 - pid-ul procesului (doar daca e vorba de un pipe deschis prin so_popen)
 - tipul fisierului (read/write/append)
 - flag care indica daca operatia anterioara a fost scriere sau citire
 - buffer

Functia so_fopen deschide fisierul in modul specificat de argumentul mode
Daca argumentul e invalid returneaza -1
In functie de mod seteaza flagurile de permisiuni read/write
Aloca o structura de tipul SO_FILE* si o returneaza

Functia so_fgetc:
Verifica permisiunile
Daca bufferul e gol sau s-a ajuns la capatul lui il goleste, reseteaza cursorul si citeste din fisier in bufffer.
In caz de eroare seteaza flagul si intoarce -1
Muta cursorul la dreapta, seteaza operatia anterioara ca read si returneaza caracterul de la pozitia anterioara

Functia so_fputc:
Verifica permisiunile
Daca bufferul e plin (cursorul de scriere e la BUFLEN) se scrie bufferul, se goleste si se reseteaza cursorul
Incearca sa scrie pana cand fie scrie tot bufferul fie da eroare 
In caz de eroare seteaza flagul si intoarce -1
Seteaza operatia anterioara la write pune caracterul in buffer si muta cursorul la dreapta

Functia so_fflush:
Daca operatia anterioara a fost write se scrie bufferul in fisier exact ca la functia anterioara

Functia so_fseek:
Daca operatia anterioara a fost write, apeleaza fflush
Daca operatia anterioara a fost read, goleste bufferul
Muta cursorul fisierului la pozitia dorita si verifica rezultatul

Functia so_ftell:
Calculeaza pozitia cursorului in functie de pozitia cursorului de fisier si cursorului meu de citire/scriere

Functia so_fread:
Citeste caracter cu caracter folosind apeluri de so_fgetc si pune caracterul intors la pozitia corespunzatoare in zona de memorie primita ca parametru

Functia so_fwrite:
Scrie caracter cu caracter de la pozitia corespunzatoare din zona de memorie primita ca parametru folosind apeluri repetate de so_fputc

Functia fclose:
Apeleaza fflush
Inchide file descriptorul
Elibereaza structura SO_FILE*

Functiile so_eof, so_ferror, so_fileno returneaza campurile respective din structura SO_FILE*

Functia so_popen:
Deschide un pipe
Creeaza un proces copil
In procesul copil:
In functie de tipul pipe-ului (read/write) redirecteaza stdin/stdout si inchide capetele nefolosite de pipe
Ruleaza comanda primita ca param
In procesul parinte:
Inchide capatul nefolosit de pipe in functie de tip (red/write)
Aloca si returneaza o structura SO_FILE* exact ca la so_fopen dar seteaza si PID-ul copilului

Functia so_pclose:
Apeleaza so_fflush
Inchide file descriptorul
Asteapta procesul copil
Elibereaza structura SO_FILE*

Tema MUUUUUULT mai decenta decat prima. Chiar ok. Insa, din ce am citit pe stack overflow, Dennis Ritchie spune in cartea aia mare a lui cand vorbeste despre FILE*: "I believe that nobody in their right mind should work with the internals of this structure". Oh, well :))