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

#include "Generator/GLS_Global.hpp"
#include "CommandLine/GLS_CommandLine.hpp"
#include "ParserTree/GLS_Token.hpp"
#include "ParserTree/GLS_Action.hpp"
#include "ParserTree/GLS_Input.hpp"
#include "ParserTree/GLS_Pattern.hpp"
#include "Generator/GLS_Prefix.hpp"
#include "Settings/GLS_Defaults.hpp"

#include "GLS_Compiler.hpp"

#include <ostream>
#include <fstream>
#include <iomanip>

#define SP Glockenspiel::Generator::symbolsPrefix()
#define MP Glockenspiel::Generator::macroPrefix()

namespace Glockenspiel {
namespace Generator {
  
// Das gleiche fuer all actions
   
void caption(std::ostream &out, const std::string &title)
{
   out <<
"//##############################################################################\n"
"// " << title << "\n"
"//##############################################################################\n"
"\n";
}

static void outputInfoAboutSpecificOverride(std::ostream &out)
{
   out <<
"// The following macro can be defined in a preamble header to customize \n"
"// Papagenos input-action behavior.\n"
"//\n";
}

static void outputInformationOfDefinition(std::ostream &out, const ParserTree::Node &node)
{
   const auto &lod = node.getLOD();
   
//    if(lod) {
//       out <<
// "#line " << lod.location_.first_line << " \"" << lod.file_ << "\"\n";
//    }
//    else {
//       out <<
// "#line\n";
//    }

   out <<
"// " << lod << "\n";
   out <<
"//\n";
}

void reportActionsAndInputs(std::ostream &out);
void reportTree(std::ostream &out);
void reportCodeParsed(std::ostream &);

void startExternC(std::ostream &out)
{
   out <<
"#ifdef __cplusplus\n"
"extern \"C\" {\n"
"#endif\n"
"\n";
}

void endExternC(std::ostream &out)
{
   out <<
"#ifdef __cplusplus\n"
"} // end extern \"C\"\n"
"#endif\n"
"\n";
}

void generateFileHeader(std::ostream &out) {
   
   out <<
"// Generated by Papageno compiler version " << PAPAGENO_VERSION << "\n";
   out <<
"\n"
"#pragma once"
"\n";

   reportCodeParsed(out);
   
   reportTree(out);
   
   reportActionsAndInputs(out);

   startExternC(out);

   out <<
"#include \"detail/ppg_context_detail.h\"\n"
"#include \"detail/ppg_token_detail.h\"\n"
"#include \"detail/ppg_note_detail.h\"\n"
"#include \"detail/ppg_chord_detail.h\"\n"
"#include \"detail/ppg_cluster_detail.h\"\n"
"#include \"detail/ppg_time_detail.h\"\n"
"#include \"detail/ppg_sequence_detail.h\"\n"
"#include \"ppg_input.h\"\n"
"#include \"ppg_action_flags.h\"\n"
"\n";
   
   endExternC(out);

   if(Glockenspiel::commandLineArgs.preamble_filename_arg) {
      out <<
"#include \"" << Glockenspiel::commandLineArgs.preamble_filename_arg << "\"\n";
      out <<
"\n";
   }
   out <<
"#ifdef " << MP << "PAPAGENO_PREAMBLE_HEADER\n"
"#include " << MP << "PAPAGENO_PREAMBLE_HEADER\n"
"#endif\n"
"\n";
   startExternC(out);

   out <<
"// C++ does not support designated initializers\n"
"//\n"
"#ifdef __cplusplus\n"
"#   define __GLS_DI__(NAME)\n"
"#   define GLS_ZERO_INIT {}\n"
"#else\n"
"#   define __GLS_DI__(NAME) .NAME =\n"
"#   define GLS_ZERO_INIT { 0 }\n"
"#endif\n"
"\n"
"#define GLS_ACTION_ZERO_INIT { GLS_ZERO_INIT }\n"
"#define GLS_INPUT_ZERO_INIT 0\n"
"\n"
"#define GLS_EMPTY()\n"
"#define GLS_DEFER(...) __VA_ARGS__ GLS_EMPTY()\n"
"\n";

   // The bit manipulation macros are unique and thus do not require prefixing
   //
  out <<
"#ifndef GLS_NUM_BITS_LEFT\n"
"#define " << "GLS_NUM_BITS_LEFT(N_BITS) \\\n"
"   (N_BITS%(8*sizeof(PPG_Bitfield_Storage_Type)))\n"
"#endif\n"
"\n"
"#ifndef GLS_NUM_BYTES\n"
"#define " << "GLS_NUM_BYTES(N_BITS) \\\n"
"   (N_BITS/(8*sizeof(PPG_Bitfield_Storage_Type)))\n"
"#endif\n"
"\n";

   defaults.outputC(out);
}

void reportCodeParsed(std::ostream &out)
{
   caption(out, "Code parsed");
   
   const char *curFile = nullptr;
   long lastLine = -1;
   
   out <<
"/*\n";

   for(const auto &codeLineInfo: Parser::code) {
      
      if(curFile != codeLineInfo.file_) {
         curFile = codeLineInfo.file_;
         out <<
"**** " << curFile << "\n";
      }
      else if(lastLine + 1 != codeLineInfo.lineNumber_) {
         out <<
"...\n";
      }
      lastLine = codeLineInfo.lineNumber_;
      
      out << 
std::setw(4) << codeLineInfo.lineNumber_ << " " << codeLineInfo.code_ << "\n";
   }
   out <<
"*/\n"
"\n";
}

template<typename EntityType,
         typename EntityAssignments>
void generateEntityInformation(
            std::ostream &out,
            const EntityAssignments &entityAssignments,
            const std::string &entityType,
            const std::string &entityTypeUpper,
            const std::string &entityTypeAllCaps
     )
{  
   const auto &entitiesByType = EntityType::getEntitiesByType(true /* only requested */);
   
   const auto &types = EntityType::getTypes();
   
   caption(out, entityTypeUpper + "s");
      
   out <<
"// Add a flag to enable local initialization for\n"
"// all input classes.\n"
"//\n"
"#ifdef " << MP << "GLS_ENABLE_" << entityTypeAllCaps << "S_LOCAL_INITIALIZATION_ALL\n"
"\n";

   for(const auto &type: types) {
      
      out <<
"// A flag to toggle specific initialization for type class \'" << type << "\'.\n"
"//\n"
"#   ifndef " << MP << "GLS_ENABLE_" << entityTypeAllCaps << "S_LOCAL_INITIALIZATION___" << type << " \n"
"#      define " << MP << "GLS_ENABLE_" << entityTypeAllCaps << "S_LOCAL_INITIALIZATION___" << type << " \n"
"#   endif \n"
"\n";
   }
   out <<
"#endif\n\n";

   out <<
"// Enable the same type of initialization for all type classes\n"
"//\n"
"#ifndef " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE\n"
"#   define " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE(UNIQUE_ID, USER_ID, ...) __VA_ARGS__\n"
"#endif\n\n";
   
   out <<
"// Distinguish between global and local\n"
"// initialization by type class.\n"
"//\n"
"// If no class wise initialization is desired\n"
"// we fall back to the common initialization method\n"
"//\n";
   for(const auto &type: types) {

      out <<
"// " << entityTypeUpper << " type \'" << type << "\'.\n" <<
"#ifndef " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE___" << type << "\n"
"#   define " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE___" << type << "(UNIQUE_ID, USER_ID, ...) \\\n" <<
"      " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE(UNIQUE_ID, GLS_DEFER(USER_ID), ##__VA_ARGS__)\n"
"#endif\n"
"\n"
"#ifdef " << MP << "GLS_ENABLE_" << entityTypeAllCaps << "S_LOCAL_INITIALIZATION___" << type << "\n" <<
"#   ifndef " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_LOCAL___" << type << "\n" <<
"#      define " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_LOCAL___" << type << "(UNIQUE_ID, USER_ID, PATH, ...) \\\n" <<
"            PATH = " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE___" << type << "(UNIQUE_ID, GLS_DEFER(USER_ID), ##__VA_ARGS__);\n" <<
"#   endif\n"
"#   ifndef " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_GLOBAL___" << type << "\n" <<
"#      define " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_GLOBAL___" << type << "(UNIQUE_ID, USER_ID, ...) GLS_" << entityTypeAllCaps << "_ZERO_INIT\n" <<
"#   endif\n"
"#else\n"
"#   ifndef " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_LOCAL___" << type << "\n" <<
"#      define " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_LOCAL___" << type << "(UNIQUE_ID, USER_ID, PATH, ...)\n"
"#   endif\n"
"#   ifndef " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_GLOBAL___" << type << "\n" <<
"#      define " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_GLOBAL___" << type << "(UNIQUE_ID, USER_ID, ...) \\\n" <<
"            " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE___" << type << "(UNIQUE_ID, GLS_DEFER(USER_ID), ##__VA_ARGS__)\n" <<
"#   endif\n"
"#endif\n"
"\n"         
"// Define a common entry for each type class to be used for\n"
"// local initialization.\n"
"//\n"
"#define " << MP << "GLS_" << entityTypeAllCaps << "S_INITIALIZE_LOCAL_ALL___" << type << " \\\n";

      auto it = entityAssignments.find(type);
      
      if(it != entityAssignments.end()) {

         for(const auto &assignment: it->second) {
            out <<
"   " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_LOCAL___" << type << "("
         << assignment.entity_->getUniqueId() << ", GLS_DEFER(" << assignment.entity_->getId().getText() << "), " << SP << assignment.pathString_;
            if(assignment.entity_->getParametersDefined()) {
               out << ", " << assignment.entity_->getParameters().getText();
            }
            out << ") \\\n";
         }
      }
      out <<
"\n";
   }
   
   out <<
"// Define a common entry point for local initialization\n"
"//\n"
"#define " << MP << "GLS_" << entityTypeAllCaps << "S_INITIALIZE_LOCAL_ALL \\\n";

   for(const auto &type: types) {
      out <<
"   " << MP << "GLS_" << entityTypeAllCaps << "S_INITIALIZE_LOCAL_ALL___" << type << " \\\n";
   }
   out <<
"\n";

   out <<
"// This macro can be used to add configuration of " << entityType << "s at global scope.\n"
"// The default is no configuration at global scope.\n"
"//\n"
"#ifndef " << MP << "GLS_"<< entityTypeAllCaps << "_CONFIGURE_GLOBAL\n"
"#   define " << MP << "GLS_"<< entityTypeAllCaps << "_CONFIGURE_GLOBAL(...)\n"
"#endif\n"
"\n";

   out <<
"// This macro can be used to add configuration of " << entityType << "s at global scope.\n"
"// The default is no configuration at local scope.\n"
"//\n"
"#ifndef " << MP << "GLS_"<< entityTypeAllCaps << "_CONFIGURE_LOCAL\n"
"#   define " << MP << "GLS_"<< entityTypeAllCaps << "_CONFIGURE_LOCAL(...)\n"
"#endif\n"
"\n";

   for(const auto &type: types) {
      
      out <<
"// Implement the following macro to enable specific initialization \n"
"// at global scope for type class \'" << type << "\'.\n" <<
"//\n"
"#ifndef " << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_GLOBAL___" << type << "\n"
"#   define " << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_GLOBAL___" << type << "(...) \\\n"
"       " << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_GLOBAL(GLS_DEFER(__VA_ARGS__))\n"
"#endif\n"
"\n"
"// Implement the following macro to enable specific initialization \n"
"// at local scope for type class " << type << ".\n" <<
"//\n"
"#ifndef " << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_LOCAL___" << type << "\n"
"#   define " << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_LOCAL___" << type << "(...) \\\n"
"       " << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_LOCAL(GLS_DEFER(__VA_ARGS__))\n"
"#endif\n"
"\n";

      out <<
"// Use this macro to perform specific initializations of " << entityType << "s with\n"
"// type class \'" << type << "\'.\n"
"//\n"
"#define " << MP << "GLS_" << entityTypeAllCaps << "S___" << type << "(OP) \\\n";

      auto it = entitiesByType.find(type);
      
      if(it != entitiesByType.end()) {
         for(const auto &entityPtr: it->second) {
            out <<
"   OP(" << entityPtr->getUniqueId() << ", " << entityPtr->getId().getText();
            if(entityPtr->getParametersDefined()) {
               out << ", " << entityPtr->getParameters().getText();
            }
           out << ")\\\n";
        }
      }
   out <<
"\n";
   }

   out <<
"#define " << MP << "GLS_" << entityTypeAllCaps << "S_ALL(OP) \\\n";

   for(const auto &type: types) {
      out <<
"   " << MP << "GLS_" << entityTypeAllCaps << "S___" << type << "(OP) \\\n";
   }
   out << "\n"; 

   out <<
"#define " << MP << "GLS_" << entityTypeAllCaps << "S_CONFIGURE_LOCAL_ALL \\\n";
   for(const auto &type: types) {
      out <<
"   " << MP << "GLS_" << entityTypeAllCaps << "S___" << type << "(" << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_LOCAL___" << type << ") \\\n";
   }
   out << 
"\n";  
}

void recursivelyCollectInputAssignments(ParserTree::InputAssignmentsByTag &iabt, const ParserTree::Token &token)
{
   token.collectInputAssignments(iabt);
    
   for(const auto &childTokenPtr: token.getChildren()) {
      recursivelyCollectInputAssignments(iabt, *childTokenPtr);
   }
}

void generateGlobalInputInformation(std::ostream &out)
{
   auto root = ParserTree::Pattern::getTreeRoot();
   
   ParserTree::InputAssignmentsByTag iabt;
   recursivelyCollectInputAssignments(iabt, *root);
   
   generateEntityInformation<ParserTree::Input>(
      out,
      iabt,
      "input",
      "Input",
      "INPUT"
   );
}
   
static void recursivelyCollectActionAssignments(ParserTree::ActionAssignmentsByTag &aabt, const ParserTree::Token &token)
{
   token.collectActionAssignments(aabt);
    
   for(const auto &childTokenPtr: token.getChildren()) {
      recursivelyCollectActionAssignments(aabt, *childTokenPtr);
   }
}

void generateGlobalActionInformation(std::ostream &out)
{
   auto root = ParserTree::Pattern::getTreeRoot();
   
   ParserTree::ActionAssignmentsByTag aabt;
   recursivelyCollectActionAssignments(aabt, *root);
   
   generateEntityInformation<ParserTree::Action>(
      out,
      aabt,
      "action",
      "Action",
      "ACTION"
   );
}

template<typename EntitiesByType>
void globallyInitializeEntities(
            std::ostream &out,
            const EntitiesByType &entitiesByType,
            const std::string &entityTypeUpper,
            const std::string &entityTypeAllCaps
     )
{  
   caption(out, entityTypeUpper + "s global configuration");
   
   for(const auto &abtEntry: entitiesByType) {
      const auto &tag = abtEntry.first;
      out <<
"// " << entityTypeUpper << " type \'" << tag << "\'\n"
"//\n"
<< MP << "GLS_" << entityTypeAllCaps << "S___" << tag << "(" << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_GLOBAL___" << tag << ")\n"
"\n";
   }
}

void globallyInitializeInputs(std::ostream &out)
{
   auto inputsByType = ParserTree::Input::getEntitiesByType(true /* only requested */);
   
   globallyInitializeEntities(
      out,
      inputsByType,
      "Input",
      "INPUT"
   );
}

void globallyInitializeActions(std::ostream &out)
{
   auto actionsByType = ParserTree::Action::getEntitiesByType(true /* only requested */);
   
   globallyInitializeEntities(
      out,
      actionsByType,
      "Action",
      "ACTION"
   );
}

void globallyInitializeAllEntities(std::ostream &out)
{
   globallyInitializeInputs(out);
   globallyInitializeActions(out);
}

template<typename EntitiesByType>
void reportUnusedEntities(
            std::ostream &out,
            const EntitiesByType &entitiesByType,
            const std::string &entityTypeUpper)
{
   bool anyEntities = false;
   for(const auto &abtEntry: entitiesByType) {
      const auto &tag = abtEntry.first;
      
      bool anyUnusedOfCurrentType = false;
     
      for(const auto &entityPtr: abtEntry.second) {
         
         if(entityPtr->getWasRequested()) { continue; }
         
         if(!anyUnusedOfCurrentType) {
            if(!anyEntities) {
               caption(out, "Unused " + entityTypeUpper + "s");
               anyEntities = true;
            }
            out <<
"// " << entityTypeUpper << " type \'" << tag << "\': ";
            anyUnusedOfCurrentType = true;
         }
         out << entityPtr->getId().getText() << ", ";
      }
      if(anyUnusedOfCurrentType) { out << "\n"; }
   }
   if(anyEntities) {
      out << "\n";
   }
}

template<typename JoinedEntities>
void reportJoinedEntities(
            std::ostream &out,
            const JoinedEntities &joinedEntities,
            const std::string &entityTypeUpper)
{
   bool anyEntities = false;
   for(const auto &abtEntry: joinedEntities) {
      const auto &tag = abtEntry.first;
      
      bool anyUnusedOfCurrentType = false;
     
      for(const auto &entityPair: abtEntry.second) {
         
         if(!entityPair.first->getWasRequested()) { continue; }
         
         if(!anyUnusedOfCurrentType) {
            if(!anyEntities) {
               caption(out, "Joined " + entityTypeUpper + "s");
               anyEntities = true;
            }
            out <<
"// " << entityTypeUpper << " type \'" << tag << "\': ";
            anyUnusedOfCurrentType = true;
         }
         out << entityPair.first->getId().getText() << "->" << entityPair.second->getId().getText() << ", ";
      }
      if(anyUnusedOfCurrentType) { out << "\n"; }
   }
   if(anyEntities) {
      out << "\n";
   }
}

void reportActionsAndInputs(std::ostream &out)
{
   auto inputsByType = ParserTree::Input::getEntitiesByType();
   auto actionsByType = ParserTree::Action::getEntitiesByType();

   reportUnusedEntities(out, ParserTree::Input::getEntitiesByType(), "Inputs");
   reportJoinedEntities(out, ParserTree::Input::getJoinedEntities(), "Inputs");
   reportUnusedEntities(out, ParserTree::Action::getEntitiesByType(), "Actions");
   reportJoinedEntities(out, ParserTree::Action::getJoinedEntities(), "Actions");
}

void recursivelyOutputTree(std::ostream &out, const ParserTree::Token &token, int indent)
{
   out <<
"// ";

   for(int i = 0; i < indent; ++i) {
      out << "   ";
   }
   
   out << SP << token.getId().getText() << "(" << token.getInputs() 
      << ") [" << token.getLayer().getText() << "]";
   
   if(!token.getAction().getText().empty()) {
      out << " : " << token.getAction().getText();
   }
   
   out << "\n";
   
   for(const auto &child: token.getChildren()) {
      recursivelyOutputTree(out, *child, indent + 1);
   }
}

void reportTree(std::ostream &out)
{
   caption(out, "Tree");

   auto root = ParserTree::Pattern::getTreeRoot();
   recursivelyOutputTree(out, *root, 0);
   
   out << "\n";
}

void recursivelyOutputToken(std::ostream &out, const ParserTree::Token &token)
{
   for(const auto &childTokenPtr: token.getChildren()) {
      recursivelyOutputToken(out, *childTokenPtr);
   }
   
   outputInformationOfDefinition(out, token);
   token.generateCCode(out);
}

void recursivelyOutputTokenForwardDeclaration(std::ostream &out, const ParserTree::Token &token)
{
   out <<
"extern ";
   token.outputCTokenDeclaration(out);
   out << ";\n";

   for(const auto &childTokenPtr: token.getChildren()) {
      recursivelyOutputTokenForwardDeclaration(out, *childTokenPtr);
   }
}

void recursivelyGetMaxEvents(const ParserTree::Token &token, int curDepth, int &maxDepth, int curInputs, int &maxInputs)
{
   curInputs += token.getNumInputs();
   ++curDepth;
   
   if(token.getChildren().empty()) {
      if(curInputs > maxInputs) {
         maxInputs = curInputs;
      }
      if(curDepth > maxDepth) {
         maxDepth = curDepth;
      }
   }
   else {
      for(const auto &childTokenPtr: token.getChildren()) {
         recursivelyGetMaxEvents(*childTokenPtr, curDepth, maxDepth, curInputs, maxInputs);
      }
   }
}

void generateGlobalContext(std::ostream &out)
{
   auto root = ParserTree::Pattern::getTreeRoot();
   
   caption(out, "Initialization");
   
   endExternC(out);
   out <<
"#ifdef " << MP << "GLS_GLOBAL_INITIALIZATION_INCLUDE\n"
"#include " << MP << "GLS_GLOBAL_INITIALIZATION_INCLUDE\n"
"#endif\n"
"\n";

   out <<
"#ifndef " << MP << "GLS_GLOBAL_INITIALIZATION\n"
"#define " << MP << "GLS_GLOBAL_INITIALIZATION\n"
"#endif\n"
"\n"
<< MP << "GLS_GLOBAL_INITIALIZATION\n"
"\n";
   startExternC(out);

   globallyInitializeAllEntities(out);

   caption(out, "Token tree forward declarations");
   
   recursivelyOutputTokenForwardDeclaration(out, *root);
   out << "\n";
   
   caption(out, "Token tree");
   
   // Recursively output token tree
   //
   recursivelyOutputToken(out, *root);
   
   caption(out, "Context");
   
   int maxDepth = 0;
   int maxEvents = 0;
   recursivelyGetMaxEvents(*root, 0, maxDepth, 0, maxEvents);
   assert(maxDepth > 0);
   assert(maxEvents > 0);
   
   out <<
"PPG_Event_Queue_Entry " << SP << "event_buffer[" << 2*maxEvents << "] = GLS_ZERO_INIT;\n"
//    for(int i = 0; i < 2*maxEvents; ++i) {
//       out <<
// "                     {{0, 0, 0}, 0, {0, 0}}";
//       if(i < 2*maxEvents - 1) {
//          out << ",";
//       }
//       out << "\n";
//    }
//    out <<
// "};\n"
"\n";

   out <<
"PPG_Furcation " << SP << "furcations[" << maxDepth << "] = GLS_ZERO_INIT;\n"
//    for(int i = 0; i < maxDepth; ++i) {
//       out <<
// "                     {0, 0, 0, 0}";
//       if(i < maxDepth - 1) {
//          out << ",";
//       }
//       out << "\n";
//    }
//    out <<
// "};\n"
"\n";
   
   out <<
"PPG_Token__ *" << SP << "tokens[" << maxDepth << "] = GLS_ZERO_INIT;\n";
/*
   for(int i = 0; i < maxDepth; ++i) {
      out <<
"                     NULL";
      if(i < maxDepth - 1) {
         out << ",";
      }
      out << "\n";
   }
   out <<
"};\n"*/
"\n";

   out <<
"PPG_Context " << SP << "context = {\n"
"   __GLS_DI__(event_buffer) {\n"
"      __GLS_DI__(events) " << SP << "event_buffer,\n"
"      __GLS_DI__(start) 0,\n"
"      __GLS_DI__(end) 0,\n"
"      __GLS_DI__(cur) 0,\n"
"      __GLS_DI__(size) 0,\n"
"      __GLS_DI__(max_size) " << 2*maxEvents << "\n";
   out <<
"   },\n"
"   __GLS_DI__(furcation_stack) {\n"
"      __GLS_DI__(furcations) " << SP << "furcations,\n"
"      __GLS_DI__(n_furcations) 0,\n"
"      __GLS_DI__(cur_furcation) 0,\n"
"      __GLS_DI__(max_furcations) " << maxDepth << "\n";
   out <<
"   },\n"
"   __GLS_DI__(active_tokens) {\n"
"      __GLS_DI__(tokens) " << SP << "tokens,\n"
"      __GLS_DI__(n_tokens) 0,\n"
"      __GLS_DI__(max_tokens) " << maxDepth << "\n";
   out <<
"   },\n"
"   __GLS_DI__(pattern_root) &" << SP << root->getId().getText() << ",\n"
"   __GLS_DI__(current_token) NULL,\n"
"   __GLS_DI__(properties) {\n"
"      __GLS_DI__(timeout_enabled) " << MP << "GLS_INITIAL_TIMEOUT_ENABLED,\n"
"      __GLS_DI__(papageno_enabled) " << MP << "GLS_INITIAL_PAPAGENO_ENABLED,\n"
"#     if PPG_HAVE_LOGGING\n"
"      __GLS_DI__(logging_enabled) " << MP << "GLS_INITIAL_LOGGING_ENABLED,\n"
"#     endif\n"
"      __GLS_DI__(destruction_enabled) false\n"
"    },\n"
"   __GLS_DI__(tree_depth) " << maxDepth << ",\n"
"   __GLS_DI__(layer) " << MP << "GLS_INITIAL_LAYER,\n"
"   __GLS_DI__(abort_trigger_input) " << MP << "GLS_INITIAL_ABORT_TRIGGER_INPUT,\n"
"   __GLS_DI__(time_last_event) 0,\n"
"   __GLS_DI__(event_timeout) " << MP << "GLS_INITIAL_EVENT_TIMEOUT,\n"
"   __GLS_DI__(event_processor) " << MP << "GLS_INITIAL_EVENT_PROCESSOR,\n"
"   __GLS_DI__(time_manager) {\n"
"      __GLS_DI__(time) &" << MP << "GLS_INITIAL_TIME_FUNCTION,\n"
"      __GLS_DI__(time_difference) &" << MP << "GLS_INITIAL_TIME_DIFFERENCE_FUNCTION,\n"
"      __GLS_DI__(compare_times) &" << MP << "GLS_INITIAL_TIME_COMPARISON_FUNCTION\n"
"   },\n"
"   __GLS_DI__(signal_callback) {\n"
"      __GLS_DI__(func) " << MP << "GLS_INITIAL_SIGNAL_CALLBACK_FUNC,\n"
"      __GLS_DI__(user_data) " << MP << "GLS_INITIAL_SIGNAL_CALLBACK_USER_DATA\n"
"   }\n"
"#  if PPG_HAVE_STATISTICS\n"
"   ,\n"
"   __GLS_DI__(statistics) {\n"
"      __GLS_DI__(n_nodes_visited) 0,\n"
"      __GLS_DI__(n_token_checks) 0,\n"
"      __GLS_DI__(n_furcations) 0,\n"
"      __GLS_DI__(n_reversions) 0\n"
"   }\n"
"#  endif\n"
"};\n"
"\n";
}

void generateInitializationFunction(std::ostream &out)
{
   caption(out, "Initialization");
   
   out << 
"#define " << MP << "GLS_LOCAL_INITIALIZATION \\\n"
"    /* Configuration of all actions. \\\n"
"     */ \\\n"
"    " << MP << "GLS_ACTIONS_CONFIGURE_LOCAL_ALL \\\n"
"    \\\n"
"    /* Configuration of all inputs. \\\n"
"     */ \\\n"
"    " << MP << "GLS_INPUTS_CONFIGURE_LOCAL_ALL  \\\n"
"    \\\n"
"    /* Local initialization of actions.\\\n"
"     */ \\\n"
"    " << MP << "GLS_ACTIONS_INITIALIZE_LOCAL_ALL \\\n"
"    \\\n"
"    /* Local initialization of inputs. \\\n"
"     */ \\\n"
"    " << MP << "GLS_INPUTS_INITIALIZE_LOCAL_ALL\n"
"\n"
"void " << SP << "papageno_initialize_context()\n"
"{\n"
"#  ifndef " << MP << "GLS_NO_AUTOMATIC_LOCAL_INITIALIZATION\n"
"   " << MP << "GLS_LOCAL_INITIALIZATION\n"
"#  endif\n"
"\n"
"   ppg_context = &" << SP << "context;\n"
"}\n\n";
}

void generateGlobal(const std::string &outputFilename)
{
   std::ofstream outputFile(outputFilename);
   
   generateFileHeader(outputFile);
   
   // Generate global action information
   generateGlobalActionInformation(outputFile);
   
   generateGlobalInputInformation(outputFile);
   
   generateGlobalContext(outputFile);
   
   generateInitializationFunction(outputFile);
   
   endExternC(outputFile);
//    outputFile <<
// "#line\n"
// "\n";
}

} // namespace ParserTree
} // namespace Glockenspiel
