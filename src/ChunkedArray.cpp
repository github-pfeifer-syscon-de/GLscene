/* -*- Mode: c++; c-basic-offset: 4; tab-width: 4; coding: utf-8; -*-  */
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


#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <stdexcept>


#include "ChunkedArray.hpp"


template<typename T>
ChunkedArray<T>::ChunkedArray(uint32_t channels)
: m_channels{channels}
{
}

template<typename T>
void
ChunkedArray<T>::add(const std::shared_ptr<std::vector<T>>& data)
{
    size_t start{};
    if (!m_data.empty()) {
        auto lastEnty = m_data.rbegin();
        start = lastEnty->first;
        start += data->size();      // we need only include -1 only once
    }
    else {
        start += data->size() - 1;  // -1 as we want for size to go to next
    }
    m_data.insert(std::make_pair(start, data));
    m_size += data->size();
}

template<typename T>
T
ChunkedArray<T>::operator[] (size_t i) const
{
    // the map reduces the runtime from 13s to 0.1s
    auto low = m_data.lower_bound(i);
    if (low != m_data.end()) {
        size_t diff{};
        if (low != m_data.begin()) {
            auto prev = low;
            --prev;
            diff = prev->first;
            i -= diff + 1;
        }
        //std::cout << "i " << i
        //          << " found low " << low->first
        //          << " diff " << diff << std::endl;
        return low->second->operator[] (i);
    }
    //high = map.upper_bound(pos);

//    size_t idx{i};
//    //std::cout << "operator " << i << " of " << m_size << std::endl;
//    for (auto chunk = m_data.begin(); chunk != m_data.end(); ++chunk) {
//        auto act = *chunk;
//        if (idx < act->size()) {
//            return act->operator[] (idx);
//        }
//        idx -= act->size();
//    }
    //std::cout << "operator " << i << " beyond end " << m_size << std::endl;
    //return 0;
    throw std::runtime_error("reached end of chunked array!");
}

template<typename T>
bool
ChunkedArray<T>::empty() const
{
    return m_data.empty();
}

template<typename T>
size_t
ChunkedArray<T>::size() const
{
    return m_size;
}

template<typename T>
uint32_t
ChunkedArray<T>::getChannels() const
{
    return m_channels;
}

template<typename T>
double
ChunkedArray<T>::getInputScale() const
{
    // in practice using normalized input leaves only a very small signal
    return 1.0 / static_cast<double>(std::numeric_limits<T>::max() / 64);
}

// instantiate with the most likely type
template class ChunkedArray<int16_t>;
