#include <config.h>
#include <fstream>
#include <string>
#include <pan/general/log.h>
#include <pan/general/time-elapsed.h>
#include <pan/usenet-utils/filter-info.h>
#include <pan/data/article.h>
#include "data-impl.h"

using namespace pan;

DataImpl data (true);

const Quark server ("w00tnewz");
const Quark server2 ("l33tnews");
const Quark group ("alt.religion.buddhism");

void test_add_article()
{
  Data::ArticleTree * tree;
  Data::ArticleTree::articles_t children;
  const Article * a;

  data.xover_ref (group);

  // TEST: can we add an article?
  data.xover_add (server, group, "Subject", "Author", 0,
      "<article1@foo.com>", "", 40, 100, "");
  tree = data.group_get_articles (group);
  g_assert_cmpint (tree->size(), ==, 1ul);
  a = tree->get_article ("<article1@foo.com>");
  g_assert (a != 0);
  g_assert_cmpstr (a->get_subject().c_str(), ==, "Subject");
  g_assert_cmpstr (a->get_message_id().c_str(), ==, "<article1@foo.com>");
  g_assert (tree->get_parent(a->get_message_id()) == 0);
  delete tree;

  data.xover_unref (group);
  data.group_clear_articles (group);
}

void test_add_child()
{
  Data::ArticleTree * tree;
  Data::ArticleTree::articles_t children;
  const Article * a;

  data.xover_ref (group);
  data.xover_add (server, group, "Subject", "Author", 0,
      "<article1@foo.com>", "", 40, 100, "");

  // TEST: can we add a child?
  data.xover_add (server, group, "Re: Subject", "Author", 0,
      "<article2@blah.com>", "<article1@foo.com>", 40, 100, "");
  tree = data.group_get_articles (group);
  g_assert_cmpint (tree->size(), ==, 2ul);
  a = tree->get_article ("<article2@blah.com>");
  g_assert (a != 0);
  g_assert_cmpstr (a->get_subject().c_str(), ==, "Re: Subject");
  g_assert_cmpstr (a->get_author().c_str(), ==, "Author");
  g_assert_cmpstr (a->get_message_id().c_str(), ==, "<article2@blah.com>");
  a = tree->get_parent (a->get_message_id());
  g_assert (a != 0);
  g_assert_cmpstr (a->get_subject().c_str(), ==, "Subject");
  g_assert_cmpstr (a->get_message_id().c_str(), ==, "<article1@foo.com>");
  g_assert_cmpstr (a->get_author().c_str(), ==, "Author");
  g_assert (tree->get_parent (a->get_message_id()) == 0);
  tree->get_children ("<article1@foo.com>", children);
  g_assert_cmpint (children.size(), ==, 1u);
  g_assert_cmpstr (children[0]->get_message_id().c_str(), ==,
      "<article2@blah.com>");
  delete tree;

  data.xover_unref (group);
  data.group_clear_articles (group);
}

/****
*****
*****  XOVER + filtered tree
*****
****/
void test_filter()
{
  Data::ArticleTree * tree;
  Data::ArticleTree::articles_t children;
  const Article * a;

  data.xover_ref (group);
  // match articles whose subject has the letter 'a'
  TextMatch::Description description;
  description.type = TextMatch::CONTAINS;
  description.text = "a";
  FilterInfo filter_info;
  filter_info.set_type_text ("Subject", description);

  // show articles whose subject has the letter 'a'
  tree = data.group_get_articles (group);
  tree->set_filter (Data::SHOW_ARTICLES, &filter_info);
  g_assert_cmpint (tree->size(), ==, 0ul);

  // TEST: add an article that passes the test and it appears
  data.xover_add (server, group, "a subject", "Author", 0,
      "<article1@foo.com>", "", 40, 100, "");
  data.xover_flush (group);
  g_assert_cmpint (tree->size(), ==, 1ul);
  a = tree->get_article ("<article1@foo.com>");
  g_assert (a != 0);
  g_assert_cmpstr (a->get_message_id().c_str(), ==, "<article1@foo.com>");

  // TEST: add an article that doesn't pass the test and doesn't appear
  data.xover_add (server, group, "subject two", "Author", 0,
      "<article2@foo.com>", "", 40, 100, "");
  g_assert_cmpint (tree->size(), ==, 1ul);
  g_assert (tree->get_article ("<article1@foo.com>") != 0);
  g_assert (tree->get_article ("<article2@foo.com>") == 0);

  data.xover_unref (group);
}

int main (int argc, char **argv)
{

  g_test_init(&argc, &argv, NULL);
  g_test_add_func("/pan/data-impl/add_article", test_add_article);
  g_test_add_func("/pan/data-impl/add_child", test_add_child);
  g_test_add_func("/pan/data-impl/filter", test_filter);

  return g_test_run();
}

void foo()
{

#if 0
   //Quark q;
   //Article a1, a2, a3;
   quark_v children;
   //check (data.article_get_children (group, a1.message_id, children))
   //check (children.size() == 1u)
   //check (children.front() == a2.message_id)
   //check (data.group_get_article_count(group) == 2ul)

   // add an article that needs a parent that we'll get in a moment with xover...
   a3.subject = "Re: I hope I find my mommy!";
   a3.message_id = "<i-want-my-mommy@foo.com>";
   a3.references = "<i-am-your-mommy@foo.com>";
   data.add_article (group, a3);

   data.xover_ref (group);

   // TEST: does xover add articles and thread them right to existing parents?
   data.xover_add (server, group, "10033	Re: it is all a dream.	\"Bob Barker\" <bob@wtf.com>	Wed, 27 Apr 2005 16:53:02 GMT	<wftmid@newsread1.news.pas.earthlink.net>	<article2@blah.com>	1000	100	Xref: news.netfront.net alt.zen:175094 alt.religion.buddhism:10033 talk.religion.buddhism:170169");
   //check (data.group_get_article_count(group) == 4ul)
   check (data.group_find_article (group, "<wftmid@newsread1.news.pas.earthlink.net>", tmp))
   check (tmp.subject == "Re: it is all a dream.")
   check (tmp.get_line_count() == 100)
   check (tmp.get_byte_count() == 1000)
   //check (data.article_get_parent (group, tmp.message_id, q))
   //check (q == a2.message_id)
   check (tmp.parts.size() == 1u)
   part = tmp.get_part (0);
   check (part != 0)
   check (part->xref.find (server, group, ul))
   check (ul == 10033)
 
   // TEST: does xover add articles and thread them right to existing grandparents when their parents can't be found?
   data.xover_add (server, group, "10034	Re: it is all a dream again.	\"Bob Barker\" <bob@wtf.com>	Wed, 27 Apr 2005 16:53:02 GMT	<wftmid2@newsread1.news.pas.earthlink.net>	<article1@foo.com> <article2b@blah.com>	999	90	Xref: news.netfront.net alt.zen:175095 alt.religion.buddhism:10034 talk.religion.buddhism:170168");
   //check (data.group_get_article_count(group) == 5ul)
   check (data.group_find_article (group, "<wftmid2@newsread1.news.pas.earthlink.net>", tmp))
   check (tmp.subject == "Re: it is all a dream again.")
   check (tmp.get_line_count() == 90)
   check (tmp.get_byte_count() == 999)
   //check (data.article_get_parent (group, tmp.message_id, q))
   //check (q == a1.message_id)
   Article floating_child (tmp);
 
   // TEST: and when the parent shows up, does the child get reparented right?
   data.xover_add (server, group, "10035	Re: it is all a dream again.	\"Bob Barker\" <bob@wtf.com>	Wed, 27 Apr 2005 16:53:02 GMT	<article2b@blah.com>	<article1@foo.com>	2487	31	Xref: news.netfront.net alt.zen:175096 alt.religion.buddhism:10035 talk.religion.buddhism:170169");
   //check (data.group_get_article_count(group) == 6ul)
   check (data.group_find_article (group, "<article2b@blah.com>", tmp))
   check (tmp.subject == "Re: it is all a dream again.")
   //check (data.article_get_parent (group, floating_child.message_id, q))
   //check (q == "<article2b@blah.com>")

   // TEST: when an xover parent of an existing article comes in, does the existing article get reparented?
   //check (!data.article_get_parent (group, a3.message_id, q))
   data.xover_add (server, group, "10032	I am your mommy.	\"Mommy\" <mom@mom.com>	Wed, 27 Apr 2005 16:53:02 GMT	<i-am-your-mommy@foo.com>		2487	31	Xref: news.netfront.net alt.zen:15094 alt.religion.buddhism:1003 talk.religion.buddhism:10169");
   //check (data.group_get_article_count(group) == 7ul)
   //check (data.article_get_parent (group, a3.message_id, q))
   //check (q == "<i-am-your-mommy@foo.com>")

   // TEST: single-part attachment
   data.xover_add (server, group, "10033	Some Attachment (1/1)	\"Bob Barker\" <bob@wtf.com>	Wed, 27 Apr 2005 16:53:02 GMT	<single-complete@foo.com>	<article2@blah.com>	5000	800	Xref: news.netfront.net alt.zen:175094");
   check (data.group_find_article (group, "<single-complete@foo.com>", tmp));
   check (tmp.subject == "Some Attachment (1/1)")
   check (tmp.get_line_count() == 800)
   check (tmp.get_byte_count() == 5000)
   check (tmp.part_state == tmp.COMPLETE);

   // TEST: two-part attachment
   data.xover_add (server, group, "10034	Some Big Attachment (1/2)	\"Bob Barker\" <bob@wtf.com>	Wed, 27 Apr 2005 16:53:02 GMT	<multi-part-1@foo.com>	<article2@blah.com>	5000	800	Xref: news.netfront.net alt.religion.buddhism:10034 alt.zen:175094");
   check (data.group_find_article (group, "<multi-part-1@foo.com>", tmp));
   check (tmp.subject == "Some Big Attachment (1/2)")
   check (tmp.get_line_count() == 800)
   check (tmp.get_byte_count() == 5000)
   check (tmp.part_state == tmp.INCOMPLETE);
   part = tmp.get_part (1);
   check (part != 0)
   check (part->xref.find (server, group, ul))
   check (ul == 10034)

   data.xover_add (server, group, "10035	Some Big Attachment (2/2)	\"Bob Barker\" <bob@wtf.com>	Wed, 27 Apr 2005 16:53:02 GMT	<multi-part-2@foo.com>	<article2@blah.com>	999	99	Xref: w00tnewz.com alt.religion.buddhism:10035 alt.zen:10035");
   check (data.group_find_article (group, "<multi-part-1@foo.com>", tmp));
   check (tmp.subject == "Some Big Attachment (1/2)")
   check (tmp.get_line_count() == 899)
   check (tmp.get_byte_count() == 5999)
   check (tmp.part_state == tmp.COMPLETE);
   part = tmp.get_part (2);
   check (part != 0)
   check (part->xref.find (server, group, ul))
   check (ul == 10035)

   // TEST: multiserver
   data.xover_add (server2, group, "6666	Some Big Attachment (2/2)	\"Bob Barker\" <bob@wtf.com>	Wed, 27 Apr 2005 16:53:02 GMT	<multi-part-2@foo.com>	<article2@blah.com>	5000	800	Xref: l33tnewz.com alt.religion.buddhism:6666 alt.zen:6666");

   ArticleTree * tree = data.group_get_articles (group, FilterInfo());
   const Article * a = tree->get_article ("<multi-part-1@foo.com>");
   check (a != 0);
   check (a->subject == "Some Big Attachment (1/2)")
   check (a->part_state == tmp.COMPLETE)
   part = a->get_part (2);
   check (part != 0);
   check (part->xref.targets.size() == 4)
   check (part->xref.find (server, group, ul))
   check (ul == 10035)
   check (part->xref.find (server2, group, ul))
   check (ul == 6666)
 
#if 0
   TimeElapsed xover_timer;
   std::ifstream in ("header-impl-test.dat");
   std::string line;
   int lines = 0;
   while (std::getline (in, line)) {
      data.xover_add (server, group, line);
      ++lines;
   }
   Log::get().add_va (Log::INFO, "XOver added %lu headers in %.2f seconds",
      lines,
      xover_timer.get_seconds_elapsed());

   data.unref_group (group);
#endif

   data.xover_unref (group);
   data.unref_group (group);
   data.save ();

   //std::cerr << "headers file: [" << std::endl << data_source->_group_headers[group] << std::endl;

#if 0
   ScriptedDataSource * source_2 = new ScriptedDataSource (*data_source);
   DataImpl data2 (source_2);
   check (data2 == data);
#endif
#endif

}
