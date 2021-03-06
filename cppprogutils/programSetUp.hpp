#pragma once
//
//  programSetUp.hpp
//  sequenceTools
//
//  Created by Nicholas Hathaway on 9/27/13.
//  Copyright (c) 2013 Nicholas Hathaway. All rights reserved.
//

#include "cppprogutils/utils.h"
#include "cppprogutils/commandLineArguments.hpp"
#include "cppprogutils/runLog.hpp"
#include "cppprogutils/common.hpp"
#include "cppprogutils/flag.hpp"

namespace cppprogutils {

/**@brief Class for helping set up current program with flag parsing, logging run
 *times, getting help messages
 *
 */
class programSetUp {

public:
	// constructors
	/**@brief Construct the setUp with the generic argc and argv
	 *
	 * @param argc The number of arguments
	 * @param argv The array of char pointers of the arguments
	 *
	 */
	programSetUp(int argc, char *argv[]) :
			commands_(commandLineArguments(argc, argv)) {
		commands_.arguments_["-program"] = argv[0];
		// get rid of the ./ if program is being called from current dir, it can
		// mess things up latter
		programName_ = replaceString(argv[0], "./", "");
		init();
	}

	/**@brief Construct the setUp with the commandLineArguments class converted
	 *from argc and argv
	 *
	 * @param inputCommands The commandLineArguments object likely converted from
	 *argc and argv
	 *
	 */
	programSetUp(const commandLineArguments &inputCommands) :
			commands_(inputCommands) {
		auto progSearch = commands_.arguments_.find("-program");
		// if no program name (though there really should be) just call it program
		if (commands_.arguments_.end() == progSearch) {
			programName_ = "program";
		} else {
			programName_ = progSearch->second;
		}
		init();
	}

	/**@brief Construct the setUp with a map of string pairs converted from argc
	 *and argv
	 *
	 * @param inputCommands A map of string pairs object likely converted from argc
	 *and argv
	 *
	 */
	programSetUp(const std::map<std::string, std::string> &inputCommands) :
			commands_(commandLineArguments(inputCommands)) {
		auto progSearch = commands_.arguments_.find("-program");
		// if no program name (though there really should be) just call it program
		if (commands_.arguments_.end() == progSearch) {
			programName_ = "program";
		} else {
			programName_ = progSearch->second;
		}
		init();
	}
	/**@brief Initialize any defaults
	 *
	 */
	void init() {
		if (commands_.containsFlagCaseInsensitive("-getFlags")
				|| commands_.containsFlagCaseInsensitive("-flags")
				|| commands_.containsFlagCaseInsensitive("-h")
				|| commands_.containsFlagCaseInsensitive("-help")) {
			failed_ = true;
		}
		if (commands_.containsFlagCaseInsensitive("-h")
				|| commands_.containsFlagCaseInsensitive("-help")) {
			printingHelp_ = true;
		}
		if (commands_.containsFlagCaseInsensitive("-getFlags")
				|| commands_.containsFlagCaseInsensitive("-flags")) {
			gettingFlags_ = true;
		}
	}

	virtual ~programSetUp() {
	}

	/**@brief A commandLineArguments class to store the objects and help with flag
	 *parsing, see commandLineArguments for more details
	 *
	 */
	commandLineArguments commands_;

	/**@brief A class using the std::chrono library to log infomration about run times
	 *
	 *
	 */
	stopWatch timer_;

protected:

	/**@brief A vector of strings of warnings to be printed at the end of setUp
	 *
	 */
	std::vector<std::string> warnings_;

public:
	/**@brief A bool to indicated whether the set up has failed somewhere
	 *
	 */
	bool failed_ = false;

	/**@brief A bool to indicated whether a help statement was requested
	 *
	 */
	bool printingHelp_ = false;

	/**@brief A bool to indicated whether simply flags were requested
	 *
	 */
	bool gettingFlags_ = false;

	/**@brief The maximum width to allow for messages
	 *
	 */
	uint32_t width_ = 80;

	/**@brief The indent amount for wrapping messages
	 *
	 */
	uint32_t indent_ = 4;

	/**@brief A container for parameters set during set up, see parametersHolder
	 *for more details
	 *
	 */
	flagHolder flags_;

	/**@brief The name of the program associated with the current setUp object
	 *
	 */
	std::string programName_;

	/**@brief A runLog object for optional logging of run time info, won't be
	 *started unless programSetUp::startARunLog is called
	 *
	 */
	runLog rLog_;

	/**@brief A function to start a runLog in the named directory
	 * @param dirName The name of the directory to start the runLog, runLog name
	 *will be runLog_[NAME_OF_PROGRAM]
	 *
	 */
	void startARunLog(const std::string &dirName) {
		rLog_.setFilenameAndOpen(
				dirName + "runLog_" + replaceString(programName_, "./", "") + ".txt",
				timer_.start_);
		rLog_.startRunLog(commands_.arguments_);
	}

	/**@brief A function to write info on the parameters set during the set up
	 * process
	 * @param fileName The name of the parameter
	 * @param overWrite A bool indicating whether to overwrite any file with that
	 * name
	 * @param failOnWriteFailure A bool whether to exit if the parameters file
	 * couldn't be written
	 */
	void writeParametersFile(const std::string &fileName, bool overWrite,
			bool failOnWriteFailure) {
		std::ofstream parameterFile;
		std::ostream out(determineOutBuf(parameterFile, fileName, ".txt", overWrite,
				false,failOnWriteFailure));
		//parameterFile << commands_.commandLine_ << std::endl;
		flags_.outputParsFile(out);
	}

	/**@brief Called at the end of set up to look for any invalid options but
	 *comparing current arguments to validOptions_
	 *
	 */
	void lookForInvalidOptions() {
		std::vector<std::string> currentFlags = getVecOfMapKeys(flags_.flags_);
		currentFlags.push_back("-program");
		if (commands_.arguments_.find("-program") != commands_.arguments_.end()) {
			currentFlags.push_back(commands_["-program"]);
		}
		currentFlags.push_back("-commandline");
		strVecToLower(currentFlags);
		std::vector<std::string> expandedOptions;
		for (const auto &op : currentFlags) {
			addOtherVec(expandedOptions, tokenizeString(op, ","));
		}
		for (const auto &com : commands_.arguments_) {
			if (!contains(expandedOptions, com.first)) {
				warnings_.emplace_back("Unrecognized option, " + com.first
								+ " not using");
			}
		}
	}

	/**@brief Print any warnings incurred during set up to out
	 * @param out The std::ostream out object to print to
	 *
	 */
	void printWarnings(std::ostream &out) {
		for (const auto &warn : warnings_) {
			out << warn << std::endl;
		}
	}

	/**@brief Print flags but looking at validOptions_ so needs to be called at
	 * the end of set up so that options can be placed into validOptions_
	 * @param out The std::ostream out object to print to
	 */
	void printFlags(std::ostream &out) {
		std::map<std::string, std::string> infoOutRequried;
		std::map<std::string, std::string> infoOutNotRequried;
		for (const auto & f : flags_.flags_) {
			if (f.second.required_) {
				infoOutRequried[f.first] = f.second.helpInfo();
			} else {
				infoOutNotRequried[f.first] = f.second.helpInfo();
			}
		}
		std::map<std::string, std::string> infoOutHelp;
		infoOutHelp["-flags,-getFlags"] = "Print flags";
		infoOutHelp["-h,-help"] = "Print a more detail help message if available";
		std::pair<uint32_t, uint32_t> paddings { 0, 0 };
		auto keyFunc = [](const std::pair<std::string, std::string> & kv) {return kv.first;};
		auto valFunc =
				[](const std::pair<std::string, std::string> & kv) {return bashCT::trimForNonTerminalOut(kv.second);};
		if (!infoOutRequried.empty()) {
			compareColPaddings(infoOutRequried, paddings, keyFunc, valFunc);
		}
		if (!infoOutNotRequried.empty()) {
			compareColPaddings(infoOutNotRequried, paddings, keyFunc, valFunc);
		}
		if (!infoOutHelp.empty()) {
			compareColPaddings(infoOutHelp, paddings, keyFunc, valFunc);
		}
		const std::string middleSep = "\t";
		//assuming tab is len of 4
		std::string requiredOptsTitle = bashCT::bold + "Required Options"
				+ bashCT::reset;
		std::string optionalOptsTitle = bashCT::bold + "Optional Options"
				+ bashCT::reset;
		std::string helpOptsTitle = bashCT::bold + "Help Options" + bashCT::reset;
		if (!infoOutRequried.empty()) {
			out
					<< std::string(
							paddings.first + 2
									- round(bashCT::getPrintLen(requiredOptsTitle) / 2.0), ' ')
					<< requiredOptsTitle << std::endl;
			mapOutColAdjust(infoOutRequried, out, middleSep, paddings.first,
					paddings.second);
		}
		if (!infoOutNotRequried.empty()) {
			out
					<< std::string(
							paddings.first + 2
									- round(bashCT::getPrintLen(optionalOptsTitle) / 2.0), ' ')
					<< optionalOptsTitle << std::endl;
			mapOutColAdjust(infoOutNotRequried, out, middleSep, paddings.first,
					paddings.second);
		}
		if (!infoOutHelp.empty()) {
			out
					<< std::string(
							paddings.first + 2
									- round(bashCT::getPrintLen(helpOptsTitle) / 2.0), ' ')
					<< helpOptsTitle << std::endl;
			mapOutColAdjust(infoOutHelp, out, middleSep, paddings.first,
					paddings.second);
		}
	}

	/**@brief Once all options have been looked for call finishSetUp() so that any
	 *warnings or help messages can be print and to find out if set up was
	 *successful
	 * @param out The std::ostream out object to print to
	 *
	 */
	void finishSetUp(std::ostream &out) {
		if (commands_.containsFlagCaseInsensitive("-getFlags")
				|| commands_.containsFlagCaseInsensitive("-flags")
				|| commands_.containsFlagCaseInsensitive("-h")
				|| commands_.containsFlagCaseInsensitive("-help")) {
			printFlags(out);
			exit(1);
		}
		lookForInvalidOptions();
		printWarnings(out);
		if (failed_) {
			exit(1);
		}
	}

	/**@brief Look for option and set the bool option to false if flag is found
	 * @param option The bool option to set
	 * @param flag The flag to be searched for
	 * @param parName The name under which to store the option being searched for
	 * @param required A bool indicating if the option is required and set up
	 *should be stopped if not found, default is false
	 *
	 * @return Returns true if option is found or false if option is not found
	 *
	 */
	bool setBoolOptionFalse(bool &option, std::string flagStr,
			const std::string &shortDescription, bool required = false) {
		flag currentFlag = flag(option, flagStr, shortDescription, required);
		bool found = false;
		for (const auto &t : currentFlag.flags_) {
			if (commands_.lookForOptionFalse(option, t)) {
				currentFlag.setValue(option);
				found = true;
				break;
			} else {
				found = false;
			}
		}
		if (required && !found) {
			std::stringstream tempStream;
			tempStream << bashCT::bold + bashCT::black << "Need to have "
					<< bashCT::red << vectorToString(tokenizeString(flagStr, ","), " or ")
					<< bashCT::black << " see " << bashCT::red
					<< commands_["-program"] + " -help " << bashCT::black
					<< "for more details" << bashCT::reset;
			warnings_.emplace_back(tempStream.str());
			failed_ = true;
		}
		flags_.addFlag(currentFlag);
		return found;
	}

	/**@brief A templated function to look for options and implementation for
	 *setting the option is handled by commandLineArguments so look there for more
	 *details
	 * @param option The option to set
	 * @param flag The flag to be searched for
	 * @param parName The name under which to store the option being searched for
	 * @param required A bool indicating if the option is required and set up
	 *should be stopped if not found, default is false
	 *
	 * @return Returns true if option is found or false if option is not found
	 *
	 */
	template<typename T>
	bool setOption(T &option, std::string flagStr,
			const std::string &shortDescription, bool required = false) {
		flag currentFlag = flag(option, flagStr, shortDescription, required);
		bool found = false;
		for (const auto &fTok : currentFlag.flags_) {
			if (commands_.lookForOption(option, fTok)) {
				currentFlag.setValue(option);
				found = true;
				break;
			} else {
				found = false;
			}
		}

		if (required && !found) {
			std::stringstream tempStream;
			tempStream << bashCT::bold + bashCT::black << "Need to have "
					<< bashCT::red << vectorToString(tokenizeString(flagStr, ","), " or ")
					<< bashCT::black << " see " << bashCT::red
					<< commands_["-program"] + " -help " << bashCT::black
					<< "for more details" << bashCT::reset;
			warnings_.emplace_back(tempStream.str());
			failed_ = true;
		}
		flags_.addFlag(currentFlag);
		return found;
	}

	/**@brief Get the current run time since time point start_ in seconds
	 * @return A double with the run time converted into seconds
	 *
	 */
	double getRunTimeInSecs() {
		return timer_.totalTime();
	}

	/**@brief Get a string formated run time in seconds rounded to the nearest
	 * hundreth
	 * @return A string wiht run time formated to tell time for hrs, minutes,
	 * seconds etc.
	 */
	std::string getRunTime() {
		return programName_ + " (" + timer_.totalTimeFormatted(2) + ")";
	}

	/**@brief Function to log the run time to a std::ostream
	 * @param out The std::ostream to log the info to
	 */
	void logRunTime(std::ostream &out) {
		out << getRunTime() << std::endl;
		return;
	}

	/**@brief A function to find out if help is need or required, called with a
	 * minimum arguments parameter to indicated the num of args to print out if no
	 * help flag is indicated
	 * @param minAmountOfArgs Number of arguments to print help, defualts to one
	 * so if only the program name is given it indicates help is needed
	 * @return return true if a help message should be printed or false if no help
	 * is needed
	 */
	bool needsHelp(uint32_t minAmountOfArgs = 1) {
		return (commands_.containsFlagCaseInsensitive("-help")
				|| commands_.containsFlagCaseInsensitive("-h")
				|| commands_.numberOfCommands() <= minAmountOfArgs);
	}

	/**@brief A Function to print out all the current arguments in an organized
	 *table
	 * @param commandLineArgumentsMap A map of strings pairs of the current
	 *arguments
	 * @param out The std::ostream object to print the info to
	 *
	 */
	void writeOutCommandLineArguments(
			const std::map<std::string, std::string> &commandLineArgumentsMap,
			std::ostream &out) {
		int optionCount = 0;
		std::vector<std::string> columnNames = { "OptionNum", "Flag", "Option" };
		std::vector<std::vector<std::string>> content;
		for (const auto &mContent : commandLineArgumentsMap) {
			if (mContent.first != "-commandline" && mContent.first != "-program") {
				content.emplace_back(
						std::vector<std::string> { to_string(optionCount) + ")",
								mContent.first, mContent.second });
				++optionCount;
			}
		}
		printTableOrganized(content, columnNames, out);
	}
};

}  // namespace cppprogutils
