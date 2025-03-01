#include "linalg.hpp"
#include "Accessor.hpp"
#include <vector>
#include "Tensor.hpp"
#include "UniTensor.hpp"
#include "algo.hpp"

namespace cytnx {
  namespace linalg {
    typedef Accessor ac;
    std::vector<Tensor> Svd_truncate(const Tensor &Tin, const cytnx_uint64 &keepdim,
                                     const double &err, const bool &is_UvT,
                                     const bool &return_err) {
      std::vector<Tensor> tmps = Svd(Tin, is_UvT);

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

        if (is_UvT) {
          id++;
          tmps[id] = tmps[id].get({ac::all(), ac::range(0, truc_dim)});

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

    void _svd_truncate_Dense_UT(std::vector<UniTensor> &outCyT,const cytnx::UniTensor &Tin,
                                               const cytnx_uint64 &keepdim, const double &err,
                                               const bool &is_UvT,
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
          cytnx::linalg::Svd_truncate(tmp, keepdim, err, is_UvT, return_err);

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

        if (is_UvT) {
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

        if (is_UvT) {
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
          if (is_UvT) {
            cytnx::UniTensor &Cy_U = outCyT[t];
            Cy_U._impl->_is_tag = true;
            for (int i = 0; i < Cy_U.rowrank(); i++) {
              Cy_U.bonds()[i].set_type(Tin.bonds()[i].type());
            }
            Cy_U.bonds().back().set_type(cytnx::BD_BRA);
            Cy_U._impl->_is_braket_form = Cy_U._impl->_update_braket();
            t++;
          }
          if (is_UvT) {
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

    
    void _svd_truncate_Sparse_UT(std::vector<UniTensor> &outCyT,const cytnx::UniTensor &Tin,
                                               const cytnx_uint64 &keepdim, const double &err,
                                               const bool &is_UvT,
                                               const bool &return_err){

      cytnx_uint64 keep_dim = keepdim;

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

      if (is_UvT) Uls.resize(comm_qnums.size());
      if (is_UvT) vTls.resize(comm_qnums.size());

      std::vector<Tensor> &i_blocks = ipt.get_blocks_();
      // std::vector<cytnx_uint64> degs(comm_qnums.size()); //deg of each blocks
      cytnx_uint64 total_comm_dim = 0;
      std::vector<std::vector<cytnx_int64>> tmp_qns;

      Tensor Sall;

      for (int blk = 0; blk < comm_qnums.size(); blk++) {
        // std::cout << "QN block: " << blk << std::endl;
        int idd = 0;
        auto out = linalg::Svd(i_blocks[blk], is_UvT);

        sls[blk] = out[idd];
        if (Sall.dtype() == Type.Void)
          Sall = sls[blk];
        else
          Sall = algo::Concatenate(Sall, sls[blk]);

        // calculate new bond qnums:
        // cytnx_uint64 deg = sls[blk].shape()[0];
        // total_comm_dim+=deg;

        // std::vector< std::vector<cytnx_int64> > this_qnums(deg,comm_qnums[blk]);

        // tmp_qns.insert(tmp_qns.end(),this_qnums.begin(),this_qnums.end());
        //

        idd++;
        if (is_UvT) {
          Uls[blk] = out[idd];
          idd++;
        }
        if (is_UvT) {
          vTls[blk] = out[idd];
        }
      } // for each block

      // cytnx_error_msg(keepdim>Sall.shape()[0],"[ERROR][Svd_truncate] keepdim should <=
      // dimension of total singular values%s","\n");
      std::vector<Tensor> o_sls, o_Uls, o_vTls;
      if (keepdim < Sall.shape()[0]) {  // keep_dim = Sall.shape()[0];

        // sorting:
        Sall = algo::Sort(Sall);  // small to large:
        // cout << Sall;
        // cout << Sall.shape()[0]-keepdim << endl;
        // cout << Sall(15);
        Scalar Smin = Sall(Sall.shape()[0] - keep_dim).item();

        std::vector<cytnx_int64> ambig_deg(comm_qnums.size());
        std::vector<cytnx_int64> degs(comm_qnums.size());
        // calculate new bond qnums and do truncate:
        for (int blk = 0; blk < comm_qnums.size(); blk++) {
          // std::cout << "QN block: " << blk << std::endl;
          int idd = 0;

          cytnx_int64 &deg = degs[blk];
          for (cytnx_int64 i = 0; i < sls[blk].shape()[0]; i++) {
            if (sls[blk](i).item() == Smin) {
              ambig_deg[blk]++;
            }
            if (sls[blk](i).item() >= Smin) {
              deg++;
            } else
              break;
          }
          total_comm_dim += deg;
        }

        // cout << degs << endl;
        // cout << total_comm_dim << endl;

        // checking

        // remove ambig_deg to fit keepdim:
        cytnx_int64 exceed = total_comm_dim - keep_dim;
        for (int blk = 0; blk < comm_qnums.size(); blk++) {
          if (exceed > 0) {
            if (ambig_deg[blk]) {
              if (ambig_deg[blk] > exceed) {
                degs[blk] -= exceed;
                exceed = 0;
              } else {
                exceed -= ambig_deg[blk];
                degs[blk] -= ambig_deg[blk];
              }
            }
          }

          // truncate
          if (degs[blk]) {
            std::vector<std::vector<cytnx_int64>> this_qnums(degs[blk], comm_qnums[blk]);
            tmp_qns.insert(tmp_qns.end(), this_qnums.begin(), this_qnums.end());

            // cout << "blk" << blk << "deg:" << degs[blk] << endl;

            // truncate:
            sls[blk] = sls[blk].get({ac::range(0, degs[blk])});
            o_sls.push_back(sls[blk]);

            if (is_UvT) {
              Uls[blk] = Uls[blk].get({ac::all(), ac::range(0, degs[blk])});
              if (Uls[blk].shape().size() == 1) Uls[blk].reshape_(Uls[blk].shape()[0], 1);
              o_Uls.push_back(Uls[blk]);
            }
            if (is_UvT) {
              vTls[blk] = vTls[blk].get({ac::range(0, degs[blk]), ac::all()});
              if (vTls[blk].shape().size() == 1) vTls[blk].reshape_(1, vTls[blk].shape()[0]);
              o_vTls.push_back(vTls[blk]);
            }
          }
        }

      } else {
        keep_dim = Sall.shape()[0];
        for (int blk = 0; blk < comm_qnums.size(); blk++) {
          cytnx_uint64 deg = sls[blk].shape()[0];
          total_comm_dim += deg;

          std::vector<std::vector<cytnx_int64>> this_qnums(deg, comm_qnums[blk]);

          tmp_qns.insert(tmp_qns.end(), this_qnums.begin(), this_qnums.end());

          o_sls.push_back(sls[blk]);

          if (is_UvT) {
            o_Uls.push_back(Uls[blk]);
          }
          if (is_UvT) {
            o_vTls.push_back(vTls[blk]);
          }
        }

      }  // if keepdim >= max dim

      // std::cout << tmp_qns.size() << std::endl;
      // std::cout << total_comm_dim << std::endl;

      // construct common bond:
      Bond comm_bdi(keep_dim, bondType::BD_KET, tmp_qns);
      Bond comm_bdo = comm_bdi.clone().set_type(bondType::BD_BRA);

      Ubds.push_back(comm_bdo);
      vTbds[0] = comm_bdi;


      vector<string> oldlabel = ipt.labels();

      // s
      SparseUniTensor *tmps = new SparseUniTensor();
      tmps->Init(
        {comm_bdi, comm_bdo}, {"_aux_L","_aux_R"}, 1, Type.Double,
        Device.cpu, // type and device does not matter here, cauz we are going to not alloc
        true, true);

      // check:
      cytnx_error_msg(tmps->get_blocks_().size() != o_sls.size(), "[ERROR] internal error s.%s",
                      "\n");

      // wrapping:
      tmps->_blocks = o_sls;
      UniTensor s;
      s._impl = boost::intrusive_ptr<UniTensor_base>(tmps);
      outCyT.push_back(s);

      if (is_UvT) {
        SparseUniTensor *tmpu = new SparseUniTensor();
        std::vector<string> LBLS = vec_clone(oldlabel, ipt.rowrank());
        LBLS.push_back(s.labels()[0]);
        tmpu->Init(
          Ubds, LBLS, ipt.rowrank(), Type.Double,
          Device.cpu, // type and device does not matter here, cauz we are going to not alloc
          false, true);

        // check:
        cytnx_error_msg(tmpu->get_blocks_().size() != o_Uls.size(), "[ERROR] internal error U.%s",
                        "\n");

        tmpu->_blocks = o_Uls;

        UniTensor u;
        u._impl = boost::intrusive_ptr<UniTensor_base>(tmpu);
        outCyT.push_back(u);
      }

      if (is_UvT) {
        SparseUniTensor *tmpv = new SparseUniTensor();
        std::vector<string> LBLS(ipt.rank() - ipt.rowrank() + 1);  // old_label,ipt.rowrank());
        // LBLS[0] = newlbl - 1;
        LBLS[0] = s.labels()[1];
        std::copy(oldlabel.begin()+ipt.rowrank(),oldlabel.end(),LBLS.begin()+1);


        tmpv->Init(
          vTbds, LBLS, 1, Type.Double,
          Device.cpu, // type and device does not matter here, cauz we are going to not alloc
          false, true);

        // check:
        cytnx_error_msg(tmpv->get_blocks_().size() != o_vTls.size(),
                        "[ERROR] internal error vT.%s", "\n");

        tmpv->_blocks = o_vTls;
        UniTensor vT;
        vT._impl = boost::intrusive_ptr<UniTensor_base>(tmpv);
        outCyT.push_back(vT);
      }

    }; // svdt Sparse


    void _svd_truncate_Block_UT(std::vector<UniTensor> &outCyT,const cytnx::UniTensor &Tin,
                                               const cytnx_uint64 &keepdim, const double &err,
                                               const bool &is_UvT,
                                               const bool &return_err){

       cytnx_uint64 keep_dim = keepdim;

       outCyT = linalg::Svd(Tin, is_UvT); 

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
          if(is_UvT){
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

          if(is_UvT){
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


    std::vector<cytnx::UniTensor> Svd_truncate(const cytnx::UniTensor &Tin,
                                               const cytnx_uint64 &keepdim, const double &err,
                                               const bool &is_UvT,
                                               const bool &return_err) {
      // using rowrank to split the bond to form a matrix.
      cytnx_error_msg((Tin.rowrank() < 1 || Tin.rank() == 1 || Tin.rowrank()==Tin.rank()),
                      "[Svd][ERROR] Svd for UniTensor should have rank>1 and rank>rowrank>0%s",
                      "\n");

      std::vector<UniTensor> outCyT;
      if( Tin.uten_type() == UTenType.Dense){
        _svd_truncate_Dense_UT(outCyT, Tin, keepdim, err,
                                               is_UvT,
                                               return_err);

      }else if(Tin.uten_type() == UTenType.Block){
        _svd_truncate_Block_UT(outCyT, Tin, keepdim, err,
                                               is_UvT,
                                               return_err);

      }else{
        _svd_truncate_Sparse_UT(outCyT, Tin, keepdim, err,
                                               is_UvT,
                                               return_err);

      }
      return outCyT;


    }  // Svd_truncate

  }  // namespace linalg
}  // namespace cytnx
