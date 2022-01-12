/**
 * Copyright (C) 2021, 2022 : Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#pragma once
#include "graph/vertex.h"

using namespace Algora;
class path {
    public:
        path(): length(0), active(false) {}
        path(Vertex * v): head(v), tail(v), length(0), active(true) {}
        ~path() {};

        void init(Vertex * v);

        Vertex * get_tail() const;
        void set_tail(Vertex * v);

        Vertex * get_head() const;
        void set_head(Vertex * v);

        void set_length(const unsigned & length);
        unsigned get_length() const;

        bool is_endpoint(const Vertex * v) const;

        bool is_cycle() const;

        bool is_active() const;
        void set_active(const bool active);

    private:
        Vertex * head;

        Vertex * tail;

        unsigned length;

        bool active;
};

inline void path::init(Vertex * v) {
    head = v;
    tail = v;
    length = 0;
    active = true;
}

inline Vertex * path::get_tail() const {
    return tail;
}

inline void path::set_tail(Vertex * v) {
    tail = v;
}

inline Vertex * path::get_head() const {
    return head;
}

inline void path::set_head(Vertex * v) {
    head = v;
}

inline unsigned path::get_length() const {
    return length;
}

inline void path::set_length(const unsigned & len) {
    length = len;
}

inline bool path::is_endpoint(const Vertex * v) const {
    return (v == tail) || (v == head);
}

inline bool path::is_cycle() const {
    return (tail == head) && (length > 0);
}

inline bool path::is_active() const {
    return active;
}

inline void path::set_active(const bool act) {
    active = act;
}
