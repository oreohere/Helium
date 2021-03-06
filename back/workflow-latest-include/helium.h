#ifndef __HELIUM_H__
#define __HELIUM_H__

#include "helium/utils/common.h"
#include "helium/workflow/segment.h"
#include "helium/parser/point_of_interest.h"
#include "analyzer.h"


typedef enum _HeliumStatus {
  HS_Success,
  HS_POIInvalid,
  HS_Default
} HeliumStatus;


class Helium {
public:
  // Helium(FailurePoint *fp);
  Helium(PointOfInterest *poi);
  HeliumStatus GetStatus() {return m_status;}
  ~Helium() {}
private:
  // void init(ASTNode *node);
  void process();
  void debugRemoveAlg(Segment *segment);
  Analyzer* testProfile(Segment *segment);
  std::vector<Segment*> select(Segment *query);
  std::set<Segment*> find_mergable_query(CFGNode *node, Segment *orig_query);
  bool sameTransfer(Segment *s1, Segment *s2);
  bool sameAllTransfer(Segment *s1, Segment *s2);
  // std::string derive_pre_cond(std::vector<std::string> invs, std::vector<std::string> trans);
  // bool pre_entry_point(std::string pre);
  // std::string merge_failure_condition(std::vector<std::string> invs);


  std::deque<Segment*> m_worklist;
  std::map<CFGNode*, std::set<Segment*> > m_waiting_quries;
  std::map<Segment*, std::set<Segment*> > m_propagating_queries;
  // std::map<Segment*, Analyzer*> m_segment_profiles;


  std::string m_failure_condition;
  PointOfInterest *m_poi = NULL;
  HeliumStatus m_status = HS_Default;
  // int countFunction();
};

std::set<Segment*> find_mergable_query(CFGNode *node, Segment *orig_query);
std::vector<Segment*> select(Segment *query);

std::vector<Variable> get_input_variables(std::set<CFGNode*> nodes);


#endif
