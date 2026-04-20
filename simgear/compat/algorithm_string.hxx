// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Drop-in replacements (C++20) for the Boost.Algorithm / Boost.Range headers
// used by simgear:
//
//   boost/algorithm/string/find_iterator.hpp  -> make_split_iterator, first_finder
//   boost/algorithm/string/predicate.hpp      -> equals, is_equal
//   boost/range.hpp                           -> iterator_range, make_iterator_range
//
// All symbols are injected into the boost:: namespace so call-sites only need
// to change their #include lines.

#pragma once

#include <algorithm>
#include <iterator>
#include <string_view>
#include <type_traits>

namespace boost {

// ── iterator_range ────────────────────────────────────────────────────────────

template<class Itr>
class iterator_range
{
public:
    using iterator        = Itr;
    using value_type      = typename std::iterator_traits<Itr>::value_type;
    using reference       = typename std::iterator_traits<Itr>::reference;
    using difference_type = typename std::iterator_traits<Itr>::difference_type;

    iterator_range() : _begin{}, _end{} {}
    iterator_range(Itr b, Itr e) : _begin(b), _end(e) {}

    Itr  begin() const { return _begin; }
    Itr  end()   const { return _end; }
    bool empty() const { return _begin == _end; }
    difference_type size() const { return std::distance(_begin, _end); }

    reference operator[](difference_type i) const { return _begin[i]; }

private:
    Itr _begin, _end;
};

template<class Itr>
iterator_range<Itr> make_iterator_range(Itr first, Itr last)
{
    return {first, last};
}

// ── equals ────────────────────────────────────────────────────────────────────

namespace detail {

template<class R>
auto range_begin(const R& r) { return r.begin(); }
template<class R>
auto range_end(const R& r)   { return r.end(); }

inline const char* range_begin(const char* s) { return s; }
inline const char* range_end(const char* s)   { return s + std::char_traits<char>::length(s); }

} // namespace detail

template<class Range1, class Range2>
bool equals(const Range1& r1, const Range2& r2)
{
    auto b1 = detail::range_begin(r1);
    auto e1 = detail::range_end(r1);
    auto b2 = detail::range_begin(r2);
    auto e2 = detail::range_end(r2);
    auto len1 = std::distance(b1, e1);
    auto len2 = std::distance(b2, e2);
    return len1 == len2 && std::equal(b1, e1, b2);
}

// ── is_equal ─────────────────────────────────────────────────────────────────

struct is_equal
{
    template<class T, class U>
    bool operator()(T a, U b) const { return a == b; }
};

// ── first_finder ─────────────────────────────────────────────────────────────

template<class NeedleRange, class Pred>
class first_finder_t
{
    NeedleRange _needle;
    Pred        _pred;

public:
    first_finder_t(NeedleRange needle, Pred pred)
        : _needle(std::move(needle)), _pred(std::move(pred))
    {}

    template<class SearchRange>
    iterator_range<typename SearchRange::iterator>
    operator()(const SearchRange& haystack) const
    {
        using Itr = typename SearchRange::iterator;
        auto nb = detail::range_begin(_needle);
        auto ne = detail::range_end(_needle);
        auto hb = haystack.begin();
        auto he = haystack.end();

        auto needle_len = std::distance(nb, ne);
        if (needle_len == 0)
            return {hb, hb};

        auto found = std::search(hb, he, nb, ne, _pred);
        if (found == he)
            return {he, he};  // not found

        auto found_end = found;
        std::advance(found_end, needle_len);
        return {found, found_end};
    }
};

inline first_finder_t<std::string_view, is_equal>
first_finder(const char* needle, is_equal pred = {})
{
    return {std::string_view(needle), pred};
}

template<class NeedleRange, class Pred>
first_finder_t<NeedleRange, Pred>
first_finder(const NeedleRange& needle, Pred pred)
{
    return {needle, pred};
}

// ── split_iterator ────────────────────────────────────────────────────────────

template<class Range, class Finder>
class split_iterator
{
public:
    using value_type = Range;

    split_iterator() : _eof(true), _no_more(true) {}

    split_iterator(const Range& range, Finder finder)
        : _rest(range), _finder(std::move(finder)), _eof(false), _no_more(false)
    {
        compute_current();
    }

    bool eof() const { return _eof; }

    value_type operator*() const { return _current; }

    // Proxy to support itr->member() on the current segment.
    struct proxy {
        value_type val;
        const value_type* operator->() const { return &val; }
    };
    proxy operator->() const { return {_current}; }

    split_iterator& operator++()
    {
        if (_eof) return *this;
        if (_no_more) {
            _eof = true;
        } else {
            _rest = _after_delim;
            compute_current();
        }
        return *this;
    }

private:
    Range   _rest;
    Range   _current;
    Range   _after_delim;
    Finder  _finder;
    bool    _eof;
    bool    _no_more;

    void compute_current()
    {
        auto match = _finder(_rest);
        // _finder returns {hay_e, hay_e} when delimiter is not found.
        if (match.begin() == _rest.end()) {
            _current      = _rest;
            _after_delim  = Range(_rest.end(), _rest.end());
            _no_more      = true;
        } else {
            _current      = Range(_rest.begin(), match.begin());
            _after_delim  = Range(match.end(),   _rest.end());
            _no_more      = false;
        }
    }
};

template<class Range, class Finder>
split_iterator<Range, Finder>
make_split_iterator(const Range& range, Finder finder)
{
    return {range, std::move(finder)};
}

} // namespace boost
