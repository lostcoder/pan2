#include <config.h>
#include <iostream>
#include <glib.h>
#include <pan/general/string-view.h>
//#include <pan/general/test.h>
#include "article.h"

using namespace pan;

#define test_message_id(s) \
  in = s; \
  a.add_part (++i, in, 1); \
  g_assert (a.get_part_info (i, out, bytes)); \
  g_assert_cmpint (bytes, ==, 1); \
  g_assert_cmpstr (out.c_str() ,== ,in.c_str());

void test()
{
  Parts::number_t i = 0;
  Parts::bytes_t bytes;
  Quark key = "<abcd@efghijk.com>";
  std::string in, out;

  ArticleNZB a;
  a.message_id = key;
  a.set_part_count (2);

  test_message_id (key.to_string()); // two equal strings
  test_message_id ("<abcdefg@efghijk.com>"); // extra in the middle...
  test_message_id ("<abcd@hijk.com>"); // missing in the middle...
  test_message_id ("zzz<abcd@efghijkl.com>"); // crazy test: extra at front
  test_message_id ("<abcd@efghijkl.com>zzz"); // crazy test: extra at end
  test_message_id ("zzz<abcd@efghijkl.com>zzz"); // crazy test: extra at both ends
  test_message_id ("abcd@efghijkl.com>"); // crazy test: less at front
  test_message_id ("<abcd@efghijkl.com"); // crazy test: less at end
  test_message_id ("abcd@efghijkl.com"); // crazy test: less at both ends
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/pan/data/article", test);
  return g_test_run();
}
