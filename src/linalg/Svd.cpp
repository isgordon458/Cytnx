#include "linalg.hpp"
#include "linalg_internal_interface.hpp"
#include "Tensor.hpp"
#include "UniTensor.hpp"
#include "algo.hpp"
#include <iostream>
#include <vector>

using namespace std;
namespace cytnx {
  namespace linalg {

    std::vector<Tensor> Svd(const Tensor &Tin, const bool &is_U, const bool &is_vT) {
      cytnx_error_msg(Tin.shape().size() != 2,
                      "[Svd] error, Svd can only operate on rank-2 Tensor.%s", "\n");
      // cytnx_error_msg(!Tin.is_contiguous(), "[Svd] error tensor must be contiguous. Call
      // Contiguous_() or Contiguous() first%s","\n");

      cytnx_uint64 n_singlu = std::max(cytnx_uint64(1), std::min(Tin.shape()[0], Tin.shape()[1]));

      Tensor in = Tin.contiguous();
      if (Tin.dtype() > Type.Float) in = in.astype(Type.Double);

      // std::cout << n_singlu << std::endl;

      Tensor U, S, vT;
      S.Init({n_singlu}, in.dtype() <= 2 ? in.dtype() + 2 : in.dtype(),
             in.device());  // if type is complex, S should be real
      S.storage().set_zeros();
      if (is_U) {
        U.Init({in.shape()[0], n_singlu}, in.dtype(), in.device());
        U.storage().set_zeros();
      }
      if (is_vT) {
        vT.Init({n_singlu, in.shape()[1]}, in.dtype(), in.device());
        vT.storage().set_zeros();
      }

      if (Tin.device() == Device.cpu) {
        cytnx::linalg_internal::lii.Svd_ii[in.dtype()](
          in._impl->storage()._impl, U._impl->storage()._impl, vT._impl->storage()._impl,
          S._impl->storage()._impl, in.shape()[0], in.shape()[1]);

        std::vector<Tensor> out;
        out.push_back(S);
        if (is_U) out.push_back(U);
        if (is_vT) out.push_back(vT);

        return out;

      } else {
#ifdef UNI_GPU
        checkCudaErrors(cudaSetDevice(in.device()));
        cytnx::linalg_internal::lii.cuSvd_ii[in.dtype()](
          in._impl->storage()._impl, U._impl->storage()._impl, vT._impl->storage()._impl,
          S._impl->storage()._impl, in.shape()[0], in.shape()[1]);

        std::vector<Tensor> out;
        out.push_back(S);
        if (is_U) out.push_back(U);
        if (is_vT) out.push_back(vT);

        return out;
#else
        cytnx_error_msg(true, "[Svd] fatal error,%s",
                        "try to call the gpu section without CUDA support.\n");
        return std::vector<Tensor>();
#endif
      }
    }

  }  // namespace linalg

}  // namespace cytnx

namespace cytnx {
  namespace linalg {

    // actual impls: 
    void _svd_Dense_UT(std::vector<cytnx::UniTensor> &outCyT, const cytnx::UniTensor &Tin, const bool &is_U,
                                      const bool &is_vT){
        //[Note] outCyT must be empty!    
    
        // DenseUniTensor:
        // cout << "entry Dense UT" << endl;

        Tensor tmp;
        if (Tin.is_contiguous())
          tmp = Tin.get_block_();
        else {
          tmp = Tin.get_block();
          tmp.contiguous_();
        }

        vector<cytnx_uint64> tmps = tmp.shape();
        vector<cytnx_int64> oldshape(tmps.begin(), tmps.end());
        tmps.clear();
        vector<string> oldlabel = Tin.labels();

        // collapse as Matrix:
        cytnx_int64 rowdim = 1;
        for (cytnx_uint64 i = 0; i < Tin.rowrank(); i++) rowdim *= tmp.shape()[i];
        tmp.reshape_({rowdim, -1});

        vector<Tensor> outT = cytnx::linalg::Svd(tmp, is_U, is_vT);
        if (Tin.is_contiguous()) tmp.reshape_(oldshape);

        int t = 0;
        outCyT.resize(outT.size());

        // s
        cytnx::UniTensor &Cy_S = outCyT[t];
        cytnx::Bond newBond(outT[t].shape()[0]);
        // cytnx_int64 newlbl = -1;
        // for (int i = 0; i < oldlabel.size(); i++) {
        //   if (oldlabel[i] <= newlbl) newlbl = oldlabel[i] - 1;
        // }
        string newlbl = "newlbl";
        for (int i = 0; i < oldlabel.size(); i++) {
          if (oldlabel[i] == newlbl) newlbl = newlbl + "new";
        }
        // Cy_S.Init({newBond, newBond}, {newlbl, newlbl - 1}, 1, Type.Double, Device.cpu,
        //           true);  // it is just reference so no hurt to alias ^^
        Cy_S.Init({newBond, newBond}, {newlbl, newlbl + "new"}, 1, Type.Double, Device.cpu,
                  true);  // it is just reference so no hurt to alias ^^
        // cout << "[AFTER INIT]" << endl;
        Cy_S.put_block_(outT[t]);
        t++;
        if (is_U) {
          cytnx::UniTensor &Cy_U = outCyT[t];
          vector<cytnx_int64> shapeU = vec_clone(oldshape, Tin.rowrank());
          shapeU.push_back(-1);
          outT[t].reshape_(shapeU);
          Cy_U.Init(outT[t], false, Tin.rowrank());
          vector<string> labelU = vec_clone(oldlabel, Tin.rowrank());
          labelU.push_back(Cy_S.labels()[0]);
          Cy_U.set_labels(labelU);
          t++;  // U
        }

        if (is_vT) {
          cytnx::UniTensor &Cy_vT = outCyT[t];
          vector<cytnx_int64> shapevT(Tin.rank() - Tin.rowrank() + 1);
          shapevT[0] = -1;
          memcpy(&shapevT[1], &oldshape[Tin.rowrank()], sizeof(cytnx_int64) * (shapevT.size() - 1));

          outT[t].reshape_(shapevT);
          Cy_vT.Init(outT[t], false, 1);
          vector<string> labelvT(shapevT.size());
          labelvT[0] = Cy_S.labels()[1];
          memcpy(&labelvT[1], &oldlabel[Tin.rowrank()], sizeof(cytnx_int64) * (labelvT.size() - 1));
          Cy_vT.set_labels(labelvT);
          t++;  // vT
        }

        // if tag, then update  the tagging informations
        if (Tin.is_tag()) {
          Cy_S.tag();
          t = 1;
          if (is_U) {
            cytnx::UniTensor &Cy_U = outCyT[t];
            Cy_U._impl->_is_tag = true;
            for (int i = 0; i < Cy_U.rowrank(); i++) {
              Cy_U.bonds()[i].set_type(Tin.bonds()[i].type());
            }
            Cy_U.bonds().back().set_type(cytnx::BD_BRA);
            Cy_U._impl->_is_braket_form = Cy_U._impl->_update_braket();
            t++;
          }
          if (is_vT) {
            cytnx::UniTensor &Cy_vT = outCyT[t];
            Cy_vT._impl->_is_tag = true;
            Cy_vT.bonds()[0].set_type(cytnx::BD_KET);
            for (int i = 1; i < Cy_vT.rank(); i++) {
              Cy_vT.bonds()[i].set_type(Tin.bonds()[Tin.rowrank() + i - 1].type());
            }
            Cy_vT._impl->_is_braket_form = Cy_vT._impl->_update_braket();
            t++;
          }

        }  // if tag

        
    }

    void _svd_Sparse_UT(std::vector<cytnx::UniTensor> &outCyT, const cytnx::UniTensor &Tin, const bool &is_U,
                                      const bool &is_vT){
        //[NOTE] outCyT must be empty vector!        

        // cytnx_error_msg(true,"[Svd][Developing] Svd for SparseUniTensor is developing.%s","\n");

        UniTensor ipt = Tin.contiguous();

        cytnx_uint64 i_Rk = ipt.rank();
        cytnx_uint64 i_rowrank = ipt.rowrank();
        vector<Bond> Ubds;
        vector<Bond> vTbds(1);  // pre-set for left bd of vT
        auto comm_qnums = ipt.get_blocks_qnums();

        for (int i = 0; i < i_Rk; i++) {
          if (i < i_rowrank)
            Ubds.push_back(ipt.bonds()[i]);
          else
            vTbds.push_back(ipt.bonds()[i]);
        }

        // std::cout << Ubds << std::endl;
        // std::cout << vTbds << std::endl;

        // now, calculate svd for each blocks:
        std::vector<Tensor> Uls;
        std::vector<Tensor> sls(comm_qnums.size());
        std::vector<Tensor> vTls;

        if (is_U) Uls.resize(comm_qnums.size());
        if (is_vT) vTls.resize(comm_qnums.size());

        std::vector<Tensor> &i_blocks = ipt.get_blocks_();
        // std::vector<cytnx_uint64> degs(comm_qnums.size()); //deg of each blocks
        cytnx_uint64 total_comm_dim = 0;
        std::vector<std::vector<cytnx_int64>> tmp_qns;

        for (int blk = 0; blk < comm_qnums.size(); blk++) {
          // std::cout << "QN block: " << blk << std::endl;
          int idd = 0;
          auto out = linalg::Svd(i_blocks[blk], is_U, is_vT);

          sls[blk] = out[idd];
          cytnx_uint64 deg = sls[blk].shape()[0];
          total_comm_dim += deg;

          std::vector<std::vector<cytnx_int64>> this_qnums(deg, comm_qnums[blk]);

          tmp_qns.insert(tmp_qns.end(), this_qnums.begin(), this_qnums.end());

          idd++;
          if (is_U) {
            Uls[blk] = out[idd];
            idd++;
          }
          if (is_vT) {
            vTls[blk] = out[idd];
          }

        }// for blk 

        // std::cout << tmp_qns.size() << std::endl;
        // std::cout << total_comm_dim << std::endl;

        // construct common bond:
        Bond comm_bdi(total_comm_dim, bondType::BD_KET, tmp_qns);
        Bond comm_bdo = comm_bdi.clone().set_type(bondType::BD_BRA);

        Ubds.push_back(comm_bdo);
        vTbds[0] = comm_bdi;

        // prepare output:
        //std::vector<UniTensor> outCyT;

        vector<string> oldlabel = ipt.labels();
        // cytnx_int64 newlbl = -1;
        // for (int i = 0; i < oldlabel.size(); i++) {
        //   if (oldlabel[i] <= newlbl) newlbl = oldlabel[i] - 1;
        // }
        string newlbl = "newlbl";
        for (int i = 0; i < oldlabel.size(); i++) {
          if (oldlabel[i] == newlbl) newlbl = newlbl + "new";
        }

        // s
        SparseUniTensor *tmps = new SparseUniTensor();
        // tmps->Init(
        //   {comm_bdi, comm_bdo}, {newlbl, newlbl - 1}, 1, Type.Double,
        //   Device.cpu, /* type and device does not matter here, cauz we are going to not alloc*/
        //   true, true);
        tmps->Init(
          {comm_bdi, comm_bdo}, {newlbl, newlbl + "new"}, 1, Type.Double,
          Device.cpu, /* type and device does not matter here, cauz we are going to not alloc*/
          true, true);
        // check:
        cytnx_error_msg(tmps->get_blocks_().size() != sls.size(), "[ERROR] internal error s.%s",
                        "\n");

        // wrapping:
        tmps->_blocks = sls;
        UniTensor s;
        s._impl = boost::intrusive_ptr<UniTensor_base>(tmps);
        outCyT.push_back(s);

        if (is_U) {
          SparseUniTensor *tmpu = new SparseUniTensor();
          std::vector<string> LBLS = vec_clone(oldlabel, ipt.rowrank());
          LBLS.push_back(newlbl);
          tmpu->Init(
            Ubds, LBLS, ipt.rowrank(), Type.Double,
            Device.cpu, /* type and device does not matter here, cauz we are going to not alloc*/
            false, true);

          // check:
          cytnx_error_msg(tmpu->get_blocks_().size() != Uls.size(), "[ERROR] internal error U.%s",
                          "\n");

          tmpu->_blocks = Uls;
          UniTensor u;
          u._impl = boost::intrusive_ptr<UniTensor_base>(tmpu);
          outCyT.push_back(u);
        }

        if (is_vT) {
          SparseUniTensor *tmpv = new SparseUniTensor();
          std::vector<string> LBLS(ipt.rank() - ipt.rowrank() + 1);  // old_label,ipt.rowrank());
          // LBLS[0] = newlbl - 1;
          LBLS[0] = newlbl + "new";
          memcpy(&LBLS[1], &oldlabel[ipt.rowrank()],
                 sizeof(cytnx_int64) * (ipt.rank() - ipt.rowrank()));

          tmpv->Init(
            vTbds, LBLS, 1, Type.Double,
            Device.cpu, /* type and device does not matter here, cauz we are going to not alloc*/
            false, true);

          // check:
          cytnx_error_msg(tmpv->get_blocks_().size() != vTls.size(), "[ERROR] internal error vT.%s",
                          "\n");

          tmpv->_blocks = vTls;
          UniTensor vT;
          vT._impl = boost::intrusive_ptr<UniTensor_base>(tmpv);
          outCyT.push_back(vT);
        }


    }; // _svd_Sparse_UT
    
    void _svd_Block_UT(std::vector<cytnx::UniTensor> &outCyT, const cytnx::UniTensor &Tin, const bool &is_U,
                                      const bool &is_vT){

        // outCyT must be empty and Tin must be checked with proper rowrank!

        // 1) getting the combineBond L and combineBond R for qnum list without grouping:
        // 
        //   BDLeft -[ ]- BDRight 
        //
        std::vector<cytnx_uint64> strides; strides.reserve(Tin.rank());
        auto BdLeft = Tin.bonds()[0].clone();
        for(int i=1;i<Tin.rowrank();i++){
            strides.push_back(Tin.bonds()[i].qnums().size());
            BdLeft._impl->force_combineBond_(Tin.bonds()[i]._impl,false); // no grouping
        } 
        //std::cout << BdLeft << std::endl;               
        strides.push_back(1);
        auto BdRight = Tin.bonds()[Tin.rowrank()].clone();
        for(int i=Tin.rowrank()+1;i<Tin.rank();i++){
            strides.push_back(Tin.bonds()[i].qnums().size());
            BdRight._impl->force_combineBond_(Tin.bonds()[i]._impl,false); // no grouping
        }
        strides.push_back(1);
        //std::cout << BdRight << std::endl;               
        //std::cout << strides << std::endl;
        
        // 2) making new inner_to_outer_idx lists for each block:
        // -> a. get stride:
        for(int i=Tin.rowrank()-2;i>=0;i--){
            strides[i]*=strides[i+1];
        }
        for(int i=Tin.rank()-2;i>=Tin.rowrank();i--){
            strides[i]*=strides[i+1];
        }
        //std::cout << strides << std::endl;
        // ->b. calc new inner_to_outer_idx!
        vec2d<cytnx_uint64> new_itoi(Tin.Nblocks(),std::vector<cytnx_uint64>(2));
        
        int cnt;
        for(cytnx_uint64 b=0;b<Tin.Nblocks();b++){
            const std::vector<cytnx_uint64> &tmpv =  Tin.get_qindices(b); 
            for(cnt=0;cnt<Tin.rowrank();cnt++){
                new_itoi[b][0] += tmpv[cnt]*strides[cnt];
            }
            for(cnt=Tin.rowrank();cnt<Tin.rank();cnt++){
                new_itoi[b][1] += tmpv[cnt]*strides[cnt];
            }
        } 
        //std::cout << new_itoi <<  std::endl;
 
        // 3) categorize:
        // key = qnum, val = list of block locations: 
        std::map<std::vector<cytnx_int64>, std::vector<cytnx_int64> > mgrp;
        for(cytnx_uint64 b=0;b<Tin.Nblocks();b++){ 
            mgrp[BdLeft.qnums()[new_itoi[b][0]]].push_back(b);
        }

        // 4) for each qcharge in key, combining the blocks into a big chunk!
        // ->a initialize an empty shell of UniTensor!
        vec2d<cytnx_int64> aux_qnums;      // for sharing bond
        std::vector<cytnx_uint64> aux_degs; // forsharing bond
        std::vector<Tensor> S_blocks;

        vec2d<cytnx_uint64> U_itoi;    // for U
        std::vector<Tensor> U_blocks;

        vec2d<cytnx_uint64> vT_itoi;   // for vT
        std::vector<Tensor> vT_blocks;

        int tr;
        for (auto const& x : mgrp)
        {
            vec2d<cytnx_uint64> itoi_indicators(x.second.size());
            //cout << x.second.size() << "-------" << endl;
            for(int i=0;i<x.second.size();i++){
                itoi_indicators[i] = new_itoi[x.second[i]];
                //std::cout << new_itoi[x.second[i]] << std::endl;
            }
            auto order = vec_sort(itoi_indicators,true);
            std::vector<Tensor> Tlist(itoi_indicators.size());
            std::vector<cytnx_uint64> row_szs(order.size(),1);
            cytnx_uint64 Rblk_dim = 0;
            cytnx_int64 tmp = -1;
            for(int i=0;i<order.size();i++){
                if(itoi_indicators[i][0] != tmp){
                    tmp = itoi_indicators[i][0];
                    Rblk_dim ++;
                }
                Tlist[i] = Tin.get_blocks()[x.second[order[i]]];
                for(int j=0;j<Tin.rowrank();j++){
                    row_szs[i]*= Tlist[i].shape()[j];
                }
                Tlist[i] = Tlist[i].reshape({row_szs[i],-1});
            }
            cytnx_error_msg(Tlist.size()%Rblk_dim,"[Internal ERROR] Tlist is not complete!%s","\n");
            // BTen is the big block!!
            cytnx_uint64 Cblk_dim = Tlist.size()/Rblk_dim;
            Tensor BTen = algo::_fx_Matric_combine(Tlist, Rblk_dim, Cblk_dim);
            
            // Now we can perform linalg!
            aux_qnums.push_back(x.first);
            auto out = linalg::Svd(BTen, is_U, is_vT);
            aux_degs.push_back(out[0].shape()[0]);
            S_blocks.push_back(out[0]);
            tr=1;

            if(is_U){
                //std::cout << row_szs << std::endl;
                //std::cout << out[tr].shape() << std::endl;
                std::vector<cytnx_uint64> split_dims;
                for(int i=0;i<Rblk_dim;i++){
                    split_dims.push_back(row_szs[i*Cblk_dim]);
                }                    
                std::vector<Tensor> blks;
                algo::Vsplit_(blks, out[tr],split_dims);
                out[tr] = Tensor();
                std::vector<cytnx_int64> new_shape(Tin.rowrank()+1); new_shape.back() = -1;
                for(int ti=0;ti<blks.size();ti++){
                    U_blocks.push_back(blks[ti]);
                    U_itoi.push_back(Tin.get_qindices(x.second[order[ti*Cblk_dim]]));

                    //reshaping:
                    for(int i=0;i<Tin.rowrank();i++){
                        new_shape[i] = Tin.bonds()[i].getDegeneracies()[ Tin.get_qindices(x.second[order[ti*Cblk_dim]])[i] ];
                    }
                    U_blocks.back().reshape_(new_shape);

                    U_itoi.back()[Tin.rowrank()] = S_blocks.size()-1;
                    U_itoi.back().resize(Tin.rowrank()+1); 
                }
                tr++;
            }// is_U

            if(is_vT){
                
                std::vector<cytnx_uint64> split_dims;
                for(int i=0;i<Cblk_dim;i++){
                    split_dims.push_back(Tlist[i].shape().back());
                }
                std::vector<Tensor> blks;
                algo::Hsplit_(blks, out[tr],split_dims);
                out[tr] = Tensor();
                
                std::vector<cytnx_int64> new_shape(Tin.rank()-Tin.rowrank()+1); new_shape[0] = -1;
                for(int ti=0;ti<blks.size();ti++){
                    vT_blocks.push_back(blks[ti]);
                    auto &tpitoi = Tin.get_qindices(x.second[order[ti]]);
                    vT_itoi.push_back({S_blocks.size()-1});
                    for(int i=Tin.rowrank();i<Tin.rank();i++){
                        vT_itoi.back().push_back(tpitoi[i]);
                    }                   
 
                    //reshaping:
                    for(int i=Tin.rowrank();i<Tin.rank();i++){
                        new_shape[i-Tin.rowrank()+1] = Tin.bonds()[i].getDegeneracies()[ tpitoi[i] ];
                    }
                    vT_blocks.back().reshape_(new_shape);

                }
                
                tr++;
            }// is_vT



        } 
        
        //process S:
        Bond Bd_aux = Bond(BD_IN, aux_qnums,aux_degs, Tin.syms());
        BlockUniTensor* S_ptr = new BlockUniTensor();
        S_ptr->Init({Bd_aux,Bd_aux.redirect()},
                    {"_aux_L","_aux_R"},1,
                    Type.Double, Device.cpu, // this two will be overwrite later, so doesnt matter.
                    true, // is_diag!
                    true); // no_alloc!
        S_ptr->_blocks = S_blocks;
        UniTensor S;
        S._impl = boost::intrusive_ptr<UniTensor_base>(S_ptr);
        
        outCyT.push_back(S);

        if(is_U){
            BlockUniTensor *U_ptr = new BlockUniTensor();
            for(int i=0;i<Tin.rowrank();i++){
                U_ptr->_bonds.push_back(Tin.bonds()[i].clone());
                U_ptr->_labels.push_back(Tin.labels()[i]);
            }
            U_ptr->_bonds.push_back(Bd_aux.redirect());
            U_ptr->_labels.push_back("_aux_L");
            U_ptr->_rowrank = Tin.rowrank();
            U_ptr->_is_diag=false;
            U_ptr->_is_braket_form   = U_ptr->_update_braket();
            U_ptr->_inner_to_outer_idx = U_itoi;
            U_ptr->_blocks = U_blocks;
            UniTensor U;
            U._impl = boost::intrusive_ptr<UniTensor_base>(U_ptr);
            outCyT.push_back(U);
        } 

        if(is_vT){
            BlockUniTensor *vT_ptr = new BlockUniTensor();
            vT_ptr->_bonds.push_back(Bd_aux);
            vT_ptr->_labels.push_back("_aux_R");

            for(int i=Tin.rowrank();i<Tin.rank();i++){
                vT_ptr->_bonds.push_back(Tin.bonds()[i].clone());
                vT_ptr->_labels.push_back(Tin.labels()[i]);
            }
            vT_ptr->_rowrank = 1;
            vT_ptr->_is_diag=false;
            vT_ptr->_is_braket_form   = vT_ptr->_update_braket();
            vT_ptr->_inner_to_outer_idx = vT_itoi;
            vT_ptr->_blocks = vT_blocks;
            UniTensor vT;
            vT._impl = boost::intrusive_ptr<UniTensor_base>(vT_ptr);
            outCyT.push_back(vT);

        }





    } // _svd_Block_UT


    std::vector<cytnx::UniTensor> Svd(const cytnx::UniTensor &Tin, const bool &is_U,
                                      const bool &is_vT) {

      // using rowrank to split the bond to form a matrix.
      cytnx_error_msg(Tin.rowrank() < 1 || Tin.rank() == 1,
                      "[Svd][ERROR] Svd for UniTensor should have rank>1 and rowrank>0%s",
                      "\n");
      
      std::vector<UniTensor> outCyT;
      if (Tin.uten_type() == UTenType.Dense) {
        _svd_Dense_UT(outCyT, Tin, is_U,is_vT);

      } else if (Tin.uten_type() == UTenType.Block){
        _svd_Block_UT(outCyT, Tin, is_U,is_vT);

      }  else {
        _svd_Sparse_UT(outCyT, Tin, is_U,is_vT);

      }// is block form ?

      return outCyT;

    }; // Svd




  }  // namespace linalg
}  // namespace cytnx
