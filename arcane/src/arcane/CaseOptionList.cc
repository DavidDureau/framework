﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* CaseOptionList.cc                                           (C) 2000-2019 */
/*                                                                           */
/* Liste d'options de configuration d'un service ou module.                  */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/utils/TraceAccessor.h"
#include "arcane/utils/FatalErrorException.h"
#include "arcane/utils/Iterator.h"
#include "arcane/utils/StringBuilder.h"

#include "arcane/CaseOptions.h"
#include "arcane/ICaseMng.h"
#include "arcane/XmlNodeList.h"
#include "arcane/XmlNodeIterator.h"
#include "arcane/CaseOptionError.h"
#include "arcane/ICaseDocumentVisitor.h"
#include "arcane/CaseOptionException.h"
#include "arcane/ICaseDocument.h"

// TODO: a supprimer
#include "arcane/IServiceInfo.h"
#include "arcane/ICaseFunction.h"

#include <vector>
#include <algorithm>

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane
{


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Vérifie la validité du contenu entre éléments.
 *
 * Cette classe permet de vérifier qu'il n'y a pas de chaînes de caractères
 * entre les éléments.
 *
 * Par exemple, le texte suivant est invalide: pour une option complexe 'toto'
 * contenant deux options 'x' et 'y', le texte suivant est considéré invalide:
 * <toto>ABCD<x>A</x><y></y></toto>
 * car 'ABCD' ne sera pas utilisé ce qui peut être trompeur pour l'utilisateur.
 * On ne tolére entre les éléments que les noeuds texte contenant des
 * caractères blancs.
 */
class XmlElementContentChecker
{
 public:
  XmlElementContentChecker(ICaseDocument* cd,ITraceMng* tm)
  : m_case_document(cd), m_space_string(" ")
  {
    ARCANE_UNUSED(tm);
  }

  //! Vérifie la validité des éléments fils de \a element
  void check(XmlNode element)
  {
    //ITraceMng* tm = m_trace_mng;
    XmlNode last_element = element;
    for( XmlNode::const_iterator i = element.begin(), end = element.end(); i != end; ++i ){
      XmlNode n = *i;
      //tm->info() << "CHECK SUB_ELEMENT: " << n.name();
      if (n.type()==XmlNode::ELEMENT){
        last_element = n;
      }
      if (n.type()==XmlNode::TEXT){
        StringBuilder sb = n.value();
        //tm->info() << "VALUE1: '" << n.value() << "'";
        sb.collapseWhiteSpace();
        String ns = sb.toString();
        //tm->info() << "VALUE2: '" << ns << "'";
        if (ns!=m_space_string)
          CaseOptionError::addWarning(m_case_document,A_FUNCINFO,last_element.xpathFullName(),
                                      String::format("Invalid text node between elements (value='{0}')",
                                                     n.value()),true);

      }
    }
  }

 private:
  ICaseDocument* m_case_document;
  String m_space_string;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \internal
 * \brief Liste d'options de configuration d'un service ou module.
 */
class CaseOptionList
: public TraceAccessor
, public ICaseOptionList
{
 public:

  typedef std::pair<CaseOptionBase*,XmlNode> CaseOptionBasePair;
  typedef std::vector<CaseOptionBasePair> CaseOptionBasePairList;

  CaseOptionList(ICaseMng* m,ICaseOptions* ref_opt,XmlNode parent_element)
  : TraceAccessor(m->traceMng()), m_case_mng(m), m_root_element(), m_parent(nullptr), m_ref_opt(ref_opt),
    m_parent_element(parent_element), m_is_present(false), m_is_multi(false), m_is_optional(false) {}
  CaseOptionList(ICaseOptionList* parent,ICaseOptions* ref_opt,XmlNode parent_element)
  : TraceAccessor(parent->caseMng()->traceMng()), m_case_mng(parent->caseMng()),
    m_root_element(), m_parent(parent),
    m_ref_opt(ref_opt), m_parent_element(parent_element), m_is_present(false),
    m_is_multi(false), m_is_optional(false) {}
  ~CaseOptionList()
  {
    // Détache les options filles qui existent encore pour ne pas qu'elles le
    // fassent lors de leur destruction. Il faut utiliser une copie de la liste
    // car cette dernière sera modifiée lors du detach().
    std::vector<ICaseOptions*> copy_list(m_case_options);
    for( ICaseOptions* co : copy_list )
      co->detach();
  }

 public:

  XmlNode rootElement() const override { return m_root_element; }
  XmlNode parentElement() const override { return m_parent_element; }
  ICaseMng* caseMng() const override { return m_case_mng; }
  virtual ICaseDocument* caseDocument() const { return m_case_mng->caseDocument(); }
  void addConfig(CaseOptionBase* cbi,XmlNode parent) override
  {
    //TODO: Vérifier la suppression et pas déjà présent
    m_config_list.push_back(CaseOptionBasePair(cbi,parent));
  }
  void addChild(ICaseOptions* c) override
  {
    info(5) << " ADD_CHILD " << c->rootTagName() << " instance=" << c
            << " this=" << this << " n=" << m_case_options.size();
    m_case_options.push_back(c);
  }
  void removeChild(ICaseOptions* c) override
  {
    info(5) << " REMOVE_CHILD " << c->rootTagName() << " instance=" << c << " this=" << this;
    auto i = std::find(m_case_options.begin(),m_case_options.end(),c);
    if (i==m_case_options.end())
      ARCANE_FATAL("Internal: option not in list");
    m_case_options.erase(i);
  }

  void readChildren(bool is_phase1) override;
  void printChildren(const String& lang,int indent) override;
  void visit(ICaseDocumentVisitor* visitor) override;
  void addInvalidChildren(XmlNodeList& nlist) override;

 public:

  void deepGetChildren(Array<CaseOptionBase*>& col) override
  {
    for( ConstIterT<CaseOptionBasePairList> i(m_config_list); i(); ++i )
      col.add(i->first);
    for( ICaseOptions* co : m_case_options )
      co->deepGetChildren(col);
  }

  String rootTagName() const override { return m_ref_opt->rootTagName(); }

  bool isOptional() const override { return m_is_optional; }
  void setOptional(bool v) { m_is_optional = v; }

  void setRootElementWithParent(XmlNode parent_element) override
  {
    _setRootElement(false,parent_element);
  }

  void setRootElement(XmlNode root_element) override
  {
    if (!m_root_element.null())
      throw CaseOptionException("CaseOptionsList::setRootElement()","root_element already set",true);
    m_root_element = root_element;
    _setRootElement(true,XmlNode());
  }

  bool isPresent() const override
  {
    return m_is_present;
  }

  /*!
   * \brief Indique si l'option peut-être présente plusieurs fois.
   *
   * Cela sert à vérifier que l'élément correspondant de l'option n'est
   * présent qu'une seule fois si \a v est faux. Si \a v est vrai,
   * la vérification a lieu ailleurs. Cette fonction doit être appelée
   * avant readChildren() pour être pris en compte.
   */
  void setIsMulti(bool v)
  {
    m_is_multi = v;
  }

  String xpathFullName() const override { return m_root_element.xpathFullName(); }

 public:

  void addReference() override { ++m_nb_ref; }
  void removeReference() override
  {
    // Décrémente et retourne la valeur d'avant.
    // Si elle vaut 1, cela signifie qu'on n'a plus de références
    // sur l'objet et qu'il faut le détruire.
    Int32 v = std::atomic_fetch_add(&m_nb_ref,-1);
    if (v==1)
      delete this;
  }

 protected:

  void _addInvalidChildren(XmlNode parent,XmlNodeList& nlist);
  void _searchChildren(bool is_phase1);
  void _setRootElement(bool force_init,XmlNode parent_element);
  void _strIndent(char* buf,int indent,int max_indent);
  void _printOption(const String& lang,int indent,CaseOptionBase* co,std::ostream& o);
  bool _isValidChildTagName(const String& name);

 protected:

  ICaseMng* m_case_mng;
  XmlNode m_root_element;  //!< Elément racine pour cette liste d'options
  ICaseOptionList* m_parent;
  ICaseOptions* m_ref_opt;
  CaseOptionBasePairList m_config_list; //!< Liste des valeurs de configuration
  std::vector<ICaseOptions*> m_case_options;
  XmlNode m_parent_element; //!< Elément parent.
  bool m_is_present;
  bool m_is_multi;
  bool m_is_optional;
  std::atomic<Int32> m_nb_ref = 0;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionList::
readChildren(bool is_phase1)
{
  info(5) << "READ CHILDREN root=" << m_root_element.xpathFullName()
          << " parent=" << m_parent_element.xpathFullName()
          << " id=" << typeid(*this).name()
          << " phase1?=" << is_phase1
          << " this=" << this;

  if (!m_is_multi && !m_parent_element.null()){
    // Vérifie que l'élément n'est présent qu'une fois.
    XmlNodeList all_children = m_parent_element.children(rootTagName());
    if (all_children.size()>1){
      String node_name = m_parent_element.xpathFullName()+"/"+rootTagName();
      CaseOptionError::addWarning(caseDocument(),A_FUNCINFO,node_name,
                                  String::format("Only one token of the element is allowed (nb_occur={0})",
                                                 all_children.size()),true);
    }
  }
  _setRootElement(false,XmlNode());
  for( ConstIterT< std::vector<CaseOptionBasePair> > i(m_config_list); i(); ++i )
    i->first->setRootElement(m_root_element);
  _searchChildren(is_phase1);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionList::
printChildren(const String& lang,int indent)
{
  if (!m_is_present && isOptional())
    return;

  char str_indent[128];
  _strIndent(str_indent,indent,127);

  String service_name = "";
  IServiceInfo* service = m_ref_opt->caseServiceInfo();
  if (service)
    service_name = " name=\""+ service->localName() + "\"";
  info() << str_indent << "<" << rootTagName() << service_name << ">";
  for( ConstIterT<CaseOptionBasePairList> i(m_config_list); i(); ++i ){
    _printOption(lang,indent,i->first,info().file());
  }
  for( ICaseOptions* co : m_case_options ){
    co->printChildren(lang,indent+1);
  }
  info() << str_indent << "</" << rootTagName() << ">";
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionList::
visit(ICaseDocumentVisitor* visitor)
{
  if (!m_is_present && isOptional())
    return;

  visitor->beginVisit(m_ref_opt);
  for( ConstIterT<CaseOptionBasePairList> i(m_config_list); i(); ++i ){
    i->first->visit(visitor);
  }
  for( ICaseOptions* co : m_case_options ){
    co->visit(visitor);
  }
  visitor->endVisit(m_ref_opt);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionList::
addInvalidChildren(XmlNodeList& nlist)
{
  info(5) << "CHECK INVALID CHILDREN root=" << m_root_element.xpathFullName()
          << " parent=" << m_parent_element.xpathFullName()
          << " this=" << this;

  if (!m_root_element.null())
    _addInvalidChildren(m_root_element,nlist);

  // Récursion sur les fils
  for( ICaseOptions* co : m_case_options )
    co->addInvalidChildren(nlist);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionList::
_searchChildren(bool is_phase1)
{
  // Si je suis absent et que je suis optionel, ne fait rien.
  if (!m_is_present && isOptional())
    return;
  for( ConstIterT<CaseOptionBasePairList> i(m_config_list); i(); ++i )
    i->first->search(is_phase1);
  auto read_phase = (is_phase1) ? eCaseOptionReadPhase::Phase1 : eCaseOptionReadPhase::Phase2;
  for( ICaseOptions* co : m_case_options )
    co->read(read_phase);
}
	
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionList::
_setRootElement(bool force_init,XmlNode parent_element)
{
  // Ne fait rien si déjà positionné
  if (!m_root_element.null() && !force_init)
    return;
  if (force_init){
    if (m_root_element.null())
      throw CaseOptionException("CaseOptionsList::_setRootElement()","root_element not set",true);
    m_parent_element = m_root_element.parent();
  }
  else{
    if (!parent_element.null())
      m_parent_element = parent_element;
    if (m_parent_element.null())
      m_parent_element = (m_parent) ? m_parent->rootElement() : m_case_mng->caseDocument()->rootElement();
    m_root_element = m_parent_element.child(rootTagName());
  }
  // L'élément recherché n'existe pas. Il y a alors trois possibitités:
  // 1- Il s'agit d'un bloc d'option (cet élément est directement fils
  // de l'élément racine du document). Dans ce cas, on créé l'élément
  // correspondant ce qui permet d'avoir des modules optionnels.
  // 2- L'option n'est pas obligatoire. Dans ce cas on ne fait rien.
  // 3- L'option est obligatoire. On créé tout de même l'élément correspondant
  // car s'il ne comporte que des options par défaut, il n'est pas
  // nécessaire qu'il soit présent.
  if (m_root_element.null()){
    m_is_present = false;
    if (!m_parent){
      XmlNode case_root = m_case_mng->caseDocument()->rootElement();
      m_root_element = case_root.createAndAppendElement(rootTagName());
    }
    else if (!isOptional()){
      m_root_element = m_parent_element.createAndAppendElement(rootTagName());
      //throw ExConfigNotFound(msgMng(),"_setRootElement",rootTagName(),m_parent_element);
    }
  }
  else
    m_is_present = true;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionList::
_strIndent(char* buf,int indent,int max_indent)
{
  ++indent;

  if (indent>max_indent)
    indent = max_indent;

  for( int i=0; i<indent; ++i )
    buf[i] = ' ';
  buf[indent] = '\0';
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionList::
_printOption(const String& lang,int indent,CaseOptionBase* co,std::ostream& o)
{
  std::ios_base::fmtflags f = o.flags(std::ios::left);
  o << "  ";
  for( int i=0; i<indent; ++i )
    o << ' ';
  o.width(40-indent);
  o << co->name();
  co->print(lang,o);
  ICaseFunction* func = co->function();
  if (func){
    o << " (fonction: " << func->name() << ")";
  }
  o.flags(f);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

bool CaseOptionList::
_isValidChildTagName(const String& name)
{
  for( ConstIterT<CaseOptionBasePairList> i(m_config_list); i(); ++i ){
    CaseOptionBase* co = i->first;
    if (co->name()==name)
      return true;
  }
  for( ICaseOptions* co : m_case_options ){
    info(5) << "  CHECK CHILDREN this=" << this << " instance=" << co << " tag=" << co->rootTagName();
    if (co->rootTagName()==name)
      return true;
  }
  return false;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionList::
_addInvalidChildren(XmlNode parent,XmlNodeList& nlist)
{
  for( auto i = parent.begin(), end = parent.end(); i != end; ++i ){
    if (i->type()!=XmlNode::ELEMENT)
      continue;
    const String& name = i->name();
    bool is_valid = _isValidChildTagName(name);
    info(5) << " CHECK Valid tag_name=" << name << " is_valid=" << is_valid;
    if (!is_valid)
      nlist.add(*i);
  }
  XmlElementContentChecker xecc(caseDocument(),traceMng());
  xecc.check(parent);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \internal
 * \brief Liste d'options du jeu de données contenant plusieurs fils identiques.
 */
class CaseOptionListMulti
: public CaseOptionList
{
 public:

  typedef std::pair<CaseOptionBase*,XmlNode> CaseOptionBasePair;
  typedef std::vector<CaseOptionBasePair> CaseOptionBasePairList;

 public:
	
  CaseOptionListMulti(ICaseOptionsMulti* com,ICaseOptions* co,ICaseMng* m,
                      const XmlNode& element,Integer min_occurs,Integer max_occurs)
  : CaseOptionList(m,co,element), m_case_option_multi(com),
    m_min_occurs(min_occurs), m_max_occurs(max_occurs) {}
  CaseOptionListMulti(ICaseOptionsMulti* com,ICaseOptions* co,
                      ICaseOptionList* parent,const XmlNode& element,
                      Integer min_occurs,Integer max_occurs)
  : CaseOptionList(parent,co,element), m_case_option_multi(com),
    m_min_occurs(min_occurs), m_max_occurs(max_occurs) {}

 public:

  bool isOptional() const override { return true; }
  void readChildren(bool is_phase1) override;
  void addInvalidChildren(XmlNodeList& nlist) override;
  void printChildren(const String& lang,int indent) override;
  void deepGetChildren(Array<CaseOptionBase*>& col) override;
  void visit(ICaseDocumentVisitor* visitor) override;
  void _checkMinMaxOccurs(Integer nb_occur);

 protected:

  XmlNode _rootElement(Integer position) const
  {
    return m_root_element_list[position];
  }

 private:

  ICaseOptionsMulti* m_case_option_multi;
  UniqueArray<ICaseOptionList*> m_case_config_list;
  XmlNodeList m_root_element_list;
  Integer m_min_occurs;
  Integer m_max_occurs;

 private:
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionListMulti::
readChildren(bool is_phase1)
{
  if (is_phase1){
    _setRootElement(false,XmlNode());

    //debug() << "ReadConfig " << rootTagName();

    m_root_element_list = m_parent_element.children(rootTagName());
    m_case_config_list.clear();
    Integer s = m_root_element_list.size();
    _checkMinMaxOccurs(s);
    if (s!=0)
      m_case_option_multi->multiAllocate(m_root_element_list);
    // Récupère les options créés lors de l'appel à 'multiAllocate' et
    // les ajoute à la liste.
    Integer nb_children = m_case_option_multi->nbChildren();
    for( Integer i=0; i<nb_children; ++i ){
      ICaseOptionList* co_value = m_case_option_multi->child(i);
      co_value->setRootElement(m_root_element_list[i]);
      m_case_config_list.add(co_value);
    }
  }
  for( ICaseOptionList* opt : m_case_config_list ){
    opt->readChildren(is_phase1);
  }
  _searchChildren(is_phase1);
  // Normalement on ne doit pas avoir d'éléments dans 'm_config_list'.
  if (m_config_list.size()!=0)
    ARCANE_FATAL("Invalid 'm_config_list' size ({1}) for option '{0}'",rootTagName(),m_config_list.size());
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionListMulti::
addInvalidChildren(XmlNodeList& nlist)
{
  for( ICaseOptions* co : m_case_options )
    co->addInvalidChildren(nlist);
  for( ICaseOptionList* opt : m_case_config_list ){
    opt->addInvalidChildren(nlist);
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionListMulti::
printChildren(const String& lang,int indent)
{
  for( ICaseOptions* co : m_case_options )
    co->printChildren(lang,indent);
  for( ICaseOptionList* opt : m_case_config_list )
    opt->printChildren(lang,indent);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionListMulti::
deepGetChildren(Array<CaseOptionBase*>& col)
{
  for( ICaseOptions* co : m_case_options )
    co->deepGetChildren(col);
  for( ICaseOptionList* opt : m_case_config_list )
    opt->deepGetChildren(col);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionListMulti::
visit(ICaseDocumentVisitor* visitor)
{
  for( ICaseOptions* co : m_case_options )
    co->visit(visitor);
  for( ICaseOptionList* opt_list : m_case_config_list )
    opt_list->visit(visitor);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CaseOptionListMulti::
_checkMinMaxOccurs(Integer nb_occur)
{
  if (nb_occur<m_min_occurs){
    String node_name = m_parent_element.xpathFullName()+"/"+rootTagName();
    CaseOptionError::addError(caseDocument(),A_FUNCINFO,node_name,
                              String::format("Bad number of occurences (less than min)"
                                             " nb_occur={0}"
                                             " min_occur={1}",
                                             nb_occur,m_min_occurs),true);
  }
  if (m_max_occurs>=0)
    if (nb_occur>m_max_occurs){
      String node_name = m_parent_element.xpathFullName()+"/"+rootTagName();
      CaseOptionError::addError(caseDocument(),A_FUNCINFO,node_name,
                                String::format("Bad number of occurences (greater than max)"
                                               " nb_occur={0}"
                                               " max_occur={1}",
                                               nb_occur,m_max_occurs),true);
    }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

extern "C++" ICaseOptionList*
createCaseOptionList(ICaseMng* m,ICaseOptions* ref_opt,XmlNode parent_element)
{
  return new CaseOptionList(m,ref_opt,parent_element);
}

extern "C++" ICaseOptionList*
createCaseOptionList(ICaseOptionList* parent,ICaseOptions* ref_opt,XmlNode parent_element)
{
  return new CaseOptionList(parent,ref_opt,parent_element);
}

extern "C++" ICaseOptionList*
createCaseOptionList(ICaseOptionList* parent,ICaseOptions* ref_opt,XmlNode parent_element,
                     bool is_optional,bool is_multi)
{
  auto x = new CaseOptionList(parent,ref_opt,parent_element);
  if (is_optional)
    x->setOptional(true);
  if (is_multi)
    x->setIsMulti(true);
  return x;
}

extern "C++" ICaseOptionList*
createCaseOptionList(ICaseOptionsMulti* com,ICaseOptions* co,ICaseMng* m,
                     const XmlNode& element,Integer min_occurs,Integer max_occurs)
{
  return new CaseOptionListMulti(com,co,m,element,min_occurs,max_occurs);
}

extern "C++" ICaseOptionList*
createCaseOptionList(ICaseOptionsMulti* com,ICaseOptions* co,
                     ICaseOptionList* parent,const XmlNode& element,
                     Integer min_occurs,Integer max_occurs)
{
  return new CaseOptionListMulti(com,co,parent,element,min_occurs,max_occurs);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arcane

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
