// Lock variable
//
// Il tentativo più ingenuo di mutua esclusione: una variabile condivisa lock.
// Chi vuole entrare aspetta che valga 0, poi la mette a 1.
//
//     while (lock != 0);   // attendi
//     lock = 1;            // prendi il lock
//     ... sezione critica ...
//     lock = 0;            // rilascia
//
// E non funziona. Fra il test e l'assegnamento c'è un istante in cui il
// processo può essere sospeso, e in quell'istante un altro processo può fare lo
// stesso test e trovare ancora 0. Entrano in due.
//
// Il difetto non è nella logica ma nell'ATOMICITÀ: "leggi e poi scrivi" sono
// due operazioni, e fra le due può succedere di tutto. Serve un'istruzione
// hardware che le faccia insieme (test-and-set), oppure un protocollo software
// come quello di Peterson.
#include <cstdio>
#include <string>
#include <vector>
#include "../tracer/tracer.hpp"
#include "../tracer/threads.hpp"

static Tracer T("lock_variable", "Lock variable", "Sincronizzazione", __FILE__);

using trace::ThreadProc;

// Il "programma" di ogni processo. Una riga = un passo indivisibile: è proprio
// la granularità a essere il contenuto della lezione.
static const std::vector<std::string> PROGRAM = {
    "while (lock != 0);            // attesa attiva",
    "lock = 1;                     // prendo il lock",
    "// ---- inizio sezione critica ----",
    "tmp = counter;",
    "counter = tmp + 1;",
    "lock = 0;                     // rilascio il lock",
    "// terminato",
};

static std::vector<ThreadProc> procs;
static int lockVar = 0;
static int counter = 0;
static int tmp[2] = {0, 0};
static int busyWaits = 0;

// tracer-hide-begin
static trace::Shared sharedVars() {
    return {{"lock", std::to_string(lockVar)},
            {"counter", std::to_string(counter)},
            {"tmp[P0]", std::to_string(tmp[0])},
            {"tmp[P1]", std::to_string(tmp[1])}};
}
static bool firstStep = true;
// tracer-hide-end

// Esegue UN passo del processo i e ritorna la nota che descrive che cosa è
// appena successo. Nessuna riga fa più di una cosa.
std::string stepProcess(int i) {
    ThreadProc& p = procs[i];

    switch (p.pc) {
        case 0:
            if (lockVar != 0) {
                busyWaits++;
                return p.name + " testa lock: vale " + std::to_string(lockVar) +
                       ", quindi resta fermo su questa riga e riprova. Non dorme: consuma un turno di "
                       "CPU per non fare niente. È l'ATTESA ATTIVA, e su un solo core è puro spreco.";
            }
            p.pc = 1;
            return p.name + " testa lock e lo trova a 0: la strada sembra libera, passa alla riga "
                            "successiva. ATTENZIONE: fra questo test e l'assegnamento c'è uno stacco, "
                            "e il processo può essere sospeso proprio qui.";
        case 1:
            lockVar = 1;
            p.pc = 2;
            return p.name + " scrive lock = 1. Se nel frattempo qualcun altro aveva già superato il "
                            "test, questa scrittura arriva troppo tardi per fermarlo.";
        case 2:
            p.critical = true;
            p.pc = 3;
            return p.name + " entra nella sezione critica.";
        case 3:
            tmp[i] = counter;
            p.pc = 4;
            return p.name + " legge counter (" + std::to_string(counter) + ") nella sua variabile "
                            "locale tmp.";
        case 4:
            counter = tmp[i] + 1;
            p.pc = 5;
            return p.name + " scrive counter = tmp + 1 = " + std::to_string(counter) + ".";
        case 5:
            lockVar = 0;
            p.critical = false;
            p.pc = 6;
            return p.name + " rilascia il lock e esce dalla sezione critica.";
        default:
            p.done = true;
            return p.name + " ha terminato.";
    }
}

static void run(const char* label, const std::vector<int>& schedule, const char* intro) {
    procs = {{"P0", PROGRAM, 0, false, false, false}, {"P1", PROGRAM, 0, false, false, false}};
    lockVar = 0;
    counter = 0;
    tmp[0] = tmp[1] = 0;
    busyWaits = 0;
    firstStep = true;

    T.input(label);
    T.at(__LINE__, "main")
     .threads(trace::threadsJson(procs, -1, sharedVars(), {}, true))
     .counters({{"counter", counter}, {"attese attive", 0}})
     .note(intro)
     .emit();
    firstStep = false;

    // Lo scheduler è esplicito: questa lista dice chi tocca a ogni passo. Non
    // c'è nessuna casualità, e l'interleaving si può scegliere.
    size_t k = 0;
    for (int guard = 0; guard < 60; guard++) {
        if (procs[0].done && procs[1].done) break;
        int who = schedule[k % schedule.size()];
        k++;
        if (procs[who].done) continue;

        std::string note = stepProcess(who);
        int violating = 0;
        for (const ThreadProc& p : procs) if (p.critical) violating++;

        T.at(__LINE__, "main")
         .vars({{"in esecuzione", procs[who].name}, {"lock", lockVar}, {"counter", counter}})
         .threads(trace::threadsJson(procs, who, sharedVars(), {}, false))
         .activeThread(procs[who].name)
         .counters({{"counter", counter}, {"attese attive", busyWaits}})
         .note(violating > 1
                 ? note + "  ⚠ E ORA SONO IN DUE NELLA SEZIONE CRITICA: entrambe le righe sono "
                          "evidenziate. La mutua esclusione è saltata."
                 : note)
         .emit();
    }

    T.at(__LINE__, "main")
     .vars({{"counter", counter}, {"atteso", 2}})
     .threads(trace::threadsJson(procs, -1, sharedVars(), {}, false))
     .counters({{"counter", counter}, {"attese attive", busyWaits}})
     .note(counter == 2
             ? "Entrambi hanno finito e counter vale 2, come deve. Con questo interleaving il lock ha "
               "retto — ma \"ha retto stavolta\" non è una garanzia."
             : "Entrambi hanno finito e counter vale " + std::to_string(counter) +
               " invece di 2. Gli incrementi erano due, ma uno è andato perso: avevano letto lo stesso "
               "valore di partenza. È l'AGGIORNAMENTO PERDUTO, la conseguenza pratica della "
               "violazione.")
     .emit();
}

int main() {
    T.view("threads").complexity("—", "O(1)");

    // Alternanza stretta fin dal primo passo: entrambi superano il test prima
    // che uno dei due arrivi a scrivere lock = 1.
    run("interleaving CHE ROMPE la mutua esclusione", {0, 1},
        "Due processi, lo stesso codice, un solo lock. Lo scheduler alterna a ogni passo. "
        "Guarda i primi due passi: sono quelli che rompono tutto.");

    // P0 fa in tempo a prendere il lock prima che P1 lo testi.
    run("interleaving che funziona (e mostra l'attesa attiva)", {0, 0, 1, 1, 1, 0, 0, 0, 0, 1},
        "Stesso codice, altro interleaving: P0 riesce a scrivere lock = 1 prima che P1 arrivi al test. "
        "P1 gira a vuoto sulla prima riga finché il lock non si libera.");

    T.dump("traces/lock_variable.js");
    return 0;
}
