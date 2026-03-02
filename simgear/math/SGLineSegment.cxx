// SPDX-FileCopyrightText: 2026 Florent Rougon
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <ostream>

#include "SGLineSegment.hxx"

template<typename T>
std::ostream&
operator<<(std::ostream& s, const SGLineSegment<T>& lineSegment)
{
  return s << "line segment: start = " << lineSegment.getStart()
           << ", end = " << lineSegment.getEnd();
}

// Explicit template instantiations we need
template
std::ostream&
operator<< <float>(std::ostream& s, const SGLineSegment<float>& lineSegment);

template
std::ostream&
operator<< <double>(std::ostream& s, const SGLineSegment<double>& lineSegment);
