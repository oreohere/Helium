#include "helium/resolver/SnippetAction.h"
#include "helium/utils/fs_utils.h"

#include "helium/parser/source_location.h"

using namespace v2;
// using namespace clang;
// using namespace clang::tooling;
// using namespace llvm;

static std::vector<Snippet*> snippets;


SourceLocation convertLocation(clang::ASTContext *ctx, clang::SourceLocation loc) {
  clang::FullSourceLoc fullBeginLoc = ctx->getFullLoc(loc);
  int line = fullBeginLoc.getSpellingLineNumber();
  int column = fullBeginLoc.getSpellingColumnNumber();
  // int line = fullBeginLoc.getExpansionLineNumber();
  // int column = fullBeginLoc.getExpansionColumnNumber();
  return SourceLocation(line, column);
}

class SnippetVisitor
  : public clang::RecursiveASTVisitor<SnippetVisitor> {
public:
  explicit SnippetVisitor(clang::ASTContext *Context)
    : Context(Context) {
    clang::SourceManager &sourceManager = Context->getSourceManager();
    const clang::FileEntry *entry = sourceManager.getFileEntryForID(sourceManager.getMainFileID());
    Filename = entry->getName();
  }

  bool VisitFunctionDecl(clang::FunctionDecl *func_decl) {
    clang::FunctionDecl *def = func_decl->getDefinition();
    std::string name = func_decl->getNameInfo().getName().getAsString();
    // llvm::errs() << "visiting func " << name << "\n";
    // clang::SourceRange range = func_decl->getSourceRange();
    // clang::SourceLocation begin = range.getBegin();
    // clang::SourceLocation end = range.getEnd();
    clang::SourceLocation begin = func_decl->getLocStart();
    clang::SourceLocation end = func_decl->getLocEnd();
    // I would also extract the declaration
    // TESTME body is empty, then does it have a body?
    // TESTME the start include { or not?
    // TESTME it includes function prototype or not?
    v2::Snippet *s = nullptr;
    if (def == func_decl) {
      clang::SourceLocation body_begin = func_decl->getBody()->getLocStart();
      s = new v2::FunctionSnippet(name, Filename,
                                  convertLocation(Context, begin),
                                  convertLocation(Context, end),
                                  convertLocation(Context, body_begin));
      snippets.push_back(s);
      return true;
    } else {
      s = new v2::FunctionDeclSnippet(name, Filename,
                                      convertLocation(Context, begin),
                                      convertLocation(Context, end));
      snippets.push_back(s);
      return true;
    }
  }
  // bool WalkUpFromFunctionDecl(clang::FunctionDecl *func) {
  //   std::string name = func->getNameInfo().getName().getAsString();
  //   llvm::errs() << "work up from func " << name << "\n";
  //   // FIXME check if the function name is the same
  //   clang::FunctionDecl *def = func->getDefinition();
  //   if (def == func) {
  //     infunc = false;
  //   }
  //   VisitFunctionDecl(func);
  //   return true;
  // }
  // BINGO!! This can prevent the traverse to the children
  bool TraverseFunctionDecl(clang::FunctionDecl *func) {
    std::string name = func->getNameInfo().getName().getAsString();
    // llvm::errs() << "traverse func " << name << "\n";
    WalkUpFromFunctionDecl(func);
    // I'm not go into the function
    return true;
  }
  bool VisitVarDecl(clang::VarDecl *var_decl) {
    // FIXME global or file?
    if (!var_decl->isFileVarDecl()) return true;
    if (var_decl->hasExternalStorage() && !var_decl->hasInit()) return true;
    std::string name = var_decl->getName();
    // llvm::errs() << "visiting var " << name << "\n";
    // clang::SourceRange range = var_decl->getSourceRange();
    // clang::SourceLocation begin = range.getBegin();
    // clang::SourceLocation end = range.getEnd();
    clang::SourceLocation begin = var_decl->getLocStart();
    clang::SourceLocation end = var_decl->getLocEnd();
    // clang::SourceLocation loc = var_decl->getLocation();
      
    v2::Snippet *s = new v2::VarSnippet(name, Filename,
                                        convertLocation(Context, begin),
                                        convertLocation(Context, end));
    snippets.push_back(s);
    return true;
  }
  bool VisitTypedefDecl (clang::TypedefDecl *decl) {
    std::string name = decl->getName();
    // clang::SourceRange range = decl->getSourceRange();
    // clang::SourceLocation begin = range.getBegin();
    // clang::SourceLocation end = range.getEnd();
    clang::SourceLocation begin = decl->getLocStart();
    clang::SourceLocation end = decl->getLocEnd();
    v2::Snippet *s = new v2::TypedefSnippet(name, Filename,
                                            convertLocation(Context, begin),
                                            convertLocation(Context, end));
    snippets.push_back(s);
    return true;
  }
  bool VisitEnumDecl(clang::EnumDecl *decl) {
    clang::EnumDecl *def = decl->getDefinition();
    if (def == decl) {
      std::string name = decl->getName();
      // clang::SourceRange range = decl->getSourceRange();
      // clang::SourceLocation begin = range.getBegin();
      // clang::SourceLocation end = range.getEnd();
      clang::SourceLocation begin = decl->getLocStart();
      clang::SourceLocation end = decl->getLocEnd();
      v2::EnumSnippet *s = new v2::EnumSnippet(name, Filename,
                                               convertLocation(Context, begin),
                                               convertLocation(Context, end));
      // all fields
      for (auto it=decl->enumerator_begin(), end=decl->enumerator_end(); it!=end; ++it) {
        std::string name = (*it)->getName();
        // llvm::errs() << "Field: " << name << "\n";
        s->addField(name);
      }
      snippets.push_back(s);
    }
    return true;
  }
  // struct/union/class
  bool VisitRecordDecl (clang::RecordDecl *decl) {
    // if (infunc) return true;
    clang::RecordDecl *def = decl->getDefinition();
    std::string name = decl->getName();
    // llvm::errs() << "visiting record decl " << name << "\n";
    // clang::SourceRange range = decl->getSourceRange();
    // clang::SourceLocation begin = range.getBegin();
    // clang::SourceLocation end = range.getEnd();
    clang::SourceLocation begin = decl->getLocStart();
    clang::SourceLocation end = decl->getLocEnd();
    // name can be empty, this is an anonymous record. It must have a typedef or var to enclose it.
    v2::Snippet *s = nullptr;
    if (def == decl) {
      s = new v2::RecordSnippet(name, Filename,
                                convertLocation(Context, begin),
                                convertLocation(Context, end));
    } else {
      s = new v2::RecordDeclSnippet(name, Filename,
                                    convertLocation(Context, begin),
                                    convertLocation(Context, end));
    }
    snippets.push_back(s);
    return true;
  }

  bool VisitCallExpr (clang::CallExpr *call) {
    return true;
  }

private:
  clang::ASTContext *Context;
  std::string Filename;
};


class SnippetConsumer : public clang::ASTConsumer {
public:
  explicit SnippetConsumer(clang::ASTContext *Context)
    : Visitor(Context) {}
  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
private:
  SnippetVisitor Visitor;
};

class SnippetAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    // suppress compiler diagnostics
    Compiler.getDiagnostics().setClient(new clang::IgnoringDiagConsumer());
    return std::unique_ptr<clang::ASTConsumer>
      (new SnippetConsumer(&Compiler.getASTContext()));
  }
};

std::vector<Snippet*> createSnippets(fs::path file) {
  snippets.clear();
  clang::tooling::runToolOnCode(new SnippetAction,
                                utils::read_file(file.string()),
                                file.string());
  return snippets;
}
