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

#include "ParserTree/PPG_Aggregate.hpp"
#include "ParserTree/PPG_Input.hpp"

namespace Papageno {
namespace ParserTree {
   
   Aggregate
      ::Aggregate()
{
   Input::getInputs(inputs_);
}

std::string
   Aggregate
      ::getPropertyDescription() const
{   
   std::ostringstream inputDesc;
   for(const auto &input: inputs_) {
      inputDesc << input.getText() << ", ";
   }
   return TO_STRING(Node::getPropertyDescription() << ", inputs = " 
      << inputDesc.str());
}

bool 
   Aggregate
      ::isEqual(const Token &other) const
{
   if(typeid(*this) != typeid(other)) { return false; }
   
   auto otherAggregate = dynamic_cast<const Aggregate *>(&other);
   if(!otherAggregate) { return false; }
   return inputsEqual(inputs_, otherAggregate->inputs_);
}

void 
   Aggregate
      ::generateDependencyCodeInternal(std::ostream &out) const
{   
   std::size_t n_bits = children_.size();
   
   out <<
"#define NUM_BITS_LEFT(N_BITS) \\\n"
"   (N_BITS%%(8*sizeof(PPG_Bitfield_Storage_Type)))\n\n";
      
   out << 
"#define NUM_BYTES(N_BITS) \\\n"
"   (N_BITS/(8*sizeof(PPG_Bitfield_Storage_Type)))\n\n";

   out <<
"PPG_Bitfield_Storage_Type " << this->getId() << "_bitarray[(NUM_BITS_LEFT(" << n_bits << ") != 0) ? (NUM_BYTES(" << n_bits << ") + 1 : NUM_BYTES(" << n_bits << "\n"
"   ] = { 0 };\n\n";

   out <<
"PPG_Input_Id " << this->getId() << "_inputs[" << inputs_.size() << "] = {\n";

   for(int i = 0; i < inputs_.size(); ++i) {
      
      const auto &inputPtr = Input::lookupInput(inputs_[i].getText());
      
      out <<
"   PPG_ACTION_INITIALIZATION_" << inputPtr->getType() << "(" 
      << inputPtr->getParameters() << ")";
      if(i < (inputs_.size() - 1)) {
         out << ",";
      }
      out << "\n";
   }
   out <<
"};\n\n";
}

void 
   Aggregate
      ::generateCCodeInternal(std::ostream &out) const
{   
   out <<
"PPG_" << this->getNodeType() << " " << this->getId() << " (PPG_" << this->getNodeType() << ") {\n";
"   .super = (PPG_Token__)\n"
"   {\n";
   this->Token::generateCCode(out);
   out <<
"   },\n"
"   .n_members = " << inputs_.size() << "\n" <<
"   .inputs_ = " << this->getId() << "_inputs,\n" <<
"   .member_active = (PPG_Bitfield) {\n"
"      .n_bits = " << this->children_.size() << "\n" <<
"      .bitarray = " << this->getId() << "_bitarray\n" <<
"   },\n"
"   .n_inputs_active = 0\n"
"};\n\n";
}

} // namespace ParserTree
} // namespace Papageno
