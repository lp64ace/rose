#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Execution.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Refactoring/AtomicChange.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Signals.h"

#include "DNA_sdna_types.h"

#include "LIB_assert.h"
#include "LIB_utildefines.h"

#include "dna_utils.h"

#include <iostream>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

namespace {
	
class TypedefDeclCallback : public MatchFinder::MatchCallback {
public:
	TypedefDeclCallback(SDNA *DNA, ExecutionContext &Context) : Context(Context), DNA(DNA) {
	}

	void run(const MatchFinder::MatchResult &Result) override {
		if (auto *TD = Result.Nodes.getNodeAs<clang::TypedefDecl>("typedef")) {
			ASTContext &CTX = TD->getASTContext();

			QualType Qual = TD->getUnderlyingType();
			auto *RD = Qual->getAsRecordDecl();

			if (RD) {
				if (!RD->getBeginLoc().isValid()) {
					/** Clang builtin types are annoying. */
					return;
				}

				DNAStruct *Struct = DNA_struct_new(DNA, Qual.getAsString().c_str());

				/** Bit fields are not allowed inside DNA. */
				ROSE_assert(CTX.getTypeInfo(Qual).Width % 8 == 0);
				Struct->size = CTX.getTypeInfo(Qual).Width / 8;

				for (auto *FD : RD->fields()) {
					QualType FieldQual = FD->getType();
					/** Bit fields are not allowed inside DNA. */
					ROSE_assert(CTX.getTypeInfo(FieldQual).Width % 8 == 0);
					ROSE_assert(CTX.getTypeInfo(FieldQual).Align % 8 == 0);
					size_t size = CTX.getTypeInfo(FieldQual).Width / 8;
					size_t align = CTX.getTypeInfo(FieldQual).Align / 8;
					size_t offset = CTX.getFieldOffset(FD);

					DNAField *Field = DNA_field_new(Struct, FD->getNameAsString().c_str());

					Field->size = size;
					Field->align = align;
					Field->offset = offset;

					/** Conventional so that single items can be multiplied. */
					Field->array = 1;

					if (FieldQual->isPointerType()) {
						Field->flags |= DNA_FIELD_IS_POINTER;
					}
					if (FieldQual->isFunctionPointerType()) {
						Field->flags |= DNA_FIELD_IS_FUNCTION;
					}

					if (FieldQual->isPointerType() || FieldQual->isFunctionPointerType()) {
						/** This should be treated as a pointer. */
						QualType PointeeQual = FieldQual->getPointeeType();
						std::string tp = PointeeQual.getAsString();
						strncpy(Field->type, tp.c_str(), sizeof(Field->type));
					}
					else if (FieldQual->isArrayType()) {
						/** This should be treated as an array. */

						/** Find the simplest element type of arrays. */
						const clang::ArrayType *AT = FieldQual->getAsArrayTypeUnsafe();
						while (AT->getElementType()->isArrayType()) {
							AT = AT->getElementType()->getAsArrayTypeUnsafe();
						}
						QualType ArrayElementQual = AT->getElementType();
						size_t elem_size = CTX.getTypeInfo(ArrayElementQual).Width / 8;

						Field->array = size / elem_size;

						if (ArrayElementQual->isPointerType()) {
							Field->flags |= DNA_FIELD_IS_POINTER;

							QualType PointeeQual = ArrayElementQual->getPointeeType();
							std::string tp = PointeeQual.getAsString();
							strncpy(Field->type, tp.c_str(), sizeof(Field->type));
						}
						else {
							std::string tp = ArrayElementQual.getAsString();
							strncpy(Field->type, tp.c_str(), sizeof(Field->type));
						}
					}
					else {
						/** Treat as a normal buffer of bytes. */
						std::string tp = FieldQual.getAsString();
						strncpy(Field->type, tp.c_str(), sizeof(Field->type));
					}
				}
			}
		}
	}

private:
	ExecutionContext &Context;
	SDNA *DNA;
};

}  // end anonymous namespace

// Set up the command line options
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::OptionCategory ToolTemplateCategory("rose-dna options");

static cl::opt<std::string> DNAOutput("dna", cl::desc(R"(Specify the output file for rose DNA.)"), cl::init("clang-rose.dna"), cl::cat(ToolTemplateCategory));

int main(int argc, const char **argv) {
	llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);

	auto Executor = clang::tooling::createExecutorFromCommandLineArgs(argc, argv, ToolTemplateCategory);

	if (!Executor) {
		llvm::errs() << llvm::toString(Executor.takeError()) << "\n";
		return 1;
	}

	SDNA DNA;
	memset(&DNA, 0, sizeof(SDNA));

	ast_matchers::MatchFinder Finder;
	TypedefDeclCallback Callback(&DNA, *Executor->get()->getExecutionContext());
	Finder.addMatcher(typedefDecl().bind("typedef"), &Callback);

	auto Err = Executor->get()->execute(newFrontendActionFactory(&Finder));
	if (Err) {
		llvm::errs() << llvm::toString(std::move(Err)) << "\n";
	}

	std::string DNAFile = DNAOutput.getValue();

	int ExitStatus = 0;
#ifdef WIN32
	FILE *fout = fopen(DNAFile.c_str(), "wb");
#else
	FILE *fout = fopen(DNAFile.c_str(), "w");
#endif
	if (fout) {
		DNA_sdna_compile(&DNA);
		if (fwrite(DNA.data, DNA.data_len, 1, fout) != 1) {
			std::cout << "Failed to write output DNA file." << std::endl;
			ExitStatus = -2;
		}
		fclose(fout);
	}
	else {
		std::cout << "Failed to open output DNA file." << std::endl;
		ExitStatus = -1;
	}
	return ExitStatus;
}
