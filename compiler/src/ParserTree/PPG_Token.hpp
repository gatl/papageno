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

#pragma once

#include "ParserTree/PPG_Node.hpp"
#include "ParserTree/PPG_Action.hpp"

#include <memory>
#include <vector>

namespace Papageno {
namespace ParserTree {
   
class Token : public Node
{
   public:  
      
      Token()
         :  layer_(curLayer_)
      {}
      
      Token(const Parser::Token &id)
         :  Node(id),
            layer_(curLayer_)
      {}
      
      void setAction(const std::shared_ptr<Action> &action) {
         action_ = action;
      }
      
      const std::shared_ptr<Action> &getAction() const {
         return action_;
      }
      
      void addChild(const std::shared_ptr<Token> &token) {
         children_.push_back(token);
      }
      
      const std::vector<std::shared_ptr<Token>> &getChildren() const {
         return children_;
      }
      
      bool hasChildren() const { return !children_.empty(); }
      
      virtual bool isEqual(const Token &other) const { return false; }
      
      virtual std::shared_ptr<Token> clone() const { return std::shared_ptr<Token>(); }
      
      static void setCurrentLayer(const Parser::Token &layer) { 
         curLayer_ = layer;
      }
      
      void generateCCode(std::ostream &out) const {
         
         this->generateDependencyCodeInternal(out);

         // Output children
         //
         if(!children_.empty()) {
            out << 
"PPG_Token *" << this->getId().getText() << "_children[" << children_.size() << "] = {\n";

            for(int i = 0; i < children_.size(); ++i) {
               out <<
"   &" << children_[i]->getId().getText();
               if(i < children_.size() - 1) {
                  out << ",";
               }
               out << "\n";
            }
            out <<
"};\n\n";
         }
          
         out <<
"PPG_" << this->getNodeType() << " " << this->getId().getText() << " = (PPG_" << this->getNodeType() << ") {\n";

         this->generateCCodeInternal(out);
         
         out <<
"};\n\n";


         // TODO: Howto set parent ptrs?
      }
      
      virtual std::string getNodeType() const override { return "Token"; }
           
   protected:
      
      virtual void generateDependencyCodeInternal(std::ostream &out) const {}
      
      virtual void generateCCodeInternal(std::ostream &out) const {
         
         out <<
"   .misc = (PPG_Misc_Bits) {\n"
"       .state = PPG_Token_Initialized,\n"
"       .flags = " << this->getFlags() << ",\n"
"       .action_state = 0,\n"
"       .action_flags = PPG_Action_Default\n"
"    },\n";
         
         if(!children_.empty()) {
            out <<
"    .children = " << this->getId().getText() << "_children,\n" <<
"    .n_allocated_children = sizeof(" << this->getId().getText() << "_children),\n" <<
"    .n_children = sizeof(" << this->getId().getText() << "_children),\n";
         }
         else {
            out <<
"    .children = nullptr,\n"
"    .n_allocated_children = 0,\n"
"    .n_children = 0\n";
         }
         
         if(action_) {
            out <<
"    .action = PPG_ACTION_INITIALIZATION_" << action_->getType().getText() << "(" 
            << action_->getParameters().getText() << "), // " 
               << action_->getId().getText() << ": " << action_->getLOD() << "\n";
         }
         else {
            
            out <<
"    .action = (PPG_Action) { \n"
"       .callback = (PPG_Action_Callback) {\n"
"          .func = NULL,\n"
"          .user_data = NULL\n"
"       }\n"
"    },\n";
         }
         out <<
"    .layer = " << this->layer_.getText() << "\n";
      }
      
      virtual std::string getFlags() const { return "0"; }
      
   protected:
      
      std::shared_ptr<Action>               action_;
      std::vector<std::shared_ptr<Token>>   children_;
      Parser::Token                         layer_;
      
      static Parser::Token                  curLayer_;
};

}
}
