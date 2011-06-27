/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Pan - A Newsreader for Gtk+
 * Copyright (C) 2002-2006  Charles Kerr <charles@rebelbase.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __DataImpl_h__
#define __DataImpl_h__

#include <iosfwd>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <deque>

#include <pan/general/quark.h>
#include <pan/general/macros.h>
#include <pan/general/map-vector.h>
#include <pan/general/sorted-vector.h>
#include <pan/usenet-utils/numbers.h>
#include <pan/usenet-utils/scorefile.h>
#include <pan/data/article.h>
#include <pan/data/article-cache.h>
#include <pan/data/encode-cache.h>
#include <pan/data/data.h>
#include <pan/tasks/queue.h>
#include <pan/data-impl/data-io.h>
#include <pan/data-impl/article-filter.h>
#include <pan/data-impl/profiles.h>
#include <pan/data-impl/memchunk.h>

namespace pan
{
  typedef std::vector<const Article*> articles_t;

  /**
   * File-based implementation of the `Data' backend interface.
   *
   * Most of the files are stored in $PAN_HOME, which defaults to
   * $HOME/.pan2 if the PAN_HOME environmental variable isn't set.
   *
   * @ingroup data_impl
   */
  class DataImpl:
    public Data,
    public TaskArchive,
    public ProfilesImpl
  {
    /**
    *** SERVERS
    **/

    public:
      DataImpl (bool unit_test=false, int cache_megs=10, DataIO * source=new DataIO());
      virtual ~DataImpl ();
      virtual void save_state ();

    public:
      virtual ArticleCache& get_cache () { return _cache; }
      virtual const ArticleCache& get_cache () const { return _cache; }

      virtual EncodeCache& get_encode_cache () { return _encode_cache; }
      virtual const EncodeCache& get_encode_cache () const { return _encode_cache; }
    private:
      EncodeCache _encode_cache;
      ArticleCache _cache;

    private:

      void rebuild_backend ();
      const bool _unit_test;
      DataIO * _data_io;

    /**
    *** SERVERS
    **/

    private: // implementation

      void load_server_properties (const DataIO&);

      void save_server_properties (DataIO&) const;

      struct Server
      {
         std::string username;
         std::string password;
         std::string host;
         std::string newsrc_filename;
         int port;
         int article_expiration_age;
         int max_connections;
         int rank;
         typedef sorted_vector<Quark,true,AlphabeticalQuarkOrdering> groups_t;
         groups_t groups;

         Server(): port(119), article_expiration_age(31), max_connections(2), rank(1) {}
      };

      typedef Loki::AssocVector<Quark,Server> servers_t;

      servers_t _servers;

      Server* find_server (const Quark& server);
      const Server* find_server (const Quark& server) const;

    public: // mutators

      virtual void delete_server (const Quark& server);

      virtual Quark add_new_server ();


      virtual void set_server_auth (const Quark       & server,
                                    const StringView  & username,
                                    const StringView  & password);

      virtual void set_server_addr (const Quark       & server,
                                    const StringView  & host,
                                    const int           port);

      virtual void set_server_limits (const Quark     & server,
                                      int               max_connections);

      virtual void set_server_rank (const Quark& server, int rank);

      virtual void set_server_article_expiration_age  (const Quark  & server,
                                                       int            days);

      virtual void save_server_info (const Quark& server);

    public: // accessors

      virtual quarks_t get_servers () const {
        quarks_t servers;
        foreach_const (servers_t, _servers, it)
          servers.insert (it->first);
        return servers;
      }

      virtual bool get_server_auth (const Quark   & server,
                                    std::string   & setme_username,
                                    std::string   & setme_password) const;

      virtual bool get_server_addr (const Quark   & server,
                                    std::string   & setme_host,
                                    int           & setme_port) const;

      virtual std::string get_server_address (const Quark& servername) const;

      virtual int get_server_rank (const Quark& server) const;

      virtual int get_server_limits (const Quark & server) const;

      virtual int get_server_article_expiration_age  (const Quark  & server) const;

    /**
    *** GROUPS
    **/

    private: // implementation

      typedef std::map<Quark,std::string> descriptions_t;
      mutable descriptions_t _descriptions; // groupname -> description
      mutable bool _descriptions_loaded;

      typedef sorted_vector<Quark,true> unique_sorted_quarks_t;
      typedef sorted_vector<Quark,true> groups_t;
      groups_t _moderated; // groups which are moderated
      groups_t _nopost; // groups which do not allow posting

      typedef sorted_vector<Quark,true,AlphabeticalQuarkOrdering> alpha_groups_t;
      alpha_groups_t _subscribed; // subscribed groups, sorted alphabetically
      alpha_groups_t _unsubscribed; // non-subscribed groups, sorted alphabetically

      /**
       * Represents a newsgroup that's been read.
       *
       * Since most groups are never read, the `read' fields are separated
       * out into this structure, so that it can be instantiated on demand.
       * Since most news servers have tens of thousands of newsgroups,
       * this represents a big memory savings.
       *
       * This private class should only be used by code in the data-impl module.
       */
      struct ReadGroup
      {
        /**
         * Per-server for a newsgroup that's been read.
         *
         * This private class should only be used by code in the data-impl module.
         */
        struct Server {
          Numbers _read;
          uint64_t _xover_high;
          Server(): _xover_high(0) {}
        };
        typedef Loki::AssocVector<Quark,Server> servers_t;
        servers_t _servers;

        unsigned long _article_count;
        unsigned long _unread_count;

        ReadGroup(): _article_count(0), _unread_count(0) {}

        Server& operator[](const Quark& s) { return _servers[s]; }

        static void decrement_safe (unsigned long& lhs, unsigned long dec) { lhs = (lhs>dec) ? lhs-dec : 0; }
        void decrement_unread (unsigned long dec) { decrement_safe (_unread_count, dec); }
        void decrement_count (unsigned long dec) { decrement_safe (_article_count, dec); }

        Server* find_server (const Quark& s) {
          servers_t::iterator it (_servers.find (s));
          return it == _servers.end() ? 0 : &it->second;
        }

        const Server* find_server (const Quark& s) const {
          servers_t::const_iterator it (_servers.find (s));
          return it == _servers.end() ? 0 : &it->second;
        }
      };

      typedef Loki::AssocVector<Quark,ReadGroup> read_groups_t;
      read_groups_t _read_groups;

      ReadGroup* find_read_group (const Quark& g) {
        read_groups_t::iterator it (_read_groups.find (g));
        return it == _read_groups.end() ? 0 : &it->second;
      }
      const ReadGroup* find_read_group (const Quark& g) const {
        read_groups_t::const_iterator it (_read_groups.find (g));
        return it == _read_groups.end() ? 0 : &it->second;
      }
      ReadGroup::Server* find_read_group_server (const Quark& g, const Quark& s) {
        ReadGroup * read_group = find_read_group (g);
        return read_group ? read_group->find_server (s) : 0;
      }
      const ReadGroup::Server* find_read_group_server (const Quark& g, const Quark& s) const {
        const ReadGroup * read_group = find_read_group (g);
        return read_group ? read_group->find_server (s) : 0;
      }

      void ensure_descriptions_are_loaded () const;
      void load_group_descriptions (const DataIO&) const;
      void save_group_descriptions (DataIO&) const;

      void load_group_xovers (const DataIO&);
      void save_group_xovers (DataIO&) const;

      void load_group_permissions (const DataIO&);
      void save_group_permissions (DataIO&) const;

      std::string get_newsrc_filename (const Quark& server) const;
      void load_newsrc (const Quark& server, LineReader*, alpha_groups_t&, alpha_groups_t&);
      void load_newsrc_files (const DataIO&);
      void save_newsrc_files (DataIO&) const;

    public: // mutators

      virtual void add_groups                 (const Quark       & server,
                                               const NewGroup    * new_groups,
                                               size_t              group_count);

      virtual void mark_group_read            (const Quark       & group);

      virtual void set_group_subscribed       (const Quark       & group,
                                               bool                sub);

    public: // accessors

      virtual const std::string& get_group_description (const Quark& group) const;
      virtual void get_subscribed_groups (std::vector<Quark>&) const;
      virtual void get_other_groups (std::vector<Quark>&) const;
      virtual void get_group_counts (const Quark    & group,
                                     unsigned long  & setme_unread,
                                     unsigned long  & setme_total) const;
      virtual char get_group_permission (const Quark & group) const;
      virtual void group_get_servers (const Quark& group, quarks_t&) const;
      virtual void server_get_groups (const Quark& server, quarks_t&) const;

    /**
    ***  HEADERS
    **/

    private: // implementation

      class ArticleImpl: public Article
      {
        public:
          Quark message_id;
          Quark author;
          Quark subject;
          ArticleImpl() {}
          const Quark& get_message_id() const
          {
            return message_id;
          }
          const Quark& get_author() const
          {
            return author;
          }
          const Quark& get_subject() const
          {
            return subject;
          }
          ArticleCPtr clone() const
          {
            ArticleCPtr ret(new ArticleImpl);
            *ret = *this;
            return ret;
          }
          void clear()
          {
            Article::clear();
            message_id.clear();
            author.clear();
            subject.clear();
          }
      };

      /** 'article' MUST have been allocated by GroupHeaders::alloc_new_article()!! */
      void load_article (const Quark& g, Article * article, const StringView& references);

      /** the contents of `part' are given up wholesale to our
          local GroupHeaders.  As a side-effect, the value of `part'
          after this call is undefined.  This is an ugly interface,
          but it's fast and only called by one client. */
      void load_part (const Quark& g, const Quark& mid,
                      int number,
                      const StringView& part_mid,
                      unsigned long lines,
                      unsigned long bytes);

      void load_headers (const DataIO&, const Quark& group);
      void save_headers (DataIO&, const Quark& group) const;
      bool save_headers (DataIO&, const Quark& group,
                         const std::vector<Article*>&,
                         unsigned long&, unsigned long&) const;


      /**
       * ArticleNode is a Tree node used for threading Article objects.
       *
       * GroupHeaders owns these, and also contains a lookup table from
       * Message-ID to ArticleNode for finding a starting point in the tree.
       *
       * Note that _article can be NULL here; we instantiate nodes from
       * Articles' References: header so that we can get the threading model
       * right even during an xover where we get children in before the
       * parent.  This way we never need to rethread; new articles just
       * fill in the missing pieces as they come in.
       *
       * @see GroupHeaders
       */
      struct ArticleNode
      {
        Quark _mid;
        Article * _article;

        ArticleNode * _parent;
        typedef std::list<ArticleNode*> children_t;
        children_t _children;

        ArticleNode(): _article(0), _parent(0) {}
      };

      typedef std::map<Quark,ArticleNode*> nodes_t;
      typedef std::vector<ArticleNode*> nodes_v;
      typedef std::vector<const ArticleNode*> const_nodes_v;

      struct NodeWeakOrdering
      {
        typedef std::pair<Quark,ArticleNode*> nodes_t_element;

        bool operator () (const nodes_t_element& a, const Quark& b) const {
          return a.first < b;
        }
        bool operator () (const Quark& a, const nodes_t_element& b) const {
          return a < b.first;
        }
      };

      struct GroupHeaders
      {
        int _ref;
        bool _dirty;
        nodes_t _nodes;
        MemChunk<ArticleImpl> _art_chunk;
        MemChunk<ArticleNode> _node_chunk;

        GroupHeaders();
        ~GroupHeaders ();

        ArticleImpl& alloc_new_article () {
          static const ArticleImpl blank_article;
          _art_chunk.push_back (blank_article);
          return _art_chunk.back();
        }

        ArticleNode* find_node (const Quark& mid);
        const ArticleNode* find_node (const Quark& mid) const;

        const Quark& find_parent_message_id (const Quark& mid) const;
        Article* find_article (const Quark& mid);
        const Article* find_article (const Quark& mid) const;
        void remove_articles (const quarks_t& mids);
        void build_references_header (const Article* article, std::string& setme) const;
      };

      static void find_nodes (const quarks_t           & mids,
                              nodes_t                  & nodes,
                              nodes_v                  & setme);

      static void find_nodes (const quarks_t           & mids,
                              const nodes_t            & nodes,
                              const_nodes_v            & setme);

      virtual void get_article_references (const Quark& group, const Article*, std::string& setme) const;

      /**
       * For a given ArticleNode, returns the first ancestor whose mid is in mid_pool.
       * FIXME: these should be member functions of ArticleNode
       */
      static ArticleNode* find_closest_ancestor (ArticleNode  * node,
                                                 const unique_sorted_quarks_t & mid_pool);
      static const ArticleNode* find_closest_ancestor (const ArticleNode  * node,
                                                       const unique_sorted_quarks_t & mid_pool);

      static ArticleNode* find_ancestor (ArticleNode * node,
                                         const Quark & ancestor_mid);

      typedef Loki::AssocVector<Quark,GroupHeaders*> group_to_headers_t;
      group_to_headers_t _group_to_headers;

      GroupHeaders* get_group_headers (const Quark& group);
      const GroupHeaders* get_group_headers (const Quark& group) const;
      void free_group_headers_memory (const Quark& group);
      bool is_read (const Xref&) const;

      void ref_group (const Quark& group);
      void unref_group (const Quark& group);

    private:

      class MyTree: public Data::ArticleTree
      {
        friend class DataImpl;

        public: // life cycle
          MyTree (DataImpl              & data_impl,
                  const Quark           & group,
                  const Data::ShowType    show_type,
                  const FilterInfo      * filter_info);
          virtual ~MyTree ();

        public: // from ArticleTree
          virtual void get_children (const Quark& mid, articles_t& setme) const;
          virtual const Article* get_parent (const Quark& mid) const;
          virtual const Article* get_article (const Quark& mid) const;
          virtual size_t size () const;
          virtual void set_filter (const ShowType      show_type = SHOW_ARTICLES,
                                   const FilterInfo  * criteria  = 0);

        public:
          void articles_changed (const quarks_t& mids, bool do_refilter);
          void add_articles     (const quarks_t& mids);
          void remove_articles  (const quarks_t& mids);

        private: // implementation fields
          const Quark _group;
          DataImpl & _data;
          nodes_t _nodes;
          MemChunk<ArticleNode> _node_chunk;
          FilterInfo _filter;
          Data::ShowType _show_type;
          struct NodeMidCompare;
          struct TwoNodes;

        private:
          typedef std::set<const ArticleNode*,NodeMidCompare> unique_nodes_t;
          void accumulate_descendants (unique_nodes_t&, const ArticleNode*) const;
          void add_articles (const const_nodes_v&);
          void apply_filter (const const_nodes_v&);
      };

      std::set<MyTree*> _trees;
      void on_articles_removed (const quarks_t& mids) const;
      void on_articles_added (const Quark& group, const quarks_t& mids);
      void on_articles_changed (const Quark& group, const quarks_t& mids, bool do_refilter);
      void remove_articles_from_tree (MyTree*, const quarks_t& mids) const;
      void add_articles_to_tree (MyTree*, const quarks_t& mids);


    public:  // Data interface

      virtual void delete_articles             (const unique_articles_t&);

      virtual ArticleTree* group_get_articles  (const Quark        & group,
                                                const ShowType      show_type = SHOW_ARTICLES,
                                                const FilterInfo   * criteria=0) const;

      virtual void group_clear_articles        (const Quark        & group);

      virtual bool is_read                     (const Article      *) const;

      virtual void mark_read                   (const Article      & article,
                                                bool                 mark_read);

      virtual void mark_read                   (const Article     ** articles,
                                                unsigned long        article_count,
                                                bool                 mark_read=true);

      virtual void get_article_scores          (const Quark        & newsgroup,
                                                const Article      & article,
                                                Scorefile::items_t & setme) const;

      virtual void add_score (const StringView           & section_wildmat,
                              int                          score_value,
                              bool                         score_assign_flag,
                              int                          lifespan_days,
                              bool                         all_items_must_be_true,
                              const Scorefile::AddItem   * items,
                              size_t                       item_count,
                              bool                         do_rescore);

      virtual void comment_out_scorefile_line (const StringView    & filename,
                                               size_t                begin_line,
                                               size_t                end_line,
                                               bool                  do_rescore);

      virtual void rescore_articles (const Quark& group, const quarks_t mids);

      virtual void rescore ();

    private:

      Scorefile _scorefile;

    /**
    ***  XOVER
    **/

    private: // implementation

      /**
       * This is a workarea used when we processing an XOVER command.
       */
      struct XOverEntry
      {
        time_t _last_flush_time;

        /** These are the articles which have been recently added.
            The patch is periodically flushed to on_articles_added()
            from xover_line() and xref_unref(). */
        quarks_t _added_batch;

        /** Same as _added_batch, but for changed articles. */
        quarks_t _changed_batch;

        typedef std::multimap<Quark,Quark> subject_to_mid_t;

        /** This is for multipart detection.  Pan folds multipart posts into
            a single Article holding all the parts.  This lookup helps decide,
            when we get a new multipart post, which Article to fold
            it into.  We strip out the unique part info from the Subject header
            (such as the "15" in [15/42]) and use it as a key in this lookup
            table that gives the Message-ID of the Article owning this post. */
        subject_to_mid_t _subject_lookup;

        /** We must refcount because multiple server connections can
            be assigned to the same XOVER task. */
        int refcount;

        XOverEntry(): _last_flush_time(0), refcount(0) { }
      };

      typedef Loki::AssocVector<Quark,XOverEntry> xovers_t;
      xovers_t _xovers;

      XOverEntry * _cached_xover_entry;
      Quark _cached_xover_group;

      /**
       * Destroy the workarea.
       * Don't call this directly -- xover_unref() do its job.
       */
      void xover_clear_workarea (const Quark& group);

      /**
       * Finds the XOverEntry workarea for the specified group.
       * This must be called inside an xover_ref() / xover_unref() block,
       * as the workarea is instantiated when the group's xover refcount
       * increases to one and destroyed when it goes down to zero.
       */
      XOverEntry& xover_get_workarea (const Quark& group);

    public: // Data interface

      virtual void xover_ref     (const Quark          & group);

      virtual const Article* xover_add  (const Quark          & server,
                                         const Quark          & group,
                                         const StringView     & subject,
                                         const StringView     & author,
                                         const time_t           date,
                                         const StringView     & message_id,
                                         const StringView     & references,
                                         const unsigned long    byte_count,
                                         const unsigned long    line_count,
                                         const StringView     & xref);

      /** useful for xover unit testing */
      virtual void xover_flush   (const Quark           & group);

      virtual void xover_unref   (const Quark           & group);

      virtual uint64_t get_xover_high (const Quark & group,
                                       const Quark & server) const;

      virtual void set_xover_high (const Quark          & group,
                                   const Quark          & server,
                                   const uint64_t         high);

       virtual void set_xover_low (const Quark          & group,
                                   const Quark          & server,
                                   const uint64_t         low);


    /**
    *** TaskArchive
    **/

    public:

      virtual void save_tasks (const std::vector<Task*>& saveme);

      virtual void load_tasks (std::vector<Task*>& setme);



    public:

      const ArticleFilter _article_filter;

    private:
      guint newsrc_autosave_id;
      guint newsrc_autosave_timeout;
    public:
      bool in_newsrc_cb;
      void set_newsrc_autosave_timeout(guint seconds)
        {newsrc_autosave_timeout = seconds;}
      void save_newsrc_files()
      {
        save_newsrc_files(*_data_io);
        newsrc_autosave_id = 0;
      }
  };
}

#endif
