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

#include "Generator/PPG_Global.hpp"

#include "ParserTree/PPG_Token.hpp"
#include "ParserTree/PPG_Action.hpp"
#include "ParserTree/PPG_Input.hpp"
#include "ParserTree/PPG_Pattern.hpp"

#include "CommandLine/PPG_CommandLine.hpp"

#include "PPG_Compiler.hpp"

#include <ostream>
#include <fstream>

extern struct gengetopt_args_info ai;

namespace Papageno {
namespace Generator {
   
void generateFileHeader(std::ostream &out) {
   
   out <<
"// Generated by Papageno compiler version " << PAPAGENO_VERSION << "\n";
   out <<
"\n";
   out <<
"#include \"detail/ppg_context_detail.h\"\n"
"#include \"detail/ppg_note_detail.h\"\n"
"#include \"detail/ppg_chord_detail.h\"\n"
"#include \"detail/ppg_cluster_detail.h\"\n"
"\n";

   if(ai.preamble_filename_arg) {
      out <<
"#include \"" << ai.preamble_filename_arg << "\"\n";
      out <<
"\n";
   }
   
   out <<
"#ifdef PAPAGENO_PREAMBLE_HEADER\n"
"#include PAPAGENO_PREAMBLE_HEADER\n"
"#endif\n\n";
   
  out <<
"#define NUM_BITS_LEFT(N_BITS) \\\n"
"   (N_BITS%%(8*sizeof(PPG_Bitfield_Storage_Type)))\n\n";
      
   out << 
"#define NUM_BYTES(N_BITS) \\\n"
"   (N_BITS/(8*sizeof(PPG_Bitfield_Storage_Type)))\n\n";
}
   
void caption(std::ostream &out, const std::string &title)
{
   out <<
"//##############################################################################\n"
"// " << title << "\n"
"//##############################################################################\n"
"\n";
}
   
void generateGlobalActionInformation(std::ostream &out)
{
   auto actionsByType = ParserTree::Action::getActionsByType();
   
   caption(out, "Actions");

   for(const auto &abtEntry: actionsByType) {
      const auto &tag = abtEntry.first;
      
      out << 
"#ifndef PPG_ACTION_MAP_" << tag << "\n";
      out <<
"#define PPG_ACTION_MAP_" << tag << "(...) __VA_ARGS__\n";
      out <<
"#endif\n"
"\n";
   
      out <<
"#ifndef PPG_ACTION_INITIALIZATION_" << tag << "\n";
      out << 
"#define PPG_ACTION_INITIALIZATION_" << tag << "(...) __VA_ARGS__\n";
      out <<
"#endif\n"
"\n";

      out <<
"#define PPG_ACTIONS___" << tag << "(OP) \\\n";

      for(const auto &actionPtr: abtEntry.second) {
         out <<
"   OP(" << actionPtr->getId().getText() << ")\\\n";
      }
      
      out <<
"\n";
   }
   
   out <<
"#define PPG_ACTIONS_ALL(OP) \\\n";

   for(const auto &abtEntry: actionsByType) {
      const auto &tag = abtEntry.first;
      out <<
"   PPG_ACTIONS___" << tag << "(OP) \\\n";
   }
   out << "\n";
}

void generateGlobalInputInformation(std::ostream &out)
{
   auto inputsByType = ParserTree::Input::getInputsByType();
   
   caption(out, "Inputs");
   
   for(const auto &abtEntry: inputsByType) {
      const auto &tag = abtEntry.first;
      
      out << 
"#ifndef PPG_INPUT_MAP_" << tag << "\n";
      out <<
"#define PPG_INPUT_MAP_" << tag << "(...) __VA_ARGS__\n";
      out <<
"#endif\n"
"\n";
   
      out <<
"#ifndef PPG_INPUT_INITIALIZATION_" << tag << "\n";
      out << 
"#define PPG_INPUT_INITIALIZATION_" << tag << "(...) __VA_ARGS__\n";
      out <<
"#endif\n"
"\n";

      out <<
"#define PPG_INPUTS___" << tag << "(OP) \\\n";

      for(const auto &inputPtr: abtEntry.second) {
         out <<
"   OP(" << inputPtr->getId().getText() << ")\\\n";
      }
      
      out <<
"\n";

   out <<
"#define PPG_INPUTS_ALL(OP) \\\n";

   for(const auto &abtEntry: inputsByType) {
      const auto &tag = abtEntry.first;
      out <<
"   PPG_INPUTS___" << tag << "(OP) \\\n";
   }
   out << "\n";
   }
}

static void outputInformationOfDefinition(std::ostream &out, const ParserTree::Node &node)
{
   const auto &lod = node.getLOD();
   
   if(lod) {
      out <<
"#line " << lod.location_.first_line << " \"" << lod.file_ << "\"\n";
   }

   out <<
"// " << lod << "\n";
   out <<
"//\n";
}

void outputToken(std::ostream &out, const ParserTree::Token &token)
{
   for(const auto &childTokenPtr: token.getChildren()) {
      outputToken(out, *childTokenPtr);
   }
   
   outputInformationOfDefinition(out, token);
   token.generateCCode(out);
}

void generateGlobalContext(std::ostream &out)
{
   // Output all actions
   //
   for(const auto &actionEntry: ParserTree::Action::getActions()) {
      
      const auto &action = *actionEntry.second;
      
      outputInformationOfDefinition(out, action);
      
      out <<
"PPG_Action " << action.getId().getText() << " = PPG_ACTION_INITIALIZATION_" << action.getType().getText()
   << "(" << action.getParameters().getText() << ");\n\n";
   }
   
   // Output all inputs
   //
   for(const auto &inputEntry: ParserTree::Input::getInputs()) {
      
      const auto &input = *inputEntry.second;
      
      outputInformationOfDefinition(out, input);
      
      out <<
"PPG_Input " << input.getId().getText() << " = PPG_INPUT_INITIALIZATION_" << input.getType().getText()
   << "(" << input.getParameters().getText() << ");\n\n";
   }
   
   caption(out, "Token tree");
   
   auto root = ParserTree::Pattern::getTreeRoot();
   
   // Recursively output token tree
   //
   outputToken(out, *root);
   
   caption(out, "Context");
   
   out <<
"PPG_Context context = (PPG_Context) {\n"
"   .pattern_root = &" << root->getId().getText() << "\n"
"};\n\n";
}
   
void generateInitializationFunction(std::ostream &out)
{
   caption(out, "Initialization");
   
   out <<
"void papageno_initialize_context()\n"
"{\n";
   out <<
"   ppg_global_initialize_context_static_tree(&context);\n"
"   ppg_context = &context;\n"
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
}

} // namespace ParserTree
} // namespace Papageno
