#pragma once

namespace std {
template<class To, class From>
To bit_cast( From const & from )
{
    To to;
    std::memcpy( &to, &from, sizeof(To) );
    return to;
}
}
