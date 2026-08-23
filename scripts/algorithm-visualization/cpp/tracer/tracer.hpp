// tracer.hpp - header-only, C++17, no dependencies.
//
// Instrumentation library for the algorithm visualizer. An algorithm stays a
// real C++ algorithm; the tracer records one snapshot per interesting step and
// writes them out as a JavaScript trace file that the browser player replays.
// The player never executes anything, which is what makes stepping *backwards*
// free: it is just an index decrement.
//
// The API is deliberately explicit and verbose. The instrumented source is
// itself teaching material, so a reader must be able to see exactly what each
// step records.
//
//   Tracer T("selection_sort", "Selection Sort", "Sorting", __FILE__);
//   T.view("array").watchArray("a", arr, n);
//
//   T.at(__LINE__, __func__)
//    .vars({{"i", i}, {"j", j}, {"minIndex", minIndex}})
//    .marks({{"i", i}, {"j", j}, {"minIndex", minIndex}})
//    .region("sorted", 0, i - 1)
//    .compare(j, minIndex)
//    .note("confronto a[j] con il minimo corrente")
//    .emit();
//
// __LINE__ guarantees the highlighted line is the real line of the source file
// even after the file is edited, so the code panel can never drift out of sync.
#pragma once

#include <cstdio>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace trace {

// ---------------------------------------------------------------- Val -------

// A JSON scalar. Everything the algorithms put on screen is an int, a double,
// a bool or a short string ("infinito", a node name, a case label).
struct Val {
    std::string j;

    Val(bool v) : j(v ? "true" : "false") {}
    Val(double v) {
        std::ostringstream o;
        o << v;
        j = o.str();
    }
    Val(const char* s) : j(quote(s)) {}
    Val(const std::string& s) : j(quote(s)) {}

    template <class T, class = std::enable_if_t<std::is_integral<T>::value>>
    Val(T v) : j(std::to_string(v)) {}

    static std::string quote(const std::string& s) {
        std::string out = "\"";
        for (char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\t': out += "\\t";  break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        char buf[8];
                        std::snprintf(buf, sizeof buf, "\\u%04x", c);
                        out += buf;
                    } else {
                        out += c;
                    }
            }
        }
        return out + "\"";
    }
};

using Kv = std::pair<std::string, Val>;
using KvList = std::initializer_list<Kv>;

inline std::string object(KvList items) {
    std::string s = "{";
    bool first = true;
    for (const auto& kv : items) {
        if (!first) s += ",";
        first = false;
        s += Val::quote(kv.first) + ":" + kv.second.j;
    }
    return s + "}";
}

inline std::string array(std::initializer_list<Val> items) {
    std::string s = "[";
    bool first = true;
    for (const auto& v : items) {
        if (!first) s += ",";
        first = false;
        s += v.j;
    }
    return s + "]";
}

// ------------------------------------------------------------ StepBuilder ---

class Tracer;

// Accumulates the fields of one step, then hands it to the Tracer on emit().
class StepBuilder {
  public:
    StepBuilder(Tracer* t, int line, const char* fn);

    // --- state -------------------------------------------------------------
    StepBuilder& vars(KvList v)     { return put("vars", object(v)); }
    StepBuilder& marks(KvList m)    { return put("marks", object(m)); }
    StepBuilder& counters(KvList c) { return put("counters", object(c)); }
    StepBuilder& note(const std::string& text) { return put("note", Val::quote(text)); }

    // Regions colour a contiguous index range of the primary array view.
    // kind: "sorted" | "unsorted" | "window" | "excluded" | "empty" | "active"
    StepBuilder& region(const std::string& kind, int from, int to) {
        if (from > to) return *this;  // empty range: nothing to draw
        regions_.push_back(object({{"kind", kind}, {"from", from}, {"to", to}}));
        return *this;
    }

    // A value held outside the structure (insertion sort's `key`, a detached
    // list node, the element removed from a heap).
    StepBuilder& lift(const std::string& label, Val value, int fromIndex) {
        return put("lifted", object({{"label", label}, {"value", value}, {"from", fromIndex}}));
    }

    StepBuilder& table(const std::string& name, KvList cells) {
        if (!tables_.empty()) tables_ += ",";
        tables_ += Val::quote(name) + ":" + object(cells);
        return *this;
    }
    StepBuilder& callStack(std::initializer_list<std::string> frames) {
        std::string s = "[";
        bool first = true;
        for (const auto& f : frames) {
            if (!first) s += ",";
            first = false;
            s += Val::quote(f);
        }
        return put("callStack", s + "]");
    }
    StepBuilder& activeThread(Val id)          { return put("activeThread", id.j); }
    StepBuilder& graphState(const std::string& rawJson) { return put("graph", rawJson); }
    StepBuilder& treeState(const std::string& rawJson)  { return put("tree", rawJson); }
    StepBuilder& gantt(const std::string& rawJson)      { return put("gantt", rawJson); }
    StepBuilder& threads(const std::string& rawJson)    { return put("threads", rawJson); }

    // --- actions -----------------------------------------------------------
    // Exactly one action per step: it is what the renderer animates.
    StepBuilder& compare(int a, int b)  { return act({{"kind", "compare"}, {"a", a}, {"b", b}}); }
    StepBuilder& swap(int a, int b)     { return act({{"kind", "swap"}, {"a", a}, {"b", b}}); }
    StepBuilder& assign(int idx, Val v) { return act({{"kind", "assign"}, {"index", idx}, {"value", v}}); }
    StepBuilder& shift(int from, int to, const std::string& dir) {
        return act({{"kind", "shift"}, {"from", from}, {"to", to}, {"dir", dir}});
    }
    StepBuilder& select(int idx)        { return act({{"kind", "select"}, {"index", idx}}); }
    StepBuilder& found(int idx)         { return act({{"kind", "found"}, {"index", idx}}); }
    StepBuilder& discard(int from, int to) {
        return act({{"kind", "discard"}, {"from", from}, {"to", to}});
    }
    StepBuilder& error(const std::string& what) { return act({{"kind", "error"}, {"what", what}}); }

    StepBuilder& push(Val v)            { return act({{"kind", "push"}, {"value", v}}); }
    StepBuilder& pop()                  { return act({{"kind", "pop"}}); }
    StepBuilder& enqueue(Val v)         { return act({{"kind", "enqueue"}, {"value", v}}); }
    StepBuilder& dequeue()              { return act({{"kind", "dequeue"}}); }

    StepBuilder& link(Val from, Val to)   { return act({{"kind", "link"}, {"from", from}, {"to", to}}); }
    StepBuilder& unlink(Val from, Val to) { return act({{"kind", "unlink"}, {"from", from}, {"to", to}}); }

    StepBuilder& visit(Val node)        { return act({{"kind", "visit"}, {"node", node}}); }
    StepBuilder& relax(Val u, Val v, Val newDist) {
        return act({{"kind", "relax"}, {"from", u}, {"to", v}, {"value", newDist}});
    }
    StepBuilder& accept(Val edge)       { return act({{"kind", "accept"}, {"edge", edge}}); }
    StepBuilder& reject(Val edge)       { return act({{"kind", "reject"}, {"edge", edge}}); }

    StepBuilder& block(Val proc)        { return act({{"kind", "block"}, {"proc", proc}}); }
    StepBuilder& wake(Val proc)         { return act({{"kind", "wake"}, {"proc", proc}}); }

    void emit();

  private:
    StepBuilder& put(const std::string& key, const std::string& rawJson) {
        fields_.push_back({key, rawJson});
        return *this;
    }
    StepBuilder& act(KvList a) { return put("action", object(a)); }
    bool hasField(const std::string& key) const {
        for (const auto& f : fields_) if (f.first == key) return true;
        return false;
    }

    Tracer* t_;
    std::vector<std::pair<std::string, std::string>> fields_;
    std::vector<std::string> regions_;
    std::string tables_;
    bool emitted_ = false;

    friend class Tracer;
};

// ----------------------------------------------------------------- Tracer ---

class Tracer {
  public:
    Tracer(std::string id, std::string name, std::string chapter, std::string srcFile)
        : id_(std::move(id)), name_(std::move(name)), chapter_(std::move(chapter)),
          src_(std::move(srcFile)) {}

    Tracer& complexity(const std::string& time, const std::string& space) {
        complexityTime_ = time;
        complexitySpace_ = space;
        return *this;
    }
    Tracer& view(const std::string& v) { view_ = v; return *this; }

    // An auxiliary panel rendered under the primary view, in reduced format.
    // `source` names the array/table the panel reads from a step.
    // `marks` (comma separated) restricts which pointers the panel draws: the
    // merge panels want only their own index, not every index of the step.
    Tracer& panel(const std::string& view, const std::string& title,
                  const std::string& source = "", const std::string& marks = "") {
        panels_.push_back(object({{"view", view}, {"title", title},
                                  {"source", source}, {"marks", marks}}));
        return *this;
    }

    // Watched containers are snapshotted automatically on every emit(), so the
    // instrumentation at the call site stays down to one chained expression.
    Tracer& watchArray(const std::string& name, const int* data, const int& count) {
        const int* countPtr = &count;  // by address: `count` is a reference parameter
        watches_.push_back({name, [data, countPtr]() {
            std::string s = "[";
            for (int i = 0; i < *countPtr; ++i) {
                if (i) s += ",";
                s += std::to_string(data[i]);
            }
            return s + "]";
        }});
        return *this;
    }
    Tracer& watchVector(const std::string& name, const std::vector<int>& v) {
        const std::vector<int>* vp = &v;
        watches_.push_back({name, [vp]() {
            std::string s = "[";
            for (size_t i = 0; i < vp->size(); ++i) {
                if (i) s += ",";
                s += std::to_string((*vp)[i]);
            }
            return s + "]";
        }});
        return *this;
    }
    // Free-form snapshot for anything the two helpers above do not cover.
    Tracer& watch(const std::string& name, std::function<std::string()> snapshot) {
        watches_.push_back({name, std::move(snapshot)});
        return *this;
    }
    Tracer& clearWatches() { watches_.clear(); return *this; }

    // The call stack is kept by the tracer, not by the algorithm: a recursive
    // function only has to announce its frame, and the bookkeeping lines stay
    // hidden from the source shown in the player.
    Tracer& pushFrame(const std::string& frame) { frames_.push_back(frame); return *this; }
    Tracer& popFrame() { if (!frames_.empty()) frames_.pop_back(); return *this; }
    Tracer& clearFrames() { frames_.clear(); return *this; }
    int depth() const { return static_cast<int>(frames_.size()); }

    // Starts a new predefined input. Every algorithm ships several, because the
    // player is static and cannot recompile.
    Tracer& input(const std::string& label) {
        flushInput();
        currentLabel_ = label;
        currentLayout_.clear();
        return *this;
    }
    // Hand-placed node coordinates for graph/tree views. Never a runtime
    // force-directed layout: it is unstable and unreadable.
    Tracer& layout(const std::string& rawJson) { currentLayout_ = rawJson; return *this; }

    StepBuilder at(int line, const char* fn = "") { return StepBuilder(this, line, fn); }

    int stepCount() const { return static_cast<int>(steps_.size()); }

    void dump(const std::string& path);

  private:
    void addStep(std::string json) { steps_.push_back(std::move(json)); }
    void flushInput();

    std::string id_, name_, chapter_, src_;
    std::string view_ = "array";
    std::string complexityTime_ = "?", complexitySpace_ = "?";
    std::vector<std::string> panels_;
    std::vector<std::pair<std::string, std::function<std::string()>>> watches_;

    std::vector<std::string> frames_;
    std::string currentLabel_ = "default";
    std::string currentLayout_;
    std::vector<std::string> steps_;
    std::vector<std::string> inputs_;

    friend class StepBuilder;
};

// ------------------------------------------------------- implementations ----

inline StepBuilder::StepBuilder(Tracer* t, int line, const char* fn) : t_(t) {
    put("line", std::to_string(line));
    if (fn && *fn) put("function", Val::quote(fn));
}

inline void StepBuilder::emit() {
    if (emitted_) return;
    emitted_ = true;

    std::string s = "{";
    bool first = true;
    auto append = [&](const std::string& key, const std::string& raw) {
        if (!first) s += ",";
        first = false;
        s += Val::quote(key) + ":" + raw;
    };
    for (const auto& f : fields_) append(f.first, f.second);

    if (!regions_.empty()) {
        std::string r = "[";
        for (size_t i = 0; i < regions_.size(); ++i) {
            if (i) r += ",";
            r += regions_[i];
        }
        append("regions", r + "]");
    }
    if (!tables_.empty()) append("tables", "{" + tables_ + "}");

    if (!t_->frames_.empty() && !hasField("callStack")) {
        std::string f = "[";
        for (size_t i = 0; i < t_->frames_.size(); ++i) {
            if (i) f += ",";
            f += Val::quote(t_->frames_[i]);
        }
        append("callStack", f + "]");
    }

    if (!t_->watches_.empty()) {
        std::string a = "{";
        bool f2 = true;
        for (const auto& w : t_->watches_) {
            if (!f2) a += ",";
            f2 = false;
            a += Val::quote(w.first) + ":" + w.second();
        }
        append("arrays", a + "}");
    }
    t_->addStep(s + "}");
}

inline void Tracer::flushInput() {
    if (steps_.empty()) return;
    std::string s = "{\"label\":" + Val::quote(currentLabel_);
    if (!currentLayout_.empty()) s += ",\"layout\":" + currentLayout_;
    s += ",\"steps\":[";
    for (size_t i = 0; i < steps_.size(); ++i) {
        if (i) s += ",\n      ";
        s += steps_[i];
    }
    s += "]}";
    inputs_.push_back(s);
    steps_.clear();
}

namespace detail {

inline std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r");
    return s.substr(a, b - a + 1);
}

inline bool startsWith(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

// True for lines that only exist to feed the tracer. They are stripped from the
// source shown in the player: the instrumentation noise would drown out the
// algorithm, which is the thing the student came to read.
inline bool isInstrumentation(const std::string& line) {
    std::string t = trim(line);
    if (startsWith(t, "static ")) t = t.substr(7);
    return startsWith(t, "T.") || startsWith(t, "Tracer ") ||
           startsWith(t, "#include \"../tracer/") ||
           startsWith(t, "#include \"../../tracer/") ||
           startsWith(t, "using trace::") || startsWith(t, "using namespace trace");
}

}  // namespace detail

inline void Tracer::dump(const std::string& path) {
    flushInput();

    std::ifstream in(src_);
    if (!in) {
        std::fprintf(stderr, "tracer: cannot open source '%s'\n", src_.c_str());
        return;
    }
    std::vector<std::string> lines;
    for (std::string l; std::getline(in, l);) lines.push_back(l);

    // displaySource keeps only the algorithm. lineMap goes from an absolute
    // 1-based source line to an index into displaySource; a hidden line maps to
    // the closest visible line above it, so a step never highlights nothing.
    std::vector<std::string> display;
    std::map<int, int> lineMap;
    bool inChain = false;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& raw = lines[i];
        bool hidden;
        if (inChain) {
            hidden = true;
            if (raw.find(".emit();") != std::string::npos) inChain = false;
        } else if (detail::isInstrumentation(raw)) {
            hidden = true;
            const std::string t = detail::trim(raw);
            if (detail::startsWith(t, "T.") && raw.find(".emit();") == std::string::npos &&
                raw.find(";") == std::string::npos) {
                inChain = true;
            }
        } else {
            hidden = false;
        }
        if (!hidden) display.push_back(raw);
        lineMap[static_cast<int>(i) + 1] = static_cast<int>(display.size()) - 1;
    }

    std::ofstream out(path);
    out << "// Generated by build.sh from " << src_ << " - do not edit by hand.\n";
    out << "window.TRACES = window.TRACES || {};\n";
    out << "window.TRACES[" << Val::quote(id_) << "] = {\n";
    out << "  id: " << Val::quote(id_) << ",\n";
    out << "  name: " << Val::quote(name_) << ",\n";
    out << "  chapter: " << Val::quote(chapter_) << ",\n";
    out << "  complexity: { time: " << Val::quote(complexityTime_)
        << ", space: " << Val::quote(complexitySpace_) << " },\n";
    out << "  view: " << Val::quote(view_) << ",\n";

    out << "  panels: [";
    for (size_t i = 0; i < panels_.size(); ++i) {
        if (i) out << ", ";
        out << panels_[i];
    }
    out << "],\n";

    out << "  displaySource: [\n";
    for (size_t i = 0; i < display.size(); ++i) {
        out << "    " << Val::quote(display[i]) << (i + 1 < display.size() ? ",\n" : "\n");
    }
    out << "  ],\n";

    out << "  lineMap: {";
    bool first = true;
    for (const auto& kv : lineMap) {
        if (!first) out << ",";
        first = false;
        out << "\"" << kv.first << "\":" << kv.second;
    }
    out << "},\n";

    out << "  inputs: [\n";
    for (size_t i = 0; i < inputs_.size(); ++i) {
        out << "    " << inputs_[i] << (i + 1 < inputs_.size() ? ",\n" : "\n");
    }
    out << "  ]\n};\n";
    std::fprintf(stderr, "  wrote %s (%zu input%s)\n", path.c_str(), inputs_.size(),
                 inputs_.size() == 1 ? "" : "s");
}

}  // namespace trace

using trace::Tracer;
