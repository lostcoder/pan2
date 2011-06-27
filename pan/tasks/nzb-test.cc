#include <config.h>
#include <iostream>
#include <sstream>
#include <pan/general/string-view.h>
#include <pan/general/test.h>
#include <pan/data/article-cache.h>
#include <pan/data/encode-cache.h>
#include "nzb.h"
#include "task-article.h"

using namespace pan;

struct MyServerRank: public ServerRank
{
  virtual int get_server_rank (const Quark&) const { return 1; }
};

struct MyGroupServer: public GroupServer
{
  std::map<Quark,quarks_t>& g2s;
  MyGroupServer (std::map<Quark,quarks_t>& g): g2s(g) {}
  virtual ~MyGroupServer () {}
  virtual void server_get_groups (const Quark&, quarks_t&) const { /*ignored*/ }
  virtual void group_get_servers (const Quark& group, quarks_t& setme) const {
    setme.clear ();
    if (g2s.count (group))
      setme = g2s.find(group)->second;
  }
};

struct MyArticleRead: public ArticleRead
{
  virtual ~MyArticleRead () {}
  virtual bool is_read (const Article*) const { return false; }
  virtual void mark_read (const Article&, bool) {}
  virtual void mark_read (const Article**, unsigned long, bool) {}
};

struct Fixture
{
  std::map<Quark,quarks_t> gmap;
  MyServerRank ranks;
  MyGroupServer gs;

  ArticleCache cache;
  MyArticleRead read;
  EncodeCache e_cache;
  std::vector<Task*> tasks;

  Fixture();
  ~Fixture();
};

Fixture::Fixture():gs(gmap), cache("/tmp"), e_cache("/tmp")
{
  static const char * test_1 =
     "<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>\n"
     "<!DOCTYPE nzb PUBLIC \"-//newzBin//DTD NZB 1.0//EN\" \"http://www.newzbin.com/DTD/nzb/nzb-1.0.dtd\">\n"
     "<nzb xmlns=\"http://www.newzbin.com/DTD/2003/nzb\">\n"
     " <file poster=\"Joe Bloggs <bloggs@nowhere.example>\" date=\"1071674882\" subject=\"Here's your file!  abc-mr2a.r01 (1/2)\">\n"
     "   <groups>\n"
     "     <group>alt.binaries.newzbin</group>\n"
     "     <group>alt.binaries.mojo</group>\n"
     "   </groups>\n"
     "   <segments>\n"
     "     <segment bytes=\"102394\" number=\"1\">123456789abcdef@news.newzbin.com</segment>\n"
     "     <segment bytes=\"4501\" number=\"2\">987654321fedbca@news.newzbin.com</segment>\n"
     "   </segments>\n"
     " </file>\n"
     "</nzb>";
  StringView v (test_1);

  gmap["alt.binaries.newzbin"].insert ("giganews");
  gmap["alt.binaries.newzbin"].insert ("cox");
  gmap["alt.binaries.mojo"].insert ("giganews");

  NZB :: tasks_from_nzb_string (v, StringView("/tmp"), cache, e_cache,
      read, ranks, gs, tasks);
}

void setup(gpointer fixture, gconstpointer user_data)
{
  Fixture *self  = new(fixture) Fixture;
}

Fixture::~Fixture()
{
  for (std::vector<Task*>::iterator it(tasks.begin()),
       end(tasks.end()); it!=end; ++it)
    delete *it;
}

void teardown(gpointer fixture, gconstpointer user_data)
{
  reinterpret_cast<Fixture*>(fixture)->~Fixture();
}

void test_nzbin(gpointer fixture, gconstpointer user_data)
{
  Fixture &self (*reinterpret_cast<Fixture*>(fixture));

  g_assert_cmpint (self.tasks.size(), ==, 1);
  const Article & a (dynamic_cast<TaskArticle*>(self.tasks[0])->get_article());
  g_assert_cmpstr (a.get_author().c_str(), ==,
      "Joe Bloggs <bloggs@nowhere.example>");
  g_assert_cmpstr (a.get_subject().c_str(), ==,
      "Here's your file!  abc-mr2a.r01 (1/2)");
  g_assert_cmpint (a.get_total_part_count(), ==, 2);
  g_assert_cmpint (a.get_found_part_count(), ==, 2);
  g_assert_cmpstr (a.get_message_id().c_str(), ==,
      "<123456789abcdef@news.newzbin.com>");
  g_assert_cmpint (a.time_posted, ==, 1071674882);
  g_assert_cmpint (a.xref.size(), ==, 3);

  std::string part_mid;
  Parts::bytes_t part_bytes;
  g_assert (a.get_part_info (1u, part_mid, part_bytes));
  g_assert_cmpint (part_bytes, ==, 102394);
  g_assert_cmpstr (part_mid.c_str(), ==,
      "<123456789abcdef@news.newzbin.com>");

  Quark group;
  uint64_t number;
  g_assert (a.xref.find ("cox", group, number));
  g_assert_cmpstr (group.c_str(), ==, "alt.binaries.newzbin");
  g_assert_cmpint (number, ==, 0);
  g_assert (a.xref.find ("giganews", group, number));
  g_assert (group=="alt.binaries.newzbin" || group=="alt.binaries.mojo");
  g_assert_cmpint (number, ==, 0);
  g_assert (a.get_part_info (2, part_mid, part_bytes));
  g_assert_cmpint (part_bytes, ==, 4501);
}

void test_nzbout(gpointer fixture, gconstpointer user_data)
{
  Fixture &self (*reinterpret_cast<Fixture*>(fixture));
  std::ostringstream out_stream;
  NZB :: nzb_to_xml (out_stream, self.tasks);
  std::string out (out_stream.str ());
  static const char *expected =
    "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
    "<!DOCTYPE nzb PUBLIC \"-//newzBin//DTD NZB 1.0//EN\" \"http://www.newzbin.com/DTD/nzb/nzb-1.0.dtd\">\n"
    "<nzb xmlns=\"http://www.newzbin.com/DTD/2003/nzb\">\n"
    "  <file poster=\"Joe Bloggs &lt;bloggs@nowhere.example&gt;\" date=\"1071674882\" subject=\"Here&apos;s your file!  abc-mr2a.r01 (1/2)\">\n"
    "    <path>/tmp</path>\n"
    "    <groups>\n"
    "      <group>alt.binaries.newzbin</group>\n"
    "      <group>alt.binaries.mojo</group>\n"
    "    </groups>\n"
    "    <segments>\n"
    "      <segment bytes=\"102394\" number=\"1\">123456789abcdef@news.newzbin.com</segment>\n"
    "      <segment bytes=\"4501\" number=\"2\">987654321fedbca@news.newzbin.com</segment>\n"
    "    </segments>\n"
    "  </file>\n"
    "</nzb>\n";
  g_assert_cmpstr (out.c_str(), ==, expected);
}

int main (int argc, char **argv)
{
  g_test_init(&argc, &argv, NULL);
  GTestSuite *s1 (g_test_create_suite("pan"));
  GTestSuite *suite (g_test_create_suite("nzb"));
  g_test_suite_add_suite (g_test_get_root(), s1);
  g_test_suite_add_suite (s1, suite);
  g_test_suite_add ( suite, g_test_create_case ("nzb_in", sizeof(Fixture),
        NULL, setup, test_nzbin, teardown));
  g_test_suite_add ( suite, g_test_create_case ("nzb_out", sizeof(Fixture),
        NULL, setup, test_nzbout, teardown));
  //return g_test_run_suite (s1);
  return g_test_run();
}
