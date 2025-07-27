Literaturrecherche:

Die Fused Multiply-Add Operation (FMA) ist zentral für moderne Prozessorarchitekturen, besonders bei Gleitkommaberechnungen. Sie kombiniert Multiplikation und Addition in einem Schritt ohne Zwischenrundung und verbessert dadurch Genauigkeit und Performance.
Im Bereich des maschinellen Lernens ist FMA besonders wichtig, z. B. bei Matrixmultiplikationen und gewichteten Summen. FMA reduziert Rundungsfehler und beschleunigt Berechnungen. NVIDIA integrierte FMA erstmals in den Tensor Cores der Volta-Architektur (Tesla V100), was gegenüber der Pascal-Architektur (Tesla P100) einen deutlichen Leistungssprung brachte.
FMA findet außerdem Anwendung in physikalischen Simulationen, Rendering und wissenschaftlicher Datenverarbeitung, wo numerische Stabilität wichtig ist.
Unser System orientiert sich an IEEE-754 mit konfigurierbarer Exponenten- und Mantissagröße. Diese Flexibilität erlaubt optimierte Nutzung der Bitbreite, z. B. bei eingebetteten Systemen.

Zur Berechnung der Maximalzahl wird das Vorzeichenbit auf 0 gesetzt, alle Bits des Exponenten auf 1 (außer Sonderfall Inf/NaN), und alle Bits der Mantisse auf 1. Daraus ergibt sich:
Max = (2 − 2^(-m)) · 2^(2^(e−1)−1)
mit e als Exponentenbits und m als Mantissabits. Die Mantisse entspricht 2 − 2^(-m) wegen der impliziten führenden 1 im IEEE-754-Format.
Die Minimalzahl ergibt sich durch Setzen des Vorzeichenbits auf 1:
Min = −Max
Beispiel: Bei ExpSize = 8 und ManSize = 23 gilt:
Max ≈ 3.4028235 · 10^38,
Min ≈ −3.4028235 · 10^38

Verteilung von Aufgaben :
Saifeddine Guenanna:

Load_csv_requests:
Diese Funktion erhält den Pfad zur Eingabedatei sowie Parameter für Exponentgröße, Mantissagröße, Rundungsmodus, die maximale Anzahl an Simulationszyklen und einen Zeiger auf den Ausgabewert für die Anzahl der geladenen Requests.
Sie liest eine CSV-Datei zeilenweise ein, parst jede Zeile und wandelt die Operanden abhängig von der gegebenen Mantissa- und Exponentgröße ins entsprechende IEEE-754-Format um.
Die Anzahl der Requests darf die maximale Anzahl an Zyklen nicht überschreiten.
Besonders hervorzuheben ist die effiziente Speicherverwaltung sowie die präzise Umwandlung von Fließkommazahlen (auch bei nicht-standardisierten Mantissa-/Exponentgrößen). Die Funktion verwendet Helper-Methoden aus MainHelpers.

Max Modul:
Das Max-Modul ist ein SystemC-Modul zur Bestimmung des Maximums zweier Fließkommazahlen im konfigurierbaren IEEE-754-Format.
Bei zwei NaN- oder INF-Werten wird der größere zurückgegeben. Bei INF und einer normalen Zahl wird INF zurückgegeben. Bei NaN und einer gültigen Zahl wird die Zahl gewählt.
Der Vergleichsalgorithmus: Zuerst werden die Vorzeichenbits ausgewertet. Bei gleichem Vorzeichen erfolgt ein direkter Bit-Vergleich.

Min Modul:
Das Min-Modul ist ein SystemC-Modul zur Bestimmung des Minimums zweier Fließkommazahlen im konfigurierbaren IEEE-754-Format.
Bei zwei NaN- oder INF-Werten wird der kleinere zurückgegeben. Bei INF und einer normalen Zahl wird die normale Zahl zurückgegeben. Bei NaN und einer gültigen Zahl wird die gültige Zahl gewählt.
Der Vergleichsalgorithmus: Zuerst Vorzeichenvergleich, bei gleichem Vorzeichen direkter Bit-Vergleich.

NourAllah Gargouri

AddSub:
Subtraktion wird durch Umformung in eine Addition gelöst: A−B=A+(−B).
Die Schritte sind:
Mantisse der kleineren Zahl shiften, bis Exponenten gleich sind, G/S-Bits setzen.
Je nach Vorzeichen werden Mantissen addiert oder subtrahiert.
Normalisierung und ggf. Rundung.

FMA:
Multiplikation von 1.m2 und 1.m3, Addition der Exponenten.
1.m1 in eine 64-Bit-kompatible Form bringen.
Mantisse shiften, G/S-Bits setzen.
Vorzeichen bestimmen.
Mantissen addieren oder subtrahieren.
G/S-Bits propagieren.
Finales Vorzeichen, Exponent und Ergebnis in 32 Bit zusammensetzen.
Runden.

Nutzer-Input-Testing:
Basierend auf dem CLI-Code von Ahmed Amin und Saif wurde pattern-basiertes Testing umgesetzt:
Maximal 13 Argumente
Nur Long Options mit getopt
Validierung aller Optionen
Prüfung auf gültige .csv-Datei



Ahmed Amine Loukil:



-getopt_long:
Wird verwendet, um Kommandozeilenargumente bequem auszuwerten. Unterstützt Langoptionen
(--A),die sind :[--help,--tf,--size-mantissa,--size-exponent,--cycles,--round-mode]

-Tracefiles:

Dienen zur Erzeugung von VCD-Dateien, mit denen das zeitliche Verhalten der Signale in GTKWave analysiert werden kann. So lassen sich Werteänderungen in Modulen nachvollziehen und das Verhalten des Designs systematisch überprüfen.

-FPU:
Jedes SystemC-Submodul (Min, Max, AddSub, Mul, FMA) besitzt eine eigene Clock-Leitung (z. B. clk_min, clk_max). Anstatt alle Module gleichzeitig aktiv zu halten und das Ergebnis später anhand des 4-Bit-Opcode auszuwählen, wird nur der Takt des benötigten Moduls aktiviert. Die übrigen Module bleiben inaktiv – das spart Energie und reduziert den Ressourcenverbrauch.

-Mul: 
Es gibt 5 Hauptschritte um die Multiplikation von 2 floating Point Zahlen zu bestimmen:
1- Das Exponent des Ergebnisses temporär auf e1+e2 setzen
2- Multiplikation der Mantissen nach dem Hinzufügen der Leading 1
3- Das Exponent anpassen anhand dessen + Rounding falls nötig
4- Das Sign bestimmen (einfach sign(a) XOR sign(b)) 
5- Alles zusammensetzen und das Ergebnis finden



