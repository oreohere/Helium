#include <gtest/gtest.h>

#include "helium/resolver/snippet.h"
#include "helium/resolver/snippet_db.h"
#include "helium/utils/fs_utils.h"

#include "helium/resolver/cache.h"
#include "helium/resolver/SnippetAction.h"
#include "helium/resolver/SnippetV2.h"


#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

namespace fs = boost::filesystem;

using namespace std;



const char *snippet_a_c = R"prefix(
int global_a;
struct TempA {
} temp_a;

int aoo() {
  struct A1 a;
  struct B b;
  foo();
}
)prefix";

const char *snippet_a_h = R"prefix(
struct A1 {
};
struct A2 {
  struct A1;
};

typedef struct A2 A2;
typedef struct A3 {
  struct A2 a2;
} A3Alias;
)prefix";

const char *snippet_b_c = R"prefix(
void foo() {
  bar();
  return;
}
void bar() {
  return;
}
)prefix";

const char *snippet_b_h = R"prefix(
void foo();
void bar();
struct B {
  int b;
};
)prefix";

const char *snippet_sub_c = R"prefix(
void subc() {
  foo();
  bar();
}
)prefix";

const char *snippet_lib_c = R"prefix(
void libc() {
  foo();
}
)prefix";

class SnippetTest : public ::testing::Test {
public:
  virtual void SetUp() {
    // create folder
    fs::path temp_dir = fs::temp_directory_path();
    unique_dir = fs::unique_path(temp_dir / "%%%%-%%%%-%%%%-%%%%");
    fs::create_directories(unique_dir);
    fs::create_directories(unique_dir / "src");
    fs::create_directories(unique_dir / "include");
    fs::create_directories(unique_dir / "lib");
    fs::create_directories(unique_dir / "src" / "sub");

    fs::path file_a_c = unique_dir / "src" / "a.c";
    fs::path file_a_h = unique_dir / "src" / "a.h";
    fs::path file_b_c = unique_dir / "src" / "b.c";
    fs::path file_b_h = unique_dir / "include" / "b.h";
    fs::path file_sub_c = unique_dir / "src" / "sub" / "sub.c";
    fs::path file_lib_c = unique_dir / "lib" / "lib.c";

    // write source file
    fs::ofstream of;
    utils::write_file(file_a_c.string(), snippet_a_c);
    utils::write_file(file_a_h.string(), snippet_a_h);
    utils::write_file(file_b_c.string(), snippet_b_c);
    utils::write_file(file_b_h.string(), snippet_b_h);
    utils::write_file(file_sub_c.string(), snippet_sub_c);
    utils::write_file(file_lib_c.string(), snippet_lib_c);
     

    // Create cache
    std::string target_dir_name = fs::canonical(unique_dir).string();
    std::replace(target_dir_name.begin(), target_dir_name.end(), '/', '_');
    fs::path user_home(getenv("HOME"));
    fs::path helium_home = user_home / ".helium.d";
    if (!fs::exists(helium_home)) {
      fs::create_directory(helium_home);
    }
    fs::path cache_dir = helium_home / "cache";
    target_cache_dir = fs::path(helium_home / "cache" / target_dir_name);
    fs::path target_sel_dir = fs::path(helium_home / "sel" / target_dir_name);
    // 1. preprocess
    fs::create_directories(target_cache_dir);
    create_src(unique_dir, target_cache_dir, target_sel_dir);
    create_cpp(target_cache_dir);
    
    std::cout << "Created " << unique_dir.string() << "\n";

    // Check if entries are in database
    SnippetDB::Instance()->Load(target_cache_dir / "snippet.db", target_cache_dir / "code");

    {
      set<int> ids = SnippetDB::Instance()->LookUp("A");
      ASSERT_EQ(ids.size(), 1);
      int id = *ids.begin();
      SnippetMeta meta = SnippetDB::Instance()->GetMeta(id);
      std::cout << "===== A:" << "\n";
      meta.dump(std::cout);
      std::cout << "===== Deps:" << "\n";
      std::string code = SnippetDB::Instance()->GetCode(id);
      set<int> deps = SnippetDB::Instance()->GetDep(id);
      for (int dep : deps) {
        SnippetMeta dep_meta = SnippetDB::Instance()->GetMeta(dep);
        dep_meta.dump(std::cout);
      }
      std::cout << "Code: " << code << "\n";
    }
  } 
  virtual void TearDown() {
    // TODO remove from cache
    // fs::remove_all(target_cache_dir);
    // remove it
    // fs::remove_all(unique_dir);
  }
private:
  fs::path unique_dir;
  fs::path target_cache_dir;
};

// TEST_F(SnippetTest, MySnippetTest) {
// }



class NewSnippetTest : public ::testing::Test {
protected:
  void SetUp() {
    fs::path user_home = getenv("HOME");
    fs::path dir = user_home / "github" / "benchmark" / "craft" / "snippet";
    file_a_c = dir / "src" / "a.c";
    file_a_h = dir / "src" / "a.h";
    file_var_c = dir / "src" / "var.c";
    file_b_c = dir / "src" / "b.c";
    file_b_h = dir / "include" / "b.h";
    file_sub_c = dir / "src" / "sub" / "sub.c";
    file_lib_c = dir / "lib" / "lib.c";
    file_decl_h = dir / "src" / "decl.h";
  }
  void TearDown() {}

  fs::path file_a_c;
  fs::path file_a_h;
  fs::path file_b_c;
  fs::path file_b_h;
  fs::path file_sub_c;
  fs::path file_lib_c;
  fs::path file_var_c;
  fs::path file_decl_h;
};


TEST_F(NewSnippetTest, MyTest) {
  {
    // std::cout << file_a_h.string() << "\n";
    std::vector<v2::Snippet*> snippets = createSnippets(file_a_h);
    v2::SnippetManager *manager = new v2::SnippetManager();
    manager->add(snippets);
    manager->process();
    // manager->dump(std::cout);
    v2::Snippet *A1 = manager->getone("A1", "RecordSnippet");
    v2::Snippet *A2 = manager->getone("A2", "RecordSnippet");
    v2::Snippet *A2Typedef = manager->getone("A2", "TypedefSnippet");
    v2::Snippet *A3 = manager->getone("A3", "RecordSnippet");
    v2::Snippet *A3Alias = manager->getone("A3Alias", "TypedefSnippet");
    ASSERT_TRUE(A1);
    ASSERT_TRUE(A2);
    ASSERT_TRUE(A3);
    ASSERT_TRUE(A3Alias);

    EXPECT_EQ(A1->getCode(), "struct A1 {\n}");
    EXPECT_EQ(A2->getCode(), "struct A2 {\n  struct A1 a;\n}");
    EXPECT_EQ(A2Typedef->getCode(), "typedef struct A2 A2");
    EXPECT_EQ(A3->getCode(), "struct A3 {\n  struct A2 a2;\n}");
    EXPECT_EQ(A3Alias->getCode(), "typedef struct A3 {\n  struct A2 a2;\n} A3Alias");
  }
  {
    std::vector<v2::Snippet*> snippets = createSnippets(file_a_c);
    v2::SnippetManager *manager = new v2::SnippetManager();
    manager->add(snippets);
    manager->process();

    v2::Snippet *global_a = manager->getone("global_a", "VarSnippet");
    v2::Snippet *temp_a = manager->getone("temp_a", "VarSnippet");
    v2::Snippet *myvar = manager->getone("myvar", "VarSnippet");
    v2::Snippet *aoo = manager->getone("aoo", "FunctionSnippet");
    ASSERT_TRUE(global_a);
    ASSERT_TRUE(temp_a);
    ASSERT_TRUE(myvar);
    ASSERT_TRUE(aoo);

    EXPECT_EQ(global_a->getCode(), "int global_a");
    // FIXME
    EXPECT_EQ(temp_a->getCode(), "struct TempA {\n} temp_a");
    EXPECT_EQ(myvar->getCode(), "struct ABC {\n  int a;\n} myvar");
    EXPECT_EQ(aoo->getCode(), "int aoo() {\n  struct A1 a;\n  struct B b;\n  foo();\n}");
  }
  {
    std::vector<v2::Snippet*> snippets = createSnippets(file_var_c);
    v2::SnippetManager *manager = new v2::SnippetManager();
    manager->add(snippets);
    manager->process();

    v2::Snippet *a = manager->getone("a", "VarSnippet");
    v2::Snippet *b = manager->getone("b", "VarSnippet");
    v2::Snippet *c = manager->getone("c", "VarSnippet");
    v2::Snippet *d = manager->getone("d", "VarSnippet");
    v2::Snippet *e = manager->getone("e", "VarSnippet");
    v2::Snippet *f = manager->getone("f", "VarSnippet");
    v2::Snippet *gg = manager->getone("gg", "VarSnippet");
    v2::Snippet *p = manager->getone("p", "VarSnippet");
    v2::Snippet *longvar = manager->getone("longvar", "VarSnippet");
    v2::Snippet *long_var = manager->getone("long_var", "VarSnippet");
    v2::Snippet *longvarinit = manager->getone("longvarinit", "VarSnippet");
    
    ASSERT_TRUE(a && b && c && d && e && f && gg && p && longvar && long_var && longvarinit);

    EXPECT_EQ(a->getCode(), "int a=0");
    EXPECT_EQ(b->getCode(), "int b,c");
    EXPECT_EQ(c->getCode(), "int b,c");
    EXPECT_EQ(d->getCode(), "int d=0,e");
    EXPECT_EQ(e->getCode(), "int d=0,e");
    EXPECT_EQ(f->getCode(), "int f,gg=0");
    EXPECT_EQ(gg->getCode(), "int f,gg=0");
    EXPECT_EQ(p->getCode(), "int *p");
    EXPECT_EQ(longvar->getCode(), "int longvar");
    EXPECT_EQ(long_var->getCode(), "int long_var");
    EXPECT_EQ(longvarinit->getCode(), "int longvarinit=88");
  }

  {
    // test decl
    v2::SnippetManager *manager = new v2::SnippetManager();
    manager->add(createSnippets(file_decl_h));
    manager->process();

    v2::Snippet *A = manager->getone("A", "RecordDeclSnippet");
    v2::Snippet *B = manager->getone("B", "RecordDeclSnippet");
    v2::Snippet *foo = manager->getone("foo", "FunctionDeclSnippet");
    v2::Snippet *bar = manager->getone("bar", "FunctionDeclSnippet");

    ASSERT_TRUE(A && B && foo && bar);

    EXPECT_EQ(A->getCode(), "struct A");
    EXPECT_EQ(B->getCode(), "struct B");
    EXPECT_EQ(foo->getCode(), "int foo()");
    EXPECT_EQ(bar->getCode(), "int bar()");
  }

  {
    // test dependency
    v2::SnippetManager *manager = new v2::SnippetManager();
    manager->add(createSnippets(file_a_c));
    manager->add(createSnippets(file_a_h));
    manager->add(createSnippets(file_b_c));
    manager->add(createSnippets(file_b_h));
    manager->add(createSnippets(file_sub_c));
    manager->add(createSnippets(file_lib_c));
    manager->process();

    v2::Snippet *A1 = manager->getone("A1", "RecordSnippet");
    v2::Snippet *A2 = manager->getone("A2", "RecordSnippet");
    v2::Snippet *A2Typedef = manager->getone("A2", "TypedefSnippet");
    v2::Snippet *A2Decl = manager->getone("A2", "RecordDeclSnippet");
    v2::Snippet *A3 = manager->getone("A3", "RecordSnippet");
    v2::Snippet *A3Alias = manager->getone("A3Alias", "TypedefSnippet");
    
    v2::Snippet *global_a = manager->getone("global_a", "VarSnippet");
    v2::Snippet *temp_a = manager->getone("temp_a", "VarSnippet");
    v2::Snippet *myvar = manager->getone("myvar", "VarSnippet");
    v2::Snippet *aoo = manager->getone("aoo", "FunctionSnippet");


    // manager->dump(std::cout);
    
    std::set<v2::Snippet*> dep;
    dep = A1->getDeps();
    EXPECT_EQ(dep.size(), 0);
    dep = A2->getDeps();
    EXPECT_EQ(dep.size(), 1);
    EXPECT_EQ(dep.count(A1), 1);
    dep = A3->getDeps();
    EXPECT_EQ(dep.size(), 3);
    EXPECT_EQ(dep.count(A2), 1);
    EXPECT_EQ(dep.count(A2Typedef), 1);
    EXPECT_EQ(dep.count(A2Decl), 1);

    // get all dependence
    dep = A3->getAllDeps();
    EXPECT_EQ(dep.size(), 4);
    EXPECT_EQ(dep.count(A1), 1);
    EXPECT_EQ(dep.count(A2), 1);
    EXPECT_EQ(dep.count(A2Typedef), 1);
    EXPECT_EQ(dep.count(A2Decl), 1);

    // outer
    std::set<v2::Snippet*> outer;
    // manager->dump(std::cout);
    // manager->dumpSnippetsVerbose(std::cout);
    outer = A3->getOuters();
    EXPECT_EQ(outer.size(), 1);
    EXPECT_EQ(outer.count(A3Alias), 1);

    // TODO practical usage
  }

  {
    // Test save and load back
    std::vector<v2::Snippet*> snippets = createSnippets(file_var_c);
    v2::SnippetManager *manager = new v2::SnippetManager();
    manager->add(snippets);
    manager->process();

    fs::path random_json = fs::unique_path(fs::temp_directory_path() / "%%%%-%%%%-%%%%-%%%%.json");

    manager->saveJson(random_json);
    // std::cout << "Saved to " << random_json << "\n";

    // a new manager to load
    v2::SnippetManager *manager2 = new v2::SnippetManager();
    manager2->loadJson(random_json);

    // verify
    std::vector<v2::Snippet*> snippets2 = manager2->getSnippets();
    // check snippet and snippet2
    ASSERT_EQ(snippets.size(), snippets2.size());
    for (int i=0;i<snippets.size();i++) {
      v2::Snippet *s1 = snippets[i];
      v2::Snippet *s2 = snippets2[i];
      EXPECT_EQ(s1->getName(), s2->getName());
      // dep and outers
      EXPECT_EQ(s1->getDepsAsId(), s2->getDepsAsId());
      EXPECT_EQ(s1->getOutersAsId(), s2->getOutersAsId());
    }
  }
}
