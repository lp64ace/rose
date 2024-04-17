#include "DNA_sdna_types.h"

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
		if (auto* TD = Result.Nodes.getNodeAs<clang::TypedefDecl>("typedef")) {
			ASTContext& CTX = TD->getASTContext();

			if (!TD->getBeginLoc().isValid()) {
				/** Clang builtin types are annoying. */
				return;
			}

			auto Qual = TD->getUnderlyingType();
			if (Qual.isNull()) {
				return;
			}

			auto RD = Qual->getAsRecordDecl();
			if (!RD) {
				return;
			}

			auto D = RD->getDefinition();
			if (!D) {
				return;
			}

			std::string RecordName = RD->getNameAsString();
			if (DNA_sdna_has_type(DNA, RecordName.c_str())) {
				return;
			}

			DNAStruct* Struct = DNA_struct_new(DNA, RecordName.c_str());

			ROSE_assert(CTX.getTypeInfo(Qual).Width % 8 == 0);
			ROSE_assert(CTX.getTypeInfo(Qual).Align % 8 == 0);
			Struct->size = CTX.getTypeSize(Qual) / 8;
			Struct->align = CTX.getTypeAlign(Qual) / 8;

			for (auto* FD : RD->fields()) {
				QualType FieldQual = FD->getType();
				/** Bit fields are not allowed inside DNA. */
				ROSE_assert(CTX.getTypeInfo(FieldQual).Width % 8 == 0);
				ROSE_assert(CTX.getTypeInfo(FieldQual).Align % 8 == 0);
				size_t size = CTX.getTypeInfo(FieldQual).Width / 8;
				size_t align = CTX.getTypeInfo(FieldQual).Align / 8;
				ROSE_assert(CTX.getFieldOffset(FD) % 8 == 0);
				size_t offset = CTX.getFieldOffset(FD) / 8;

				DNAField* Field = DNA_field_new(Struct, FD->getNameAsString().c_str());

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
					const clang::ArrayType* ET = FieldQual->getAsArrayTypeUnsafe();
					while (ET->getElementType()->isArrayType()) {
						ET = ET->getElementType()->getAsArrayTypeUnsafe();
					}
					QualType ArrayElementQual = ET->getElementType();
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

private:
	ExecutionContext &Context;
	SDNA *DNA;
};

}  // end anonymous namespace

// Set up the command line options
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::OptionCategory ToolTemplateCategory("rose-dna options");

static cl::opt<std::string> DNAOutput("dna", cl::desc(R"(Specify the output file for rose DNA.)"), cl::init("dna.c"), cl::cat(ToolTemplateCategory));
static cl::opt<std::string> DNAOffsets("dna-offsets", cl::desc(R"(Specify the output file for DNA offsets.)"), cl::init("dna_offsets.h"), cl::cat(ToolTemplateCategory));

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
	std::string OffFile = DNAOffsets.getValue();

	DNA_sdna_compile(&DNA);

	int ExitStatus = 0;
	FILE* fout = fopen(DNAFile.c_str(), "w");
	if (fout) {
		fprintf(fout, "/**\n");
		fprintf(fout, " * Do not edit manually this is auto generated by #makesdna executable in llvmdna.cc.\n");
		fprintf(fout, " *\n");
		fprintf(fout, " * This file contains the compiled version of the SDNA data of the application based on \n");
		fprintf(fout, " * the current building architecture and platform.\n");
		fprintf(fout, " */\n");
		fprintf(fout, "\n");
		fprintf(fout, "extern const unsigned char DNAstr[];\n");
		fprintf(fout, "const unsigned char DNAstr[] = {");
		for (int i = 0; i < DNA.data_len; i++) {
			const unsigned char* data = (const unsigned char*)DNA.data;
			if ((i % 16 == 0)) {
				fprintf(fout, "\n\t");
			}
			fprintf(fout, "%3d, ", (int)data[i]);
		}
		fprintf(fout, "\n};\n");
		fprintf(fout, "extern const int DNAlen;\n");
		fprintf(fout, "const int DNAlen = sizeof(DNAstr);\n");
		fclose(fout);
	}
	else {
		std::cout << "Failed to open output DNA file." << std::endl;
		ExitStatus = -1;
	}

	fout = fopen(OffFile.c_str(), "w");
	if (fout) {
		fprintf(fout, "/**\n");
		fprintf(fout, " * Do not edit manually this is auto generated by #makesdna executable in llvmdna.cc.\n");
		fprintf(fout, " *\n");
		fprintf(fout, " * This file contains the compiled version of the SDNA data of the application based on \n");
		fprintf(fout, " * the current building architecture and platform.\n");
		fprintf(fout, " */\n");
		fprintf(fout, "\n");
		fprintf(fout, "#include \"LIB_assert.h\"\n");
		fprintf(fout, "#include \"LIB_utildefines.h\"\n");
		fprintf(fout, "\n");
		fprintf(fout, "#include \"dna_includes_all.h\"\n");
		fprintf(fout, "\n");
		for (struct DNAStruct* type = DNA.types; type != DNA.types + DNA.types_len; type++) {
			fprintf(fout, "ROSE_STATIC_ASSERT(sizeof(%s) == %d, \"DNA struct size verify\");\n", type->name, type->size);
			for (struct DNAField* field = type->fields; field != type->fields + type->fields_len; field++) {
				fprintf(fout, "ROSE_STATIC_ASSERT(offsetof(%s, %s) == %d, \"DNA member offset verify.\");\n", type->name, field->name, field->offset);
			}
			fprintf(fout, "\n");
		}
		fprintf(fout, "\n");
		fclose(fout);
	}
	else {
		std::cout << "Failed to open output DNA offsets file." << std::endl;
		ExitStatus = -1;
	}
	return ExitStatus;
}
