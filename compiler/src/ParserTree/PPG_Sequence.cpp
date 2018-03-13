/* Copyright 2018 noseglasses <shinynoseglasses@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ParserTree/PPG_Sequence.hpp"

namespace Papageno {
namespace ParserTree {
   
void 
   Sequence
      ::generateCCodeInternal(std::ostream &out) const
{ 
   out <<
"   .aggregate = {\n";

   this->Aggregate::generateCCodeInternal(out);
   out <<
"   },\n"
"   .next_member = 0\n";
}

} // namespace ParserTree
} // namespace Papageno
