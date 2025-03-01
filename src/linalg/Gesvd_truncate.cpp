#include "linalg.hpp"
#include "Accessor.hpp"
#include <vector>
#include "Tensor.hpp"
#include "UniTensor.hpp"
#include "algo.hpp"

namespace cytnx {
  namespace linalg {
    typedef Accessor ac;
    std::vector<Tensor> Gesvd_truncate(const Tensor &Tin, const cytnx_uint64 &keepdim,
                                     const double &err, const bool &is_U, const bool &is_vT,
                                     const bool &return_err) {
      std::vector<Tensor> tmps = Gesvd(Tin, is_U,is_vT);

      cytnx_uint64 id = 0;
      cytnx_uint64 Kdim = keepdim;

      Storage ts = tmps[0].storage();

      if (ts.size() < keepdim) {
        Kdim = ts.size();
      }

      cytnx_uint64 truc_dim = Kdim;
      for (cytnx_int64 i = Kdim - 1; i >= 0; i--) {
        if (ts.at(i) < err) {
          truc_dim--;
        } else {
          break;
        }
      }

      if (truc_dim == 0) {
        truc_dim = 1;
      }
      /// std::cout << truc_dim << std::endl;
      // cytnx_error_msg(tmps[0].shape()[0] < keepdim,"[ERROR] keepdim should be <= the valid # of
      // singular value, %d!\n",tmps[0].shape()[0]);
      Tensor terr({1}, Type.Double);

      if (truc_dim != ts.size()) {
        terr = tmps[id](truc_dim);
        tmps[id] = tmps[id].get({ac::range(0, truc_dim)});

        if (is_U) {
          id++;
          tmps[id] = tmps[id].get({ac::all(), ac::range(0, truc_dim)});
        }
        if( is_vT){
          id++;
          tmps[id] = tmps[id].get({ac::range(0, truc_dim), ac::all()});
        }
      }
      if (return_err) tmps.push_back(terr);

      return tmps;
    }
  }  // namespace linalg
}  // namespace cytnx

namespace cytnx {
  namespace linalg {
    using namespace std;
    typedef Accessor ac;

    void _gesvd_truncate_Dense_UT(std::vector<UniTensor> &outCyT,const cytnx::UniTensor &Tin,
                                               const cytnx_uint64 &keepdim, const double &err,
                                               const bool &is_U, const bool &is_vT,
                                               const bool &return_err){
        // DenseUniTensor:
        cytnx_uint64 keep_dim = keepdim;

        Tensor tmp = Tin.get_block_().contiguous();
        // if(Tin.is_contiguous()) tmp = Tin.get_block_();
        // else{ tmp = Tin.get_block(); tmp.contiguous_();}

        vector<cytnx_uint64> tmps = tmp.shape();
        vector<cytnx_int64> oldshape(tmps.begin(), tmps.end());
        tmps.clear();
        vector<string> oldlabel = Tin.labels();

        // collapse as Matrix:
        cytnx_int64 rowdim = 1;
        for (cytnx_uint64 i = 0; i < Tin.rowrank(); i++) rowdim *= tmp.shape()[i];
        tmp = tmp.reshape({rowdim, -1});

        vector<Tensor> outT =
          cytnx::linalg::Gesvd_truncate(tmp, keepdim, err, is_U, is_vT, return_err);

        // if(Tin.is_contiguous()) tmp.reshape_(oldshape);

        int t = 0;
        outCyT.resize(outT.size());

        // s
        // cytnx_error_msg(keepdim>outT[t].shape()[0],"[ERROR][Svd_truncate] keepdim should <=
        // dimension of singular tensor%s","\n");

        cytnx::UniTensor &Cy_S = outCyT[t];
        cytnx::Bond newBond(outT[0].shape()[0]);
        Cy_S.Init({newBond, newBond}, {string("_aux_L"), string("_aux_R")}, 1, Type.Double, Device.cpu,
                  true);  // it is just reference so no hurt to alias ^^
        Cy_S.put_block_(outT[t]);
        t++;

        if (is_U) {
          cytnx::UniTensor &Cy_U = outCyT[t];
          // shape
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

          // shape
          vector<cytnx_int64> shapevT(Tin.rank() - Tin.rowrank() + 1);
          shapevT[0] = -1;
          memcpy(&shapevT[1], &oldshape[Tin.rowrank()], sizeof(cytnx_int64) * (shapevT.size() - 1));

          outT[t].reshape_(shapevT);

          Cy_vT.Init(outT[t], false, 1);
          vector<string> labelvT(shapevT.size());
          labelvT[0] = Cy_S.labels()[1];
          std::copy(oldlabel.begin()+Tin.rowrank(), oldlabel.end(), labelvT.begin()+1);
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

        if (return_err) outCyT.back().Init(outT.back(), false, 0);
    }; // svdt Dense

    
    void _gesvd_truncate_Sparse_UT(std::vector<UniTensor> &outCyT,const cytnx::UniTensor &Tin,
                                               const cytnx_uint64 &keepdim, const double &err,
                                               const bool &is_U, const bool &is_vT,
                                               const bool &return_err){

      cytnx_error_msg(true,"[ERROR] Sparse_UT data structure will be deprecated.%s","\n");
    }; // svdt Sparse


    void _gesvd_truncate_Block_UT(std::vector<UniTensor> &outCyT,const cytnx::UniTensor &Tin,
                                               const cytnx_uint64 &keepdim, const double &err,
                                               const bool &is_U, const bool &is_vT,
                                               const bool &return_err){

       cytnx_uint64 keep_dim = keepdim;

       outCyT = linalg::Gesvd(Tin, is_U, is_vT); 

       // process truncate:
       // 1) concate all s vals from all blk
       Tensor Sall = outCyT[0].get_block_(0);
       for(int i=1;i<outCyT[0].Nblocks();i++){
            Sall = algo::Concatenate(Sall,outCyT[0].get_block_(i));
       } 
       Sall = algo::Sort(Sall);

       // 2) get the minimum base on the args input.
       Scalar Smin;
       if(keep_dim < Sall.shape()[0]){
         Smin = Sall.storage()(Sall.shape()[0] - keep_dim);
         while( (Smin < err) ){
            keep_dim -=1;
            if(keep_dim==0) break;
            Smin = Sall.storage()(Sall.shape()[0] - keep_dim);
         }
        
       }else{
         keep_dim = Sall.shape()[0];
         Smin = Sall.storage()(0);
         while( (Smin < err) ){
            keep_dim -=1;
            if(keep_dim==0) break;
            Smin = Sall.storage()(Sall.shape()[0] - keep_dim);
         }
       }

          

          //traversal each block and truncate!
          UniTensor &S = outCyT[0];
          std::vector<cytnx_uint64> new_dims; // keep_dims for each block!
          std::vector<cytnx_int64> keep_dims; keep_dims.reserve(S.Nblocks());
          std::vector<cytnx_int64> new_qid; new_qid.reserve(S.Nblocks());
          
          std::vector<std::vector<cytnx_uint64> > new_itoi; // assume S block is in same order as qnum:
          std::vector<cytnx_uint64> to_be_remove;

          cytnx_uint64 tot_dim = 0;       
          cytnx_uint64 cnt = 0;   
          for(int b=0;b< S.Nblocks();b++){
                Storage stmp = S.get_block_(b).storage();
                cytnx_int64 kdim = 0;
                for(int i=stmp.size()-1;i>=0;i--){
                    if(stmp(i) >= Smin){
                        kdim = i+1;
                        break;
                    }
                }
                keep_dims.push_back(kdim);
                if(kdim==0){
                    to_be_remove.push_back(b);
                    new_qid.push_back(-1);

                }else{
                    new_qid.push_back(new_dims.size());
                    new_itoi.push_back({new_dims.size(),new_dims.size()});
                    new_dims.push_back(kdim);
                    tot_dim += kdim;
                    if(kdim != S.get_blocks_()[b].shape()[0])
                        S.get_blocks_()[b] = S.get_blocks_()[b].get({ac::range(0,kdim)});
                }
                
          }

          //remove:
          //vec_erase_(S.get_itoi(),to_be_remove);
          S.get_itoi() = new_itoi;
          vec_erase_(S.get_blocks_(),to_be_remove);
          vec_erase_(S.bonds()[0].qnums(), to_be_remove);
          S.bonds()[0]._impl->_degs = new_dims; 
          S.bonds()[0]._impl->_dim =tot_dim;
          S.bonds()[1] = S.bonds()[0].redirect();
        
          
          int t=1;
          if(is_U){
            UniTensor &U = outCyT[t];
            to_be_remove.clear();
            U.bonds().back() = S.bonds()[1].clone();
            std::vector<Accessor> acs(U.rank());
            for(int i=0;i<U.rowrank();i++) acs[i] = ac::all();
         
            for(int b=0;b<U.Nblocks();b++){
                if(keep_dims[U.get_qindices(b).back()]==0) to_be_remove.push_back(b);
                else{ 
                    ///process blocks:
                    if(keep_dims[U.get_qindices(b).back()] != U.get_blocks_()[b].shape().back()){
                        acs.back() = ac::range(0,keep_dims[U.get_qindices(b).back()]);
                        U.get_blocks_()[b] = U.get_blocks_()[b].get(acs);
                    }

                    // change to new qindices:
                    U.get_qindices(b).back() = new_qid[ U.get_qindices(b).back() ];
                }
            }
            vec_erase_(U.get_itoi(), to_be_remove);
            vec_erase_(U.get_blocks_(), to_be_remove);
            
            t++;
          } 

          if(is_vT){
            UniTensor &vT = outCyT[t];
            to_be_remove.clear();
            vT.bonds().front() = S.bonds()[0].clone();
            std::vector<Accessor> acs(vT.rank());
            for(int i=1;i<vT.rank();i++) acs[i] = ac::all();

            for(int b=0;b<vT.Nblocks();b++){
                if(keep_dims[vT.get_qindices(b)[0]]==0) to_be_remove.push_back(b);
                else{ 
                    ///process blocks:
                    if(keep_dims[vT.get_qindices(b)[0]] != vT.get_blocks_()[b].shape()[0]){
                        acs[0] = ac::range(0,keep_dims[vT.get_qindices(b)[0]]);
                        vT.get_blocks_()[b] = vT.get_blocks_()[b].get(acs);
                    }
                    // change to new qindices:
                    vT.get_qindices(b)[0] = new_qid[ vT.get_qindices(b)[0] ];
                }
            }
            vec_erase_(vT.get_itoi(), to_be_remove);
            vec_erase_(vT.get_blocks_(), to_be_remove);
            t++;
          }


          // handle return_err!
          if(return_err){
            outCyT.push_back(UniTensor(Tensor({1},Smin.dtype())));
            outCyT.back().get_block_().storage().at(0) = Smin; 
          } 


    }


    std::vector<cytnx::UniTensor> Gesvd_truncate(const cytnx::UniTensor &Tin,
                                               const cytnx_uint64 &keepdim, const double &err,
                                               const bool &is_U, const bool &is_vT,
                                               const bool &return_err) {
      // using rowrank to split the bond to form a matrix.
      cytnx_error_msg((Tin.rowrank() < 1 || Tin.rank() == 1 || Tin.rowrank()==Tin.rank()),
                      "[Gesvd][ERROR] Gesvd for UniTensor should have rank>1 and rank>rowrank>0%s",
                      "\n");

      std::vector<UniTensor> outCyT;
      if( Tin.uten_type() == UTenType.Dense){
        _gesvd_truncate_Dense_UT(outCyT, Tin, keepdim, err,
                                               is_U, is_vT,
                                               return_err);

      }else if(Tin.uten_type() == UTenType.Block){
        _gesvd_truncate_Block_UT(outCyT, Tin, keepdim, err,
                                               is_U, is_vT,
                                               return_err);

      }else{
        _gesvd_truncate_Sparse_UT(outCyT, Tin, keepdim, err,
                                               is_U,is_vT,
                                               return_err);

      }
      return outCyT;


    }  // Gesvd_truncate

  }  // namespace linalg
}  // namespace cytnx
