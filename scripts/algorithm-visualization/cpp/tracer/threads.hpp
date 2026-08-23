// threads.hpp - the process model for the synchronization chapter.
//
// Nessun thread vero. Ogni processo è una macchina a stati avanzabile di un
// passo, e uno scheduler esplicito decide di chi eseguire il prossimo passo.
// Così l'interleaving è deterministico, riproducibile e — soprattutto —
// SCEGLIIBILE: è l'unico modo di mostrare un bug di concorrenza in modo
// affidabile, invece di sperare che si presenti.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "tracer.hpp"

namespace trace {

struct ThreadProc {
    std::string name;
    std::vector<std::string> lines;
    int pc = 0;
    bool blocked = false;
    bool critical = false;
    bool done = false;
};

using Shared = std::vector<std::pair<std::string, std::string>>;

// withLines: solo al primo passo. Il codice dei processi non cambia mai, e
// ripeterlo a ogni passo gonfierebbe la traccia per niente.
// maxInCritical: quanti processi possono legittimamente stare dentro insieme.
// Per una mutua esclusione è 1, e superarlo è una violazione; per un semaforo
// contatore con 5 stampanti è 5, e cinque processi dentro sono la normalità.
inline std::string threadsJson(const std::vector<ThreadProc>& procs, int running,
                               const Shared& shared,
                               const std::vector<std::string>& blocked,
                               bool withLines, int maxInCritical = 1) {
    std::string s = "{\"procs\":[";
    for (size_t i = 0; i < procs.size(); ++i) {
        if (i) s += ",";
        const ThreadProc& p = procs[i];
        const char* state = p.done ? "done"
                          : p.blocked ? "blocked"
                          : ((int)i == running ? "running" : "ready");
        s += "{\"name\":" + Val::quote(p.name) +
             ",\"pc\":" + std::to_string(p.pc) +
             ",\"state\":" + Val::quote(state) +
             ",\"critical\":" + std::string(p.critical ? "true" : "false");
        if (withLines) {
            s += ",\"lines\":[";
            for (size_t k = 0; k < p.lines.size(); ++k) {
                if (k) s += ",";
                s += Val::quote(p.lines[k]);
            }
            s += "]";
        }
        s += "}";
    }
    s += "],\"shared\":{";
    for (size_t i = 0; i < shared.size(); ++i) {
        if (i) s += ",";
        s += Val::quote(shared[i].first) + ":" + Val::quote(shared[i].second);
    }
    s += "},\"blocked\":[";
    for (size_t i = 0; i < blocked.size(); ++i) {
        if (i) s += ",";
        s += Val::quote(blocked[i]);
    }
    s += "],\"maxInCritical\":" + std::to_string(maxInCritical);
    return s + "}";
}

}  // namespace trace
