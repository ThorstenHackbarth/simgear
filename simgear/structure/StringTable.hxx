// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef SIMGEAR_STRINGTABLE_HXX
#define SIMGEAR_STRINGTABLE_HXX 1

#include <string>
#include <mutex>

#ifdef SG_NO_BOOST
#  include <unordered_set>
#else
#  include <boost/multi_index_container.hpp>
#  include <boost/multi_index/hashed_index.hpp>
#  include <boost/multi_index/identity.hpp>
#endif

namespace simgear
{
#ifdef SG_NO_BOOST
typedef std::unordered_set<std::string> StringContainer;
#else
typedef boost::multi_index_container<
    std::string,
    boost::multi_index::indexed_by<
        boost::multi_index::hashed_unique<
            boost::multi_index::identity<std::string> > > >
StringContainer;
#endif

class StringTable
{
    const std::string* insert(const std::string& str);
private:
    std::mutex _mutex;
    StringContainer _strings;
};
}
#endif
