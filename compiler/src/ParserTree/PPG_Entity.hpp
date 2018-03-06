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

#include "Parser/PPG_ParserToken.hpp"

namespace Papageno {
namespace ParserTree {
   
class Entity
{
   public:
      
      static void setNextParameters(const Parser::Token &parameters) {
         parameters_ = parameters;
      }
      
      static Parser::Token getNextParameters() {
         Parser::Token tmp = parameters_;
         parameters_ = Parser::Token();
         return tmp;
      }
      
      static void setNextType(const Parser::Token &nextType) {
         nextType_ = nextType;
      }
      
      static Parser::Token getNextType() {
         Parser::Token tmp = nextType_;
         nextType_ = Parser::Token();
         return tmp;
      }
      
      static void setNextId(const Parser::Token &nextId) {
         nextId_ = nextId;
      }
      
      static Parser::Token getNextId() {
         Parser::Token tmp = nextId_;
         nextId_ = Parser::Token();
         return tmp;
      }
   
   protected:
      
      static Parser::Token   parameters_;
      
      static Parser::Token   nextType_;
      static Parser::Token   nextId_;
};

      
} // namespace ParserTree
} // namespace Papageno
