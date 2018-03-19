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

void generateFileHeader(std::ostream &out) {
   
   out <<
"// Generated by Papageno compiler version " << PAPAGENO_VERSION << "\n";
   out <<
"\n";

   reportCodeParsed(out);
   
   reportTree(out);
   
   reportActionsAndInputs(out);

   out <<
"#include \"detail/ppg_context_detail.h\"\n"
"#include \"detail/ppg_token_detail.h\"\n"
"#include \"detail/ppg_note_detail.h\"\n"
"#include \"detail/ppg_chord_detail.h\"\n"
"#include \"detail/ppg_cluster_detail.h\"\n"
"#include \"detail/ppg_time_detail.h\"\n"
"#include \"detail/ppg_sequence_detail.h\"\n"
"#include \"ppg_input.h\"\n"
"\n";

   if(Glockenspiel::commandLineArgs.preamble_filename_arg) {
      out <<
"#include \"" << Glockenspiel::commandLineArgs.preamble_filename_arg << "\"\n";
      out <<
"\n";
   }
   
   out <<
"#ifdef " << MP << "PAPAGENO_PREAMBLE_HEADER\n"
"#include " << MP << "PAPAGENO_PREAMBLE_HEADER\n"
"#endif\n\n";
   
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

template<typename EntitiesByType,
         typename EntityAssignments>
void generateEntityInformation(
            std::ostream &out,
            const EntitiesByType &entitiesByType,
            const EntityAssignments &entityAssignments,
            const std::string &entityType,
            const std::string &entityTypeUpper,
            const std::string &entityTypeAllCaps
     )
{  
   caption(out, entityTypeUpper + "s");
      
   out <<
"// Add a flag to enable local initialization for\n"
"// all input classes.\n"
"//\n"
"#ifdef " << MP << "GLS_ENABLE_" << entityTypeAllCaps << "S_LOCAL_INITIALIZATION_ALL\n";

   for(const auto &entityAssignmentsEntry: entityAssignments) {
      const auto &tag = entityAssignmentsEntry.first;
      
      out <<
"// A flag to toggle specific initialization for tag class \'" << tag << "\'.\n"
"//\n"
"#   ifndef " << MP << "GLS_ENABLE_" << entityTypeAllCaps << "S_LOCAL_INITIALIZATION___" << tag << " \n"
"#      define " << MP << "GLS_ENABLE_" << entityTypeAllCaps << "S_LOCAL_INITIALIZATION___" << tag << " \n"
"#   endif \n";
   }
   out <<
"#endif\n\n";

   out <<
"// Enable the same type of initialization for all tag classes\n"
"//\n"
"#ifndef " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE\n"
"#   define " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE(ID, ...) __VA_ARGS__\n"
"#endif\n\n";
   
   out <<
"// Distinguish between global and local\n"
"// initialization by tag class.\n"
"//\n"
"// If no class wise initialization is desired\n"
"// we fall back to the common initialization method\n"
"//\n";
   for(const auto &entityAssignmentsEntry: entityAssignments) {
      const auto &tag = entityAssignmentsEntry.first;

      out <<
"// Tag class \'" << tag << "\'.\n" <<
"#ifndef " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE___" << tag << "\n"
"#   define " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE___" << tag << "(ID, ...) \\\n" <<
"      " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE(ID, ##__VA_ARGS__)\n"
"#endif\n"
"\n"
"#ifdef " << MP << "GLS_ENABLE_" << entityTypeAllCaps << "S_LOCAL_INITIALIZATION___" << tag << "\n" <<
"#   ifndef " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_LOCAL___" << tag << "\n" <<
"#      define " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_LOCAL___" << tag << "(ID, PATH, ...) \\\n" <<
"            PATH = " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE___" << tag << "(ID, ##__VA_ARGS__);\n" <<
"#   endif\n"
"#   ifndef " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_GLOBAL___" << tag << "\n" <<
"#      define " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_GLOBAL___" << tag << "(ID, ...) 0\n" <<
"#   endif\n"
"#else\n"
"#   ifndef " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_LOCAL___" << tag << "\n" <<
"#      define " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_LOCAL___" << tag << "(ID, PATH, ...)\n"
"#   endif\n"
"#   ifndef " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_GLOBAL___" << tag << "\n" <<
"#      define " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_GLOBAL___" << tag << "(ID, ...) \\\n" <<
"            " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE___" << tag << "(ID, ##__VA_ARGS__)\n" <<
"#   endif\n"
"#endif\n"
"\n"         
"// Define a common entry for each tag class to be used for\n"
"// local initialization.\n"
"//\n"
"#define " << MP << "GLS_" << entityTypeAllCaps << "S_INITIALIZE_LOCAL_ALL___" << tag << " \\\n";
      for(const auto &assignment: entityAssignmentsEntry.second) {
         out <<
"   " << MP << "GLS_" << entityTypeAllCaps << "_INITIALIZE_LOCAL___" << tag << "(" << assignment.entity_->getId().getText() << ", " << SP << assignment.pathString_;
         if(assignment.entity_->getParametersDefined()) {
            out << ", " << assignment.entity_->getParameters().getText();
         }
         out << ") \\\n";
      }
      out <<
"\n";
   }
   
   out <<
"// Define a common entry point for local initialization\n"
"//\n"
"#define " << MP << "GLS_" << entityTypeAllCaps << "S_INITIALIZE_LOCAL_ALL \\\n";

   for(const auto &entityAssignmentsEntry: entityAssignments) {
      const auto &tag = entityAssignmentsEntry.first;
      
      out <<
"   " << MP << "GLS_" << entityTypeAllCaps << "S_INITIALIZE_LOCAL_ALL___" << tag << " \\\n";
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

   for(const auto &abtEntry: entitiesByType) {
      const auto &tag = abtEntry.first;
      
      out <<
"// Implement the following macro to enable specific initialization \n"
"// at global scope for tag class \'" << tag << "\'.\n" <<
"//\n"
"#ifndef " << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_GLOBAL___" << tag << "\n"
"#   define " << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_GLOBAL___" << tag << "(...) \\\n"
"       " << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_GLOBAL(__VA_ARGS__)\n"
"#endif\n"
"\n"
"// Implement the following macro to enable specific initialization \n"
"// at local scope for tag class " << tag << ".\n" <<
"//\n"
"#ifndef " << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_LOCAL___" << tag << "\n"
"#   define " << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_LOCAL___" << tag << "(...) \\\n"
"       " << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_LOCAL(__VA_ARGS__)\n"
"#endif\n"
"\n";

      out <<
"// Use this macro to perform specific initializations of " << entityType << "s with\n"
"// tag class \'" << tag << "\'.\n"
"//\n"
"#define " << MP << "GLS_" << entityTypeAllCaps << "S___" << tag << "(OP) \\\n";

      for(const auto &entityPtr: abtEntry.second) {
         out <<
"   OP(" << entityPtr->getId().getText();
         if(entityPtr->getParametersDefined()) {
            out << ", " << entityPtr->getParameters().getText();
         }
         out << ")\\\n";
      }
   out <<
"\n";
   }

   out <<
"#define " << MP << "GLS_" << entityTypeAllCaps << "S_ALL(OP) \\\n";

   for(const auto &abtEntry: entitiesByType) {
      const auto &tag = abtEntry.first;
      out <<
"   " << MP << "GLS_" << entityTypeAllCaps << "S___" << tag << "(OP) \\\n";
   }
   out << "\n"; 

   out <<
"#define " << MP << "GLS_" << entityTypeAllCaps << "S_CONFIGURE_LOCAL_ALL \\\n";
   for(const auto &abtEntry: entitiesByType) {
      const auto &tag = abtEntry.first;
      out <<
"   " << MP << "GLS_" << entityTypeAllCaps << "S___" << tag << "(" << MP << "GLS_" << entityTypeAllCaps << "_CONFIGURE_LOCAL___" << tag << ") \\\n";
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
   auto inputsByType = ParserTree::Input::getEntitiesByType(true /* only requested */);
   
   auto root = ParserTree::Pattern::getTreeRoot();
   
   ParserTree::InputAssignmentsByTag iabt;
   recursivelyCollectInputAssignments(iabt, *root);
   
   generateEntityInformation(
      out,
      inputsByType,
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
   auto actionsByType = ParserTree::Action::getEntitiesByType(true /* only requested */);
   
   auto root = ParserTree::Pattern::getTreeRoot();
   
   ParserTree::ActionAssignmentsByTag aabt;
   recursivelyCollectActionAssignments(aabt, *root);
   
   generateEntityInformation(
      out,
      actionsByType,
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
"// Tag class \'" << tag << "\'\n"
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
            const std::string &entityType)
{
   bool anyEntities = false;
   for(const auto &abtEntry: entitiesByType) {
      const auto &tag = abtEntry.first;
      
      bool anyUnusedOfCurrentType = false;
     
      for(const auto &entityPtr: abtEntry.second) {
         
         if(entityPtr->getWasRequested()) { continue; }
         
         if(!anyUnusedOfCurrentType) {
            if(!anyEntities) {
               caption(out, "Unused " + entityType);
               anyEntities = true;
            }
            out <<
"// Tag class \'" << tag << "\': ";
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
            const std::string &entityType)
{
   bool anyEntities = false;
   for(const auto &abtEntry: joinedEntities) {
      const auto &tag = abtEntry.first;
      
      bool anyUnusedOfCurrentType = false;
     
      for(const auto &entityPair: abtEntry.second) {
         
         if(!entityPair.first->getWasRequested()) { continue; }
         
         if(!anyUnusedOfCurrentType) {
            if(!anyEntities) {
               caption(out, "Joined " + entityType);
               anyEntities = true;
            }
            out <<
"// Tag class \'" << tag << "\': ";
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
   
   out <<
"#ifdef " << MP << "GLS_GLOBAL_INITIALIZATION_INCLUDE\n"
"#include " << MP << "GLS_GLOBAL_INITIALIZATION_INCLUDE\n"
"#endif\n"
"\n"
"#ifndef " << MP << "GLS_GLOBAL_INITIALIZATION\n"
"#define " << MP << "GLS_GLOBAL_INITIALIZATION\n"
"#endif\n"
"\n"
<< MP << "GLS_GLOBAL_INITIALIZATION\n"
"\n";

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
"PPG_Event_Queue_Entry " << SP << "event_buffer[" << 2*maxEvents << "] = { 0 };\n\n";

   out <<
"PPG_Furcation " << SP << "furcations[" << maxDepth << "] = { 0 };\n\n";
   
   out <<
"PPG_Token__ *" << SP << "tokens[" << maxDepth << "] = { 0 };\n\n";
   out <<
"PPG_Context " << SP << "context = {\n"
"   .event_buffer = {\n"
"      .events = " << SP << "event_buffer,\n"
"      .start = 0,\n"
"      .end = 0,\n"
"      .cur = 0,\n"
"      .size = 0,\n"
"      .max_size = " << 2*maxEvents << "\n";
   out <<
"   },\n"
"   .furcation_stack = {\n"
"      .furcations = " << SP << "furcations,\n"
"      .n_furcations = 0,\n"
"      .cur_furcation = 0,\n"
"      .max_furcations = " << maxDepth << "\n";
   out <<
"   },\n"
"   .active_tokens = {\n"
"      .tokens = " << SP << "tokens,\n"
"      .n_tokens = 0,\n"
"      .max_tokens = " << maxDepth << "\n";
   out <<
"   },\n"
"   .pattern_root = &" << SP << root->getId().getText() << ",\n"
"   .current_token = NULL,\n"
"   .properties = {\n"
"      .timeout_enabled = " << MP << "GLS_INITIAL_TIMEOUT_ENABLED,\n"
"      .papageno_enabled = " << MP << "GLS_INITIAL_PAPAGENO_ENABLED,\n"
"#     if PPG_HAVE_LOGGING\n"
"      .logging_enabled = " << MP << "GLS_INITIAL_LOGGING_ENABLED,\n"
"#     endif\n"
"      .destruction_enabled = false\n"
"    },\n"
"   .tree_depth = " << maxDepth << ",\n"
"   .layer = " << MP << "GLS_INITIAL_LAYER,\n"
"   .abort_trigger_input = " << MP << "GLS_INITIAL_ABORT_TRIGGER_INPUT,\n"
"   .time_last_event = 0,\n"
"   .event_timeout = " << MP << "GLS_INITIAL_EVENT_TIMEOUT,\n"
"   .event_processor = " << MP << "GLS_INITIAL_EVENT_PROCESSOR,\n"
"   .time_manager = {\n"
"      .time = &" << MP << "GLS_INITIAL_TIME_FUNCTION,\n"
"      .time_difference = &" << MP << "GLS_INITIAL_TIME_DIFFERENCE_FUNCTION,\n"
"      .compare_times = &" << MP << "GLS_INITIAL_TIME_COMPARISON_FUNCTION\n"
"   },\n"
"   .signal_callback = {\n"
"      .func = " << MP << "GLS_INITIAL_SIGNAL_CALLBACK_FUNC,\n"
"      .user_data = " << MP << "GLS_INITIAL_SIGNAL_CALLBACK_USER_DATA\n"
"   }\n"
"#  if PPG_HAVE_STATISTICS\n"
"   ,\n"
"   .statistics = {\n"
"      .n_nodes_visited = 0,\n"
"      .n_token_checks = 0,\n"
"      .n_furcations = 0,\n"
"      .n_reversions = 0\n"
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
   
//    outputFile <<
// "#line\n"
// "\n";
}

} // namespace ParserTree
} // namespace Glockenspiel
