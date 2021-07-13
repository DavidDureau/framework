﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* CartesianMesh.cc                                            (C) 2000-2020 */
/*                                                                           */
/* Maillage cartésien.                                                       */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/utils/TraceAccessor.h"
#include "arcane/utils/NotImplementedException.h"
#include "arcane/utils/AutoDestroyUserData.h"
#include "arcane/utils/IUserDataList.h"
#include "arcane/utils/Ref.h"
#include "arcane/utils/ScopedPtr.h"

#include "arcane/IMesh.h"
#include "arcane/ItemPrinter.h"
#include "arcane/IItemFamily.h"
#include "arcane/IParallelMng.h"
#include "arcane/VariableTypes.h"
#include "arcane/Properties.h"
#include "arcane/IMeshModifier.h"
#include "arcane/MeshStats.h"

#include "arcane/cea/ICartesianMesh.h"
#include "arcane/cea/CartesianConnectivity.h"
#include "arcane/cea/internal/CartesianMeshPatch.h"

#include <set>

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \defgroup ArcaneCartesianMesh Maillages cartésiens.
 *
 * Ensemble des classes assurant la gestion des maillage cartésiens.
 *
 * Pour plus de renseignements, se reporter à la page \ref arcanedoc_cartesianmesh.
 */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Infos spécifiques à un maillage cartésien.
 */
class CartesianMesh
: public TraceAccessor
, public ICartesianMesh
{
 public:

  CartesianMesh(IMesh* mesh);
  ~CartesianMesh() override;

 public:

  void build() override;

  //! Maillage associé à ce maillage cartésien
  IMesh* mesh() const override { return m_mesh; }

  //! Gestionnaire de trace associé.
  ITraceMng* traceMng() const override { return TraceAccessor::traceMng(); }

  CellDirectionMng cellDirection(eMeshDirection dir) override
  {
    return m_all_items_direction_info->cellDirection(dir);
  }

  CellDirectionMng cellDirection(Integer idir) override
  {
    return m_all_items_direction_info->cellDirection(idir);
  }

  FaceDirectionMng faceDirection(eMeshDirection dir) override
  {
    return m_all_items_direction_info->faceDirection(dir);
  }

  FaceDirectionMng faceDirection(Integer idir) override
  {
    return m_all_items_direction_info->faceDirection(idir);
  }

  NodeDirectionMng nodeDirection(eMeshDirection dir) override
  {
    return m_all_items_direction_info->nodeDirection(dir);
  }

  NodeDirectionMng nodeDirection(Integer idir) override
  {
    return m_all_items_direction_info->nodeDirection(idir);
  }

  void computeDirections() override;

  void recreateFromDump() override;

  CartesianConnectivity connectivity() override
  {
    return m_connectivity;
  }

  Integer nbPatch() const override { return m_amr_patches.size(); }
  ICartesianMeshPatch* patch(Integer index) const { return m_amr_patches[index].get(); }

  void refinePatch2D(Real2 position,Real2 length) override;

  void checkValid() const override;

 private:

  //! Indice dans la numérotation locale de la maille, de la face dans
  // la direction X, Y ou Z
  Int32 m_local_face_direction[3];
  IMesh* m_mesh = nullptr;
  Ref<CartesianMeshPatch> m_all_items_direction_info;
  CartesianConnectivity m_connectivity;
  UniqueArray<CartesianConnectivity::Index> m_nodes_to_cell_storage;
  UniqueArray<CartesianConnectivity::Index> m_cells_to_node_storage;
  bool m_is_amr = false;
  //! Groupe de mailles parentes pour chaque patch AMR.
  UniqueArray<CellGroup> m_amr_patch_cell_groups;
  UniqueArray<Ref<CartesianMeshPatch>> m_amr_patches;
  ScopedPtrT<Properties> m_properties;

 private:

  void _computeMeshDirection(CartesianMeshPatch& cdi,eMeshDirection dir,
                             VariableCellReal3& cells_center,
                             VariableFaceReal3& faces_center,CellGroup all_cells,
                             NodeGroup all_nodes);
  void _applyRefine(ConstArrayView<Int32> cells_local_id);
  void _saveInfosInProperties();

  std::tuple<CellGroup,NodeGroup>
  _buildPatchGroups(const CellGroup& cells,Integer patch_level);
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

extern "C++" ICartesianMesh*
arcaneCreateCartesianMesh(IMesh* mesh)
{
  CartesianMesh* cm = new CartesianMesh(mesh);
  cm->build();
  return cm;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

CartesianMesh::
CartesianMesh(IMesh* mesh)
: TraceAccessor(mesh->traceMng())
, m_mesh(mesh)
{
  m_all_items_direction_info = makeRef(new CartesianMeshPatch(this,-1));
  m_amr_patches.add(m_all_items_direction_info);
  Integer nb_dir = mesh->dimension();
  for( Integer i=0; i<nb_dir; ++i ){
    m_local_face_direction[i] = -1;
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

CartesianMesh::
~CartesianMesh()
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CartesianMesh::
build()
{
  m_properties = new Properties(*(mesh()->properties()),"CartesianMesh");
}

namespace
{
const Int32 SERIALIZE_VERSION = 1;
}

void CartesianMesh::
_saveInfosInProperties()
{
  // Sauve le numéro de version pour être sur que c'est OK en reprise
  m_properties->set("Version",SERIALIZE_VERSION);

  // Sauve les informations des patches
  UniqueArray<String> patch_group_names;
  for( const CellGroup& x : m_amr_patch_cell_groups ){
    patch_group_names.add(x.name());
  }
  m_properties->set("PatchGroupNames",patch_group_names);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CartesianMesh::
recreateFromDump()
{
  info() << "Creating 'CartesianMesh' infos from dump";

  // Sauve le numéro de version pour être sur que c'est OK en reprise
  Int32 v = m_properties->getInt32("Version");
  if (v!=SERIALIZE_VERSION)
    ARCANE_FATAL("Bad serializer version: trying to read from incompatible checkpoint v={0} expected={1}",
                 v,SERIALIZE_VERSION);

  // Récupère les noms des groupes des patchs
  UniqueArray<String> patch_group_names;
  m_properties->get("PatchGroupNames",patch_group_names);
  info(4) << "Found n=" << patch_group_names.size() << " patchs";
  m_amr_patch_cell_groups.clear();
  IItemFamily* cell_family = m_mesh->cellFamily();
  for( const String& x : patch_group_names ){
    CellGroup group = cell_family->findGroup(x);
    if (group.null())
      ARCANE_FATAL("Can not find cell group '{0}'",x);
    m_amr_patch_cell_groups.add(group);
  }

  computeDirections();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CartesianMesh::
computeDirections()
{
  m_amr_patches.clear();
  m_amr_patches.add(m_all_items_direction_info);

  m_is_amr = m_mesh->isAmrActivated();

  VariableCellReal3 cells_center(VariableBuildInfo(m_mesh,"TemporaryCartesianMeshCellCenter"));
  VariableFaceReal3 faces_center(VariableBuildInfo(m_mesh,"TemporaryCartesianMeshFaceCenter"));

  // Calcule les coordonnées du centre des mailles.
  VariableNodeReal3& nodes_coord = m_mesh->nodesCoordinates();
  ENUMERATE_CELL(icell,m_mesh->allCells()){
    Cell cell = *icell;
    Real3 center;
    for( NodeEnumerator inode(cell.nodes()); inode.hasNext(); ++inode )
      center += nodes_coord[inode];
    center /= cell.nbNode();
    cells_center[icell] = center;
  }
  ENUMERATE_FACE(iface,m_mesh->allFaces()){
    Face face = *iface;
    Real3 center;
    for( NodeEnumerator inode(face.nodes()); inode.hasNext(); ++inode )
      center += nodes_coord[inode];
    center /= face.nbNode();
    faces_center[iface] = center;
  }

  IItemFamily* cell_family = m_mesh->cellFamily();
  IItemFamily* node_family = m_mesh->nodeFamily();
  Int32 next_face_x = -1;
  Int32 next_face_y = -1;
  Int32 next_face_z = -1;

  CellVectorView cell_view = cell_family->allItems().view();
  Cell cell0 = cell_view[0];
  Integer nb_face = cell0.nbFace();
  Integer nb_node = cell0.nbNode();
  Real3 cell_center = cells_center[cell0];

  info(4) << "Cartesian mesh compute directions is_amr=" << m_is_amr;

  for( Integer i=0; i<nb_node; ++i ){
    Node node = cell0.node(i);
    info(4) << "Node I=" << i << " node=" << ItemPrinter(node) << " pos=" << nodes_coord[node];
  }

  // On suppose que toutes les mailles ont le même sens de numérotation dans le maillage.
  // Par exemple, pour toutes les mailles, la face d'indice 0 est celle du haut, celle
  // d'indice 1 celle de droite.
  for( Integer i=0; i<nb_face; ++i ){
    Face f = cell0.face(i);
    if (f.isSubDomainBoundary())
      continue;
    Cell next_cell = (f.backCell()==cell0) ? f.frontCell() : f.backCell();
    Real3 next_center = cells_center[next_cell];
    info(4) << "NEXT_CELL=" << ItemPrinter(next_cell) << " center=" << next_center \
            << " back=" << f.backCell().uniqueId()
            << " front=" << f.frontCell().uniqueId();

    Real diff_x = math::abs(next_center.x - cell_center.x);
    Real diff_y = math::abs(next_center.y - cell_center.y);
    Real diff_z = math::abs(next_center.z - cell_center.z);
    info(4) << "NEXT_CELL=" << ItemPrinter(next_cell) << " diff=" << Real3(diff_x,diff_y,diff_z);
    //TODO: Verifier qu'il s'agit bien de la maille apres et pas avant.
    // (tenir compte du signe de diff)
    if (diff_x>diff_y && diff_x>diff_z){
      // INC X
      next_face_x = i;
      info() << "Advance in direction X -> " << next_face_x;
    }
    else if (diff_y>diff_x && diff_y>diff_z){
      // INC Y
      next_face_y = i;
      info() << "Advance in direction Y -> " << next_face_y;
    }
    else if (diff_z>diff_x && diff_z>diff_y){
      // INC Z
      next_face_z = i;
      info() << "Advance in direction Z -> " << next_face_z;
    }
    else
      ARCANE_FATAL("Bad value for next cell");
  }

  bool is_3d = m_mesh->dimension()==3;
  m_all_items_direction_info->_internalComputeNodeCellInformations(cell0,cells_center[cell0],nodes_coord);

  info() << "Informations from IMesh properties:";
  Properties* mesh_properties = mesh()->properties();
  Int64 global_nb_cell_x = mesh_properties->getInt64WithDefault("GlobalNbCellX",-1);
  Int64 global_nb_cell_y = mesh_properties->getInt64WithDefault("GlobalNbCellY",-1);
  Int64 global_nb_cell_z = mesh_properties->getInt64WithDefault("GlobalNbCellZ",-1);
  info() << "GlobalNbCell: "
         << " X=" << global_nb_cell_x
         << " Y=" << global_nb_cell_y
         << " Z=" << global_nb_cell_z;

  Int32 own_nb_cell_x = mesh_properties->getInt32WithDefault("OwnNbCellX",-1);
  Int32 own_nb_cell_y = mesh_properties->getInt32WithDefault("OwnNbCellY",-1);
  Int32 own_nb_cell_z = mesh_properties->getInt32WithDefault("OwnNbCellZ",-1);
  info() << "OwnNbCell: "
         << " X=" << own_nb_cell_x
         << " Y=" << own_nb_cell_y
         << " Z=" << own_nb_cell_z;

  Int32 sub_domain_offset_x = mesh_properties->getInt32WithDefault("SubDomainOffsetX",-1);
  Int32 sub_domain_offset_y = mesh_properties->getInt32WithDefault("SubDomainOffsetY",-1);
  Int32 sub_domain_offset_z = mesh_properties->getInt32WithDefault("SubDomainOffsetZ",-1);
  info() << "SubDomainOffset: "
         << " X=" << sub_domain_offset_x
         << " Y=" << sub_domain_offset_y
         << " Z=" << sub_domain_offset_z;

  Int64 own_cell_offset_x = mesh_properties->getInt64WithDefault("OwnCellOffsetX",-1);
  Int64 own_cell_offset_y = mesh_properties->getInt64WithDefault("OwnCellOffsetY",-1);
  Int64 own_cell_offset_z = mesh_properties->getInt64WithDefault("OwnCellOffsetZ",-1);
  info() << "OwnCellOffset: "
         << " X=" << own_cell_offset_x
         << " Y=" << own_cell_offset_y
         << " Z=" << own_cell_offset_z;

  CellGroup all_cells = cell_family->allItems();
  NodeGroup all_nodes = node_family->allItems();
  if (m_is_amr){
    auto x = _buildPatchGroups(mesh()->allLevelCells(0),0);
    all_cells = std::get<0>(x);
    all_nodes = std::get<1>(x);
  }

  if (next_face_x!=(-1)){
    m_local_face_direction[MD_DirX] = next_face_x;
    _computeMeshDirection(*m_all_items_direction_info.get(),MD_DirX,cells_center,faces_center,all_cells,all_nodes);
    CellDirectionMng& cdm = m_all_items_direction_info->cellDirection(MD_DirX);
    cdm.m_global_nb_cell = global_nb_cell_x;
    cdm.m_own_nb_cell = own_nb_cell_x;
    cdm.m_sub_domain_offset = sub_domain_offset_x;
    cdm.m_own_cell_offset = own_cell_offset_x;
  }
  if (next_face_y!=(-1)){
    m_local_face_direction[MD_DirY] = next_face_y;
    _computeMeshDirection(*m_all_items_direction_info.get(),MD_DirY,cells_center,faces_center,all_cells,all_nodes);
    CellDirectionMng& cdm = m_all_items_direction_info->cellDirection(MD_DirY);
    cdm.m_global_nb_cell = global_nb_cell_y;
    cdm.m_own_nb_cell = own_nb_cell_y;
    cdm.m_sub_domain_offset = sub_domain_offset_y;
    cdm.m_own_cell_offset = own_cell_offset_y;
  }
  if (next_face_z!=(-1)){
    m_local_face_direction[MD_DirZ] = next_face_z;
    _computeMeshDirection(*m_all_items_direction_info.get(),MD_DirZ,cells_center,faces_center,all_cells,all_nodes);
    CellDirectionMng& cdm = m_all_items_direction_info->cellDirection(MD_DirZ);
    cdm.m_global_nb_cell = global_nb_cell_z;
    cdm.m_own_nb_cell = own_nb_cell_z;
    cdm.m_sub_domain_offset = sub_domain_offset_z;
    cdm.m_own_cell_offset = own_cell_offset_z;
  }

  info() << "Compute cartesian connectivity";

  m_nodes_to_cell_storage.resize(mesh()->nodeFamily()->maxLocalId());
  m_cells_to_node_storage.resize(mesh()->cellFamily()->maxLocalId());
  m_connectivity.setStorage(m_nodes_to_cell_storage,m_cells_to_node_storage);
  m_connectivity.computeInfos(mesh(),nodes_coord,cells_center);

  // Ajoute informations de connectivités pour les patchs AMR
  // TODO: supporter plusieurs appels à cette méthode
  for( const CellGroup& cells : m_amr_patch_cell_groups ){
    Integer patch_index = m_amr_patches.size();
    info() << "AMR Patch name=" << cells.name() << " size=" << cells.size() << " index=" << patch_index;
    auto* cdi = new CartesianMeshPatch(this,patch_index);
    m_amr_patches.add(makeRef(cdi));
    cdi->_internalComputeNodeCellInformations(cell0,cells_center[cell0],nodes_coord);
    auto [ patch_cells, patch_nodes ] = _buildPatchGroups(cells,patch_index);
    _computeMeshDirection(*cdi,MD_DirX,cells_center,faces_center,patch_cells,patch_nodes);
    _computeMeshDirection(*cdi,MD_DirY,cells_center,faces_center,patch_cells,patch_nodes);
    if (is_3d)
      _computeMeshDirection(*cdi,MD_DirZ,cells_center,faces_center,patch_cells,patch_nodes);
  }

  if (arcaneIsCheck())
    checkValid();

  _saveInfosInProperties();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

std::tuple<CellGroup,NodeGroup> CartesianMesh::
_buildPatchGroups(const CellGroup& cells,Integer patch_level)
{
  // On créé un groupe pour chaque patch en garantissant que l'ordre de parcours
  // est celui des uniqueId() des entités
  // TODO: à terme, il faudrait que l'ordre de parcours soit le même que
  // celui du maillage cartésien. Pour cela, il faut soit que les uniqueId()
  // des mailles/noeuds créés soient dans le même ordre que le maillage cartésien,
  // soit que la fonction de tri soit spécifique à ce type de maillage.
  NodeGroup nodes = cells.nodeGroup();
  IItemFamily* cell_family = cells.itemFamily();
  IItemFamily* node_family = nodes.itemFamily();

  String cell_group_name = String("AMRPatchCells") + patch_level;
  CellGroup patch_cells = cell_family->createGroup(cell_group_name,Int32ConstArrayView(),true);
  // Met les mêmes mailles que \a cells mais force le tri
  patch_cells.setItems(cells.view().localIds(),true);

  String node_group_name = String("AMRPatchNodes") + patch_level;
  NodeGroup patch_nodes = node_family->createGroup(node_group_name,Int32ConstArrayView(),true);
  // Met les mêmes noeuds que \a nodes mais force le tri
  patch_nodes.setItems(nodes.view().localIds(),true);
  info(4) << "PATCH_CELLS name=" << patch_cells.name() << " size=" << patch_cells.size();
  info(4) << "PATCH_NODES name=" << patch_nodes.name() << " size=" << patch_nodes.size();
  return { patch_cells, patch_nodes };
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CartesianMesh::
_computeMeshDirection(CartesianMeshPatch& cdi,eMeshDirection dir,VariableCellReal3& cells_center,
                      VariableFaceReal3& faces_center,CellGroup all_cells,NodeGroup all_nodes)
{
  IItemFamily* cell_family = m_mesh->cellFamily();
  IItemFamily* face_family = m_mesh->faceFamily();
  IItemFamily* node_family = m_mesh->nodeFamily();

  Int32 max_cell_id = cell_family->maxLocalId();
  Int32 max_face_id = face_family->maxLocalId();
  Int32 max_node_id = node_family->maxLocalId();

  CellDirectionMng& cell_dm = cdi.cellDirection(dir);
  cell_dm.m_infos.resize(max_cell_id);

  FaceDirectionMng& face_dm = cdi.faceDirection(dir);
  face_dm.m_infos.resize(max_face_id);

  NodeDirectionMng& node_dm = cdi.nodeDirection(dir);
  node_dm.m_infos.resize(max_node_id);

  //TODO: attention à remettre à jour après changement de maillage.
  info(4) << "COMPUTE DIRECTION dir=" << dir;

  Int32 prev_local_face = -1;
  Int32 next_local_face = m_local_face_direction[dir];
  Integer mesh_dim = m_mesh->dimension();
  // Calcul le numero local de face oppose à la face suivante.
  if (mesh_dim==2)
    prev_local_face = (next_local_face + 2) % 4;
  else if (mesh_dim==3)
    prev_local_face = (next_local_face + 3) % 6;

  cell_dm._internalSetLocalFaceIndex(next_local_face,prev_local_face);

  // Positionne pour chaque maille les faces avant et après dans la direction.
  // On s'assure que ces entités sont dans le groupe des entités de la direction correspondante
  std::set<Int32> cells_set;
  ENUMERATE_CELL(icell,all_cells){
    cells_set.insert(icell.itemLocalId());
  }

  // Calcule les mailles devant/derrière. En cas de patch AMR, il faut que ces deux mailles
  // soient de même niveau
  ENUMERATE_CELL(icell,all_cells){
    Cell cell = *icell;
    Int32 my_level = cell.level();
    Face next_face = cell.face(next_local_face);
    Cell next_cell = next_face.backCell()==cell ? next_face.frontCell() : next_face.backCell();
    if (cells_set.find(next_cell.localId())==cells_set.end())
      next_cell = Cell();
    else if (next_cell.level()!=my_level)
      next_cell = Cell();

    Face prev_face = cell.face(prev_local_face);
    Cell prev_cell = prev_face.backCell()==cell ? prev_face.frontCell() : prev_face.backCell();
    if (cells_set.find(prev_cell.localId())==cells_set.end())
      prev_cell = Cell();
    else if (prev_cell.level()!=my_level)
      prev_cell = Cell();
    cell_dm.m_infos[icell.itemLocalId()] = CellDirectionMng::ItemDirectionInfo(next_cell.internal(),prev_cell.internal());
  }
  cell_dm._internalComputeInnerAndOuterItems(all_cells);
  face_dm._internalComputeInfos(cell_dm,cells_center,faces_center);
  node_dm._internalComputeInfos(cell_dm,all_nodes,cells_center);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CartesianMesh::
refinePatch2D(Real2 position,Real2 length)
{
  VariableNodeReal3& nodes_coord = m_mesh->nodesCoordinates();
  UniqueArray<Int32> cells_local_id;
  // Parcours les mailles actives et ajoute dans la liste des mailles
  // à raffiner celles qui sont contenues dans le boîte englobante
  // spécifiée dans le jeu de données.
  info() << "REFINEMENT ..." << position << " L=" << length;
  Real2 min_pos = position;
  Real2 max_pos = min_pos + length;
  cells_local_id.clear();
  ENUMERATE_CELL(icell,m_mesh->allActiveCells()){
    Cell cell = *icell;
    Real3 center;
    for( NodeEnumerator inode(cell.nodes()); inode.hasNext(); ++inode )
      center += nodes_coord[inode];
    center /= cell.nbNode();
    if (center.x>min_pos.x && center.x<max_pos.x && center.y>min_pos.y && center.y<max_pos.y)
      cells_local_id.add(icell.itemLocalId());
  }
  _applyRefine(cells_local_id);
  _saveInfosInProperties();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CartesianMesh::
_applyRefine(ConstArrayView<Int32> cells_local_id)
{
  IItemFamily* cell_family = m_mesh->cellFamily();
  Integer nb_cell = cells_local_id.size();
  info(4) << "Local_NbCellToRefine = " << nb_cell;
  Integer index = m_amr_patch_cell_groups.size();
  String parent_group_name = String("CartesianMeshPatchParentCells")+index;
  CellGroup parent_cells = cell_family->createGroup(parent_group_name,cells_local_id,true);

  IParallelMng* pm = m_mesh->parallelMng();
  Int64 total_nb_cell = pm->reduce(Parallel::ReduceSum,nb_cell);
  info(4) << "Global_NbCellToRefine = " << total_nb_cell;
  if (total_nb_cell==0)
    return;
  m_mesh->modifier()->flagCellToRefine(cells_local_id);
  m_mesh->modifier()->adapt();
  {
    MeshStats ms(traceMng(),m_mesh,m_mesh->parallelMng());
    ms.dumpStats();
  }
  {
    // Créé le groupe contenant les mailles AMR
    // Il s'agit des mailles filles de \a parent_cells
    String children_group_name = String("CartesianMeshPatchCells")+index;
    UniqueArray<Int32> children_local_id;
    ENUMERATE_CELL(icell,parent_cells){
      Cell c = *icell;
      for(Integer k=0; k<c.nbHChildren(); ++k ){
        Cell child = c.hChild(k);
        children_local_id.add(child.localId());
      }
    }
    CellGroup children_cells = cell_family->createGroup(children_group_name,children_local_id,true);
    m_amr_patch_cell_groups.add(children_cells);
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void CartesianMesh::
checkValid() const
{
  info(4) << "Check valid CartesianMesh";
  Integer nb_patch = nbPatch();
  for( Integer i=0; i<nb_patch; ++i ){
    ICartesianMeshPatch* p = patch(i);
    p->checkValid();
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ICartesianMesh* ICartesianMesh::
getReference(IMesh* mesh,bool create)
{
  //TODO: faire lock pour multi-thread
  const char* name = "CartesianMesh";
  IUserDataList* udlist = mesh->userDataList();

  IUserData* ud = udlist->data(name,true);
  if (!ud){
    if (!create)
      return 0;
    ICartesianMesh* cm = arcaneCreateCartesianMesh(mesh);
    udlist->setData(name,new AutoDestroyUserData<ICartesianMesh>(cm));
    return cm;
  }
  AutoDestroyUserData<ICartesianMesh>* adud = dynamic_cast<AutoDestroyUserData<ICartesianMesh>*>(ud);
  if (!adud)
    ARCANE_FATAL("Can not cast to ICartesianMesh*");
  return adud->data();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arcane

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
