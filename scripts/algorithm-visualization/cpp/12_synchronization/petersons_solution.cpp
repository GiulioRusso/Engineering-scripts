// Peterson's Solution
//
// Mutua esclusione corretta per due processi, senza nessun supporto hardware:
// solo due variabili condivise e tre righe di protocollo.
//
//     flag[i] = true;                  // dichiaro che voglio entrare
//     turn = j;                        // cedo il turno all'altro
//     while (flag[j] && turn == j);    // aspetto solo se serve davvero
//     ... sezione critica ...
//     flag[i] = false;
//
// La riga che sembra assurda è la seconda: perché mai cedere il turno proprio
// quando si vuole entrare? È lì che sta tutto. Se entrambi dichiarano
// l'intenzione, entrambi cedono, ma turn è UNA variabile: la seconda scrittura
// cancella la prima, quindi turn finisce per indicare uno solo dei due. Chi ha
// scritto per ultimo perde, l'altro entra. Non c'è modo che la condizione
// d'attesa sia falsa per entrambi nello stesso momento.
//
// Lo stesso interleaving che rompe la lock variable, qui, regge.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/threads.hpp"

static Tracer T("petersons_solution", "Peterson's Solution", "Sincronizzazione", __FILE__);

using trace::ThreadProc;

static const std::vector<std::string> PROGRAM = {
    "flag[i] = true;                  // voglio entrare",
    "turn = j;                        // cedo il turno all'altro",
    "while (flag[j] && turn == j);    // attendo",
    "// ---- inizio sezione critica ----",
    "tmp = counter;",
    "counter = tmp + 1;",
    "flag[i] = false;                 // esco",
    "// terminato",
};

static std::vector<ThreadProc> procs;
static bool flagArr[2] = {false, false};
static int turn = 0;
static int counter = 0;
static int tmp[2] = {0, 0};
static int waits = 0;

// tracer-hide-begin
static trace::Shared sharedVars() {
    return {{"flag[0]", flagArr[0] ? "true" : "false"},
            {"flag[1]", flagArr[1] ? "true" : "false"},
            {"turn", std::to_string(turn)},
            {"counter", std::to_string(counter)}};
}
// tracer-hide-end

std::string stepProcess(int i) {
    ThreadProc& p = procs[i];
    const int j = 1 - i;

    switch (p.pc) {
        case 0:
            flagArr[i] = true;
            p.pc = 1;
            return p.name + " alza la propria flag: dichiara di voler entrare. Da sola questa riga non "
                            "basta, perché anche l'altro può alzarla.";
        case 1:
            turn = j;
            p.pc = 2;
            return p.name + " scrive turn = " + std::to_string(j) +
                   ", cioè cede il turno all'altro. Sembra controproducente, ma è la mossa che rompe "
                   "la simmetria: se scrivono entrambi, l'ultima scrittura vince e uno solo resta "
                   "\"indicato\" da turn.";
        case 2:
            if (flagArr[j] && turn == j) {
                waits++;
                return p.name + " aspetta: l'altro vuole entrare (flag[" + std::to_string(j) +
                       "] = true) E il turno è suo (turn = " + std::to_string(turn) +
                       "). Entrambe le condizioni sono vere, quindi cede il passo.";
            }
            p.pc = 3;
            return p.name + " passa: " +
                   (flagArr[j] ? "l'altro vuole entrare, ma turn = " + std::to_string(turn) +
                                 " indica " + procs[turn].name + ", quindi tocca a me."
                               : "l'altro non ha alzato la flag, la strada è libera.");
        case 3:
            p.critical = true;
            p.pc = 4;
            return p.name + " entra nella sezione critica. L'altro, se ci prova, resterà sulla riga "
                            "di attesa: le due condizioni non possono essere false per entrambi.";
        case 4:
            tmp[i] = counter;
            p.pc = 5;
            return p.name + " legge counter (" + std::to_string(counter) + ") in tmp.";
        case 5:
            counter = tmp[i] + 1;
            p.pc = 6;
            return p.name + " scrive counter = " + std::to_string(counter) +
                   ". Nessun aggiornamento perduto: era solo lui a lavorarci.";
        case 6:
            flagArr[i] = false;
            p.critical = false;
            p.pc = 7;
            return p.name + " abbassa la flag ed esce. Ora l'altro può smettere di aspettare.";
        default:
            p.done = true;
            return p.name + " ha terminato.";
    }
}

static void run(const char* label, const std::vector<int>& schedule, const char* intro) {
    procs = {{"P0", PROGRAM, 0, false, false, false}, {"P1", PROGRAM, 0, false, false, false}};
    flagArr[0] = flagArr[1] = false;
    turn = 0;
    counter = 0;
    tmp[0] = tmp[1] = 0;
    waits = 0;

    T.input(label);
    T.at(__LINE__, "main")
     .threads(trace::threadsJson(procs, -1, sharedVars(), {}, true))
     .counters({{"counter", counter}, {"attese", 0}})
     .note(intro)
     .emit();

    size_t k = 0;
    for (int guard = 0; guard < 80; guard++) {
        if (procs[0].done && procs[1].done) break;
        int who = schedule[k % schedule.size()];
        k++;
        if (procs[who].done) continue;

        std::string note = stepProcess(who);
        int inCritical = 0;
        for (const ThreadProc& p : procs) if (p.critical) inCritical++;

        T.at(__LINE__, "main")
         .vars({{"in esecuzione", procs[who].name}, {"turn", turn},
                {"flag[0]", flagArr[0]}, {"flag[1]", flagArr[1]}})
         .threads(trace::threadsJson(procs, who, sharedVars(), {}, false))
         .activeThread(procs[who].name)
         .counters({{"counter", counter}, {"attese", waits}})
         .note(inCritical > 1 ? note + "  ⚠ VIOLAZIONE — non dovrebbe poter accadere." : note)
         .emit();
    }

    T.at(__LINE__, "main")
     .vars({{"counter", counter}})
     .threads(trace::threadsJson(procs, -1, sharedVars(), {}, false))
     .counters({{"counter", counter}, {"attese", waits}})
     .note("Finito, counter = " + std::to_string(counter) +
           ". Mai due processi insieme nella sezione critica, con nessuno di questi interleaving — "
           "compreso quello che alla lock variable era fatale.")
     .emit();
}

int main() {
    T.view("threads").complexity("—", "O(1)");

    run("l'interleaving che rompeva la lock variable", {0, 1},
        "Alternanza stretta a ogni passo: è esattamente lo scenario che faceva entrare due processi "
        "insieme con la lock variable. Qui non succede.");
    run("entrambi dichiarano, poi si contendono il turno", {0, 0, 1, 1, 0, 1, 0, 1},
        "Entrambi alzano la flag prima che uno dei due arrivi al while: il caso peggiore per un "
        "protocollo di mutua esclusione.");
    run("P0 parte in vantaggio", {0, 0, 0, 0, 1, 1, 1, 0, 0, 1},
        "P0 arriva alla sezione critica prima che P1 alzi la flag: P1 poi aspetta.");

    T.dump("traces/petersons_solution.js");
    return 0;
}
