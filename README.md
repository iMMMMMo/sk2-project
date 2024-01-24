# Projekt synchronizacji katalogów

## Cel projektu
Projekt zakłada stworzenie systemu umożliwiającego ciągłą synchronizację katalogów między wieloma klientami, wykorzystując architekturę klient-serwer. Implementacja opiera się na języku C oraz korzysta z API bsd-sockets w systemie operacyjnym GNU/Linux.

## Opis działania
Po uruchomieniu programu użytkownik może wybrać katalog z listy, który ma być synchronizowany. Klienci mają możliwość dodawania, modyfikowania i usuwania plików w wybranym katalogu. Wszelkie zmiany są natychmiast rejestrowane przez serwer i przekazywane do pozostałych klientów.

## Implementacja
- **Serwer:** Zaimplementowany w języku C przy użyciu API bsd-sockets. Posiada mechanizmy współbieżności, umożliwiające obsługę wielu klientów jednocześnie.
  
- **Klient:** Również napisany w języku C, co zapewnia pełną zgodność architektoniczną z serwerem oraz efektywną komunikację.

## Tabela komunikacji serwer-klient

| Klient wysyła    | Serwer odpowiada   | Opis                                                |
|------------------|--------------------|-----------------------------------------------------|
| Wybór katalogu   | Potwierdzenie       | Potwierdzenie wyboru katalogu przez klienta.         |
| Dodanie pliku    | Potwierdzenie       | Potwierdzenie dodania pliku przez klienta.           |
| Modyfikacja pliku | Potwierdzenie       | Potwierdzenie modyfikacji pliku przez klienta.      |
| Usunięcie pliku   | Potwierdzenie       | Potwierdzenie usunięcia pliku przez klienta.         |
| Błąd             | Komunikat o błędzie | Informacja o błędzie operacji.                       |

## Jak uruchomić aplikację

1. **Serwer:**
   - Skompiluj kod źródłowy serwera (`server.c`).
   - Uruchom plik wykonywalny serwera (np. `./server`).

2. **Klient:**
   - Skompiluj kod źródłowy klienta (`client.c`).
   - Uruchom plik wykonywalny klienta (np. `./client`).

## Podsumowanie
Projekt skupił się na efektywnej synchronizacji katalogów w środowisku klient-serwer, wykorzystując język C. Implementacja spełnia zarówno wymagania techniczne, jak i architektoniczne, stanowiąc solidną bazę do dalszego rozwoju. Otrzymane umiejętności w obszarze komunikacji sieciowej, współbieżności oraz integracji różnych elementów systemu przyczynią się do dalszego rozwoju umiejętności programistycznych.

