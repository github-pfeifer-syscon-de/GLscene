/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4;  coding: utf-8; -*-  */
/*
 * Copyright (C) 2025 RPf <gpl3@pfeifer-syscon.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <vector>
#include <map>
#include <memory>


// a implementation of a array with chunked storage
//   this allows storage as data is provided by a audio interface (in chunks)
//   and avoids creating a flat copy, for the price of computation time
template<typename T>
class ChunkedArray
{
public:
    ChunkedArray(uint32_t channels);
    ChunkedArray(const ChunkedArray& orig) = default;
    virtual ~ChunkedArray() = default;

    void add(const std::shared_ptr<std::vector<T>>& data);
    T operator[] (size_t i) const;
    bool empty() const;
    size_t size() const;
    uint32_t getChannels() const;
    double getInputScale() const; // use this to normalize input to -1..1
private:
    uint32_t m_channels;
    size_t m_size{};
    std::map<size_t, std::shared_ptr<std::vector<T>>> m_data;
};

//
//template<typename T>
//class ChunkedArrayIterator {
//    using iterator_category = std::forward_iterator_tag;
//    using difference_type   = int64_t;
//    using value_type        = T;
//    using pointer_type      = T*;
//    using reference_type    = T&;
//
//public:
//    ChunkedArrayIterator(std::iterator<T>& listIterator)
//    : m_listIterator{listIterator}
//    {
//    }
//    ChunkedArrayIterator(const LogViewIterator& other) = default;
//    virtual ~LogViewIterator() = default;
//    ChunkedArrayIterator operator++()
//    {
//        inc();
//        return *this;
//    }
//    LogViewIterator operator++(int)
//    {
//        LogViewIterator temp = *this;
//        inc();
//        return temp;
//    }
//    value_type operator*()
//    {
//        return m_iterInner->get();
//    }
//    pointer_type operator->()
//    {
//        if (!m_logEntry) {
//            m_logEntry = std::make_shared<LogViewEntry>(std::move(m_iterInner->get()));
//        }
//        return m_logEntry;
//    }
//    friend bool operator== (const LogViewIterator& a, const LogViewIterator& b)
//    {
//        return a.m_iterInner->equal(b.m_iterInner);
//    }
//    friend bool operator!= (const LogViewIterator& a, const LogViewIterator& b)
//    {
//        return !a.m_iterInner->equal(b.m_iterInner);
//    }
//protected:
//    void inc()
//    {
//
//    }
//private:
//    std::iterator<T> m_listIterator;
//        std::iterator<T> m_listIterator;
//};
