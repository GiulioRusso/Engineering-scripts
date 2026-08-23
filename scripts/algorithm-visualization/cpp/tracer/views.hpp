// views.hpp - snapshot helpers for the views that are not a flat array.
//
// The algorithms keep their own natural data structures (a real Node*, a real
// tree, a real adjacency matrix); these helpers only turn the current state
// into the JSON the matching renderer expects. Nothing here changes how an
// algorithm is written.
#pragma once

#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "tracer.hpp"

namespace trace {

// A stable, short id for a node: the renderer uses it to keep the same SVG
// element across steps, which is what lets a node animate into the chain
// instead of blinking into place.
//
// Ids are handed out in order of first appearance rather than derived from the
// address. Traces are committed, and address-derived ids would rewrite every
// line of every list trace on each rebuild for no reason.
inline std::map<const void*, std::string>& nodeIdMap() {
    static std::map<const void*, std::string> m;
    return m;
}

// Call between inputs: freed addresses get recycled, and a stale mapping would
// silently give two different nodes the same id.
inline void resetNodeIds() { nodeIdMap().clear(); }

inline std::string nodeId(const void* p) {
    if (!p) return "";
    auto& m = nodeIdMap();
    auto it = m.find(p);
    if (it != m.end()) return it->second;
    std::string id = "n" + std::to_string(m.size() + 1);
    m[p] = id;
    return id;
}

// ------------------------------------------------------------ linked list --

// A node that is not (yet) part of the chain: a freshly allocated one waiting
// to be linked in, or the one just bypassed and not yet deleted.
template <class N>
struct Loose {
    N* node;
    const char* state;  // "new" | "orphan"
    int at = -1;        // chain position to hover above, -1 = after the tail
};

// Detects a `prev` member, so one walk serialises both singly and doubly linked
// lists instead of two near-identical ones.
template <class T, class = void>
struct HasPrev : std::false_type {};
template <class T>
struct HasPrev<T, std::void_t<decltype(std::declval<T&>().prev)>> : std::true_type {};

// kind: "singly" | "doubly" | "circular"
template <class N>
inline std::string listJson(N* head, const char* kind, N* cursor = nullptr,
                            std::initializer_list<Loose<N>> loose = {}) {
    std::string s = "{\"kind\":" + Val::quote(kind);
    s += ",\"head\":" + Val::quote(nodeId(head));
    s += ",\"cursor\":" + Val::quote(nodeId(cursor));
    s += ",\"nodes\":[";

    bool first = true;
    auto emitNode = [&](N* n, const char* state, int at) {
        if (!first) s += ",";
        first = false;
        s += "{\"id\":" + Val::quote(nodeId(n));
        s += ",\"data\":" + std::to_string(n->data);
        s += ",\"next\":" + Val::quote(nodeId(n->next));
        if constexpr (HasPrev<N>::value) s += ",\"prev\":" + Val::quote(nodeId(n->prev));
        s += ",\"state\":" + Val::quote(state);
        if (at >= 0) s += ",\"at\":" + std::to_string(at);
        s += "}";
    };

    // Walk the chain with a hard cap: a circular list comes back to head, and a
    // half-relinked list can be temporarily malformed, which is exactly the
    // moment we most want to draw rather than hang on.
    int guard = 0;
    for (N* p = head; p && guard < 64; p = p->next, ++guard) {
        emitNode(p, "chain", -1);
        if (p->next == head) break;
    }
    for (const auto& l : loose) if (l.node) emitNode(l.node, l.state, l.at);

    return s + "]}";
}

// ------------------------------------------------------------------ tree --

// Per-node decoration. The algorithm says what each node currently *is*; the
// renderer only knows how to colour it.
//   "current" | "path" | "visited" | "found" | "pruned" | "moved" | "doomed"
struct TreeMarks {
    std::map<const void*, std::string> state;
    std::map<const void*, std::string> label;

    TreeMarks& set(const void* p, const char* s) { if (p) state[p] = s; return *this; }
    TreeMarks& tag(const void* p, const std::string& l) { if (p) label[p] = l; return *this; }

    // Whole binary subtree at once: marking the half a search just discarded is
    // what turns "O(h)" from a claim into something visible.
    template <class N>
    TreeMarks& subtree(N* n, const char* s) {
        if (!n) return *this;
        state[n] = s;
        subtree(n->left, s);
        subtree(n->right, s);
        return *this;
    }

    void emit(const void* p, std::string& out) const {
        auto it = state.find(p);
        if (it != state.end()) out += ",\"state\":" + Val::quote(it->second);
        auto il = label.find(p);
        if (il != label.end()) out += ",\"label\":" + Val::quote(il->second);
    }
};

template <class N>
inline void emitBinaryNode(N* n, const TreeMarks& m, std::string& out, bool& first) {
    if (!n) return;
    if (!first) out += ",";
    first = false;
    out += "{\"id\":" + Val::quote(nodeId(n));
    out += ",\"value\":" + std::to_string(n->data);
    // A leaf gets no children at all; a half-full node keeps an empty slot on
    // the missing side so left and right stay visually distinguishable.
    if (!n->left && !n->right) {
        out += ",\"children\":[]";
    } else {
        out += ",\"children\":[" + Val::quote(nodeId(n->left)) + "," + Val::quote(nodeId(n->right)) + "]";
    }
    m.emit(n, out);
    out += "}";
    emitBinaryNode(n->left, m, out, first);
    emitBinaryNode(n->right, m, out, first);
}

template <class N>
inline std::string binaryTreeJson(N* root, const TreeMarks& m = TreeMarks()) {
    std::string out = "{\"root\":" + Val::quote(nodeId(root)) + ",\"nodes\":[";
    bool first = true;
    emitBinaryNode(root, m, out, first);
    return out + "]}";
}

template <class N>
inline void emitNaryNode(N* n, const TreeMarks& m, std::string& out, bool& first) {
    if (!n) return;
    if (!first) out += ",";
    first = false;
    out += "{\"id\":" + Val::quote(nodeId(n));
    out += ",\"value\":" + std::to_string(n->data);
    out += ",\"children\":[";
    for (size_t i = 0; i < n->children.size(); ++i) {
        if (i) out += ",";
        out += Val::quote(nodeId(n->children[i]));
    }
    out += "]";
    m.emit(n, out);
    out += "}";
    for (auto* c : n->children) emitNaryNode(c, m, out, first);
}

template <class N>
inline std::string naryTreeJson(N* root, const TreeMarks& m = TreeMarks()) {
    std::string out = "{\"root\":" + Val::quote(nodeId(root)) + ",\"nodes\":[";
    bool first = true;
    emitNaryNode(root, m, out, first);
    return out + "]}";
}

}  // namespace trace
