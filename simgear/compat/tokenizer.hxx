// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 SimGear contributors
//
// Drop-in replacement for boost/tokenizer.hpp (C++20).
// Provides boost::char_separator<Char> and boost::tokenizer<TokenFunc,Iterator>
// in the boost:: namespace so call sites need no changes.
//
// Implements the subset of the Boost Tokenizer API used in simgear:
//   - char_separator with dropped-delimiter set (no kept delimiters, no empty tokens)
//   - tokenizer(begin, end, sep) / const_iterator / *it, it->, it.current_token()

#pragma once

#include <string>
#include <iterator>

namespace boost {

// ── char_separator ────────────────────────────────────────────────────────────

template<class Char = char>
class char_separator
{
public:
    using string_type = std::basic_string<Char>;

    // dropped_delims: characters that split tokens (not emitted)
    // kept_delims:    characters that split AND are emitted as their own token
    // empty_tokens:   whether consecutive delimiters yield empty tokens
    explicit char_separator(const Char* dropped_delims,
                            const Char* kept_delims  = "",
                            bool        empty_tokens  = false)
        : dropped_(dropped_delims)
        , kept_(kept_delims)
        , empty_tokens_(empty_tokens)
    {}

    bool is_dropped(Char c) const { return dropped_.find(c) != string_type::npos; }
    bool is_kept   (Char c) const { return kept_.find(c)    != string_type::npos; }
    bool keep_empty_tokens() const { return empty_tokens_; }

private:
    string_type dropped_;
    string_type kept_;
    bool        empty_tokens_;
};

// ── tokenizer ─────────────────────────────────────────────────────────────────

template< class TokenFunc = char_separator<char>,
          class Iterator  = typename std::string::const_iterator >
class tokenizer
{
public:
    // ── const_iterator ────────────────────────────────────────────────────────
    class const_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = std::string;
        using difference_type   = std::ptrdiff_t;
        using reference         = const std::string&;
        using pointer           = const std::string*;

        // End sentinel
        const_iterator() : end_(), pos_(), at_end_(true), sep_(nullptr) {}

        reference operator* () const { return token_; }
        pointer   operator->() const { return &token_; }

        // Boost-compatible: returns current token by const ref
        reference current_token() const { return token_; }

        const_iterator& operator++()
        {
            advance();
            return *this;
        }
        const_iterator operator++(int)
        {
            auto tmp = *this;
            advance();
            return tmp;
        }

        bool operator==(const const_iterator& o) const
        {
            if (at_end_ && o.at_end_) return true;
            if (at_end_ != o.at_end_) return false;
            return pos_ == o.pos_;
        }
        bool operator!=(const const_iterator& o) const { return !(*this == o); }

    private:
        friend class tokenizer;

        const_iterator(Iterator pos, Iterator end, const TokenFunc& sep)
            : end_(end), pos_(pos), at_end_(false), sep_(&sep)
        {
            advance_to_first();
        }

        // Move pos_ past dropped chars and load the first token.
        void advance_to_first() { load_next(); }

        void advance()
        {
            if (at_end_) return;
            load_next();
        }

        void load_next()
        {
            // Skip dropped delimiters (unless we keep empty tokens — not used
            // in simgear, but respected for correctness)
            if (!sep_->keep_empty_tokens()) {
                while (pos_ != end_ && sep_->is_dropped(*pos_))
                    ++pos_;
            }

            if (pos_ == end_) { at_end_ = true; return; }

            // Kept delimiter: emit it as a single-character token
            if (sep_->is_kept(*pos_)) {
                token_.assign(1, *pos_);
                ++pos_;
                return;
            }

            // Normal token: collect until next dropped or kept delimiter
            auto start = pos_;
            while (pos_ != end_
                   && !sep_->is_dropped(*pos_)
                   && !sep_->is_kept(*pos_))
            {
                ++pos_;
            }

            if (pos_ == start) {
                // Only possible when keep_empty_tokens is true and we are
                // sitting on a dropped delimiter — emit empty and advance.
                ++pos_;
                token_.clear();
            } else {
                token_.assign(start, pos_);
                // If keep_empty_tokens: do NOT skip the delimiter; the next
                // call will enter the empty-token path. For the dropped case
                // without keep_empty we already skipped above.
            }
        }

        Iterator         end_;
        Iterator         pos_;
        bool             at_end_;
        const TokenFunc* sep_;
        std::string      token_;
    };

    // ── tokenizer constructors ────────────────────────────────────────────────

    tokenizer(Iterator begin, Iterator end, const TokenFunc& sep = TokenFunc{})
        : begin_(begin), end_(end), sep_(sep)
    {}

    // Convenience: construct from a std::string
    template<class String>
        requires std::is_same_v<String, std::string>
    explicit tokenizer(const String& s, const TokenFunc& sep = TokenFunc{})
        : begin_(s.begin()), end_(s.end()), sep_(sep)
    {}

    const_iterator begin() const { return const_iterator(begin_, end_, sep_); }
    const_iterator end()   const { return const_iterator(); }

private:
    Iterator  begin_;
    Iterator  end_;
    TokenFunc sep_;
};

} // namespace boost
