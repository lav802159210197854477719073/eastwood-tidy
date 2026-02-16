
//===--- Rule8eCheck.cpp - clang-tidy -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Rule8eCheck.h"
#include <iostream>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace eastwood {
class Rule8ePPCallBack : public PPCallbacks {
private:
    Rule8eCheck *Check;
    const SourceManager &SM;

public:
    Rule8ePPCallBack(Rule8eCheck *Check, const SourceManager &SM)
        : Check(Check), SM(SM){};

    void InclusionDirective(SourceLocation HashLoc, const Token &IncludeTok,
                            StringRef FileName, bool isAngled,
                            CharSourceRange FilenameRange, OptionalFileEntryRef File,
                            StringRef SearchPath, StringRef RelativePath,
                            const Module *Imported, bool ModuleImported,
                            SrcMgr::CharacteristicKind FileType) override {
        if (this->SM.isWrittenInMainFile(HashLoc)) {
            unsigned cur_line_no = this->SM.getSpellingLineNumber(HashLoc);
            if (this->Check->incls.empty()) {
                this->Check->last_line_no = cur_line_no;
                this->Check->incls.push_back(std::make_pair(isAngled, FileName.str()));
            } else {
                unsigned expected_line_no = this->Check->last_line_no + 1;
                if (this->Check->incls.back().first != isAngled) {
                    expected_line_no++;
                }
                printf("%u %u %u\n", cur_line_no, this->Check->last_line_no,
                       expected_line_no);
                if (expected_line_no != cur_line_no) {
                    Check->diag(HashLoc,
                                "Expecting include to be on line %0 and not "
                                "line %1. Global and local includes blocks must "
                                "have one empty line separating them. There may not "
                                "be empty lines separating includes if they are both "
                                "globals or if they are both locals.")
                        << std::to_string(expected_line_no)
                        << std::to_string(cur_line_no);
                }
                this->Check->last_line_no = cur_line_no;
                if (this->Check->incls.size() > 1) {
                    if (not this->Check->incls.back().first && isAngled) {
                        Check->diag(HashLoc,
                                    "All global includes must preceed any local "
                                    "include, less the associated header.");
                    }
                    if (this->Check->incls.back().first == isAngled) {
                        // std::cout << "Checking" <<
                        // this->Check->incls.back().second << " vs " <<
                        // FileName.str() << " " <<
                        // (this->Check->incls.back().second >
                        // FileName.str()) << std::endl;
                        if (this->Check->incls.back().second > FileName.str()) {
                            Check->diag(HashLoc, "Includes should be ordered "
                                                 "lexicographically ascending.");
                        }
                    }
                }
                this->Check->incls.push_back(std::make_pair(isAngled, FileName.str()));
            }
        }
    }
};

Rule8eCheck::Rule8eCheck(StringRef Name, ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context), EastwoodTidyCheckBase(Name),
      debug_enabled(Options.get("debug", "false")) {
    if (this->debug_enabled == "true") {
        this->debug = true;
    }
}
void Rule8eCheck::registerPPCallbacks(const SourceManager &SM, Preprocessor *PP,
                                      Preprocessor *ModuleExpanderPP) {
    PP->addPPCallbacks(std::make_unique<Rule8ePPCallBack>(this, SM));
}

void Rule8eCheck::registerMatchers(MatchFinder *Finder) {}

void Rule8eCheck::check(const MatchFinder::MatchResult &Result) {}
} // namespace eastwood
} // namespace tidy
} // namespace clang
