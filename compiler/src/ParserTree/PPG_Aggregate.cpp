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
   Input::getNextEntities(inputs_);
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
   std::size_t n_bits = inputs_.size();
   
   out <<
"PPG_Bitfield_Storage_Type " << this->getId().getText() << "_member_active[(PPG_NUM_BITS_LEFT(" << n_bits << ") != 0) ? (PPG_NUM_BYTES(" << n_bits << ") + 1) : PPG_NUM_BYTES(" << n_bits << ")]\n"
"   = { 0 };\n\n";

   out <<
"PPG_Input_Id " << this->getId().getText() << "_inputs[" << inputs_.size() << "] = {\n";

   for(int i = 0; i < inputs_.size(); ++i) {
      
      const auto &inputPtr = Input::lookup(inputs_[i].getText());
      
      out <<
"   PPG_INPUT_INITIALIZE_GLOBAL___" << inputPtr->getType().getText() << "(" 
      << inputPtr->getId().getText() << ", "
      << inputPtr->getParameters().getText() << ")";
      if(i < (inputs_.size() - 1)) {
         out << ",";
      }
      out << " // " 
         << inputPtr->getId().getText() << ": " << inputPtr->getLOD();
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
"   .super = {\n";
//    out <<
// "   .super = (PPG_Token__) {\n";
   this->Token::generateCCodeInternal(out);
   out <<
"   },\n"
"   .n_members = " << inputs_.size() << ",\n" <<
"   .inputs = " << this->getId().getText() << "_inputs,\n" <<
"   .member_active = {\n"
"      .n_bits = " << this->inputs_.size() << ",\n" <<
"      .bitarray = " << this->getId().getText() << "_member_active\n" <<
"   },\n"
"   .n_inputs_active = 0\n";
}      

void  
   Aggregate
      ::collectInputAssignments(InputAssignmentsByTag &iabt) const
{
   for(int i = 0; i < inputs_.size(); ++i) {
      
      const auto &input = Input::lookup(inputs_[i].getText());
      
      std::ostringstream path;
      path << this->getId().getText() << "." << this->getEntitiesPath() << "[" << i << "]";
      iabt[input->getType().getText()].push_back( 
         (InputAssignment) { 
            path.str(),
            input
         }
      );
   }
}

void   
   Aggregate
      ::touchActionsAndInputs()
{
   this->Token::touchActionsAndInputs();
   for(auto &inputToken: inputs_) {
      const auto &input = Input::lookup(inputToken.getText());
      input->setWasRequested(true);
   }
}
      
} // namespace ParserTree
} // namespace Papageno
