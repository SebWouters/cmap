/*
    cmap is a collapsible hierarchically ordered coordinate map

    Copyright (c) 2020, Sebastian Wouters
    All rights reserved.

    This file is part of cmap, licensed under the BSD 3-Clause License.
    A copy of the License can be found in the file LICENSE in the root
    folder of this project.
*/

#include <iostream>
#include <fstream>
#include <sstream>

void permute_printer(std::ofstream& output, const std::string& Type, const uint32_t NBITS, const uint32_t DIM)
{
    /*
        perm[ip][bp] = coord[ic][bc];
        ic + _DIM * bc = ip * _NBITS + bp
        ip = (ic + _DIM * bc) / _NBITS;
        bp = (ic + _DIM * bc) % _NBITS;
        ic = (bp + _NBITS * ip) % _DIM;
        bc = (bp + _NBITS * ip) / _DIM;
    */

    output << "template<> inline constexpr void permute<" << Type << ", " << DIM << ">(const " << Type << " * coord, " << Type << " * perm) noexcept" << std::endl;
    output << "{" << std::endl;
    for (uint32_t ip = 0U; ip < DIM; ++ip)
    {
        output << "    perm[" << ip << "] =" << std::endl;
        for (uint32_t bp = 0U; bp < NBITS; ++bp)
        {
            const uint32_t ic = ( ip * NBITS + bp ) % DIM;
            const uint32_t bc = ( ip * NBITS + bp ) / DIM;
            const std::string bpspc = (bp < 10U) ? " " : "";
            const std::string bcspc = (bc < 10U) ? " " : "";
            const std::string start = (bp == 0U) ? "          " : "        ^ "; 
            const std::string end   = (bp + 1U == NBITS) ? ";" : "";
            output << start << "(((coord[" << ic << "] >> " << bcspc << bc << ") & 1U) << " << bpspc << bp << ")" << end << std::endl;
        }
    }
    output << "}" << std::endl << std::endl << std::endl;
}

void unravel_printer(std::ofstream& output, const std::string& Type, const uint32_t NBITS, const uint32_t DIM)
{
    /*
        perm[ip][bp] = coord[ic][bc];
        ic + _DIM * bc = ip * _NBITS + bp
        ip = (ic + _DIM * bc) / _NBITS;
        bp = (ic + _DIM * bc) % _NBITS;
        ic = (bp + _NBITS * ip) % _DIM;
        bc = (bp + _NBITS * ip) / _DIM;
    */

    output << "template<> inline constexpr void unravel<" << Type << ", " << DIM << ">(const " << Type << " * perm, " << Type << " * coord) noexcept" << std::endl;
    output << "{" << std::endl;
    for (uint32_t ic = 0U; ic < DIM; ++ic)
    {
        output << "    coord[" << ic << "] =" << std::endl;
        for (uint32_t bc = 0U; bc < NBITS; ++bc)
        {
            const uint32_t ip = ( ic + DIM * bc ) / NBITS;
            const uint32_t bp = ( ic + DIM * bc ) % NBITS;
            const std::string bpspc = (bp < 10U) ? "  " : " ";
            const std::string bcspc = (bc < 10U) ? "  " : " ";
            const std::string start = (bc == 0U) ? "          " : "        ^ "; 
            const std::string end   = (bc + 1U == NBITS) ? ";" : "";
            output << start << "(((perm[" << ip << "] >> " << bpspc << bp << ") & 1U) << " << bcspc << bc << ")" << end << std::endl;
        }
    }
    output << "}" << std::endl << std::endl << std::endl;
}

int main()
{
    std::ofstream output;
    output.open("permutation.hpp", std::ios::out | std::ios::trunc);

    output << "/*" << std::endl
           << "    cmap is a collapsible hierarchically ordered coordinate map" << std::endl
           << std::endl
           << "    Copyright (c) 2020, Sebastian Wouters" << std::endl
           << "    All rights reserved." << std::endl
           << std::endl
           << "    This file is part of cmap, licensed under the BSD 3-Clause License." << std::endl
           << "    A copy of the License can be found in the file LICENSE in the root" << std::endl
           << "    folder of this project." << std::endl
           << "*/" << std::endl
           << std::endl
           << std::endl
           << "namespace cmap" << std::endl
           << "{" << std::endl
           << std::endl
           << "template<class _Type, size_t _DIM>" << std::endl
           << "inline constexpr void permute(const _Type * coord, _Type * perm) noexcept;" << std::endl
           << std::endl
           << "template<class _Type, size_t _DIM>" << std::endl
           << "inline constexpr void unravel(const _Type * perm, _Type * coord) noexcept;" << std::endl
           << std::endl;

    for (uint32_t bit = 16; bit <= 64; bit *= 2)
    {
        std::stringstream _Typename;
        _Typename << "uint" << bit << "_t";
        for (uint32_t dim = 2; dim <= 8; ++dim)
        {
            permute_printer(output, _Typename.str(), bit, dim);
            unravel_printer(output, _Typename.str(), bit, dim);
        }
    }

    output << "} // End of namespace cmap" << std::endl;
    output << std::endl << std::endl;

}


