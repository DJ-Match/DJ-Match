/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#pragma once
#include "graph.dyn/dynamicweighteddigraph.h"
#include "path.h"
using namespace Algora;
using PathID = unsigned;

class path_set {
    public:
        path_set(DiGraph *graph, unsigned int max_vertex_id): diGraph(graph), no_of_paths(max_vertex_id) {
            vertex_to_path.resetAll(no_of_paths);
            paths.resetAll(no_of_paths);
            next.resetAll(no_of_paths);
            prev.resetAll(no_of_paths);
            next_edge.setDefaultValue(nullptr);
            next_edge.resetAll(no_of_paths);
            prev_edge.setDefaultValue(nullptr);
            prev_edge.resetAll(no_of_paths);

            diGraph->mapVertices([&] (Vertex * v) {
                vertex_to_path[v] = v;
                paths[v].init(v);
                next[v] = v;
                prev[v] = v;
            });
        }
        ~path_set() {};

        const path& get_path(const Vertex * v);

        PathID path_count() const;

        bool add_if_applicable(Arc * arc);

        Vertex * next_vertex(const Vertex * v);
        Vertex * prev_vertex(const Vertex * v);

        Arc * edge_to_next(const Vertex * v);
        Arc * edge_to_prev(const Vertex * v);

    private:
        DiGraph *diGraph;

        PathID no_of_paths;

        // each vertex v => vertex_to_path[v] is v which owns the path
        FastPropertyMap<Vertex *> vertex_to_path;

        // set of all paths
        FastPropertyMap<path> paths;

        // successor of v in its path
        FastPropertyMap<Vertex *> next;

        // predecessor of v in its path
        FastPropertyMap<Vertex *> prev;

        // edge which connects v to its successor
        FastPropertyMap<Arc *> next_edge;

        // edge which connects v to its predecessor
        FastPropertyMap<Arc *> prev_edge;

        inline bool is_endpoint(const Vertex * v) {
            return (next[v] == v || prev[v] == v);
        }
};

inline const path& path_set::get_path(const Vertex * v) {
    auto id = vertex_to_path[v];
    return paths[id];
}

inline PathID path_set::path_count() const {
    return no_of_paths;
}

inline Vertex * path_set::next_vertex(const Vertex * v) {
    return next[v];
}

inline Vertex * path_set::prev_vertex(const Vertex * v) {
    return prev[v];
}

inline Arc * path_set::edge_to_next(const Vertex * v) {
    return next_edge[v];
}

inline Arc * path_set::edge_to_prev(const Vertex * v) {
    return prev_edge[v];
}

inline bool path_set::add_if_applicable(Arc * arc) {
    const auto s = arc->getHead();
    const auto t = arc->getTail();

    path & source_path = paths[vertex_to_path[s]];
    path & target_path = paths[vertex_to_path[t]];

    if (!is_endpoint(s) || !is_endpoint(t)) {
        // both vertices have to be endpoints in order to be applicable
        return false;
    }

    if (source_path.is_cycle() || target_path.is_cycle()) {
        // if one path is a cycle => not applicable
        return false;
    }

    // edge endpoints belong to different paths => not creating a cycle
    // joining the two paths
    if (vertex_to_path[s] != vertex_to_path[t]) {
        source_path.set_length(source_path.get_length() + target_path.get_length() + 1);

        // update path data structure
        // new endpoints of the larger path need to be updated, there's 4 cases
        if (source_path.get_head() == s && target_path.get_head() == t) {
            vertex_to_path[target_path.get_tail()] = vertex_to_path[s];
            source_path.set_head(target_path.get_tail());

        } else if (source_path.get_head() == s && target_path.get_tail() == t) {
            vertex_to_path[target_path.get_head()] = vertex_to_path[s];
            source_path.set_head(target_path.get_head());

        } else if (source_path.get_tail() == s && target_path.get_head() == t) {
            vertex_to_path[target_path.get_tail()] = vertex_to_path[s];
            source_path.set_tail(target_path.get_tail());

        } else if (source_path.get_tail() == s && target_path.get_tail() == t) {
            vertex_to_path[target_path.get_head()] = vertex_to_path[s];
            source_path.set_tail(target_path.get_head());
        }

        // update doubly linked list
        if (next[s] == s) {
            next[s] = t;
            next_edge[s] = arc;
        } else {
            prev[s] = t;
            prev_edge[s] = arc;
        }

        if (next[t] == t) {
            next[t] = s;
            next_edge[t] = arc;
        } else {
            prev[t] = s;
            prev_edge[t] = arc;
        }

        target_path.set_active(false);
        no_of_paths--;

        return true;
    } else if (vertex_to_path[s] != vertex_to_path[t] && source_path.get_length() % 2 == 1) {
        // vertices belong to same path but we close an odd-length path
        source_path.set_length(source_path.get_length() + 1);

        if (next[source_path.get_head()] == source_path.get_head()) {
            next[source_path.get_head()] = source_path.get_tail();
            next_edge[source_path.get_head()] = arc;
        } else {
            prev[source_path.get_head()] = source_path.get_tail();
            prev_edge[source_path.get_head()] = arc;
        }

        if (next[source_path.get_tail()] == source_path.get_tail()) {
            next[source_path.get_tail()] = source_path.get_head();
            next_edge[source_path.get_tail()] = arc;
        } else {
            prev[source_path.get_tail()] = source_path.get_head();
            prev_edge[source_path.get_tail()] = arc;
        }

        source_path.set_tail(source_path.get_head());

        return true;
    }

    return false;
}
