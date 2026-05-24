Am folosit Gemini pentru ambele faze ale proiectului.
FAZA 1:
I-am spus ca am o structura Report si sa ma ajute sa fac 2 functii: parse_condition si match_condition. La parse_condition i-am zis ca trebuie sa imi desparta un text de forma camp:operator:valoare in 3 bucati folosind semnul ":" ca separator, iar la match_condition i-am spus ca trebuie sa compare datele dintr-un raport cu valorile primite(de exemplu, daca sveritatea este mai mare decat 2).

AI-ul mi-a propus initial o functie complicata cu sscanf si cod formatat automat pentru compararea tuturor campurilor.

Am schimbat sscanf cu strtok pentru ca mi s-a parut mai simplu sa inteleg cum separ textul in bucati folosind ":". Am mai observat ca AI-ul uita uneori sa transforme textul in numar inainte sa compare severitatea, asa ca am adaugat eu conversia pentru ca semnele "<" sau ">" sa functioneze corect.

FAZA 2:
În această etapă, am apelat la AI pentru a înțelege conceptele noi de procese și semnale UNIX, pe care apoi le-am aplicat în cod.
Am cerut o explicație despre fluxul de lucru pentru fork() și modul în care un proces fiu poate rula o comandă de sistem precum rm -rf. Pe baza acestor explicații, am scris funcția remove_district, adăugând verificări de siguranță pentru a preveni ștergerea accidentală a directoarelor rădăcină (verificând dacă numele districtului nu este / sau .).
Am consultat AI-ul pentru a înțelege diferența dintre signal() și sigaction(), deoarece cerința interzicea explicit folosirea signal(). Am configurat singură structura sigaction pentru a gestiona SIGINT și SIGUSR1, folosind flag-ul SA_RESTART pentru a asigura stabilitatea programului monitor.
Am cerut sfaturi despre cum poate un program să citească un identificator de proces dintr-un fișier ascuns. Ulterior, am scris codul prin care city_manager extrage PID-ul din .monitor_pid și folosește kill() pentru a trimite notificarea către monitor_reports.

FAZA 3:
Pentru ultima etapa, AI-ul a fost un punct de sprijin in intelegerea mecanismului de redirectionare a fluxurilor de date prin pipes. Am cerut lamuriri despre cum functioneaza pipe() combinat cu dup2() pentru a prinde iesirea standard a unui proces apelat prin exec.
Pe baza teoriei intelese, am configurat buclele de lansare pentru procesele scorer in paralel. Am gestionat citirea curata din descriptorii de pipe din programul principal (folosind numarul exact de octeti n cititi, nu dimensiunea maxima a buffer-ului). Am implementat un sistem de stocare agregata in tablouri paralele in cadrul programului score.c pentru a calcula suma severitatilor per inspector(Workload Score).
Am corectat manual ordinea apelurilor in start_monitor_command. Initial, existau linii lasate dupa apelul execvp (precum wait), iar AI-ul mi-a explicat ca execvp inlocuieste complet imaginea procesului, facandu-ma sa realizez ca trebuia sa inchid capetele de pipe corect in procesul intermediar hub_mon pentru a evita blocarea citirii.