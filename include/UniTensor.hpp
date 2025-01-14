#ifndef _H_UniTensor_
#define _H_UniTensor_

#include "Type.hpp"
#include "cytnx_error.hpp"
#include "Storage.hpp"
#include "Device.hpp"
#include "Tensor.hpp"
#include "Scalar.hpp"
#include "utils/utils.hpp"
#include "intrusive_ptr_base.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <utility>
#include <initializer_list>
#include <fstream>
#include <algorithm>
#include "Symmetry.hpp"
#include "Bond.hpp"
// #include "linalg.hpp"

// namespace cytnx{
namespace cytnx {
  using namespace cytnx;
  /// @cond
  class UniTensorType_class {
   public:
    enum : int {
      Void = -99,
      Dense = 0,
      Sparse = 1,
      Block = 2,
    };
    std::string getname(const int &ut_type);
  };
  /// @endcond
  /**
   * @brief UniTensor type.
   * @details It is about the type of the UniTensor.\n
   *     The supported enumerations are as following:
   *
   *  enumeration  |  description
   * --------------|--------------------
   *  Void         |  -1, void UniTensor
   *  Dense        |  0, dense UniTensor
   *  Sparse       |  1, sparse UniTensor (deprecated)
   *  Block        |  2, block UniTensor
   *
   *  @warning the type \em Sparse is deprecated. Use \em Block instead.
   *  @see UniTensor::uten_type(), UniTensor::uten_type_str()
   */

  extern UniTensorType_class UTenType;

  /// @cond
  // class DenseUniTensor;
  // class SparseUniTensor;
  class UniTensor_base : public intrusive_ptr_base<UniTensor_base> {
   public:
    int uten_type_id;  // the unitensor type id.
    bool _is_braket_form;
    bool _is_tag;
    bool _is_diag;
    cytnx_int64 _rowrank;
    std::string _name;
    std::vector<std::string> _labels;
    std::vector<Bond> _bonds;

    bool _update_braket() {
      if (_bonds.size() == 0) return false;

      if (this->_bonds[0].type() != bondType::BD_REG) {
        // check:
        for (unsigned int i = 0; i < this->_bonds.size(); i++) {
          if (i < this->_rowrank) {
            if (this->_bonds[i].type() != bondType::BD_KET) return false;
          } else {
            if (this->_bonds[i].type() != bondType::BD_BRA) return false;
          }
        }
        return true;
      } else {
        return false;
      }
    }

    friend class UniTensor;  // allow wrapper to access the private elems
    friend class DenseUniTensor;
    friend class SparseUniTensor;
    friend class BlockUniTensor;


    UniTensor_base()
        : _is_tag(false),
          _name(std::string("")),
          _is_braket_form(false),
          _rowrank(0),
          _is_diag(false),
          uten_type_id(UTenType.Void){};

    // copy&assignment constr., use intrusive_ptr's !!
    UniTensor_base(const UniTensor_base &rhs);
    UniTensor_base &operator=(UniTensor_base &rhs);

    cytnx_uint64 rowrank() const { return this->_rowrank; }
    bool is_diag() const { return this->_is_diag; }
    const bool &is_braket_form() const { return this->_is_braket_form; }
    const bool &is_tag() const { return this->_is_tag; }
    const std::vector<std::string> &labels() const { return this->_labels; }
    /**
     * @brief Get the index of an desired label string
     *
     * @param lbl Label you want to find
     * @return The index of the label. If not found, return -1
     */
    cytnx_int64 get_index(std::string lbl) const {
      std::vector<std::string> lbls = this->_labels;
      for (cytnx_uint64 i = 0; i < lbls.size(); i++) {
        if (lbls[i] == lbl) return i;
      }
      return -1;
    }
    const std::vector<Bond> &bonds() const { return this->_bonds; }
    std::vector<Bond> &bonds() { return this->_bonds; }
    const std::string &name() const { return this->_name; }
    cytnx_uint64 rank() const { return this->_labels.size(); }
    void set_name(const std::string &in) { this->_name = in; }
    /**
     * @brief Set the label object
	 * @details Replace the old label by new label.
     * @param[in] oldlbl The old label you want to replace.
     * @param[in] new_lable The label you want to replace with.
	 * @pre
	 * 1. \p oldlbl should be exist in this UniTensor.
	 * 2. The new label \p new_label cannot set as others exit labels (cannot be duplicated.)
	 * @see set_label(const cytnx_int64 &inx, const std::string &new_label)
     */
    void set_label(const std::string &oldlbl, const std::string &new_label) {
      cytnx_int64 idx;
      auto res = std::find(this->_labels.begin(), this->_labels.end(), oldlbl);
      cytnx_error_msg(res == this->_labels.end(), "[ERROR] label %s not exists.\n", oldlbl.c_str());
      idx = std::distance(this->_labels.begin(), res);

      cytnx_error_msg(idx >= this->_labels.size(), "[ERROR] index exceed the rank of UniTensor%s",
                      "\n");
      // check in:
      bool is_dup = false;
      for (cytnx_uint64 i = 0; i < this->_labels.size(); i++) {
        if (i == idx) continue;
        if (new_label == this->_labels[i]) {
          is_dup = true;
          break;
        }
      }
      cytnx_error_msg(is_dup, "[ERROR] alreay has a label that is the same as the input label%s",
                      "\n");
      this->_labels[idx] = new_label;
    }
    void set_label(const cytnx_int64 &inx, const std::string &new_label) {
      cytnx_error_msg(inx < 0 , "[ERROR] index is negative%s",
                      "\n");
      cytnx_error_msg(inx >= this->_labels.size(), "[ERROR] index exceed the rank of UniTensor%s",
                      "\n");
      // check in:
      bool is_dup = false;
      for (cytnx_uint64 i = 0; i < this->_labels.size(); i++) {
        if (i == inx) continue;
        if (new_label == this->_labels[i]) {
          is_dup = true;
          break;
        }
      }
      cytnx_error_msg(is_dup, "[ERROR] alreay has a label that is the same as the input label%s",
                      "\n");
      this->_labels[inx] = new_label;
    }
    /**
     * @brief Set the label object
	 * @details Set the label with respect to the input index.
     * @deprecated
	 *  This function is deprecated, use 
	 *  \ref set_label(const cytnx_int64 &inx, const cytnx_int64 &_new_label, const bool &by_label)
	 *  instread.
     */
    void set_label(const cytnx_int64 &inx, const cytnx_int64 &new_label) {
      set_label(inx, std::to_string(new_label));
    }
    /**
     * @brief Set the label object
     * @deprecated
	 *  This function is deprecated, use 
	 *  \ref set_label(const cytnx_int64 &inx, const cytnx_int64 &_new_label, const bool &by_label)
	 *  instread.
     */
    void set_label(const cytnx_int64 &inx, const cytnx_int64 &_new_label, const bool &by_label) {
      std::string new_label = std::to_string(_new_label);
      cytnx_int64 idx;
      if (by_label) {
        auto res = std::find(this->_labels.begin(), this->_labels.end(), std::to_string(inx));
        cytnx_error_msg(res == this->_labels.end(), "[ERROR] label %s not exists.\n", inx);
        idx = std::distance(this->_labels.begin(), res);
      } else {
        idx = inx;
      }
     
      set_label(idx,new_label);

    }
    /**
     * @brief Set the labels object
     *
     * @deprecated
     *
     * @param new_labels
     */
    void set_labels(const std::vector<cytnx_int64> &new_labels);
    void set_labels(const std::vector<std::string> &new_labels);
    


    /*
    template<class T>
    T get_elem(const std::vector<cytnx_uint64> &locator) const{
        if(this->is_blockform()){
            if(this->elem_exists(locator)){
                T aux; // [workaround] use aux to dispatch.
                return this->at_for_sparse(locator,aux);
            }else{
                return 0;
            }
        }else{
            return this->at<T>(locator);
        }
    }
    template<class T>
    void set_elem(const std::vector<cytnx_uint64> &locator, const T &input){
        if(this->uten_type()==UTenType.Sparse){
            if(this->elem_exists(locator)){
                T aux;
                this->at_for_sparse(locator,aux) = input;
            }else{
                cytnx_error_msg(true,"[ERROR][SparseUniTensor] invalid location. break qnum
    block.%s","\n");
            }
        }else{
            this->at<T>(locator) = input;
        }
    }
    */

    int uten_type() { return this->uten_type_id; }
    std::string uten_type_str() { return UTenType.getname(this->uten_type_id); }
    

    /// VIRTUAL FUNCTIONS:

    // string labels! 
    virtual void Init(const std::vector<Bond> &bonds,
                      const std::vector<std::string> &in_labels = {},
                      const cytnx_int64 &rowrank = -1, const unsigned int &dtype = Type.Double,
                      const int &device = Device.cpu, const bool &is_diag = false,
                      const bool &no_alloc = false, const std::string &name = "");
    
    virtual void Init_by_Tensor(const Tensor &in, const bool &is_diag = false,
                                const cytnx_int64 &rowrank = -1, const std::string &name = "");
    virtual std::vector<cytnx_uint64> shape() const;
    virtual bool is_blockform() const;
    virtual bool is_contiguous() const;
    virtual void to_(const int &device);
    virtual boost::intrusive_ptr<UniTensor_base> to(const int &device);
    virtual boost::intrusive_ptr<UniTensor_base> clone() const;
    virtual unsigned int dtype() const;
    virtual int device() const;
    virtual std::string dtype_str() const;
    virtual std::string device_str() const;
    virtual void set_rowrank(const cytnx_uint64 &new_rowrank);
   


    virtual boost::intrusive_ptr<UniTensor_base> permute(const std::vector<cytnx_int64> &mapper,
                                                         const cytnx_int64 &rowrank = -1,
                                                         const bool &by_label= false);
    virtual boost::intrusive_ptr<UniTensor_base> permute(const std::vector<std::string> &mapper,
                                                         const cytnx_int64 &rowrank = -1);
    //virtual boost::intrusive_ptr<UniTensor_base> permute(const std::vector<cytnx_int64> &mapper,
    //                                                     const cytnx_int64 &rowrank = -1);

    virtual void permute_(const std::vector<cytnx_int64> &mapper, const cytnx_int64 &rowrank=-1,
                          const bool &by_label=false);
    virtual void permute_(const std::vector<std::string> &mapper, const cytnx_int64 &rowrank = -1);
    //virtual void permute_(const std::vector<cytnx_int64> &mapper, const cytnx_int64 &rowrank = -1);
    virtual boost::intrusive_ptr<UniTensor_base> contiguous_();
    virtual boost::intrusive_ptr<UniTensor_base> contiguous();
    virtual void print_diagram(const bool &bond_info = false);
    virtual void print_blocks(const bool &full_info=true) const; 
    virtual void print_block(const cytnx_int64 &idx, const bool &full_info=true) const;

    virtual boost::intrusive_ptr<UniTensor_base> astype(const unsigned int &dtype) const;

    virtual cytnx_uint64 Nblocks() const { return 0; };
    virtual Tensor get_block(const cytnx_uint64 &idx = 0) const;  // return a copy of block
    virtual Tensor get_block(const std::vector<cytnx_int64> &qnum,
                             const bool &force) const;  // return a copy of block

    virtual const Tensor &get_block_(const cytnx_uint64 &idx = 0)
      const;  // return a share view of block, this only work for non-symm tensor.
    virtual const Tensor &get_block_(const std::vector<cytnx_int64> &qnum,
                                     const bool &force) const;  // return a copy of block
    virtual Tensor &get_block_(const cytnx_uint64 &idx = 0);  // return a share view of block, this
                                                              // only work for non-symm tensor.
    virtual Tensor &get_block_(const std::vector<cytnx_int64> &qnum,
                               const bool &force);  // return a copy of block
    virtual bool same_data(const boost::intrusive_ptr<UniTensor_base> &rhs) const;

    virtual std::vector<Tensor> get_blocks() const;
    virtual const std::vector<Tensor> &get_blocks_(const bool &) const;
    virtual std::vector<Tensor> &get_blocks_(const bool &);

    virtual void put_block(const Tensor &in, const cytnx_uint64 &idx = 0);
    virtual void put_block_(Tensor &in, const cytnx_uint64 &idx = 0);
    virtual void put_block(const Tensor &in, const std::vector<cytnx_int64> &qnum,
                           const bool &force);
    virtual void put_block_(Tensor &in, const std::vector<cytnx_int64> &qnum, const bool &force);

    // this will only work on non-symm tensor (DenseUniTensor)
    virtual boost::intrusive_ptr<UniTensor_base> get(const std::vector<Accessor> &accessors);

    // this will only work on non-symm tensor (DenseUniTensor)
    virtual void set(const std::vector<Accessor> &accessors, const Tensor &rhs);

    virtual void reshape_(const std::vector<cytnx_int64> &new_shape,
                          const cytnx_uint64 &rowrank = 0);
    virtual boost::intrusive_ptr<UniTensor_base> reshape(const std::vector<cytnx_int64> &new_shape,
                                                         const cytnx_uint64 &rowrank = 0);
    virtual boost::intrusive_ptr<UniTensor_base> to_dense();
    virtual void to_dense_();
    virtual void combineBonds(const std::vector<cytnx_int64> &indicators, const bool &force,
                              const bool &by_label);
    virtual void combineBonds(const std::vector<std::string> &indicators,
                              const bool &force = false);
    virtual void combineBonds(const std::vector<cytnx_int64> &indicators,
                              const bool &force = false);
    virtual boost::intrusive_ptr<UniTensor_base> contract(
      const boost::intrusive_ptr<UniTensor_base> &rhs, const bool &mv_elem_self = false,
      const bool &mv_elem_rhs = false);
    virtual std::vector<Bond> getTotalQnums(const bool &physical = false);
    virtual std::vector<std::vector<cytnx_int64>> get_blocks_qnums() const;
    virtual void Trace_(const std::string &a, const std::string &b);
    virtual void Trace_(const cytnx_int64 &a, const cytnx_int64 &b);
    virtual void Trace_(const cytnx_int64 &a, const cytnx_int64 &b, const bool &by_label);
    virtual boost::intrusive_ptr<UniTensor_base> Trace(const std::string &a, const std::string &b);
    virtual boost::intrusive_ptr<UniTensor_base> Trace(const cytnx_int64 &a, const cytnx_int64 &b);
    virtual boost::intrusive_ptr<UniTensor_base> Trace(const cytnx_int64 &a, const cytnx_int64 &b,
                                                       const bool &by_label);
    virtual boost::intrusive_ptr<UniTensor_base> relabels(
      const std::vector<cytnx_int64> &new_labels);
    virtual boost::intrusive_ptr<UniTensor_base> relabels(
      const std::vector<std::string> &new_labels);
    virtual boost::intrusive_ptr<UniTensor_base> relabel(const cytnx_int64 &inx,
                                                         const cytnx_int64 &new_label,
                                                         const bool &by_label);
    virtual boost::intrusive_ptr<UniTensor_base> relabel(const std::string &inx,
                                                         const std::string &new_label);
    virtual boost::intrusive_ptr<UniTensor_base> relabel(const cytnx_int64 &inx,
                                                         const cytnx_int64 &new_label);
    virtual boost::intrusive_ptr<UniTensor_base> relabel(const cytnx_int64 &inx,
                                                         const std::string &new_label);

    virtual std::vector<Symmetry> syms() const;

    // arithmetic
    virtual void Add_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    virtual void Add_(const Scalar &rhs);

    virtual void Mul_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    virtual void Mul_(const Scalar &rhs);

    virtual void Sub_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    virtual void Sub_(const Scalar &rhs);
    virtual void lSub_(const Scalar &lhs);

    virtual void Div_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    virtual void Div_(const Scalar &rhs);
    virtual void lDiv_(const Scalar &lhs);

    virtual Tensor Norm() const;
    virtual boost::intrusive_ptr<UniTensor_base> normalize();
    virtual void normalize_();

    virtual boost::intrusive_ptr<UniTensor_base> Conj();
    virtual void Conj_();

    virtual boost::intrusive_ptr<UniTensor_base> Transpose();
    virtual void Transpose_();

    virtual boost::intrusive_ptr<UniTensor_base> Dagger();
    virtual void Dagger_();

    virtual void tag();

    virtual void truncate_(const cytnx_int64 &bond_idx, const cytnx_uint64 &dim,
                           const bool &by_label);
    virtual void truncate_(const std::string &bond_idx, const cytnx_uint64 &dim);
    virtual void truncate_(const cytnx_int64 &bond_idx, const cytnx_uint64 &dim);

    virtual bool elem_exists(const std::vector<cytnx_uint64> &locator) const;

    // this a workaround, as virtual function cannot template.
    virtual Scalar::Sproxy at_for_sparse(const std::vector<cytnx_uint64> &locator);
    virtual const Scalar::Sproxy at_for_sparse(const std::vector<cytnx_uint64> &locator) const;

    virtual cytnx_complex128 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                            const cytnx_complex128 &aux);
    virtual cytnx_complex64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                           const cytnx_complex64 &aux);
    virtual cytnx_double &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                        const cytnx_double &aux);
    virtual cytnx_float &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                       const cytnx_float &aux);
    virtual cytnx_uint64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                        const cytnx_uint64 &aux);
    virtual cytnx_int64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                       const cytnx_int64 &aux);
    virtual cytnx_uint32 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                        const cytnx_uint32 &aux);
    virtual cytnx_int32 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                       const cytnx_int32 &aux);
    virtual cytnx_uint16 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                        const cytnx_uint16 &aux);
    virtual cytnx_int16 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                       const cytnx_int16 &aux);

    virtual const cytnx_complex128 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                                  const cytnx_complex128 &aux) const;
    virtual const cytnx_complex64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                                 const cytnx_complex64 &aux) const;
    virtual const cytnx_double &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                              const cytnx_double &aux) const;
    virtual const cytnx_float &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                             const cytnx_float &aux) const;
    virtual const cytnx_uint64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                              const cytnx_uint64 &aux) const;
    virtual const cytnx_int64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                             const cytnx_int64 &aux) const;
    virtual const cytnx_uint32 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                              const cytnx_uint32 &aux) const;
    virtual const cytnx_int32 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                             const cytnx_int32 &aux) const;
    virtual const cytnx_uint16 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                              const cytnx_uint16 &aux) const;
    virtual const cytnx_int16 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                             const cytnx_int16 &aux) const;

    virtual void group_basis_();
    virtual const std::vector<cytnx_uint64>& get_qindices(const cytnx_uint64 &bidx) const;
    virtual std::vector<cytnx_uint64>& get_qindices(const cytnx_uint64 &bidx);
    virtual const vec2d<cytnx_uint64> & get_itoi() const;
    virtual vec2d<cytnx_uint64> & get_itoi();


    virtual void _save_dispatch(std::fstream &f) const;
    virtual void _load_dispatch(std::fstream &f);

    virtual ~UniTensor_base(){};
  };
  /// @endcond

  //======================================================================
  /// @cond
  class DenseUniTensor : public UniTensor_base {
   protected:
   public:
    Tensor _block;
    std::vector<Tensor> _interface_block;  // this is serves as interface for get_blocks_();
    DenseUniTensor *clone_meta() const {
      DenseUniTensor *tmp = new DenseUniTensor();
      tmp->_bonds = vec_clone(this->_bonds);
      tmp->_labels = this->_labels;
      tmp->_is_braket_form = this->_is_braket_form;
      tmp->_rowrank = this->_rowrank;
      tmp->_is_diag = this->_is_diag;
      tmp->_name = this->_name;
      tmp->_is_tag = this->_is_tag;
      return tmp;
    }
    //------------------------------------------

    DenseUniTensor() { this->uten_type_id = UTenType.Dense; };
    friend class UniTensor;  // allow wrapper to access the private elems
    // virtual functions
    
    //void Init(const std::vector<Bond> &bonds, const std::vector<cytnx_int64> &in_labels = {},
    //          const cytnx_int64 &rowrank = -1, const unsigned int &dtype = Type.Double,
    //          const int &device = Device.cpu, const bool &is_diag = false,
    //          const bool &no_alloc = false);

    void Init(const std::vector<Bond> &bonds, const std::vector<std::string> &in_labels = {},
              const cytnx_int64 &rowrank = -1, const unsigned int &dtype = Type.Double,
              const int &device = Device.cpu, const bool &is_diag = false,
              const bool &no_alloc = false, const std::string &name = "");
    // this only work for non-symm tensor
    void Init_by_Tensor(const Tensor &in_tensor, const bool &is_diag = false,
                        const cytnx_int64 &rowrank = -1, const std::string &name = "");
    std::vector<cytnx_uint64> shape() const {
      if (this->_is_diag) {
        std::vector<cytnx_uint64> shape = this->_block.shape();
        shape.push_back(shape[0]);
        return shape;
      } else {
        return this->_block.shape();
      }
    }
    bool is_blockform() const { return false; }
    void to_(const int &device) { this->_block.to_(device); }
    boost::intrusive_ptr<UniTensor_base> to(const int &device) {
      if (this->device() == device) {
    std::vector<Tensor> _interface_block;  // this is serves as interface for get_blocks_();
        return this;
      } else {
        boost::intrusive_ptr<UniTensor_base> out = this->clone();
        out->to_(device);
        return out;
      }
    }
    void set_rowrank(const cytnx_uint64 &new_rowrank) {
      cytnx_error_msg(new_rowrank > this->_labels.size(),
                      "[ERROR] rowrank cannot exceed the rank of UniTensor.%s", "\n");
      if(this->is_diag()){
        cytnx_error_msg(new_rowrank != 1,
                      "[ERROR] rowrank should be [==1] when is_diag =true!.%s",
                      "\n");
      }

      this->_rowrank = new_rowrank;
    }

    boost::intrusive_ptr<UniTensor_base> clone() const {
      DenseUniTensor *tmp = this->clone_meta();
      tmp->_block = this->_block.clone();
      boost::intrusive_ptr<UniTensor_base> out(tmp);
      return out;
    };
    bool is_contiguous() const { return this->_block.is_contiguous(); }
    unsigned int dtype() const { return this->_block.dtype(); }
    int device() const { return this->_block.device(); }
    std::string dtype_str() const { return Type.getname(this->_block.dtype()); }
    std::string device_str() const { return Device.getname(this->_block.device()); }
    /**
     * @brief
     *
     * @deprecated
     *
     * @param mapper
     * @param rowrank
     * @param by_label
     * @return boost::intrusive_ptr<UniTensor_base>
     */
    boost::intrusive_ptr<UniTensor_base> permute(const std::vector<cytnx_int64> &mapper,
                                                 const cytnx_int64 &rowrank=-1, const bool &by_label=false);
    boost::intrusive_ptr<UniTensor_base> permute(const std::vector<std::string> &mapper,
                                                 const cytnx_int64 &rowrank = -1);
    //boost::intrusive_ptr<UniTensor_base> permute(const std::vector<cytnx_int64> &mapper,
    //                                             const cytnx_int64 &rowrank = -1);
    /**
     * @brief
     *
     * @deprecated
     *
     * @param mapper
     * @param rowrank
     * @param by_label
     */
    void permute_(const std::vector<cytnx_int64> &mapper, const cytnx_int64 &rowrank=-1,
                  const bool &by_label=false);
    void permute_(const std::vector<std::string> &mapper, const cytnx_int64 &rowrank = -1);
    //void permute_(const std::vector<cytnx_int64> &mapper, const cytnx_int64 &rowrank = -1);
    boost::intrusive_ptr<UniTensor_base> relabels(const std::vector<std::string> &new_labels);
    /**
     * @brief
     *
     * @deprecated
     *
     * @param new_labels
     * @return boost::intrusive_ptr<UniTensor_base>
     */
    boost::intrusive_ptr<UniTensor_base> relabels(const std::vector<cytnx_int64> &new_labels);
    /**
     * @brief
     *
     * @deprecated
     *
     * @param inx
     * @param new_label
     * @param by_label
     * @return boost::intrusive_ptr<UniTensor_base>
     */
    boost::intrusive_ptr<UniTensor_base> relabel(const cytnx_int64 &inx,
                                                 const cytnx_int64 &new_label,
                                                 const bool &by_label);
    boost::intrusive_ptr<UniTensor_base> relabel(const std::string &inx,
                                                 const std::string &new_label);
    boost::intrusive_ptr<UniTensor_base> relabel(const cytnx_int64 &inx,
                                                 const std::string &new_label);
    boost::intrusive_ptr<UniTensor_base> relabel(const cytnx_int64 &inx,
                                                 const cytnx_int64 &new_label);

    boost::intrusive_ptr<UniTensor_base> astype(const unsigned int &dtype) const {
      DenseUniTensor *tmp = this->clone_meta();
      tmp->_block = this->_block.astype(dtype);
      boost::intrusive_ptr<UniTensor_base> out(tmp);
      return tmp;
    }

    std::vector<Symmetry> syms() const {
      cytnx_error_msg(true, "[ERROR][DenseUniTensor] dense unitensor does not have symmetry.%s",
                      "\n");
      return std::vector<Symmetry>();
    }

    boost::intrusive_ptr<UniTensor_base> contiguous_() {
      this->_block.contiguous_();
      return boost::intrusive_ptr<UniTensor_base>(this);
    }
    boost::intrusive_ptr<UniTensor_base> contiguous() {
      // if contiguous then return self!
      if (this->is_contiguous()) {
        boost::intrusive_ptr<UniTensor_base> out(this);
        return out;
      } else {
        DenseUniTensor *tmp = this->clone_meta();
        tmp->_block = this->_block.contiguous();
        boost::intrusive_ptr<UniTensor_base> out(tmp);
        return out;
      }
    }
    void print_diagram(const bool &bond_info = false);
    void print_blocks(const bool &full_info=true) const;
    void print_block(const cytnx_int64 &idx, const bool &full_info=true) const;
    Tensor get_block(const cytnx_uint64 &idx = 0) const { return this->_block.clone(); }

    Tensor get_block(const std::vector<cytnx_int64> &qnum, const bool &force) const {
      cytnx_error_msg(
        true, "[ERROR][DenseUniTensor] try to get_block() using qnum on a non-symmetry UniTensor%s",
        "\n");
      return Tensor();
    }
    // return a share view of block, this only work for non-symm tensor.
    const Tensor &get_block_(const std::vector<cytnx_int64> &qnum, const bool &force) const {
      cytnx_error_msg(
        true,
        "[ERROR][DenseUniTensor] try to get_block_() using qnum on a non-symmetry UniTensor%s",
        "\n");
      return this->_block;
    }
    Tensor &get_block_(const std::vector<cytnx_int64> &qnum, const bool &force) {
      cytnx_error_msg(
        true,
        "[ERROR][DenseUniTensor] try to get_block_() using qnum on a non-symmetry UniTensor%s",
        "\n");
      return this->_block;
    }

    // return a share view of block, this only work for non-symm tensor.
    Tensor &get_block_(const cytnx_uint64 &idx = 0) { return this->_block; }
    // return a share view of block, this only work for non-symm tensor.
    const Tensor &get_block_(const cytnx_uint64 &idx = 0) const { return this->_block; }

    cytnx_uint64 Nblocks() const { return 1; };
    std::vector<Tensor> get_blocks() const {
      std::vector<Tensor> out;
      cytnx_error_msg(
        true, "[ERROR][DenseUniTensor] cannot use get_blocks(), use get_block() instead!%s", "\n");
      return out;  // this will not share memory!!
    }
    const std::vector<Tensor> &get_blocks_(const bool &silent = false) const {
      cytnx_error_msg(
        true, "[ERROR][DenseUniTensor] cannot use get_blocks_(), use get_block_() instead!%s",
        "\n");
      return this->_interface_block;  // this will not share memory!!
    }
    std::vector<Tensor> &get_blocks_(const bool &silent = false) {
      cytnx_error_msg(
        true, "[ERROR][DenseUniTensor] cannot use get_blocks_(), use get_block_() instead!%s",
        "\n");
      return this->_interface_block;  // this will not share memory!!
    }

    void put_block(const Tensor &in, const cytnx_uint64 &idx = 0) {
      if (this->is_diag()) {
        cytnx_error_msg(
          in.shape() != this->_block.shape(),
          "[ERROR][DenseUniTensor] put_block, the input tensor shape does not match.%s", "\n");
        this->_block = in.clone();
      } else {
        cytnx_error_msg(
          in.shape() != this->shape(),
          "[ERROR][DenseUniTensor] put_block, the input tensor shape does not match.%s", "\n");
        this->_block = in.clone();
      }
    }
    // share view of the block
    void put_block_(Tensor &in, const cytnx_uint64 &idx = 0) {
      if (this->is_diag()) {
        cytnx_error_msg(
          in.shape() != this->_block.shape(),
          "[ERROR][DenseUniTensor] put_block, the input tensor shape does not match.%s", "\n");
        this->_block = in;
      } else {
        cytnx_error_msg(
          in.shape() != this->shape(),
          "[ERROR][DenseUniTensor] put_block, the input tensor shape does not match.%s", "\n");
        this->_block = in;
      }
    }

    void put_block(const Tensor &in, const std::vector<cytnx_int64> &qnum, const bool &force) {
      cytnx_error_msg(
        true, "[ERROR][DenseUniTensor] try to put_block using qnum on a non-symmetry UniTensor%s",
        "\n");
    }
    void put_block_(Tensor &in, const std::vector<cytnx_int64> &qnum, const bool &force) {
      cytnx_error_msg(
        true, "[ERROR][DenseUniTensor] try to put_block using qnum on a non-symmetry UniTensor%s",
        "\n");
    }
    // this will only work on non-symm tensor (DenseUniTensor)
    boost::intrusive_ptr<UniTensor_base> get(const std::vector<Accessor> &accessors) {
      boost::intrusive_ptr<UniTensor_base> out(new DenseUniTensor());
      out->Init_by_Tensor(this->_block.get(accessors), false, 0);  // wrapping around.
      return out;
    }
    // this will only work on non-symm tensor (DenseUniTensor)
    void set(const std::vector<Accessor> &accessors, const Tensor &rhs) {
      this->_block.set(accessors, rhs);
    }

    void reshape_(const std::vector<cytnx_int64> &new_shape, const cytnx_uint64 &rowrank = 0);
    boost::intrusive_ptr<UniTensor_base> reshape(const std::vector<cytnx_int64> &new_shape,
                                                 const cytnx_uint64 &rowrank = 0);
    boost::intrusive_ptr<UniTensor_base> to_dense();
    void to_dense_();
    /**
     * @brief
     *
     * @deprecated
     *
     * @param indicators
     * @param permute_back
     * @param by_label
     */
    void combineBonds(const std::vector<cytnx_int64> &indicators, const bool &force,
                      const bool &by_label);
    void combineBonds(const std::vector<std::string> &indicators, const bool &force = true);
    void combineBonds(const std::vector<cytnx_int64> &indicators, const bool &force = true);
    boost::intrusive_ptr<UniTensor_base> contract(const boost::intrusive_ptr<UniTensor_base> &rhs,
                                                  const bool &mv_elem_self = false,
                                                  const bool &mv_elem_rhs = false);
    std::vector<Bond> getTotalQnums(const bool &physical = false) {
      cytnx_error_msg(true, "[ERROR][DenseUniTensor] %s",
                      "getTotalQnums can only operate on UniTensor with symmetry.\n");
      return std::vector<Bond>();
    }

    std::vector<std::vector<cytnx_int64>> get_blocks_qnums() const {
      cytnx_error_msg(true, "[ERROR][DenseUniTensor] %s",
                      "get_blocks_qnums can only operate on UniTensor with symmetry.\n");
      return std::vector<std::vector<cytnx_int64>>();
    }

    bool same_data(const boost::intrusive_ptr<UniTensor_base> &rhs) const {
      if (rhs->uten_type() != UTenType.Dense) return false;

      return this->get_block_().same_data(rhs->get_block_());
    }

    ~DenseUniTensor(){};

    // arithmetic
    void Add_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    void Add_(const Scalar &rhs);

    void Mul_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    void Mul_(const Scalar &rhs);

    void Sub_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    void Sub_(const Scalar &rhs);
    void lSub_(const Scalar &lhs);

    void Div_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    void Div_(const Scalar &rhs);
    void lDiv_(const Scalar &lhs);

    void Conj_() { this->_block.Conj_(); };

    boost::intrusive_ptr<UniTensor_base> Conj() {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->Conj_();
      return out;
    }

    boost::intrusive_ptr<UniTensor_base> Transpose() {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->Transpose_();
      return out;
    }
    void Transpose_();

    boost::intrusive_ptr<UniTensor_base> normalize() {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->normalize_();
      return out;
    }
    void normalize_();

    boost::intrusive_ptr<UniTensor_base> Dagger() {
      boost::intrusive_ptr<UniTensor_base> out = this->Conj();
      out->Transpose_();
      return out;
    }
    void Dagger_() {
      this->Conj_();
      this->Transpose_();
    }
    /**
     * @brief
     *
     * @deprecated
     *
     * @param a
     * @param b
     * @param by_label
     */
    void Trace_(const cytnx_int64 &a, const cytnx_int64 &b, const bool &by_label);
    void Trace_(const cytnx_int64 &a, const cytnx_int64 &b);
    void Trace_(const std::string &a, const std::string &b);
    boost::intrusive_ptr<UniTensor_base> Trace(const std::string &a, const std::string &b) {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->Trace_(a, b);
      return out;
    }
    boost::intrusive_ptr<UniTensor_base> Trace(const cytnx_int64 &a, const cytnx_int64 &b) {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->Trace_(a, b);
      return out;
    }
    /**
     * @brief
     *
     * @deprecated
     *
     * @param a
     * @param b
     * @param by_label
     */
    boost::intrusive_ptr<UniTensor_base> Trace(const cytnx_int64 &a, const cytnx_int64 &b,
                                               const bool &by_label) {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->Trace_(a, b, by_label);
      return out;
    }

    Tensor Norm() const;

    const Scalar::Sproxy at_for_sparse(const std::vector<cytnx_uint64> &locator) const {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return cytnx_complex128(0,0);
    }
    const cytnx_complex128 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                          const cytnx_complex128 &aux) const {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return cytnx_complex128(0,0);
    }
    const cytnx_complex64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                         const cytnx_complex64 &aux) const {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return cytnx_complex64(0,0);
    }
    const cytnx_double &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                      const cytnx_double &aux) const {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    const cytnx_float &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                     const cytnx_float &aux) const {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    const cytnx_uint64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                      const cytnx_uint64 &aux) const {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    const cytnx_int64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                     const cytnx_int64 &aux) const {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    const cytnx_uint32 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                      const cytnx_uint32 &aux) const {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    const cytnx_int32 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                     const cytnx_int32 &aux) const {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    const cytnx_uint16 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                      const cytnx_uint16 &aux) const {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    const cytnx_int16 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                     const cytnx_int16 &aux) const {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }

    Scalar::Sproxy at_for_sparse(const std::vector<cytnx_uint64> &locator) {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return cytnx_complex128(0,0);
    }
    cytnx_complex128 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                    const cytnx_complex128 &aux) {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return cytnx_complex128(0,0);
    }
    cytnx_complex64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                   const cytnx_complex64 &aux) {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return cytnx_complex64(0,0);
    }
    cytnx_double &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_double &aux) {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    cytnx_float &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_float &aux) {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    cytnx_uint64 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_uint64 &aux) {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    cytnx_int64 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_int64 &aux) {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    cytnx_uint32 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_uint32 &aux) {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    cytnx_int32 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_int32 &aux) {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    cytnx_uint16 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_uint16 &aux) {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }
    cytnx_int16 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_int16 &aux) {
      cytnx_error_msg(
        true, "[ERROR][Internal] This shouldn't be called by DenseUniTensor, something wrong.%s",
        "\n");
      // return 0;
    }

    bool elem_exists(const std::vector<cytnx_uint64> &locator) const {
      cytnx_error_msg(
        true, "[ERROR][DenseUniTensor] elem_exists can only be used on UniTensor with Symmetry.%s",
        "\n");
    }
    void tag() {
      if (!this->is_tag()) {
        for (int i = 0; i < this->_rowrank; i++) {
          this->_bonds[i].set_type(BD_KET);
        }
        for (int i = this->_rowrank; i < this->_bonds.size(); i++) {
          this->_bonds[i].set_type(BD_BRA);
        }
        this->_is_tag = true;
        this->_is_braket_form = this->_update_braket();
      }
    }
    /**
     * @brief
     *
     * @deprecated
     *
     * @param bond_idx
     * @param dim
     * @param by_label
     */
    void truncate_(const cytnx_int64 &bond_idx, const cytnx_uint64 &dim, const bool &by_label);
    void truncate_(const cytnx_int64 &bond_idx, const cytnx_uint64 &dim);
    void truncate_(const std::string &bond_idx, const cytnx_uint64 &dim);

    void group_basis_(){
        cytnx_warning_msg(true,"[WARNING] group basis will not have any effect on DensUniTensor.%s","\n");
    }

    void _save_dispatch(std::fstream &f) const;
    void _load_dispatch(std::fstream &f);

    const std::vector<cytnx_uint64>& get_qindices(const cytnx_uint64 &bidx) const{
        cytnx_error_msg(true,"[ERROR] get_qindices can only be unsed on UniTensor with Symmetry.%s","\n");
        
    }
    std::vector<cytnx_uint64>& get_qindices(const cytnx_uint64 &bidx){
        cytnx_error_msg(true,"[ERROR] get_qindices can only be unsed on UniTensor with Symmetry.%s","\n");
    }

    const vec2d<cytnx_uint64> & get_itoi() const{
        cytnx_error_msg(true,"[ERROR] get_itoi can only be unsed on UniTensor with Symmetry.%s","\n");
    }
    vec2d<cytnx_uint64> & get_itoi(){
        cytnx_error_msg(true,"[ERROR] get_itoi can only be unsed on UniTensor with Symmetry.%s","\n");
    }


    // end virtual function
  };
  /// @endcond

  //======================================================================
  /// @cond
  class SparseUniTensor : public UniTensor_base {
   protected:
   public:
    cytnx_uint64 _inner_rowrank;
    std::vector<std::vector<cytnx_int64>> _blockqnums;
    std::vector<cytnx_uint64> _mapper;
    std::vector<cytnx_uint64> _inv_mapper;
    std::vector<std::vector<cytnx_uint64>> _inner2outer_row;
    std::vector<std::vector<cytnx_uint64>> _inner2outer_col;
    std::map<cytnx_uint64, std::pair<cytnx_uint64, cytnx_uint64>> _outer2inner_row;
    std::map<cytnx_uint64, std::pair<cytnx_uint64, cytnx_uint64>> _outer2inner_col;

    std::vector<Tensor> _blocks;

    bool _contiguous;
    void set_meta(SparseUniTensor *tmp, const bool &inner, const bool &outer) const {
      // outer meta
      if (outer) {
        tmp->_bonds = vec_clone(this->_bonds);
        tmp->_labels = this->_labels;
        tmp->_is_braket_form = this->_is_braket_form;
        tmp->_rowrank = this->_rowrank;
        tmp->_name = this->_name;
      }
      // comm meta
      tmp->_mapper = this->_mapper;
      tmp->_inv_mapper = this->_inv_mapper;
      tmp->_contiguous = this->_contiguous;
      tmp->_is_diag = this->_is_diag;

      // inner meta
      if (inner) {
        tmp->_inner_rowrank = this->_inner_rowrank;
        tmp->_inner2outer_row = this->_inner2outer_row;
        tmp->_inner2outer_col = this->_inner2outer_col;
        tmp->_outer2inner_row = this->_outer2inner_row;
        tmp->_outer2inner_col = this->_outer2inner_col;
        tmp->_blockqnums = this->_blockqnums;
      }
    }
    SparseUniTensor *clone_meta(const bool &inner, const bool &outer) const {
      SparseUniTensor *tmp = new SparseUniTensor();
      this->set_meta(tmp, inner, outer);
      return tmp;
    };

    //===================================
    friend class UniTensor;  // allow wrapper to access the private elems
    SparseUniTensor() {
      this->uten_type_id = UTenType.Sparse;
      this->_is_tag = true;
    };

    // virtual functions
    //void Init(const std::vector<Bond> &bonds, const std::vector<cytnx_int64> &in_labels = {},
    //          const cytnx_int64 &rowrank = -1, const unsigned int &dtype = Type.Double,
    //          const int &device = Device.cpu, const bool &is_diag = false,
    //          const bool &no_alloc = false);
    void Init(const std::vector<Bond> &bonds, const std::vector<std::string> &in_labels = {},
              const cytnx_int64 &rowrank = -1, const unsigned int &dtype = Type.Double,
              const int &device = Device.cpu, const bool &is_diag = false,
              const bool &no_alloc = false, const std::string &name = "");

    void Init_by_Tensor(const Tensor &in_tensor, const bool &is_diag = false,
                        const cytnx_int64 &rowrank = -1, const std::string &name = "") {
      cytnx_error_msg(
        true, "[ERROR][SparseUniTensor] cannot use Init_by_tensor() on a SparseUniTensor.%s", "\n");
    }
    std::vector<cytnx_uint64> shape() const {
      std::vector<cytnx_uint64> out(this->_bonds.size());
      for (cytnx_uint64 i = 0; i < out.size(); i++) {
        out[i] = this->_bonds[i].dim();
      }
      return out;
    }
    bool is_blockform() const { return true; }
    void to_(const int &device) {
      for (cytnx_uint64 i = 0; i < this->_blocks.size(); i++) {
        this->_blocks[i].to_(device);
      }
    };
    boost::intrusive_ptr<UniTensor_base> to(const int &device) {
      if (this->device() == device) {
        return this;
      } else {
        boost::intrusive_ptr<UniTensor_base> out = this->clone();
        out->to_(device);
        return out;
      }
    };
    boost::intrusive_ptr<UniTensor_base> clone() const {
      SparseUniTensor *tmp = this->clone_meta(true, true);
      tmp->_blocks = vec_clone(this->_blocks);
      boost::intrusive_ptr<UniTensor_base> out(tmp);
      return out;
    };

    bool is_contiguous() const { return this->_contiguous; };
    void set_rowrank(const cytnx_uint64 &new_rowrank) {
      cytnx_error_msg((new_rowrank < 1) || (new_rowrank >= this->rank()),
                      "[ERROR][SparseUniTensor] rowrank should be [>=1] and [<UniTensor.rank].%s",
                      "\n");
      cytnx_error_msg(new_rowrank >= this->_labels.size(),
                      "[ERROR] rowrank cannot exceed the rank of UniTensor.%s", "\n");
      if (this->_inner_rowrank != new_rowrank) this->_contiguous = false;

      this->_rowrank = new_rowrank;
      this->_is_braket_form = this->_update_braket();
    }
    /**
     * @brief
     *
     * @deprecated
     *
     * @param new_labels
     * @return boost::intrusive_ptr<UniTensor_base>
     */
    boost::intrusive_ptr<UniTensor_base> relabels(const std::vector<cytnx_int64> &new_labels);
    boost::intrusive_ptr<UniTensor_base> relabels(const std::vector<std::string> &new_labels);
    /**
     * @brief
     *
     * @deprecated
     *
     * @param inx
     * @param new_label
     * @param by_label
     * @return boost::intrusive_ptr<UniTensor_base>
     */
    boost::intrusive_ptr<UniTensor_base> relabel(const cytnx_int64 &inx,
                                                 const cytnx_int64 &new_label,
                                                 const bool &by_label);
    boost::intrusive_ptr<UniTensor_base> relabel(const cytnx_int64 &inx,
                                                 const cytnx_int64 &new_label);
    boost::intrusive_ptr<UniTensor_base> relabel(const cytnx_int64 &inx,
                                                 const std::string &new_label);
    boost::intrusive_ptr<UniTensor_base> relabel(const std::string &inx,
                                                 const std::string &new_label);
    unsigned int dtype() const {
#ifdef UNI_DEBUG
      cytnx_error_msg(this->_blocks.size() == 0, "[ERROR][internal] empty blocks for blockform.%s",
                      "\n");
#endif
      return this->_blocks[0].dtype();
    };
    int device() const {
#ifdef UNI_DEBUG
      cytnx_error_msg(this->_blocks.size() == 0, "[ERROR][internal] empty blocks for blockform.%s",
                      "\n");
#endif
      return this->_blocks[0].device();
    };
    std::string dtype_str() const {
#ifdef UNI_DEBUG
      cytnx_error_msg(this->_blocks.size() == 0, "[ERROR][internal] empty blocks for blockform.%s",
                      "\n");
#endif
      return this->_blocks[0].dtype_str();
    };
    std::string device_str() const {
#ifdef UNI_DEBUG
      cytnx_error_msg(this->_blocks.size() == 0, "[ERROR][internal] empty blocks for blockform.%s",
                      "\n");
#endif
      return this->_blocks[0].device_str();
    };

    boost::intrusive_ptr<UniTensor_base> astype(const unsigned int &dtype) const {
      SparseUniTensor *tmp = this->clone_meta(true, true);
      tmp->_blocks.resize(this->_blocks.size());
      for (cytnx_int64 blk = 0; blk < this->_blocks.size(); blk++) {
        tmp->_blocks[blk] = this->_blocks[blk].astype(dtype);
      }
      boost::intrusive_ptr<UniTensor_base> out(tmp);
      return out;
    };

    /**
     * @brief
     *
     * @deprecated
     *
     * @param mapper
     * @param rowrank
     * @param by_label
     */
    void permute_(const std::vector<cytnx_int64> &mapper, const cytnx_int64 &rowrank=-1,
                  const bool &by_label=false);
    //void permute_(const std::vector<cytnx_int64> &mapper, const cytnx_int64 &rowrank = -1);
    void permute_(const std::vector<std::string> &mapper, const cytnx_int64 &rowrank = -1);
    /**
     * @brief
     *
     * @deprecated
     *
     * @param mapper
     * @param rowrank
     * @param by_label
     * @return boost::intrusive_ptr<UniTensor_base>
     */
    boost::intrusive_ptr<UniTensor_base> permute(const std::vector<cytnx_int64> &mapper,
                                                 const cytnx_int64 &rowrank=-1, const bool &by_label=false);
    //boost::intrusive_ptr<UniTensor_base> permute(const std::vector<cytnx_int64> &mapper,
    //                                             const cytnx_int64 &rowrank = -1);
    boost::intrusive_ptr<UniTensor_base> permute(const std::vector<std::string> &mapper,
                                                 const cytnx_int64 &rowrank = -1);
    boost::intrusive_ptr<UniTensor_base> contiguous();
    boost::intrusive_ptr<UniTensor_base> contiguous_() {
      if (!this->_contiguous) {
        boost::intrusive_ptr<UniTensor_base> titr = this->contiguous();
        SparseUniTensor *tmp = (SparseUniTensor *)titr.get();
        tmp->set_meta(this, true, true);
        this->_blocks = tmp->_blocks;
      }
      return boost::intrusive_ptr<UniTensor_base>(this);
    }
    void print_diagram(const bool &bond_info = false);
    void print_blocks(const bool &full_info=true)const;
    void print_block(const cytnx_int64 &idx, const bool &full_info=true) const;
    std::vector<Symmetry> syms() const;

    Tensor get_block(const cytnx_uint64 &idx = 0) const {
      cytnx_error_msg(idx >= this->_blocks.size(), "[ERROR][SparseUniTensor] index out of range%s",
                      "\n");
      if (this->_contiguous) {
        return this->_blocks[idx].clone();
      } else {
        cytnx_error_msg(true,
                        "[Developing] get block from a non-contiguous SparseUniTensor is currently "
                        "not support. Call contiguous()/contiguous_() first.%s",
                        "\n");
        return Tensor();
      }
    };
    cytnx_uint64 Nblocks() const { return this->_blocks.size(); };
    Tensor get_block(const std::vector<cytnx_int64> &qnum, const bool &force) const {
      if (!force)
        cytnx_error_msg(
          !this->is_braket_form(),
          "[ERROR][Un-physical] cannot get the block by qnums when bra-ket/in-out bonds mismatch "
          "the row/col space.\n permute to the correct physical space first, then get block.%s",
          "\n");
      // std::cout << "get_block" <<std::endl;
      if (this->_contiguous) {
        // std::cout << "contiguous" << std::endl;
        // get dtype from qnum:
        cytnx_int64 idx = -1;
        for (int i = 0; i < this->_blockqnums.size(); i++) {
          // for(int j=0;j<this->_blockqnums[i].size();j++)
          //     std::cout << this->_blockqnums[i][j]<< " ";
          // std::cout << std::endl;
          if (qnum == this->_blockqnums[i]) {
            idx = i;
            break;
          }
        }
        cytnx_error_msg(
          idx < 0,
          "[ERROR][SparseUniTensor] no block with [qnum] exists in the current UniTensor.%s", "\n");
        return this->get_block(idx);
      } else {
        cytnx_error_msg(true,
                        "[Developing] get block from a non-contiguous SparseUniTensor is currently "
                        "not support. Call contiguous()/contiguous_() first.%s",
                        "\n");
        return Tensor();
      }
      return Tensor();
    };

    // return a share view of block, this only work for symm tensor in contiguous form.
    Tensor &get_block_(const cytnx_uint64 &idx = 0) {
      cytnx_error_msg(
        this->is_contiguous() == false,
        "[ERROR][SparseUniTensor] cannot use get_block_() on non-contiguous UniTensor with "
        "symmetry.\n suggest options: \n  1) Call contiguous_()/contiguous() first, then call "
        "get_block_()\n  2) Try get_block()/get_blocks()%s",
        "\n");

      cytnx_error_msg(idx >= this->_blocks.size(),
                      "[ERROR][SparseUniTensor] index exceed the number of blocks.%s", "\n");

      return this->_blocks[idx];
    }
    const Tensor &get_block_(const cytnx_uint64 &idx = 0) const {
      cytnx_error_msg(
        this->is_contiguous() == false,
        "[ERROR][SparseUniTensor] cannot use get_block_() on non-contiguous UniTensor with "
        "symmetry.\n suggest options: \n  1) Call contiguous_()/contiguous() first, then call "
        "get_block_()\n  2) Try get_block()/get_blocks()%s",
        "\n");

      cytnx_error_msg(idx >= this->_blocks.size(),
                      "[ERROR][SparseUniTensor] index exceed the number of blocks.%s", "\n");

      return this->_blocks[idx];
    }

    Tensor &get_block_(const std::vector<cytnx_int64> &qnum, const bool &force) {
      if (!force)
        cytnx_error_msg(
          !this->is_braket_form(),
          "[ERROR][Un-physical] cannot get the block by qnums when bra-ket/in-out bonds mismatch "
          "the row/col space.\n permute to the correct physical space first, then get block.%s",
          "\n");

      cytnx_error_msg(
        this->is_contiguous() == false,
        "[ERROR][SparseUniTensor] cannot use get_block_() on non-contiguous UniTensor with "
        "symmetry.\n suggest options: \n  1) Call contiguous_()/contiguous() first, then call "
        "get_blocks_()\n  2) Try get_block()/get_blocks()%s",
        "\n");

      // get dtype from qnum:
      cytnx_int64 idx = -1;
      for (int i = 0; i < this->_blockqnums.size(); i++) {
        if (qnum == this->_blockqnums[i]) {
          idx = i;
          break;
        }
      }
      cytnx_error_msg(
        idx < 0, "[ERROR][SparseUniTensor] no block with [qnum] exists in the current UniTensor.%s",
        "\n");
      return this->get_block_(idx);
      // cytnx_error_msg(true,"[Developing]%s","\n");
    }
    const Tensor &get_block_(const std::vector<cytnx_int64> &qnum, const bool &force) const {
      if (!force)
        cytnx_error_msg(
          !this->is_braket_form(),
          "[ERROR][Un-physical] cannot get the block by qnums when bra-ket/in-out bonds mismatch "
          "the row/col space.\n permute to the correct physical space first, then get block.%s",
          "\n");

      cytnx_error_msg(
        this->is_contiguous() == false,
        "[ERROR][SparseUniTensor] cannot use get_block_() on non-contiguous UniTensor with "
        "symmetry.\n suggest options: \n  1) Call contiguous_()/contiguous() first, then call "
        "get_blocks_()\n  2) Try get_block()/get_blocks()%s",
        "\n");

      // get dtype from qnum:
      cytnx_int64 idx = -1;
      for (int i = 0; i < this->_blockqnums.size(); i++) {
        if (qnum == this->_blockqnums[i]) {
          idx = i;
          break;
        }
      }
      cytnx_error_msg(
        idx < 0, "[ERROR][SparseUniTensor] no block with [qnum] exists in the current UniTensor.%s",
        "\n");
      return this->get_block_(idx);
    }

    std::vector<Tensor> get_blocks() const {
      if (this->_contiguous) {
        return vec_clone(this->_blocks);
      } else {
        // cytnx_error_msg(true,"[Developing]%s","\n");
        boost::intrusive_ptr<UniTensor_base> tmp = this->clone();
        tmp->contiguous_();
        SparseUniTensor *ttmp = (SparseUniTensor *)tmp.get();
        return ttmp->_blocks;
      }
    };

    const std::vector<Tensor> &get_blocks_(const bool &silent = false) const {
      // cout << "[call this]" << endl;
      if (this->_contiguous) {
        return this->_blocks;
      } else {
        // cytnx_error_msg(true,"[Developing]%s","\n");
        if (!silent)
          cytnx_warning_msg(
            true,
            "[WARNING][SparseUniTensor] call get_blocks_() with a non-contiguous UniTensor should "
            "be used with caution. \ntry: \n1) get_blocks()\n2) call contiguous/contiguous_() "
            "first, then get_blocks_() to get concise results%s",
            "\n");

        return this->_blocks;
      }
    };
    std::vector<Tensor> &get_blocks_(const bool &silent = false) {
      // cout << "[call this]" << endl;
      if (this->_contiguous) {
        return this->_blocks;
      } else {
        if (!silent)
          cytnx_warning_msg(
            true,
            "[WARNING][SparseUniTensor] call get_blocks_() with a non-contiguous UniTensor should "
            "be used with caution. \ntry: \n1) get_blocks()\n2) call contiguous/contiguous_() "
            "first, then get_blocks_() to get concise results%s",
            "\n");

        return this->_blocks;
      }
    };

    bool same_data(const boost::intrusive_ptr<UniTensor_base> &rhs) const {
      if (rhs->uten_type() != UTenType.Sparse) return false;
      if (rhs->get_blocks_(1).size() != this->get_blocks_(1).size()) return false;

      for (int i = 0; i < rhs->get_blocks_(1).size(); i++)
        if (this->get_blocks_(1)[i].same_data(rhs->get_blocks_(1)[i]) == false) return false;

      return true;
    }

    void put_block_(Tensor &in, const cytnx_uint64 &idx = 0) {
      cytnx_error_msg(
        this->is_contiguous() == false,
        "[ERROR][SparseUniTensor] cannot use put_block_() on non-contiguous UniTensor with "
        "symmetry.\n suggest options: \n  1) Call contiguous_()/contiguous() first, then call "
        "put_blocks_()\n  2) Try put_block()/put_blocks()%s",
        "\n");

      cytnx_error_msg(idx >= this->_blocks.size(), "[ERROR][SparseUniTensor] index out of range%s",
                      "\n");
      cytnx_error_msg(in.shape() != this->_blocks[idx].shape(),
                      "[ERROR][SparseUniTensor] the shape of input tensor does not match the shape "
                      "of block @ idx=%d\n",
                      idx);
      this->_blocks[idx] = in;
    };
    void put_block(const Tensor &in, const cytnx_uint64 &idx = 0) {
      cytnx_error_msg(idx >= this->_blocks.size(), "[ERROR][SparseUniTensor] index out of range%s",
                      "\n");
      if (this->_contiguous) {
        cytnx_error_msg(in.shape() != this->_blocks[idx].shape(),
                        "[ERROR][SparseUniTensor] the shape of input tensor does not match the "
                        "shape of block @ idx=%d\n",
                        idx);
        this->_blocks[idx] = in.clone();
      } else {
        cytnx_error_msg(true,
                        "[Developing] put block to a non-contiguous SparseUniTensor is currently "
                        "not support. Call contiguous()/contiguous_() first.%s",
                        "\n");
      }
    };
    void put_block(const Tensor &in, const std::vector<cytnx_int64> &qnum, const bool &force) {
      if (!force)
        cytnx_error_msg(
          !this->is_braket_form(),
          "[ERROR][Un-physical] cannot get the block by qnums when bra-ket/in-out bonds mismatch "
          "the row/col space.\n permute to the correct physical space first, then get block.%s",
          "\n");

      // get dtype from qnum:
      cytnx_int64 idx = -1;
      for (int i = 0; i < this->_blockqnums.size(); i++) {
        if (qnum == this->_blockqnums[i]) {
          idx = i;
          break;
        }
      }
      cytnx_error_msg(
        idx < 0, "[ERROR][SparseUniTensor] no block with [qnum] exists in the current UniTensor.%s",
        "\n");
      this->put_block(in, idx);
    };
    void put_block_(Tensor &in, const std::vector<cytnx_int64> &qnum, const bool &force) {
      if (!force)
        cytnx_error_msg(
          !this->is_braket_form(),
          "[ERROR][Un-physical] cannot get the block by qnums when bra-ket/in-out bonds mismatch "
          "the row/col space.\n permute to the correct physical space first, then get block.%s",
          "\n");

      // get dtype from qnum:
      cytnx_int64 idx = -1;
      for (int i = 0; i < this->_blockqnums.size(); i++) {
        if (qnum == this->_blockqnums[i]) {
          idx = i;
          break;
        }
      }
      cytnx_error_msg(
        idx < 0, "[ERROR][SparseUniTensor] no block with [qnum] exists in the current UniTensor.%s",
        "\n");
      this->put_block_(in, idx);
    };

    // this will only work on non-symm tensor (DenseUniTensor)
    boost::intrusive_ptr<UniTensor_base> get(const std::vector<Accessor> &accessors) {
      cytnx_error_msg(true,
                      "[ERROR][SparseUniTensor][get] cannot use get on a UniTensor with "
                      "Symmetry.\n suggestion: try get_block()/get_blocks() first.%s",
                      "\n");
      return nullptr;
    }
    // this will only work on non-symm tensor (DenseUniTensor)
    void set(const std::vector<Accessor> &accessors, const Tensor &rhs) {
      cytnx_error_msg(true,
                      "[ERROR][SparseUniTensor][set] cannot use set on a UniTensor with "
                      "Symmetry.\n suggestion: try get_block()/get_blocks() first.%s",
                      "\n");
    }
    void reshape_(const std::vector<cytnx_int64> &new_shape, const cytnx_uint64 &rowrank = 0) {
      cytnx_error_msg(true, "[ERROR] cannot reshape a UniTensor with symmetry.%s", "\n");
    }
    boost::intrusive_ptr<UniTensor_base> reshape(const std::vector<cytnx_int64> &new_shape,
                                                 const cytnx_uint64 &rowrank = 0) {
      cytnx_error_msg(true, "[ERROR] cannot reshape a UniTensor with symmetry.%s", "\n");
      return nullptr;
    }
    boost::intrusive_ptr<UniTensor_base> to_dense() {
      cytnx_error_msg(true, "[ERROR] cannot to_dense a UniTensor with symmetry.%s", "\n");
      return nullptr;
    }
    void to_dense_() {
      cytnx_error_msg(true, "[ERROR] cannot to_dense_ a UniTensor with symmetry.%s", "\n");
    }
    /**
     * @brief
     *
     * @deprecated
     *
     * @param indicators
     * @param permute_back
     * @param by_label
     */
    void combineBonds(const std::vector<cytnx_int64> &indicators, const bool &force,
                      const bool &by_label) {
      cytnx_error_msg(true, "[Developing]%s", "\n");
    };
    void combineBonds(const std::vector<std::string> &indicators, const bool &force = true) {
      cytnx_error_msg(true, "[Developing]%s", "\n");
    };
    void combineBonds(const std::vector<cytnx_int64> &indicators, const bool &force = true) {
      cytnx_error_msg(true, "[Developing]%s", "\n");
    };
    boost::intrusive_ptr<UniTensor_base> contract(const boost::intrusive_ptr<UniTensor_base> &rhs,
                                                  const bool &mv_elem_self = false,
                                                  const bool &mv_elem_rhs = false);
    std::vector<Bond> getTotalQnums(const bool &physical = false);
    std::vector<std::vector<cytnx_int64>> get_blocks_qnums() const { return this->_blockqnums; }
    ~SparseUniTensor(){};

    // arithmetic
    void Add_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    void Add_(const Scalar &rhs);

    void Mul_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    void Mul_(const Scalar &rhs);

    void Sub_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    void Sub_(const Scalar &rhs);
    void lSub_(const Scalar &lhs);

    void Div_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    void Div_(const Scalar &rhs);
    void lDiv_(const Scalar &lhs);

    boost::intrusive_ptr<UniTensor_base> Conj() {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->Conj_();
      return out;
    }

    void Conj_() {
      for (int i = 0; i < this->_blocks.size(); i++) {
        this->_blocks[i].Conj_();
      }
    };
    boost::intrusive_ptr<UniTensor_base> Trace(const cytnx_int64 &a, const cytnx_int64 &b);
    boost::intrusive_ptr<UniTensor_base> Trace(const std::string &a, const std::string &b);
    /**
     * @brief
     *
     * @deprecated
     *
     * @param a
     * @param b
     * @param by_label
     * @return boost::intrusive_ptr<UniTensor_base>
     */
    boost::intrusive_ptr<UniTensor_base> Trace(const cytnx_int64 &a, const cytnx_int64 &b,
                                               const bool &by_label);
    void Trace_(const cytnx_int64 &a, const cytnx_int64 &b) {
      cytnx_error_msg(true,
                      "[ERROR] Currently SparseUniTensor does not support inplace Trace!, call "
                      "Trace() instead!%s",
                      "\n");
    }
    void Trace_(const std::string &a, const std::string &b) {
      cytnx_error_msg(true,
                      "[ERROR] Currently SparseUniTensor does not support inplace Trace!, call "
                      "Trace() instead!%s",
                      "\n");
    }
    /**
     * @brief
     *
     * @deprecated
     *
     * @param a
     * @param b
     * @param by_label
     */
    void Trace_(const cytnx_int64 &a, const cytnx_int64 &b, const bool &by_label) {
      cytnx_error_msg(true,
                      "[ERROR] Currently SparseUniTensor does not support inplace Trace!, call "
                      "Trace() instead!%s",
                      "\n");
    }

    void Transpose_();
    boost::intrusive_ptr<UniTensor_base> Transpose() {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->Transpose_();
      return out;
    }

    boost::intrusive_ptr<UniTensor_base> Dagger() {
      boost::intrusive_ptr<UniTensor_base> out = this->Conj();
      out->Transpose_();
      return out;
    }
    void Dagger_() {
      this->Conj_();
      this->Transpose_();
    }

    Tensor Norm() const;
    
    boost::intrusive_ptr<UniTensor_base> normalize() {
      boost::intrusive_ptr<UniTensor_base> out = this->Conj();
      out->normalize_();
      return out;
    }
    void normalize_(){
        cytnx_error_msg(true,"[ERROR] SparseUniTensor is about to deprecated.%s","\n");
    }

    void tag() {
      // no-use!
    }
    /**
     * @brief
     *
     * @deprecated
     *
     * @param bond_idx
     * @param dim
     * @param by_label
     */
    void truncate_(const cytnx_int64 &bond_idx, const cytnx_uint64 &dim, const bool &by_label);
    void truncate_(const cytnx_int64 &bond_idx, const cytnx_uint64 &dim);
    void truncate_(const std::string &bond_idx, const cytnx_uint64 &dim);
    const Scalar::Sproxy at_for_sparse(const std::vector<cytnx_uint64> &locator) const;
    const cytnx_complex128 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                          const cytnx_complex128 &aux) const;
    const cytnx_complex64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                         const cytnx_complex64 &aux) const;
    const cytnx_double &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                      const cytnx_double &aux) const;
    const cytnx_float &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                     const cytnx_float &aux) const;
    const cytnx_uint64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                      const cytnx_uint64 &aux) const;
    const cytnx_int64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                     const cytnx_int64 &aux) const;
    const cytnx_uint32 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                      const cytnx_uint32 &aux) const;
    const cytnx_int32 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                     const cytnx_int32 &aux) const;
    const cytnx_uint16 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                      const cytnx_uint16 &aux) const;
    const cytnx_int16 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                     const cytnx_int16 &aux) const;

    Scalar::Sproxy at_for_sparse(const std::vector<cytnx_uint64> &locator);
    cytnx_complex128 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                    const cytnx_complex128 &aux);
    cytnx_complex64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                   const cytnx_complex64 &aux);
    cytnx_double &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_double &aux);
    cytnx_float &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_float &aux);
    cytnx_uint64 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_uint64 &aux);
    cytnx_int64 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_int64 &aux);
    cytnx_uint32 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_uint32 &aux);
    cytnx_int32 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_int32 &aux);
    cytnx_uint16 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_uint16 &aux);
    cytnx_int16 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_int16 &aux);

    void group_basis_(){
        cytnx_warning_msg(true,"[WARNING] group basis will not have any effect on SparseUniTensor.%s","\n");
    }

    bool elem_exists(const std::vector<cytnx_uint64> &locator) const;
    void _save_dispatch(std::fstream &f) const;
    void _load_dispatch(std::fstream &f);

    const std::vector<cytnx_uint64>& get_qindices(const cytnx_uint64 &bidx) const{
        cytnx_error_msg(true,"[ERROR][SparseUniTensor] get_qindices can only be unsed on BlockUniTensor with Symmetry.%s","\n");
    }
    std::vector<cytnx_uint64>& get_qindices(const cytnx_uint64 &bidx){
        cytnx_error_msg(true,"[ERROR][SparseUniTensor] get_qindices can only be unsed on BlockUniTensor.%s","\n");
    }
    const vec2d<cytnx_uint64> & get_itoi() const{
        cytnx_error_msg(true,"[ERROR][SparseUniTensor] get_itoi can only be unsed on BlockUniTensor with Symmetry.%s","\n");
    }
    vec2d<cytnx_uint64> & get_itoi(){
        cytnx_error_msg(true,"[ERROR][SparseUniTensor] get_itoi can only be unsed on BlockUniTensor with Symmetry.%s","\n");
    }


    // end virtual func

  };
  /// @endcond

  //======================================================================
  /// @cond
  class BlockUniTensor : public UniTensor_base {
    protected:
    public:

        std::vector<std::vector<cytnx_uint64> > _inner_to_outer_idx; 
        std::vector<Tensor> _blocks;
        Tensor NullRefTensor; // this returns when access block is not exists! 

        // given an index list [loc], get qnums from this->_bonds[loc] and return the combined qnums calculated from Symm object!
        // this assume 1. symmetry are the same for each bond! 
        //             2. total_qns are feeded with size len(symmetry)
        void _fx_get_total_fluxs(std::vector<cytnx_uint64> &loc, const std::vector<Symmetry> &syms, std::vector<cytnx_int64> &total_qns){
            memset(&total_qns[0],0,sizeof(cytnx_int64)*total_qns.size());
           
            for(cytnx_int32 i=0;i<syms.size();i++){
                if(this->_bonds[0].type() == BD_BRA)
                    total_qns[i] = syms[0].reverse_rule(this->_bonds[0]._impl->_qnums[loc[0]][i]);
                else
                    total_qns[i] = this->_bonds[0]._impl->_qnums[loc[0]][i];

                for(auto j=1;j<loc.size();j++){
                    if(this->_bonds[j].type() == BD_BRA)
                        total_qns[i] = syms[i].combine_rule(total_qns[i],syms[i].reverse_rule(this->_bonds[j]._impl->_qnums[loc[j]][i]));
                    else{
                        total_qns[i] = syms[i].combine_rule(total_qns[i],this->_bonds[j]._impl->_qnums[loc[j]][i]);
                    }
                }
            }             


        }

        void _fx_locate_elem(cytnx_int64 &bidx, std::vector<cytnx_uint64> &loc_in_T,const std::vector<cytnx_uint64> &locator) const;

        // internal function, grouping all duplicate qnums in all bonds
        void _fx_group_duplicates(const std::vector<cytnx_uint64> &dup_bond_idxs, const std::vector<std::vector<cytnx_uint64> > &idx_mappers);        


        void set_meta(BlockUniTensor *tmp, const bool &inner, const bool &outer) const {
          // outer meta
          if (outer) {
            tmp->_bonds = vec_clone(this->_bonds);
            tmp->_labels = this->_labels;
            tmp->_is_braket_form = this->_is_braket_form;
            tmp->_rowrank = this->_rowrank;
            tmp->_name = this->_name;
          }

          tmp->_is_diag = this->_is_diag;

          // inner meta
          if (inner) {
            tmp->_inner_to_outer_idx = this->_inner_to_outer_idx;
          }
        }


        BlockUniTensor *clone_meta(const bool &inner, const bool &outer) const {
          BlockUniTensor *tmp = new BlockUniTensor();
          this->set_meta(tmp, inner, outer);
          return tmp;
        };



    friend class UniTensor;
    BlockUniTensor(){
        this->uten_type_id = UTenType.Block;
        this->_is_tag = true;
    } 

    //virtual functions:
    //void Init(const std::vector<Bond> &bonds, const std::vector<cytnx_int64> &in_labels = {},
    //          const cytnx_int64 &rowrank = -1, const unsigned int &dtype = Type.Double,
    //          const int &device = Device.cpu, const bool &is_diag = false,
    //          const bool &no_alloc = false);
    
    void Init(const std::vector<Bond> &bonds, const std::vector<std::string> &in_labels = {},
              const cytnx_int64 &rowrank = -1, const unsigned int &dtype = Type.Double,
              const int &device = Device.cpu, const bool &is_diag = false,
              const bool &no_alloc = false, const std::string &name = "");

    void Init_by_Tensor(const Tensor &in_tensor, const bool &is_diag = false,
                        const cytnx_int64 &rowrank = -1, const std::string &name = "") {
      cytnx_error_msg(
        true, "[ERROR][BlockUniTensor] cannot use Init_by_tensor() on a BlockUniTensor.%s", "\n");
    }
    

    std::vector<cytnx_uint64> shape() const {
      std::vector<cytnx_uint64> out(this->_bonds.size());
      for (cytnx_uint64 i = 0; i < out.size(); i++) {
        out[i] = this->_bonds[i].dim();
      }
      return out;
    }

    bool is_blockform() const { return true; }
    bool is_contiguous() const { 
        bool out=true;
        for(int i=0;i<this->_blocks.size();i++){
            out &= this->_blocks[i].is_contiguous();
        }
        return out; 
    };
    
    cytnx_uint64 Nblocks() const { return this->_blocks.size(); };

    void to_(const int &device) {
      for (cytnx_uint64 i = 0; i < this->_blocks.size(); i++) {
        this->_blocks[i].to_(device);
      }
    };

    boost::intrusive_ptr<UniTensor_base> to(const int &device) {
      if (this->device() == device) {
        return this;
      } else {
        boost::intrusive_ptr<UniTensor_base> out = this->clone();
        out->to_(device);
        return out;
      }
    };
    
    boost::intrusive_ptr<UniTensor_base> clone() const {
      BlockUniTensor *tmp = this->clone_meta(true, true);
      tmp->_blocks = vec_clone(this->_blocks);
      boost::intrusive_ptr<UniTensor_base> out(tmp);
      return out;
    };

    unsigned int dtype() const {
#ifdef UNI_DEBUG
      cytnx_error_msg(this->_blocks.size() == 0, "[ERROR][internal] empty blocks for blockform.%s",
                      "\n");
#endif
      return this->_blocks[0].dtype();
    };
    int device() const {
#ifdef UNI_DEBUG
      cytnx_error_msg(this->_blocks.size() == 0, "[ERROR][internal] empty blocks for blockform.%s",
                      "\n");
#endif
      return this->_blocks[0].device();
    };
    std::string dtype_str() const {
#ifdef UNI_DEBUG
      cytnx_error_msg(this->_blocks.size() == 0, "[ERROR][internal] empty blocks for blockform.%s",
                      "\n");
#endif
      return this->_blocks[0].dtype_str();
    };
    std::string device_str() const {
#ifdef UNI_DEBUG
      cytnx_error_msg(this->_blocks.size() == 0, "[ERROR][internal] empty blocks for blockform.%s",
                      "\n");
#endif
      return this->_blocks[0].device_str();

    };

    Tensor get_block(const cytnx_uint64 &idx = 0) const {
      cytnx_error_msg(idx >= this->_blocks.size(), "[ERROR][BlockUniTensor] index out of range%s",
                      "\n");
      return this->_blocks[idx].clone();  
    };

    // this one for Block will return the indicies!!
    Tensor get_block(const std::vector<cytnx_int64> &indices, const bool &force_return) const {
         
        cytnx_error_msg(indices.size()!=this->rank(),"[ERROR][get_block][BlockUniTensor] len(indices) must be the same as the Tensor rank (number of legs).%s","\n");
        
        std::vector<cytnx_uint64> inds(indices.begin(),indices.end());


        //find if the indices specify exists!
        cytnx_int64 b = -1;
        for(cytnx_uint64 i=0;i<this->_inner_to_outer_idx.size();i++){
            if(inds == this->_inner_to_outer_idx[i]){
                b = i;
                break;
            }
        }
        
        if(b<0){
            if(force_return){
                return NullRefTensor;
            }else{
                cytnx_error_msg(true,"[ERROR][get_block][BlockUniTensor] no avaliable block exists, force_return=false, so error throws. \n    If you want to return an empty block without error when block is not avaliable, set force_return=True.%s","\n");
            }
        }else{
            return this->_blocks[b].clone();
        } 
    }
    
    const Tensor& get_block_(const cytnx_uint64 &idx = 0) const {
      cytnx_error_msg(idx >= this->_blocks.size(), "[ERROR][BlockUniTensor] index out of range%s",
                      "\n");
      return this->_blocks[idx];
    };
    
    Tensor& get_block_(const cytnx_uint64 &idx = 0) {
      cytnx_error_msg(idx >= this->_blocks.size(), "[ERROR][BlockUniTensor] index out of range%s",
                      "\n");
      return this->_blocks[idx];
    };
    
    const Tensor &get_block_(const std::vector<cytnx_int64> &indices, const bool &force_return) const {
        cytnx_error_msg(indices.size()!=this->rank(),"[ERROR][get_block][BlockUniTensor] len(indices) must be the same as the Tensor rank (number of legs).%s","\n");
        
        std::vector<cytnx_uint64> inds(indices.begin(),indices.end());

        //find if the indices specify exists!
        cytnx_int64 b = -1;
        for(cytnx_uint64 i=0;i<this->_inner_to_outer_idx.size();i++){
            if(inds == this->_inner_to_outer_idx[i]){
                b = i;
                break;
            }
        }
        
        if(b<0){
            if(force_return){
                return this->NullRefTensor;
            }else{
                cytnx_error_msg(true,"[ERROR][get_block][BlockUniTensor] no avaliable block exists, force_return=false, so error throws. \n    If you want to return an empty block without error when block is not avaliable, set force_return=True.%s","\n");
            }
        }else{
            return this->_blocks[b];
        } 

    }
    
    Tensor &get_block_(const std::vector<cytnx_int64> &indices, const bool &force_return){
        cytnx_error_msg(indices.size()!=this->rank(),"[ERROR][get_block][BlockUniTensor] len(indices) must be the same as the Tensor rank (number of legs).%s","\n");
        
        std::vector<cytnx_uint64> inds(indices.begin(),indices.end());

        //find if the indices specify exists!
        cytnx_int64 b = -1;
        for(cytnx_uint64 i=0;i<this->_inner_to_outer_idx.size();i++){
            if(inds == this->_inner_to_outer_idx[i]){
                b = i;
                break;
            }
        }
        
        if(b<0){
            if(force_return){
                return this->NullRefTensor;
            }else{
                cytnx_error_msg(true,"[ERROR][get_block][BlockUniTensor] no avaliable block exists, force_return=false, so error throws. \n    If you want to return an empty block without error when block is not avaliable, set force_return=True.%s","\n");
            }
        }else{
            return this->_blocks[b];
        } 

    }

    std::vector<Tensor> get_blocks() const{
        return vec_clone(this->_blocks);
    }
    const std::vector<Tensor> &get_blocks_(const bool &) const{
        return this->_blocks;
    }
    std::vector<Tensor> &get_blocks_(const bool &){
        return this->_blocks;
    }


    
    bool same_data(const boost::intrusive_ptr<UniTensor_base> &rhs) const {
      if (rhs->uten_type() != UTenType.Block) return false;
      if (rhs->get_blocks_(1).size() != this->get_blocks_(1).size()) return false;

      for (int i = 0; i < rhs->get_blocks_(1).size(); i++)
        if (this->get_blocks_(1)[i].same_data(rhs->get_blocks_(1)[i]) == false) return false;

      return true;
    }



    void set_rowrank(const cytnx_uint64 &new_rowrank) {
      cytnx_error_msg(new_rowrank > this->rank(),
                      "[ERROR][BlockUniTensor] rowrank should be [>=0] and [<=UniTensor.rank].%s",
                      "\n");
      if(this->is_diag()){
        cytnx_error_msg(new_rowrank != 1,
                      "[ERROR][BlockUniTensor] rowrank should be [==1] when is_diag =true!.%s",
                      "\n");
      }
      this->_rowrank = new_rowrank;
      this->_is_braket_form = this->_update_braket();
    }
   
    boost::intrusive_ptr<UniTensor_base> permute(const std::vector<cytnx_int64> &mapper,
                                                         const cytnx_int64 &rowrank=-1,
                                                         const bool &by_label=false);
    boost::intrusive_ptr<UniTensor_base> permute(const std::vector<std::string> &mapper,
                                                         const cytnx_int64 &rowrank = -1);
    //boost::intrusive_ptr<UniTensor_base> permute(const std::vector<cytnx_int64> &mapper,
    //                                                     const cytnx_int64 &rowrank = -1);

    void permute_(const std::vector<cytnx_int64> &mapper, const cytnx_int64 &rowrank=-1,
                          const bool &by_label=false);
    void permute_(const std::vector<std::string> &mapper, const cytnx_int64 &rowrank = -1);
    //void permute_(const std::vector<cytnx_int64> &mapper, const cytnx_int64 &rowrank = -1);

 
    boost::intrusive_ptr<UniTensor_base> contiguous_() {
      for(unsigned int b=0;b<this->_blocks.size();b++)
        this->_blocks[b].contiguous_();
      return boost::intrusive_ptr<UniTensor_base>(this); 
    }

    boost::intrusive_ptr<UniTensor_base> contiguous();


    void print_diagram(const bool &bond_info = false);
    void print_blocks(const bool &full_info=true) const;
    void print_block(const cytnx_int64 &idx, const bool &full_info=true) const;

    boost::intrusive_ptr<UniTensor_base> contract(
      const boost::intrusive_ptr<UniTensor_base> &rhs, const bool &mv_elem_self = false,
      const bool &mv_elem_rhs = false);
   

    boost::intrusive_ptr<UniTensor_base> relabels(
      const std::vector<cytnx_int64> &new_labels);
    boost::intrusive_ptr<UniTensor_base> relabels(
      const std::vector<std::string> &new_labels);
    boost::intrusive_ptr<UniTensor_base> relabel(const cytnx_int64 &inx,
                                                         const cytnx_int64 &new_label,
                                                         const bool &by_label);
    boost::intrusive_ptr<UniTensor_base> relabel(const std::string &inx,
                                                         const std::string &new_label);
    boost::intrusive_ptr<UniTensor_base> relabel(const cytnx_int64 &inx,
                                                         const cytnx_int64 &new_label);
    boost::intrusive_ptr<UniTensor_base> relabel(const cytnx_int64 &inx,
                                                         const std::string &new_label);
 
    std::vector<Symmetry> syms() const;

    void reshape_(const std::vector<cytnx_int64> &new_shape, const cytnx_uint64 &rowrank = 0) {
      cytnx_error_msg(true, "[ERROR] cannot reshape a UniTensor with symmetry.%s", "\n");
    }
    boost::intrusive_ptr<UniTensor_base> reshape(const std::vector<cytnx_int64> &new_shape,
                                                 const cytnx_uint64 &rowrank = 0) {
      cytnx_error_msg(true, "[ERROR] cannot reshape a UniTensor with symmetry.%s", "\n");
      return nullptr;
    }

    boost::intrusive_ptr<UniTensor_base> astype(const unsigned int &dtype) const {
      BlockUniTensor *tmp = this->clone_meta(true, true);
      tmp->_blocks.resize(this->_blocks.size());
      for (cytnx_int64 blk = 0; blk < this->_blocks.size(); blk++) {
        tmp->_blocks[blk] = this->_blocks[blk].astype(dtype);
      }
      boost::intrusive_ptr<UniTensor_base> out(tmp);
      return out;
    };

    // this will only work on non-symm tensor (DenseUniTensor)
    boost::intrusive_ptr<UniTensor_base> get(const std::vector<Accessor> &accessors){
        cytnx_error_msg(true,
                      "[ERROR][BlockUniTensor][get] cannot use get on a UniTensor with "
                      "Symmetry.\n suggestion: try get_block/get_block_/get_blocks/get_blocks_ first.%s",
                      "\n");
      return nullptr;

    }

    // this will only work on non-symm tensor (DenseUniTensor)
    void set(const std::vector<Accessor> &accessors, const Tensor &rhs){
        cytnx_error_msg(true,
                      "[ERROR][BlockUniTensor][get] cannot use get on a UniTensor with "
                      "Symmetry.\n suggestion: try get_block/get_block_/get_blocks/get_blocks_ first.%s",
                      "\n");
    }

    void put_block(const Tensor &in, const cytnx_uint64 &idx = 0){
        cytnx_error_msg(idx >= this->_blocks.size(), "[ERROR][BlockUniTensor] index out of range%s",
                      "\n");
        cytnx_error_msg(in.shape() != this->_blocks[idx].shape(),
                      "[ERROR][BlockUniTensor] the shape of input tensor does not match the shape "
                      "of block @ idx=%d\n",
                      idx);

        this->_blocks[idx] = in.clone();
    }
    void put_block_(Tensor &in, const cytnx_uint64 &idx = 0){
        cytnx_error_msg(idx >= this->_blocks.size(), "[ERROR][BlockUniTensor] index out of range%s",
                      "\n");
        cytnx_error_msg(in.shape() != this->_blocks[idx].shape(),
                      "[ERROR][BlockUniTensor] the shape of input tensor does not match the shape "
                      "of block @ idx=%d\n",
                      idx);

        this->_blocks[idx] = in;

    }
    void put_block(const Tensor &in, const std::vector<cytnx_int64> &indices,
                   const bool &check){

        cytnx_error_msg(indices.size()!=this->rank(),"[ERROR][put_block][BlockUniTensor] len(indices) must be the same as the Tensor rank (number of legs).%s","\n");

        std::vector<cytnx_uint64> inds(indices.begin(),indices.end());

        //find if the indices specify exists!
        cytnx_int64 b = -1;
        for(cytnx_uint64 i=0;i<this->_inner_to_outer_idx.size();i++){
            if(inds == this->_inner_to_outer_idx[i]){
                b = i;
                break;
            }
        }

        if(b<0){
            if(check){
                cytnx_error_msg(true,"[ERROR][put_block][BlockUniTensor] no avaliable block exists, check=true, so error throws. \n    If you want without error when block is not avaliable, set check=false.%s","\n");
            }
        }else{
            cytnx_error_msg(in.shape() != this->_blocks[b].shape(),
                          "[ERROR][BlockUniTensor] the shape of input tensor does not match the shape "
                          "of block @ idx=%d\n",
                          b);

            this->_blocks[b] = in.clone();
        }


    }
    void put_block_(Tensor &in, const std::vector<cytnx_int64> &indices, const bool &check){
        cytnx_error_msg(indices.size()!=this->rank(),"[ERROR][put_block][BlockUniTensor] len(indices) must be the same as the Tensor rank (number of legs).%s","\n");

        std::vector<cytnx_uint64> inds(indices.begin(),indices.end());

        //find if the indices specify exists!
        cytnx_int64 b = -1;
        for(cytnx_uint64 i=0;i<this->_inner_to_outer_idx.size();i++){
            if(inds == this->_inner_to_outer_idx[i]){
                b = i;
                break;
            }
        }

        if(b<0){
            if(check){
                cytnx_error_msg(true,"[ERROR][put_block][BlockUniTensor] no avaliable block exists, check=true, so error throws. \n    If you want without error when block is not avaliable, set check=false.%s","\n");
            }
        }else{
            cytnx_error_msg(in.shape() != this->_blocks[b].shape(),
                          "[ERROR][BlockUniTensor] the shape of input tensor does not match the shape "
                          "of block @ idx=%d\n",
                          b);
            this->_blocks[b] = in;
        }

    }

    void tag() {
      // no-use!
    }


    boost::intrusive_ptr<UniTensor_base> Conj() {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->Conj_();
      return out;
    }

    void Conj_() {
      for (int i = 0; i < this->_blocks.size(); i++) {
        this->_blocks[i].Conj_();
      }
    };

    void Transpose_();
    boost::intrusive_ptr<UniTensor_base> Transpose() {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->Transpose_();
      return out;
    }
    
    void normalize_();
    boost::intrusive_ptr<UniTensor_base> normalize() {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->normalize_();
      return out;
    }


    boost::intrusive_ptr<UniTensor_base> Dagger() {
      boost::intrusive_ptr<UniTensor_base> out = this->Conj();
      out->Transpose_();
      return out;
    }
    void Dagger_() {
      this->Conj_();
      this->Transpose_();
    }

    void Trace_(const std::string &a, const std::string &b);
    void Trace_(const cytnx_int64 &a, const cytnx_int64 &b);
    void Trace_(const cytnx_int64 &a, const cytnx_int64 &b, const bool &by_label);

    boost::intrusive_ptr<UniTensor_base> Trace(const std::string &a, const std::string &b) {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->Trace_(a, b);
      if(out->rank()==0){
        DenseUniTensor *tmp = new DenseUniTensor();
        tmp->_block = ((BlockUniTensor*)out.get())->_blocks[0];
        out = boost::intrusive_ptr<UniTensor_base>(tmp);
      }
      return out;
    }
    boost::intrusive_ptr<UniTensor_base> Trace(const cytnx_int64 &a, const cytnx_int64 &b) {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->Trace_(a, b);
      if(out->rank()==0){
        DenseUniTensor *tmp = new DenseUniTensor();
        tmp->_block = ((BlockUniTensor*)out.get())->_blocks[0];
        out = boost::intrusive_ptr<UniTensor_base>(tmp);
      }
      return out;
    }
    /**
     * @brief
     *
     * @deprecated
     *
     * @param a
     * @param b
     * @param by_label
     */
    boost::intrusive_ptr<UniTensor_base> Trace(const cytnx_int64 &a, const cytnx_int64 &b,
                                               const bool &by_label) {
      boost::intrusive_ptr<UniTensor_base> out = this->clone();
      out->Trace_(a, b, by_label);
      if(out->rank()==0){
        DenseUniTensor *tmp = new DenseUniTensor();
        tmp->_block = ((BlockUniTensor*)out.get())->_blocks[0];
        out = boost::intrusive_ptr<UniTensor_base>(tmp);
      }
      return out;
    }

    Tensor Norm() const;

    bool elem_exists(const std::vector<cytnx_uint64> &locator) const;

    const Scalar::Sproxy at_for_sparse(const std::vector<cytnx_uint64> &locator) const;
    const cytnx_complex128 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                          const cytnx_complex128 &aux) const;
    const cytnx_complex64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                         const cytnx_complex64 &aux) const;
    const cytnx_double &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                      const cytnx_double &aux) const;
    const cytnx_float &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                     const cytnx_float &aux) const;
    const cytnx_uint64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                      const cytnx_uint64 &aux) const;
    const cytnx_int64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                     const cytnx_int64 &aux) const;
    const cytnx_uint32 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                      const cytnx_uint32 &aux) const;
    const cytnx_int32 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                     const cytnx_int32 &aux) const;
    const cytnx_uint16 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                      const cytnx_uint16 &aux) const;
    const cytnx_int16 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                     const cytnx_int16 &aux) const;

    Scalar::Sproxy at_for_sparse(const std::vector<cytnx_uint64> &locator);
    cytnx_complex128 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                    const cytnx_complex128 &aux);
    cytnx_complex64 &at_for_sparse(const std::vector<cytnx_uint64> &locator,
                                   const cytnx_complex64 &aux);
    cytnx_double &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_double &aux);
    cytnx_float &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_float &aux);
    cytnx_uint64 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_uint64 &aux);
    cytnx_int64 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_int64 &aux);
    cytnx_uint32 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_uint32 &aux);
    cytnx_int32 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_int32 &aux);
    cytnx_uint16 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_uint16 &aux);
    cytnx_int16 &at_for_sparse(const std::vector<cytnx_uint64> &locator, const cytnx_int16 &aux);

    void _save_dispatch(std::fstream &f) const;
    void _load_dispatch(std::fstream &f);

    // this will remove the [q_index]-th qnum at [bond_idx]-th Bond! 
    void truncate_(const cytnx_int64 &bond_idx, const cytnx_uint64 &q_index,
                           const bool &by_label);
    void truncate_(const std::string &bond_idx, const cytnx_uint64 &q_index);
    void truncate_(const cytnx_int64 &bond_idx, const cytnx_uint64 &q_index);


    void Add_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    void Add_(const Scalar &rhs){
        cytnx_error_msg(true, "[ERROR] cannot perform elementwise arithmetic '+' btwn Scalar and BlockUniTensor.\n %s \n",
                    "This operation will destroy block structure. [Suggest] using get/set_block(s) to do operation on the block(s).");
    }

    void Mul_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    void Mul_(const Scalar &rhs);

    void Sub_(const boost::intrusive_ptr<UniTensor_base> &rhs);
    void Sub_(const Scalar &rhs){
        cytnx_error_msg(true, "[ERROR] cannot perform elementwise arithmetic '+' btwn Scalar and BlockUniTensor.\n %s \n",
                    "This operation will destroy block structure. [Suggest] using get/set_block(s) to do operation on the block(s).");
    }
    void lSub_(const Scalar &lhs){
        cytnx_error_msg(true, "[ERROR] cannot perform elementwise arithmetic '+' btwn Scalar and BlockUniTensor.\n %s \n",
                    "This operation will destroy block structure. [Suggest] using get/set_block(s) to do operation on the block(s).");
    }

    void Div_(const boost::intrusive_ptr<UniTensor_base> &rhs){
        cytnx_error_msg(true, "[ERROR] cannot perform elementwise arithmetic '+' btwn Scalar and BlockUniTensor.\n %s \n",
                    "This operation will destroy block structure. [Suggest] using get/set_block(s) to do operation on the block(s).");

    }
    void Div_(const Scalar &rhs);
    void lDiv_(const Scalar &lhs){
        cytnx_error_msg(true, "[ERROR] cannot perform elementwise arithmetic '+' btwn Scalar and BlockUniTensor.\n %s \n",
                    "This operation will destroy block structure. [Suggest] using get/set_block(s) to do operation on the block(s).");
    }

    void group_basis_();
    
    void combineBonds(const std::vector<cytnx_int64> &indicators,
                              const bool &force = false);
    void combineBonds(const std::vector<cytnx_int64> &indicators, const bool &force,
                              const bool &by_label);
    void combineBonds(const std::vector<std::string> &indicators,
                              const bool &force = false);

    const std::vector<cytnx_uint64>& get_qindices(const cytnx_uint64 &bidx) const{
        cytnx_error_msg(bidx>=this->Nblocks(),"[ERROR][BlockUniTensor] bidx out of bound! only %d blocks in current UTen.\n",this->Nblocks());
        return this->_inner_to_outer_idx[bidx];
    }
    std::vector<cytnx_uint64>& get_qindices(const cytnx_uint64 &bidx){
        cytnx_error_msg(bidx>=this->Nblocks(),"[ERROR][BlockUniTensor] bidx out of bound! only %d blocks in current UTen.\n",this->Nblocks());
        return this->_inner_to_outer_idx[bidx];
    }

    const vec2d<cytnx_uint64> & get_itoi() const{
        return this->_inner_to_outer_idx;
    }
    vec2d<cytnx_uint64> & get_itoi(){
        return this->_inner_to_outer_idx;
    }



  };
  /// @endcond
  //======================================================================

  ///@brief An Enhanced tensor specifically designed for physical Tensor network simulation
  class UniTensor {
   public:
    ///@cond
    boost::intrusive_ptr<UniTensor_base> _impl;
    UniTensor() : _impl(new UniTensor_base()){};
    UniTensor(const UniTensor &rhs) { this->_impl = rhs._impl; }
    UniTensor &operator=(const UniTensor &rhs) {
      this->_impl = rhs._impl;
      return *this;
    }
    ///@endcond

    //@{
    /**
    @brief Construct a UniTensor from a cytnx::Tensor.
    @param[in] in_tensor a cytnx::Tensor
    @param[in] is_diag Whther the current UniTensor is a diagonal Tensor. 
	                   This will requires that the input of \p in_tensor to be 1D.
    @param[in] rowrank the rowrank of the outcome UniTensor

    @note
        1. The constructed UniTensor will have same rank as the input Tensor, with default labels,
    and a shared view (shared instance) of interal block as the input Tensor.
        2. The constructed UniTensor is always untagged.
    @attention The internal block of UniTensor is a referece of input Tensor. That is, they
    share the same memory. All the change afterward on UniTensor block will change in input Tensor
    as well. Use Tensor.clone() if a shared view is not the case.

    ## Example:
    ### c++ API:
    \include example/UniTensor/fromTensor.cpp
    #### output>
    \verbinclude example/UniTensor/fromTensor.cpp.out
    ### python API:
    \include example/UniTensor/fromTensor.py
    #### output>
    \verbinclude example/UniTensor/fromTensor.py.out

    */
    explicit UniTensor(const Tensor &in_tensor, const bool &is_diag = false,
                       const cytnx_int64 &rowrank = -1, const std::string &name = "")
        : _impl(new UniTensor_base()) {
      this->Init(in_tensor, is_diag, rowrank, name);
    }
    /**
    @brief Initialize a UniTensor with cytnx::Tensor.
    @param[in] in_tensor a cytnx::Tensor
    @param[in] is_diag if the current UniTensor is a diagonal Tensor. 
	                   This will requires input of Tensor to be 1D.
    @param[in] rowrank the rowrank of the outcome UniTensor.
    @param[in] name user specified name of the UniTensor.

    @note
        1. The constructed UniTensor will have same rank as the input Tensor, with default labels,
    and a shared view (shared instance) of interal block as the input Tensor.
        2. The constructed UniTensor is always untagged.
    @attention The internal block of UniTensor is a referece of input Tensor. That is, they
    share the same memory. All the change afterward on UniTensor block will change in input Tensor
    as well. Use Tensor.clone() if a shared view is not the case.
	@see UniTensor(const Tensor &, const bool &, const cytnx_int64 &)
    */
    void Init(const Tensor &in_tensor, const bool &is_diag = false,
              const cytnx_int64 &rowrank = -1, const std::string &name = "") {
      //std::cout << "[entry!]" << std::endl;
      boost::intrusive_ptr<UniTensor_base> out(new DenseUniTensor());
      out->Init_by_Tensor(in_tensor, is_diag, rowrank, name);
      this->_impl = out;
    }
    //@}

    //@{
    /**
    @brief Construct a UniTensor.
    @param[in] bonds the bond list. Each bond will be deep copy( not a shared view of bond object
    with input bond)
    @param[in] in_labels the labels for each rank(bond)
    @param[in] rowrank the rank of physical row space.
    @param[in] dtype the data type of the UniTensor. It can be any type defined in cytnx::Type.
    @param[in] device the device that the UniTensor is put on. It can be any device defined in
    cytnx::Device.
    @param[in] is_diag if the constructed UniTensor is a diagonal UniTensor.
        This can only be assigned true when the UniTensor is square, untagged and rank-2 UniTensor.
    @pre
        1. the bonds cannot contain simutaneously untagged bond(s) and tagged bond(s)
        2. If the bonds are with symmetry (qnums), the symmetry types should be the same across all
             the bonds.
    */
    UniTensor(const std::vector<Bond> &bonds, const std::vector<std::string> &in_labels = {},
              const cytnx_int64 &rowrank = -1, const unsigned int &dtype = Type.Double,
              const int &device = Device.cpu, const bool &is_diag = false, const std::string &name = "")
        : _impl(new UniTensor_base()) {
#ifdef UNI_DEBUG
      cytnx_warning_msg(
        true,
        "[DEBUG] message: entry for UniTensor(const std::vector<Bond> &bonds, const "
        "std::vector<std::string> &in_labels={}, const cytnx_int64 &rowrank=-1, const unsigned int "
        "&dtype=Type.Double, const int &device = Device.cpu, const bool &is_diag=false)%s",
        "\n");
#endif
      this->Init(bonds, in_labels, rowrank, dtype, device, is_diag, name);
    }
    /**
     * @deprecated
     *  This constructor (integer labels) is deprecated, use \n
    UniTensor(const std::vector<Bond>&, const std::vector<std::string> &in_labels,
              const cytnx_int64 &rowrank, const unsigned int &dtype,
              const int &device, const bool &is_diag) \n
	 *  instread.
     */
    UniTensor(const std::vector<Bond> &bonds, const std::vector<cytnx_int64> &in_labels,
              const cytnx_int64 &rowrank = -1, const unsigned int &dtype = Type.Double,
              const int &device = Device.cpu, const bool &is_diag = false, const std::string &name = "")
        : _impl(new UniTensor_base()) {
#ifdef UNI_DEBUG
      cytnx_warning_msg(
        true,
        "[DEBUG] message: entry for UniTensor(const std::vector<Bond> &bonds, const "
        "std::vector<cytnx_int64> &in_labels={}, const cytnx_int64 &rowrank=-1, const unsigned int "
        "&dtype=Type.Double, const int &device = Device.cpu, const bool &is_diag=false)%s",
        "\n");
#endif
      std::vector<std::string> vs;
      for (int i = 0; i < (int)in_labels.size(); i++) vs.push_back(std::to_string(in_labels[i]));
      this->Init(bonds, vs, rowrank, dtype, device, is_diag, name);
    }
    /**
    @brief Initialize the UniTensor with the given arguments.
	@details This is the initial function of the UniTensor. If you want to initialize 
	   your UniTensor after declaring just a 'void' UniTensor. You can use this 
       function to  initialize it.
    @param[in] bonds the bond list. Each bond will be deep copy( not a shared view of 
	   bond object with input bond)
    @param[in] in_labels the labels for each rank(bond)
    @param[in] rowrank the rank of physical row space
    @param[in] dtype the dtype of the UniTensor. It can be any type defined in cytnx::Type.
    @param[in] device the device that the UniTensor is put on. It can be any device defined in
    cytnx::Device.
    @param[in] is_diag if the constructed UniTensor is a diagonal UniTensor. This can 
	   only be assigned true when the UniTensor is square, untagged and rank-2 
	   UniTensor.
    @param[in] name user specified name of the UniTensor.
	@pre Please ensure that all of the Bond in \p bonds should be all symmetric or 
	  non-symmetric. You cannot mix them.
	@see
    UniTensor(const std::vector<Bond> &bonds, const std::vector<std::string> &in_labels,
              const cytnx_int64 &rowrank, const unsigned int &dtype, const int &device,
			  const bool &is_diag)
    */
    void Init(const std::vector<Bond> &bonds, const std::vector<std::string> &in_labels = {},
              const cytnx_int64 &rowrank = -1, const unsigned int &dtype = Type.Double,
              const int &device = Device.cpu, const bool &is_diag = false, const std::string &name = "") {
      // checking type:
      bool is_sym = false;
      int sym_fver = -1;

      for (cytnx_uint64 i = 0; i < bonds.size(); i++) {
        // check
        if (bonds[i].syms().size() != 0){
          is_sym = true;
          if(sym_fver == -1) sym_fver = bonds[i]._impl->_degs.size();
          else{ 
            //std::cout << sym_fver << " " <<
            //bonds[i]._impl->_degs.size() << std::endl;
            cytnx_error_msg((bool(sym_fver)^bool(bonds[i]._impl->_degs.size())), "[ERROR] All the Bond when init a UniTensor with symmetric must be in the same format!%s","\n");
          }
        }else
          cytnx_error_msg(
            is_sym, "[ERROR] cannot have bonds with mixing of symmetry and non-symmetry.%s", "\n");
      }

      // dynamical dispatch:
      if (is_sym) {
#ifdef UNI_DEBUG
        cytnx_warning_msg(true, "[DEBUG] message: entry dispatch: UniTensor: symmetric%s", "\n");
#endif
        // cytnx_warning_msg(true,"[warning, still developing, some functions will display
        // \"[Developing]\"][SparseUniTensor]%s","\n");
        if(sym_fver == 0){
            boost::intrusive_ptr<UniTensor_base> out(new SparseUniTensor());
            this->_impl = out;
        }else if(sym_fver==-1){
            cytnx_error_msg(true,"[ERROR] internal error! the Bond is symmetry but the version is not properly determined!%s","\n");
        }else{
            boost::intrusive_ptr<UniTensor_base> out(new BlockUniTensor());
            this->_impl = out;
        }
      } else {
        boost::intrusive_ptr<UniTensor_base> out(new DenseUniTensor());
        this->_impl = out;
      }
      this->_impl->Init(bonds, in_labels, rowrank, dtype, device, is_diag, false, name);
    }
    /**
    @deprecated
	This function is deprecated. Please use \n 
    Init(const std::vector<Bond> &bonds, const std::vector<std::string> &in_labels,
         const cytnx_int64 &rowrank, const unsigned int &dtype,
         const int &device, const bool &is_diag) \n
	 instread.
     */
    void Init(const std::vector<Bond> &bonds, const std::vector<cytnx_int64> &in_labels,
              const cytnx_int64 &rowrank = -1, const unsigned int &dtype = Type.Double,
              const int &device = Device.cpu, const bool &is_diag = false, const std::string &name = "") {
      std::vector<std::string> vs;
      cytnx_warning_msg(true,"[Deprecated warning] specify label with integers will be depreated soon. use string instead.%s","\n");
      for (int i = 0; i < (int)in_labels.size(); i++) vs.push_back(std::to_string(in_labels[i]));
      this->Init(bonds, vs, rowrank, dtype, device, is_diag, name);
    }

    //@}

    /**
    @brief Set the name of the UniTensor.
	@details You can use this function to give a name for the UniTensor.
    @param[in] in Input the name you want to set for the UniTensor. It should be a string.
	@return UniTensor
    */
    UniTensor &set_name(const std::string &in) {
      this->_impl->set_name(in);
      return *this;
    }
    /**
    @deprecated
	This function is deprecated. Please use \n 
	@ref set_label(const cytnx_int64 &idx, const std::string &new_label)\n instead.
    @note
        the new assign label cannot be the same as the label of any other bonds in the UniTensor.
        ( cannot have duplicate labels )
    */
    UniTensor &set_label(const cytnx_int64 &idx, const cytnx_int64 &new_label,
                         const bool &by_label) {
      this->_impl->set_label(idx, new_label, by_label);
      return *this;
    }

    /**
    @deprecated
	This function is deprecated. Please use \n 
	@ref set_label(const cytnx_int64 &idx, const std::string &new_label)\n instead
    @note
        the new assign label cannot be the same as the label of any other bonds in the UniTensor.
        ( cannot have duplicate labels )

    */
    UniTensor &set_label(const cytnx_int64 &idx, const cytnx_int64 &new_label) {
      this->_impl->set_label(idx, new_label);
      return *this;
    }
    /**
    @brief Set a new label for bond at the assigned index.
    @param[in] idx the index of the bond.
    @param[in] new_label the new label that is assign to the bond.
    @note
        the new assign label cannot be the same as the label of any other bonds in the UniTensor.
        ( cannot have duplicate labels )
    */
    UniTensor &set_label(const cytnx_int64 &idx, const std::string &new_label) {
      this->_impl->set_label(idx, new_label);
      return *this;
    }

    /**
	@see set_label(const cytnx_int64 &idx, const std::string &new_label)
    */
    UniTensor &set_label(const cytnx_int64 &idx, const char* new_label){
      this->_impl->set_label(idx, std::string(new_label));
      return *this;
    }

    /**
    @brief set a new label for bond to replace one of the current label.
    @param[in] old_label the current label of the bond.
    @param[in] new_label the new label that is assign to the bond.
    @note
        the new assign label cannot be the same as the label of any other bonds in the UniTensor.
        ( cannot have duplicate labels )
    */
    UniTensor &set_label(const std::string &old_label, const std::string &new_label) {
      this->_impl->set_label(old_label, new_label);
      return *this;
    }

    /**
	@see set_label(const std::string &old_label, const std::string &new_label)
    */
    UniTensor &set_label(const char* old_label, const std::string &new_label) {
      this->_impl->set_label(std::string(old_label), new_label);
      return *this;
    }
 
    /**
	@see set_label(const std::string &old_label, const std::string &new_label)
    */
    UniTensor &set_label(const std::string &old_label, const char* new_label) {
      this->_impl->set_label(old_label, std::string(new_label));
      return *this;
    }

    /**
	@see set_label(const std::string &old_label, const std::string &new_label)
    */
    UniTensor &set_label(const char* old_label, const char* new_label) {
      this->_impl->set_label(std::string(old_label), std::string(new_label));
      return *this;
    }


    /**
    @brief change a new label for bond with original label.
    @param old_lbl the original label of the bond that to be replaced.
    @param new_label the new label that is assign to replace the original label.

    [Note]
        the new assign label cannot be the same as the label of any other bonds in the UniTensor.
        ( cannot have duplicate labels )

    */
    /*
    UniTensor& change_label(const cytnx_int64 &old_lbl, const cytnx_int64 &new_label){
        this->_impl->change_label(old_lbl,new_label);
        return *this;
    }
    */

    /**
    @deprecated
	  This is deprecated function. Please use \ref
      set_labels(const std::vector<std::string> &new_labels)\n
	  instead
    */
    UniTensor &set_labels(const std::vector<cytnx_int64> &new_labels) {
      this->_impl->set_labels(new_labels);
      return *this;
    }
    /**
    @brief Set new labels for all the bonds.
    @param[in] new_labels the new labels for each bond.
    @note
        The new assign labels cannot have duplicate element(s), and should have the 
		same size as the rank of UniTensor.
    */
    UniTensor &set_labels(const std::vector<std::string> &new_labels) {
      this->_impl->set_labels(new_labels);
      return *this;
    }
    /**
	@see 
    set_labels(const std::vector<std::string> &new_labels)
	 */
    UniTensor &set_labels(const std::initializer_list<char*> &new_labels) {
      std::vector<char*> new_lbls(new_labels);
      std::vector<std::string> vs(new_lbls.size());
      transform(new_lbls.begin(),new_lbls.end(), vs.begin(),[](char * x) -> std::string { return std::string(x); });
 
      this->_impl->set_labels(vs);
      return *this;
    }

    /**
    @brief Set the row rank of the UniTensor.
	@details You can use this function to set the row rank of the UniTensor. The row rank is 
	  important if you want to use the linear algebra process.
    @param[in] new_rowrank the new row rank of the UniTensor
    */
    UniTensor &set_rowrank(const cytnx_uint64 &new_rowrank) {
      this->_impl->set_rowrank(new_rowrank);
      return *this;
    }

    template <class T>
    T &item() {
      cytnx_error_msg(this->is_blockform(),
                      "[ERROR] cannot use item on UniTensor with Symmetry.\n suggestion: use "
                      "get_block()/get_blocks() first.%s",
                      "\n");

      DenseUniTensor *tmp = static_cast<DenseUniTensor *>(this->_impl.get());
      return tmp->_block.item<T>();
    }

    Scalar::Sproxy item() const {
      cytnx_error_msg(this->is_blockform(),
                      "[ERROR] cannot use item on UniTensor with Symmetry.\n suggestion: use "
                      "get_block()/get_blocks() first.%s",
                      "\n");

      DenseUniTensor *tmp = static_cast<DenseUniTensor *>(this->_impl.get());
      return tmp->_block.item();
    }
    /**
    @brief Return the blocks' number.
    @return cytnx_uint64
    */
    cytnx_uint64 Nblocks() const { return this->_impl->Nblocks(); }

    /**
    @brief Return the rank of the UniTensor.
    @return cytnx_uint64
    */
    cytnx_uint64 rank() const { return this->_impl->rank(); }

    /**
    @brief Return the row rank of the UniTensor.
    @return cytnx_uint64
    */
    cytnx_uint64 rowrank() const { return this->_impl->rowrank(); }

    /**
    @brief Return the data type of the UniTensor.
	@details The function return the data type of the UniTensor. 
    @return unsigned int
    */
    unsigned int dtype() const { return this->_impl->dtype(); }

    /**
    @brief Return the UniTensor type (cytnx::UTenType) of the UniTensor.
	@details The function return the UniTensor type of the UniTensor. 
    @return int
	@see uten_type_str()
    */
    int uten_type() const { return this->_impl->uten_type(); }

    /**
    @brief Return the device of the UniTensor.
	@details The function return the device of the UniTensor. 
    @return int
    */
    int device() const { return this->_impl->device(); }

    /**
    @brief Return the name of the UniTensor.
    @return std::string
    */
    std::string name() const { return this->_impl->name(); }

    /**
    @brief Return the data type of the UniTensor in 'string' form.
    @return std::string
	@see dtype()
    */
    std::string dtype_str() const { return this->_impl->dtype_str(); }

    /**
    @brief Return the device of the UniTensor in 'string' form.
    @return std::string
	@see device()
    */
    std::string device_str() const { return this->_impl->device_str(); }

    /**
    @brief Return the UniTensor type (cytnx::UTenType) of the UniTensor in 'string' form.
    @return std::string
	@see uten_type()
    */
    std::string uten_type_str() const { return this->_impl->uten_type_str(); }

    /**
    @brief To tell whether the UniTensor is contiguous.
    @return bool
	@see contiguous(), contiguous_()
    */
    bool is_contiguous() const { return this->_impl->is_contiguous(); }

    /**
    @brief To tell whether the UniTensor is in diagonal form.
    @return bool
    */
    bool is_diag() const { return this->_impl->is_diag(); }
    bool is_tag() const { return this->_impl->is_tag(); }

    /**
    @brief Return the symmetry type of the UniTensor.
	@details We can get the Symmetry structure by calling this function.
    @return std::vector<Symmetry>
    */
    std::vector<Symmetry> syms() const { return this->_impl->syms(); }
    const bool &is_braket_form() const { return this->_impl->is_braket_form(); }

    /**
    @brief Return the labels of the UniTensor.
    @return std::vector<std::string>
    */
    const std::vector<std::string> &labels() const { return this->_impl->labels(); }
    /**
     * @brief Get the index of an desired label string
     *
     * @param lbl Label you want to find
     * @return The index of the label. If not found, return -1
     */
    cytnx_int64 get_index(std::string lbl) const { return this->_impl->get_index(lbl); }

    /**
    @brief Get the bonds of the UniTensor.
    @return std::vector<Bond>
    */
    const std::vector<Bond> &bonds() const { return this->_impl->bonds(); }

    /**
	@see bonds();
    */
    std::vector<Bond> &bonds() { return this->_impl->bonds(); }

    /**
    @brief Get the shape of the UniTensor.
    @return std::vector<cytnx_uint64>
    */
    std::vector<cytnx_uint64> shape() const { return this->_impl->shape(); }
    bool is_blockform() const { return this->_impl->is_blockform(); }

    /**
    @brief move the current UniTensor to the assigned device (inplace).
	@param[in] device the device-id(@ref cytnx::Device) that is moving to. It can by 
	    any device defined in cytnx::Device.
	@see to_(const int &device)
    */
    void to_(const int &device) { this->_impl->to_(device); }

    /**
    @brief move the current UniTensor to the assigned device.
	@warning if the device-id is the same as current Tensor's device, then return self.
	  otherwise, return a copy of instance that located on the target device. 
	@param[in] device the device-id(@ref cytnx::Device) that is moving to. It can by 
	any device defined in cytnx::Device.
	@return UniTensor
	@see to(const int &device)
    */
    UniTensor to(const int &device) const {
      UniTensor out;
      out._impl = this->_impl->to(device);
      return out;
    }

    /**
    @brief Clone (deep copy) the UniTensor.
	@return UniTensor
    */
    UniTensor clone() const {
      UniTensor out;
      out._impl = this->_impl->clone();
      return out;
    }

    /**
     * @deprecated
	 * It is recommanded to use \ref 
         relabels(const std::vector<std::string> &new_labels) const
     */
    UniTensor relabels(const std::vector<cytnx_int64> &new_labels) const {
      UniTensor out;
      out._impl = this->_impl->relabels(new_labels);
      return out;
    }

    /**
    @brief rebables all of the labels in UniTensor.
	@return UniTensor
     */
    UniTensor relabels(const std::vector<std::string> &new_labels) const {
      UniTensor out;
      out._impl = this->_impl->relabels(new_labels);
      return out;
    }

    /**
    @see relabels(const std::vector<std::string> &new_labels)
     */
    UniTensor relabels(const std::initializer_list<char*> &new_lbls) const{
        std::vector<char*> new_labels(new_lbls);
        std::vector<std::string> vs(new_labels.size());
        transform(new_labels.begin(),new_labels.end(), vs.begin(),[](char * x) -> std::string { return std::string(x); });
        //std::cout << new_labels.size() << std::endl;
        //std::cout << vs << std::endl;

        UniTensor out;
        out._impl =  this->_impl->relabels(vs);
        return out;
    }

    /**
     @deprecated
	 It is recommened to use \ref 
     relabel(const cytnx_int64 &inx, const std::string &new_label)
     */
    UniTensor relabel(const cytnx_int64 &inx, const cytnx_int64 &new_label,
                      const bool &by_label=false) const {
      UniTensor out;
      out._impl = this->_impl->relabel(inx, new_label, by_label);
      return out;
    }

    /**
    @brief rebable the lags in the UniTensor by given index.
	@param[in] inx a given index
	@param[in] new_label the new label of the UniTensor in the index \p inx
     */
    UniTensor relabel(const cytnx_int64 &inx, const std::string &new_label) const {
      UniTensor out;
      out._impl = this->_impl->relabel(inx, new_label);
      return out;
    }

    /**
    @brief rebable the lags in the UniTensor by a given label.
	@param[in] old_label original label you want to replace
	@param[in] new_label the new label 
     */
    UniTensor relabel(const std::string &old_label, const std::string &new_label) const {
      UniTensor out;
      out._impl = this->_impl->relabel(old_label, new_label);
      return out;
    }

    /**
    @brief Return a new UniTensor that cast to different data type.
	@param[in] new_type the new data type. It an be any type defined in cytnx::Type.
	@return UniTensor
	@attention If the \p new_type is same as dtype of the original UniTensor, return self.
     */
    UniTensor astype(const unsigned int &dtype) const {
      UniTensor out;
      if (this->dtype() == dtype) {
        out._impl = this->_impl;
      } else {
        out._impl = this->_impl->astype(dtype);
      }
      return out;
    }

    /**
     * @brief permute the lags of the UniTensor
     * @param[in] mapper the mapper of the permutation
     * @param[in] rowrank the new rowrank after the permutation
     * @param[in] by_label Whether permute the lags by label. It can only be used if labels 
	 *     are integer form, which will be deprecated soon.
     * @return UniTensor
	 *
	 * @warning \p by_label will be deprecated! 
     */
    UniTensor permute(const std::vector<cytnx_int64> &mapper, const cytnx_int64 &rowrank=-1,
                      const bool &by_label=false) const {
      UniTensor out;
      out._impl = this->_impl->permute(mapper, rowrank, by_label);
      return out;
    }
    //UniTensor permute(const std::vector<cytnx_int64> &mapper, const cytnx_int64 &rowrank = -1) {
    //  UniTensor out;
    //  out._impl = this->_impl->permute(mapper, rowrank);
    //  return out;
    //}
	//
    /**
     * @brief permute the lags of the UniTensor by labels
     * @param[in] mapper the mapper by babels
     * @param[in] rowrank the row rank
     * @return UniTensor
     */
    UniTensor permute(const std::vector<std::string> &mapper, const cytnx_int64 &rowrank = -1) const {
      UniTensor out;
      out._impl = this->_impl->permute(mapper, rowrank);
      return out;
    }

    /**
	@see permute(const std::vector<std::string> &mapper, const cytnx_int64 &rowrank = -1)
	*/
    UniTensor permute( const std::initializer_list<char*> &mapper, const cytnx_int64 &rowrank= -1) const{
        std::vector<char*> mprs = mapper;
        std::vector<std::string> vs(mprs.size());
        transform(mprs.begin(),mprs.end(),vs.begin(),[](char * x) -> std::string { return std::string(x); });

        return this->permute(vs,rowrank);
    }

    /**
    @brief permute the lags of the UniTensor, inplacely.
    @deprecated It is recommended to use \ref
      permute_(const std::vector<std::string> &mapper, const cytnx_int64 &rowrank = -1)
    @param[in] mapper the mapper by labels
    @param[in] rowrank the row rank after the permutation
    @param[in] by_label permute by label or index.
	@note bylabel will be deprecated! 
    */
    void permute_(const std::vector<cytnx_int64> &mapper, const cytnx_int64 &rowrank=-1,
                  const bool &by_label=false) {
      this->_impl->permute_(mapper, rowrank, by_label);
    }

    /**
    @brief permute the lags of the UniTensor, inplacely.
    @param[in] mapper the mapper by labels
    @param[in] rowrank the row rank after the permutation
	@see permute(const std::vector<std::string> &mapper, const cytnx_int64 &rowrank = -1)
    */
    void permute_(const std::vector<std::string> &mapper, const cytnx_int64 &rowrank = -1) {
      this->_impl->permute_(mapper, rowrank);
    }

    // void permute_( const std::initializer_list<char*> &mapper, const cytnx_int64 &rowrank= -1){
    //     std::vector<char*> mprs = mapper;
    //     std::vector<std::string> vs(mprs.size());
    //     transform(mprs.begin(),mprs.end(),vs.begin(),[](char * x) -> std::string { return std::string(x); });

    //     this->permute_(vs,rowrank);
    // }

    //void permute_(const std::vector<cytnx_int64> &mapper, const cytnx_int64 &rowrank = -1) {
    //  this->_impl->permute_(mapper, rowrank);
    //}

    /**
    @brief Make the UniTensor contiguous by coalescing the memory (storage).
	@see contiguous_()
    */
    UniTensor contiguous() const {
      UniTensor out;
      out._impl = this->_impl->contiguous();
      return out;
    }

    /**
    @brief Make the UniTensor contiguous by coalescing the memory (storage), inplacely.
	@see contiguous()
    */
    void contiguous_() { this->_impl = this->_impl->contiguous_(); }

    /**
    @brief Plot the diagram of the UniTensor.
	@param[in] bond_info whether need to print the information of the bonds of the UniTensor.
    */
    void print_diagram(const bool &bond_info = false) { this->_impl->print_diagram(bond_info); }

    /**
    @brief Print all of the blocks in the UniTensor.
	@param[in] full_info whether need to print the full information of the blocks
    */
    void print_blocks(const bool &full_info=true) const{ this->_impl->print_blocks(full_info); }

    /**
    @brief Given a index and print out the corresponding block of the UniTensor.
	@param[in] idx the input index
	@param[in] full_info whether need to print the full information of the block
    */
    void print_block(const cytnx_int64 &idx, const bool &full_info=true) const{this->_impl->print_block(idx,full_info);}

    void group_basis_(){
        this->_impl->group_basis_(); 
    }

    UniTensor group_basis() const{
        UniTensor out = this->clone();
        out.group_basis_();
        return out;
    }

 
    /**
    @brief Get an element at specific location.
	@param[in] locator the location of the element we want to access.
	@note this API is only for C++.
    */
    template <class T>
    T &at(const std::vector<cytnx_uint64> &locator){
      // std::cout << "at " << this->is_blockform()  << std::endl;
      if (this->uten_type() ==UTenType.Block){
        // [NEW] this will not check if it exists, if it is not then error will throw!
        T aux;
        return this->_impl->at_for_sparse(locator, aux);

      }else if (this->uten_type() == UTenType.Sparse) {
        if (this->_impl->elem_exists(locator)) {
          T aux;
          return this->_impl->at_for_sparse(locator, aux);
        } else {
          cytnx_error_msg(true, "[ERROR][SparseUniTensor] invalid location. break qnum block.%s",
                          "\n");
        }
      } else {
        return this->get_block_().at<T>(locator);
      }
    }

    /**
    @brief Get an element at specific location.
	@param[in] locator the location of the element we want to access.
	@note this API is only for C++.
    */
    template <class T>
    const T &at(const std::vector<cytnx_uint64> &locator) const {
      // std::cout << "at " << this->is_blockform()  << std::endl;
      if (this->uten_type() ==UTenType.Block){
        // [NEW] this will not check if it exists, if it is not then error will throw!
        T aux;
        return this->_impl->at_for_sparse(locator, aux);

      }else if (this->uten_type() == UTenType.Sparse) {
        if (this->_impl->elem_exists(locator)) {
          T aux;  // [workaround] use aux to dispatch.
          return this->_impl->at_for_sparse(locator, aux);
        } else {
          cytnx_error_msg(true, "[ERROR][SparseUniTensor] invalid location. break qnum block.%s",
                          "\n");
        }
      } else {
        return this->get_block_().at<T>(locator);
      }
    }

    const Scalar::Sproxy at(const std::vector<cytnx_uint64> &locator) const {
      if (this->uten_type() == UTenType.Block){
        return this->_impl->at_for_sparse(locator);
      }else if (this->uten_type() == UTenType.Sparse) {
        if (this->_impl->elem_exists(locator)) {
          return this->_impl->at_for_sparse(locator);
        } else {
          cytnx_error_msg(true, "[ERROR][SparseUniTensor] invalid location. break qnum block.%s",
                          "\n");
        }
      } else {
        return this->get_block_().at(locator);
      }
    }

    Scalar::Sproxy at(const std::vector<cytnx_uint64> &locator) {
      if (this->uten_type() == UTenType.Block){ 
        return this->_impl->at_for_sparse(locator);
      }else if (this->uten_type() == UTenType.Sparse) {
        if (this->_impl->elem_exists(locator)) {
          return this->_impl->at_for_sparse(locator);
        } else {
          cytnx_error_msg(true, "[ERROR][SparseUniTensor] invalid location. break qnum block.%s",
                          "\n");
        }
      } else {
        return this->get_block_().at(locator);
      }
    }

    // return a clone of block
    /**
    @brief Get the block of the UniTensor for a given index.
	@param[in] idx the index of the block we want to get
	@return Tensor
    */
    Tensor get_block(const cytnx_uint64 &idx = 0) const { return this->_impl->get_block(idx); };
    //================================
    // return a clone of block
    /**
    @brief Get the block of the UniTensor for the given quantun number.
	@param[in] qnum input the quantum number
	@param[in] force If force is true, it will return the tensor anyway (Even the 
	    corresponding block is empty, it will return void type tensor if \p force is 
		set as true. Otherwise, it will trow the exception.)
	@return Tensor
    */
    Tensor get_block(const std::vector<cytnx_int64> &qnum, const bool &force = false) const {
      return this->_impl->get_block(qnum, force);
    }

    /**
	 * @see 
     * get_block(const std::vector<cytnx_int64> &qnum, const bool &force)const
    */
    Tensor get_block(const std::initializer_list<cytnx_int64> &qnum,
                     const bool &force = false) const {
      std::vector<cytnx_int64> tmp = qnum;
      return get_block(tmp, force);
    }
    
    /**
	 * @see 
     * get_block(const std::vector<cytnx_int64> &qnum, const bool &force)const
    */
    Tensor get_block(const std::vector<cytnx_uint64> &qnum, const bool &force = false) const {
      std::vector<cytnx_int64> iqnum(qnum.begin(),qnum.end());
      return this->_impl->get_block(iqnum, force);
    }

    /**
    @brief Get the shared view of block for the given index.
	@param[in] idx input the index you want to get the corresponding block
	@return const Tensor&
	@note This function only works for non-symmetric UniTensor.
    */
    const Tensor &get_block_(const cytnx_uint64 &idx = 0) const {
      return this->_impl->get_block_(idx);
    }

    /**
    @see get_block_(const cytnx_uint64 &idx) const
	@note This function only works for non-symmetric UniTensor.
    */
    Tensor &get_block_(const cytnx_uint64 &idx = 0) { return this->_impl->get_block_(idx); }

    /**
    @brief Get the shared view of block for the given quantum number.
	@param[in] qnum input the quantum number you want to get the corresponding block
	@param[in] force If force is true, it will return the tensor anyway (Even the 
	    corresponding block is empty, it will return void type tensor if \p force is 
		set as true. Otherwise, it will trow the exception.)
	@return Tensor&
	@note This function only works for non-symmetric UniTensor.
    */
    Tensor &get_block_(const std::vector<cytnx_int64> &qnum, const bool &force = false) {
      return this->_impl->get_block_(qnum, force);
    }

    /**
    @see get_block_(const std::vector<cytnx_int64> &qnum, const bool &force)
    */
    Tensor &get_block_(const std::initializer_list<cytnx_int64> &qnum, const bool &force = false) {
      std::vector<cytnx_int64> tmp = qnum;
      return get_block_(tmp, force);
    }

    /**
    @see get_block_(const std::vector<cytnx_int64> &qnum, const bool &force)
    */
    Tensor &get_block_(const std::vector<cytnx_uint64> &qnum, const bool &force = false){
        std::vector<cytnx_int64> iqnum(qnum.begin(),qnum.end());
        return get_block_(iqnum,force);
    }
    //================================

    // this only work for non-symm tensor. return a shared view of block
    /**
    @see get_block_(const std::vector<cytnx_int64> &qnum, const bool &force)
    */
    const Tensor &get_block_(const std::vector<cytnx_int64> &qnum,
                             const bool &force = false) const {
      return this->_impl->get_block_(qnum, force);
    }

    /**
    @see get_block_(const std::vector<cytnx_int64> &qnum, const bool &force)
    */
    const Tensor &get_block_(const std::initializer_list<cytnx_int64> &qnum,
                             const bool &force = false) const {
      std::vector<cytnx_int64> tmp = qnum;
      return this->_impl->get_block_(tmp, force);
    }

    /**
    @see get_block_(const std::vector<cytnx_int64> &qnum, const bool &force)
    */
    const Tensor &get_block_(const std::vector<cytnx_uint64> &qnum, const bool &force = false) const{
        std::vector<cytnx_int64> iqnum(qnum.begin(),qnum.end());
        return get_block_(iqnum,force);
    }

    //================================
    /**
    @brief Get all the blocks of the UniTensor.
	@details get_blocks will return the blocks of the UniTensor. Furthermore, \n
   	1. For symmetric UniTensor, it will call @ref contiguous() and then return the shared 
	   view of blocks.
	2. For non-symmetric UniTensor, it will return the shared view of blocks.
	@return std::vector<Tensor>
    */
	//[dev]
    std::vector<Tensor> get_blocks() const { return this->_impl->get_blocks(); }

    /**
    @brief Get all the blocks of the UniTensor, inplacely.
	@see get_blocks()
	@param[in] silent whether need to print out the warning messages.
    */
	//[dev]
    const std::vector<Tensor> &get_blocks_(const bool &silent = false) const {
      return this->_impl->get_blocks_(silent);
    }

    /**
	@see get_blocks()_
    */
	//[dev]
    std::vector<Tensor> &get_blocks_(const bool &silent = false) {
      return this->_impl->get_blocks_(silent);
    }

    /**
    @brief Put the block into the UniTensor with given index.
	@param[in] in the block you want to put into UniTensor
	@param[in] in the index of the UniTensor you want to put the block \p in in.
	@note the put block will have shared view with the internal block, i.e. non-clone.
    */
    void put_block(const Tensor &in, const cytnx_uint64 &idx = 0) {
      this->_impl->put_block(in, idx);
    }

    /**
    @brief Put the block into the UniTensor with given quantum number.
	@param[in] in the block you want to put into UniTensor
	@param[in] in the quantum number of the UniTensor you want to put the block \p in in.
	@note the put block will have shared view with the internal block, i.e. non-clone.
    */
    void put_block(const Tensor &in, const std::vector<cytnx_int64> &qnum, const bool &force) {
      this->_impl->put_block(in, qnum, force);
    }

    /**
    @brief Put the block into the UniTensor with given index, inplacely.
	@note the put block will have shared view with the internal block, i.e. non-clone.
	@see put_block(const Tensor &in, const cytnx_uint64 &idx)
	*/
    void put_block_(Tensor &in, const cytnx_uint64 &idx = 0) { this->_impl->put_block_(in, idx); }

    /**
    @brief Put the block into the UniTensor with given quantum number, inplacely.
	@note the put block will have shared view with the internal block, i.e. non-clone.
	@see put_block(const Tensor &in, const cytnx_uint64 &idx)
	*/
    void put_block_(Tensor &in, const std::vector<cytnx_int64> &qnum, const bool &force) {
      this->_impl->put_block_(in, qnum, force);
    }
    UniTensor get(const std::vector<Accessor> &accessors) const {
      UniTensor out;
      out._impl = this->_impl->get(accessors);
      return out;
    }
    void set(const std::vector<Accessor> &accessors, const Tensor &rhs) {
      this->_impl->set(accessors, rhs);
    }

    /**
    @brief Reshape the UniTensor.
	@param[in] new_shape the new shape you want to reshape to.
	@param[in] rowrank the rowrank of the UniTensor after you reshape it.
	*/
    UniTensor reshape(const std::vector<cytnx_int64> &new_shape, const cytnx_uint64 &rowrank = 0) {
      UniTensor out;
      out._impl = this->_impl->reshape(new_shape, rowrank);
      return out;
    }

    /**
    @brief Reshape the UniTensor, inplacely.
	@see reshape(const std::vector<cytnx_int64> &new_shape, const cytnx_uint64 &rowrank)
	*/
    void reshape_(const std::vector<cytnx_int64> &new_shape, const cytnx_uint64 &rowrank = 0) {
      this->_impl->reshape_(new_shape, rowrank);
    }

    /**
    @brief Convert the UniTensor to non-diagonal form.
	@details to_dense() convert the UniTensor from diagonal form to non-diagonal structure. 
	    That means input the UniTensor with \p is_diag = true to \p is_diag = false.
	@pre 
	    1. The UniTensor need to be Dense UniTensor, that means this function is only 
		    support for UTenType.Dense.
	    2. The UniTensor need to be diagonal form (that means is_diag is true.)
	@return UniTensor
	@see to_dense_(), is_diag()
	*/
    UniTensor to_dense() {
      UniTensor out;
      out._impl = this->_impl->to_dense();
      return out;
    }

    /**
    @brief Convert the UniTensor to non-diagonal form, inplacely.
	@see to_dense(), is_diag()
	*/
    void to_dense_() { this->_impl->to_dense_(); }

    /**
     * @deprecated This function is deprecated. Please use \n
     *   combineBonds(const std::vector<std::string> &indicators, const bool &force) \n
	 *   instead.
     */
    void combineBonds(const std::vector<cytnx_int64> &indicators, const bool &force,
                      const bool &by_label) {
      this->_impl->combineBonds(indicators, force, by_label);
    }

    /**
    @brief Combine the sevral bonds of the UniTensor.
	@param[in] indicators the labels of the lags you want to combine.
	@param[in] force
	@pre 
	    1. The size of \p indicators need to >= 2.
	    2. The UniTensor cannot be diagonal form (that means is_diag is true.)
	*/
    void combineBonds(const std::vector<std::string> &indicators, const bool &force = false) {
      this->_impl->combineBonds(indicators, force);
    }

    /**
     * @deprecated This function is deprecated. Please use \n
     *   combineBonds(const std::vector<std::string> &indicators, const bool &force) \n
	 *   instead.
     */
    void combineBonds(const std::vector<cytnx_int64> &indicators, const bool &force = false) {
      this->_impl->combineBonds(indicators, force);
    }

    /**
    @brief Contract the UniTensor with common labels.
	@details This function contract the UniTensor lags with common labels. 
	@param[in] inR The UniTensor you want to contract with.
	@param[in] mv_elem_self
	@param[in] mv_elem_rhs
	@pre 
	    1. Two UniTensor need to have same UniTensor type, namely, same UTenType. 
		    You cannot contract symmetric to non-symmetric UniTensor.
	    2. You cannot contract tagged UniTensor and untagged UniTensor.
		3. For Dense diagonal UniTensor, the type of Bond (bra-ket) should match.
		4. For symmetric UniTensor (UTenType.Block), Symmetry, degeneracy, 
		    quantum numbers and Bond type should be consistent.
	@return UniTensor
	@see uten_type(), \n
	    linalg::Tensordot(const Tensor &Tl, const Tensor &Tr, 
	        	          const std::vector<cytnx_uint64> &idxl,
                          const std::vector<cytnx_uint64> &idxr, const bool &cacheL,
                          const bool &cacheR);
	*/
    UniTensor contract(const UniTensor &inR, const bool &mv_elem_self = false,
                       const bool &mv_elem_rhs = false) const {
      UniTensor out;
      out._impl = this->_impl->contract(inR._impl, mv_elem_self, mv_elem_rhs);
      return out;
    }

    /**
    @brief Get the total quantum number of the UniTensor.
	@param[in] physical
	@pre 
        The UniTensor need to be symmetric type, that is UTenType.Block.
	@return std::vector<Bond>
	@note This API just have not support.
	*/
    std::vector<Bond> getTotalQnums(const bool physical = false) const {
      return this->_impl->getTotalQnums(physical);
    }

    /**
	@note This API just have not support.
	*/
    std::vector<std::vector<cytnx_int64>> get_blocks_qnums() const {
      return this->_impl->get_blocks_qnums();
    }

    bool same_data(const UniTensor &rhs) const {
      // check same type:
      if (this->_impl->uten_type() != rhs._impl->uten_type()) return false;

      return this->_impl->same_data(rhs._impl);
    }

    /**
    @brief The addition function of the UniTensor.
	@details This is addition function of the UniTensor. Given the UniTensor
	    \f$ UT_2\f$ as the argument, it will return 
		\f[
		  UT_{self} = UT_{self} + UT_2
		\f] 
		Perform element-wise addition of two UniTensor.
	@param[in] rhs The UniTensor you want to add by.
	@return UniTensor&
	@pre 
        The two UniTensor need to have same structure.
	@note Compare to Add(const UniTensor&)const, this is an inplace function.
	@see Add_(const Scalar&), Add(const UniTensor&)const, Add(const Scalar&)const , 
	operator+=(const UniTensor&), operator+=(const Scalar&), \ref operator+
	*/
    UniTensor &Add_(const UniTensor &rhs) {
      this->_impl->Add_(rhs._impl);
      return *this;
    }

    /**
    @brief The multiplcation function of the UniTensor.
	@details This is multiplcation function of the UniTensor. Given the UniTensor
	    \f$ UT_2\f$ as the argument, it will return 
		\f[
		  UT_{self} = UT_{self} \times UT_2
		\f] 
		Perform element-wise multiplication of two UniTensor.
	@param[in] rhs The UniTensor you want to multiplcate by.
	@return UniTensor&
	@pre 
        The two UniTensor need to have same structure.
	@note Compare to Mul(const UniTensor&)const, this is an inplace function.
	@see Mul_(const Scalar&), Mul(const UniTensor&)const, Mul(const Scalar&)const , 
	operator*=(const UniTensor&), operator*=(const Scalar&), \ref operator*
	*/
    UniTensor &Mul_(const UniTensor &rhs) {
      this->_impl->Mul_(rhs._impl);
      return *this;
    }

    /**
    @brief The subtraction function of the UniTensor.
	@details This is subtraction function of the UniTensor. Given the UniTensor
	    \f$ UT_2\f$ as the argument, it will return 
		\f[
		  UT_{self} = UT_{self} - UT_2
		\f] 
		Perform element-wise subtraction of two UniTensor.
	@param[in] rhs the subtrahend
	@return UniTensor&
	@pre 
        The two UniTensor need to have same structure.
	@note Compare to Sub(const UniTensor&)const, this is an inplace function.
	@see Sub_(const Scalar&), Sub(const UniTensor&)const, Sub(const Scalar&)const , 
	operator-=(const UniTensor&), operator-=(const Scalar&), \ref operator-
	*/
    UniTensor &Sub_(const UniTensor &rhs) {
      this->_impl->Sub_(rhs._impl);
      return *this;
    }

    /**
    @brief The division function of the UniTensor.
	@details This is division function of the UniTensor. Given the UniTensor
	    \f$ UT_2\f$ as the argument, it will return 
		\f[
		  UT_{self} = UT_{self} / UT_2
		\f] 
		Perform element-wise division of two UniTensor.
	@param[in] rhs the divisor 
	@return UniTensor&
	@pre 
        The two UniTensor need to have same structure.
	@note Compare to Div(const UniTensor&)const, this is an inplace function.
	@see Div_(const Scalar&), Div(const UniTensor&)const, Div(const Scalar&)const , 
	operator/=(const UniTensor&), operator/=(const Scalar&), \ref operator/
	*/
    UniTensor &Div_(const UniTensor &rhs) {
      this->_impl->Div_(rhs._impl);
      return *this;
    }

    /**
    @brief The addition function for a given scalar.
	@details Given the Scalar \p rhs, it will perform the addition for each element 
	    in UniTensor with this Scalar \p rhs.
	@param[in] rhs a Scalar you want to add in the UniTensor.
	@return UniTensor&
	@note Compare to Add(const Scalar&)const, this is an inplace function.
	@see Add_(const UniTensor&), Add(const UniTensor&)const, Add(const Scalar&)const , 
	operator+=(const UniTensor&), operator+=(const Scalar&), \ref operator+
	*/
    UniTensor &Add_(const Scalar &rhs) {
      this->_impl->Add_(rhs);
      return *this;
    }

    /**
    @brief The multiplication function for a given scalar.
	@details Given the scalar \p rhs, it will perform the multiplication for each element 
	    in UniTensor with this scalar \p rhs.
	@param[in] rhs a scalar you want to multiplicate in the UniTensor.
	@return UniTensor&
	@note Compare to Mul(const Scalar&)const, this is an inplace function.
	@see Mul_(const UniTensor&), Mul(const UniTensor&)const, Mul(const Scalar&)const , 
	operator*=(const UniTensor&), operator*=(const Scalar&), \ref operator*
	*/
    UniTensor &Mul_(const Scalar &rhs) {
      this->_impl->Mul_(rhs);
      return *this;
    }

    /**
    @brief The subtraction function for a given scalar.
	@details Given the scalar \p rhs, it will perform the subtraction for each element 
	    in UniTensor with this scalar \p rhs.
	@param[in] rhs a scalar you want to subtract in the UniTensor.
	@return UniTensor&
	@note Compare to Sub(const Scalar&)const, this is an inplace function.
	@see Sub_(const UniTensor&), Sub(const UniTensor&)const, Sub(const Scalar&)const , 
	operator-=(const UniTensor&), operator-=(const Scalar&), \ref operator-
	*/
    UniTensor &Sub_(const Scalar &rhs) {
      this->_impl->Sub_(rhs);
      return *this;
    }

    /**
    @brief The division function for a given scalar.
	@details Given the scalar \p rhs, it will perform the division for each element 
	    in UniTensor with this scalar \p rhs.
	@param[in] rhs a scalar you want to divide in the UniTensor.
	@return UniTensor&
	@note Compare to Sub(const Scalar&)const, this is an inplace function.
	@see Div_(const UniTensor&), Div(const UniTensor&)const, Div(const Scalar&)const , 
	operator/=(const UniTensor&), operator/=(const Scalar&), \ref operator/
	*/
    UniTensor &Div_(const Scalar &rhs) {
      this->_impl->Div_(rhs);
      return *this;
    }

    /**
    @brief The addition function of the UniTensor.
	@details This is addition function of the UniTensor. Given the UniTensor
	    \f$ UT_2\f$ as the argument, it will return a new UniTensor
		\f[
		  UT = UT_{self} + UT_2
		\f] 
		Perform element-wise addition of two UniTensor.
	@param[in] rhs The UniTensor you want to add by.
	@return UniTensor
	@pre 
        The two UniTensor need to have same structure.
	@note Compare to Add_(const UniTensor&), this function will create a new UniTensor.
	@see Add_(const UniTensor&), Add_(const Scalar&), Add(const Scalar&)const , 
	operator+=(const UniTensor&), operator+=(const Scalar&), \ref operator+
	*/
    UniTensor Add(const UniTensor &rhs) const;

    /**
    @brief The addition function for a given scalar.
	@details Given the scalar \p rhs, it will perform the addition for each element 
	    in UniTensor with this scalar \p rhs.
	@param[in] rhs a scalar you want to add in the UniTensor.
	@return UniTensor
	@note Compare to Add_(const Scalar&), this function will create a new UniTensor.
	@see Add_(const Scalar&), Add_(const UniTensor&), Add(const UniTensor&)const, 
	operator+=(const UniTensor&), operator+=(const Scalar&), \ref operator+
	*/
    UniTensor Add(const Scalar &rhs) const;

    /**
    @brief The multiplication function of the UniTensor.
	@details This is multiplication function of the UniTensor. Given the UniTensor
	    \f$ UT_2\f$ as the argument, it will return a new UniTensor
		\f[
		  UT = UT_{self} \times UT_2
		\f] 
		Perform element-wise multiplcation of two UniTensor.
	@param[in] rhs The UniTensor you want to multiplicate by.
	@return UniTensor
	@pre 
        The two UniTensor need to have same structure.
	@note Compare to Mul_(const UniTensor&), this function will create a new UniTensor.
	@see Mul_(const UniTensor&), Mul_(const Scalar&), Mul(const Scalar&)const , 
	operator*=(const UniTensor&), operator*=(const Scalar&), \ref operator*
	*/
    UniTensor Mul(const UniTensor &rhs) const;

    /**
    @brief The multiplication function for a given scalar.
	@details Given the scalar \p rhs, it will perform the multiplication for each element 
	    in UniTensor with this scalar \p rhs.
	@param[in] rhs a scalar you want to multiply in the UniTensor.
	@return UniTensor
	@note Compare to Mul_(const Scalar&), this function will create a new UniTensor.
	@see Mul_(const Scalar&), Mul_(const UniTensor&), Mul(const UniTensor&)const, 
	operator*=(const UniTensor&), operator*=(const Scalar&), \ref operator*
	*/
    UniTensor Mul(const Scalar &rhs) const;

    /**
    @brief The division function of the UniTensor.
	@details This is division function of the UniTensor. Given the UniTensor
	    \f$ UT_2\f$ as the argument, it will return a new UniTensor
		\f[
		  UT = UT_{self} / UT_2
		\f] 
		Perform element-wise division of two UniTensor.
	@param[in] rhs the divisor
	@return UniTensor
	@pre 
        The two UniTensor need to have same structure.
	@note Compare to Div_(const UniTensor&), this function will create a new UniTensor.
	@see Div_(const UniTensor&), Div_(const Scalar&), Div(const Scalar&)const , 
	operator/=(const UniTensor&), operator/=(const Scalar&), \ref operator/
	*/
    UniTensor Div(const UniTensor &rhs) const;

    /**
    @brief The division function for a given scalar.
	@details Given the scalar \p rhs, it will perform the division for each element 
	    in UniTensor with this scalar \p rhs.
	@param[in] rhs a scalar you want to divide in the UniTensor.
	@return UniTensor
	@note Compare to Div_(const Scalar&), this function will create a new UniTensor.
	@see Div_(const Scalar&), Div_(const UniTensor&), Div(const UniTensor&)const, 
	operator/=(const UniTensor&), operator/=(const Scalar&), \ref operator/
	*/
    UniTensor Div(const Scalar &rhs) const;

    /**
    @brief The subtraction function of the UniTensor.
	@details This is subtraction function of the UniTensor. Given the UniTensor
	    \f$ UT_2\f$ as the argument, it will return a new UniTensor
		\f[
		  UT = UT_{self} - UT_2
		\f] 
		Perform element-wise subtraction of two UniTensor.
	@param[in] rhs the subtrahend
	@return UniTensor
	@pre 
        The two UniTensor need to have same structure.
	@note Compare to Sub_(const UniTensor&), this function will create a new UniTensor.
	@see Sub_(const UniTensor&), Sub_(const Scalar&), Sub(const Scalar&)const , 
	operator-=(const UniTensor&), operator-=(const Scalar&), \ref operator-
	*/
    UniTensor Sub(const UniTensor &rhs) const;

    /**
    @brief The subtraction function for a given scalar.
	@details Given the scalar \p rhs, it will perform the subtraction for each element 
	    in UniTensor with this scalar \p rhs.
	@param[in] rhs the subtrahend
	@return UniTensor
	@note Compare to Sub_(const Scalar&), this function will create a new UniTensor.
	@see Sub_(const Scalar&), Sub_(const UniTensor&), Sub(const UniTensor&)const, 
	operator-=(const UniTensor&), operator-=(const Scalar&), \ref operator-
	*/
    UniTensor Sub(const Scalar &rhs) const;

    /**
    @brief Return the norm of the UniTensor.
	@details Norm() return the 2-norm of the UniTensor \f$UT\f$. Namely, it return
	\f[
	||UT||_2
	\f]
	@return Tensor
	*/
    Tensor Norm() const { return this->_impl->Norm(); };

    /**
    @brief The addition assignment operator of the UniTensor.
	@details This is addition assignment operator of the UniTensor. It will perform 
	    element-wise addition and return 
		\f[
		UT += UT_R
		\f]
	@param[in] rhs The UniTensor you want to add by.
	@return UniTensor&
	@pre 
        The two UniTensor need to have same structure.
	@see 
	operator+=(const Scalar&), \ref operator+, Add_(const UniTensor&), 
	Add_(const Scalar&), Add(const UniTensor&),	Add(const Scalar&)const
	*/
    UniTensor &operator+=(const UniTensor &rhs) {
      this->Add_(rhs);
      return *this;
    }

    /**
    @brief The subtraction assignment operator of the UniTensor.
	@details This is subtraction assignment operator of the UniTensor. It will perform 
	    element-wise subtraction and return 
		\f[
		UT -= UT_R
		\f]
	@param[in] rhs the subtrahend
	@return UniTensor&
	@pre 
        The two UniTensor need to have same structure.
	@see 
	operator-=(const Scalar&), \ref operator-, Sub_(const UniTensor&), 
	Sub_(const Scalar&), Sub(const UniTensor&),	Sub(const Scalar&)const
	*/
    UniTensor &operator-=(const UniTensor &rhs) {
      this->Sub_(rhs);
      return *this;
    }

    /**
    @brief The division assignment operator of the UniTensor.
	@details This is division assignment operator of the UniTensor. It will perform 
	    element-wise division and return 
		\f[
		UT /= UT_R
		\f]
	@param[in] rhs the divisor
	@return UniTensor&
	@pre 
        The two UniTensor need to have same structure.
	@see 
	operator/=(const Scalar&), \ref operator/, Div_(const UniTensor&), 
	Div_(const Scalar&), Div(const UniTensor&),	Div(const Scalar&)const
	*/
    UniTensor &operator/=(const UniTensor &rhs) {
      this->Div_(rhs);
      return *this;
    }

    /**
    @brief The multiplication assignment operator of the UniTensor.
	@details This is multiplication assignment operator of the UniTensor. It will perform 
	    element-wise multiplication and return 
		\f[
		UT *= UT_R
		\f]
	@param[in] rhs The UniTensor you want to multilicate by.
	@return UniTensor&
	@pre 
        The two UniTensor need to have same structure.
	@see 
	operator*=(const Scalar&), \ref operator*, Mul_(const UniTensor&), 
	Mul_(const Scalar&), Mul(const UniTensor&),	Mul(const Scalar&)const
	*/
    UniTensor &operator*=(const UniTensor &rhs) {
      this->Mul_(rhs);
      return *this;
    }

    /**
    @brief The addition assignment operator for a given scalar.
	@details Given the scalar \p rhs, it will perform the addition for each element 
	    in UniTensor with this scalar \p rhs.
	@param[in] rhs a scalar you want to add in the UniTensor.
	@return UniTensor&
	@see 
	operator+=(const UniTensor&), \ref operator+, Add_(const UniTensor&), 
	Add_(const Scalar&), Add(const UniTensor&),	Add(const Scalar&)const
	*/
    UniTensor &operator+=(const Scalar &rhs) {
      this->Add_(rhs);
      return *this;
    }

    /**
    @brief The subtraction assignment operator for a given scalar.
	@details Given the scalar \p rhs, it will perform the subtraction for each element 
	    in UniTensor with this scalar \p rhs.
	@param[in] rhs the subtrahend
	@return UniTensor&
	@see 
	operator-=(const UniTensor&), \ref operator-, Sub_(const UniTensor&), 
	Sub_(const Scalar&), Sub(const UniTensor&),	Sub(const Scalar&)const
	*/
    UniTensor &operator-=(const Scalar &rhs) {
      this->Sub_(rhs);
      return *this;
    }

    /**
    @brief The division assignment operator for a given scalar.
	@details Given the scalar \p rhs, it will perform the division for each element 
	    in UniTensor with this scalar \p rhs.
	@param[in] rhs the divisor
	@return UniTensor&
	@see 
	operator/=(const UniTensor&), \ref operator/, Div_(const UniTensor&), 
	Div_(const Scalar&), Div(const UniTensor&),	Div(const Scalar&)const
	*/
    UniTensor &operator/=(const Scalar &rhs) {
      this->Div_(rhs);
      return *this;
    }

    /**
    @brief The multiplication assignment operator for a given scalar.
	@details Given the scalar \p rhs, it will perform the multiplication for each element 
	    in UniTensor with this scalar \p rhs.
	@param[in] rhs a scalar you want to multiply in the UniTensor.
	@return UniTensor&
	@see 
	operator*=(const Scalar&), \ref operator*, Mul_(const UniTensor&), 
	Mul_(const Scalar&), Mul(const UniTensor&),	Mul(const Scalar&)const
	*/
    UniTensor &operator*=(const Scalar &rhs) {
      this->Mul_(rhs);
      return *this;
    }

    /**
    @brief Apply complex conjugate on each entry of the UniTensor.
	@details Conj() apply complex conjugate on each entry of the UniTensor.
	@return UniTensor
    @note Compare to Conj_(), this fucntion will create a new object UniTensor.
	@see Conj_()
	*/
    UniTensor Conj() {
      UniTensor out;
      out._impl = this->_impl->Conj();
      return out;
    }

    /**
    @brief Apply complex conjugate on each entry of the UniTensor.
	@details Conj_() apply complex conjugate on each entry of the UniTensor, inplacely.
	@return UniTensor
    @note Compare to Conj(), this fucntion is inplace function.
	@see Conj()
	*/
    UniTensor &Conj_() {
      this->_impl->Conj_();
      return *this;
    }

    /**
    @brief Take the transpose of the UniTensor.
	@return UniTensor
    @note Compare to Transpose_(), this fucntion will return new UniTensor object.
	@see Transpose_()
	*/
    UniTensor Transpose() const {
      UniTensor out;
      out._impl = this->_impl->Transpose();
      return out;
    }

    /**
    @brief Take the transpose of the UniTensor, inplacely.
	@return UniTensor
    @note Compare to Transpose(), this fucntion is inplace function.
	@see Transpose()
	*/
    UniTensor &Transpose_() {
      this->_impl->Transpose_();
      return *this;
    }

    /**
    @brief normalize the current UniTensor instance with 2-norm.
	@return UniTensor
    @note Compare to normalize_(), this fucntion will return new UniTensor object.
	@see normalize_()
	*/
    UniTensor normalize() const {
      UniTensor out;
      out._impl = this->_impl->normalize();
      return out;
    }

    /**
    @brief normalize the UniTensor, inplacely.
	@return UniTensor
    @note Compare to normalize(), this fucntion is inplace function.
	@see normalize()
	*/
    UniTensor &normalize_() {
      this->_impl->normalize_();
      return *this;
    }


    /**
    @brief Take the partial trance to the UniTensor.
	@details Take the partial trace to the UniTensor with the give two labels.
	@param[in] a label 1
	@param[in] b label 2
	@return UniTensor
    @note Compare to Trace_(), this fucntion will return a new UniTensor object.
	@see Trace_()
	*/
    UniTensor Trace(const std::string &a, const std::string &b) const {
      UniTensor out;
      out._impl = this->_impl->Trace(a, b);
      return out;
    }

    /**
    @brief Take the partial trance to the UniTensor.
	@details Take the partial trace to the UniTensor with the give two labels.
	@param[in] a label 1
	@param[in] b label 2
	@return UniTensor
    @note Compare to Trace_(), this fucntion will return a new UniTensor object.
	@see Trace_()
	*/
    UniTensor Trace(const cytnx_int64 &a = 0, const cytnx_int64 &b = 1) const {
      UniTensor out;
      out._impl = this->_impl->Trace(a, b);
      return out;
    }

    /**
     * @deprecated
	 * This is deprecated function, please use \n
     * Trace(const std::string &a, const std::string &b) const\n
	 * instead.
     */
    UniTensor Trace(const cytnx_int64 &a, const cytnx_int64 &b, const bool &by_label) const {
      if(by_label){
        return Trace(std::to_string(a),std::to_string(b));
      }else{
        return Trace(a, b);
      }
    }

    /**
    @brief Take the partial trance to the UniTensor, inplacely.
	@details Take the partial trace to the UniTensor with the give two labels.
	@param[in] a label 1
	@param[in] b label 2
	@return UniTensor&
    @note Compare to Trace(), this is an inplace function.
	@see Trace()
	*/
    UniTensor &Trace_(const std::string &a, const std::string &b) {
      this->_impl->Trace_(a, b);
      if(this->uten_type()==UTenType.Block){
        // handle if no leg left case for BlockUniTensor.
        if(this->rank()==0){
            DenseUniTensor *tmp = new DenseUniTensor();
            tmp->_block = this->get_blocks_(true)[0];
            this->_impl = boost::intrusive_ptr<UniTensor_base>(tmp);
        }
      }
      return *this;
    }

    /**
    @brief Take the partial trance to the UniTensor, inplacely.
	@details Take the partial trace to the UniTensor with the give two labels.
	@param[in] a label 1
	@param[in] b label 2
	@return UniTensor&
    @note Compare to Trace(), this is an inplace function.
	@see Trace()
	*/
    UniTensor &Trace_(const cytnx_int64 &a = 0, const cytnx_int64 &b = 1) {
      this->_impl->Trace_(a, b);
      if(this->uten_type()==UTenType.Block){
        // handle if no leg left case for BlockUniTensor.
        if(this->rank()==0){
            DenseUniTensor *tmp = new DenseUniTensor();
            tmp->_block = this->get_blocks_(true)[0];
            this->_impl = boost::intrusive_ptr<UniTensor_base>(tmp);
        }
      }
      return *this;
    }

    /**
     * @deprecated
	 * This function is deprecated, please use \n
     * &Trace_(const std::string &a, const std::string &b)\n
	 * instread.
     */
    UniTensor &Trace_(const cytnx_int64 &a, const cytnx_int64 &b, const bool &by_label) {
      this->_impl->Trace_(a, b, by_label);
      if(this->uten_type()==UTenType.Block){
        // handle if no leg left case for BlockUniTensor.
        if(this->rank()==0){
            DenseUniTensor *tmp = new DenseUniTensor();
            tmp->_block = this->get_blocks_(true)[0];
            this->_impl = boost::intrusive_ptr<UniTensor_base>(tmp);
        }
      }
      return *this;
    }

    /**
    @brief Take the conjugate transpose to the UniTensor.
	@return UniTensor
    @note Compare to Dagger_(), this function will create a new UniTensor ojbect.
	@see Dagger_()
	*/
    UniTensor Dagger() const {
      UniTensor out;
      out._impl = this->_impl->Dagger();
      return out;
    }

    /**
    @brief Take the conjugate transpose to the UniTensor, inplacely.
	@return UniTensor&
    @note Compare to Dagger(), this is an inplace function.
	@see Dagger()
	*/
    UniTensor &Dagger_() {
      this->_impl->Dagger_();
      return *this;
    }

    UniTensor &tag() {
      this->_impl->tag();
      return *this;
    }

    /**
    @brief Power function.
	@details Take power \p p on all the elements in the UniTensor.
	@param p power
	@return UniTensor
    @note Compare to Pow_(), this function will create a new UniTensor ojbect.
	@see Pow_()
	*/
    UniTensor Pow(const double &p) const;

    /**
    @brief Power function.
	@details Take power \p p on all the elements in the UniTensor, inplacely.
	@param p power
	@return UniTensor&
    @note Compare to Pow(), this function is an inplacely function.
	@see Pow()
	*/
    UniTensor &Pow_(const double &p);

    bool elem_exists(const std::vector<cytnx_uint64> &locator) const {
      return this->_impl->elem_exists(locator);
    }

    /**
     * @deprecated
	 * This function is deprecated, please use at() instread.
	 * @note C++: Deprecated soon, use at()
     */
    template <class T>
    T get_elem(const std::vector<cytnx_uint64> &locator) const {
      return this->at<T>(locator);
    }

    /**
     * @deprecated
	 * This function is deprecated, please use at() instread.
	 * @note C++: Deprecated soon, use at()
     */
    template <class T2>
    void set_elem(const std::vector<cytnx_uint64> &locator, const T2 &rc) {
      // cytnx_error_msg(true,"[ERROR] invalid type%s","\n");
      this->at(locator) = rc;
    }

    /**
    @brief save a UniTensor to file
    @param[in] fname the file name (exclude the file extension).
    @post The file extension will be extended as '.cytnx'
    */
    void Save(const std::string &fname) const;

    /**
    @brief save a UniTensor to file
    @param[in] fname the file name (exclude the file extension).
    @post The file extension will be extended as '.cytnx'
    */
    void Save(const char *fname) const;

    /**
    @brief load a UniTensor from file
    @param[in] fname the file name
    @return the loaded UniTensor
    @warning This is static function, if called through UniTensor object, 
    it will return a new UniTensor object instead of modifying the current one.
    */
    static UniTensor Load(const std::string &fname);

    /**
    @brief load a UniTensor from file
    @param fname: the file name
    @return the loaded UniTensor
    @warning This is static function, if called through UniTensor object, 
        it will return a new UniTensor object instead of modifying the current one.
    */
    static UniTensor Load(const char *fname);

    /**
     * @brief
     *
     * @deprecated
     *
     * @param bond_idx
     * @param dim
     * @param by_label
     * @return UniTensor&
     */
    UniTensor &truncate_(const cytnx_int64 &bond_idx, const cytnx_uint64 &dim,
                         const bool &by_label) {
      this->_impl->truncate_(bond_idx, dim, by_label);
      return *this;
    }
    UniTensor &truncate_(const std::string &bond_idx, const cytnx_uint64 &dim) {
      this->_impl->truncate_(bond_idx, dim);
      return *this;
    }
    UniTensor &truncate_(const cytnx_int64 &bond_idx, const cytnx_uint64 &dim) {
      this->_impl->truncate_(bond_idx, dim);
      return *this;
    }
    /**
     * @brief
     *
     * @deprecated
     *
     * @param bond_idx
     * @param dim
     * @param by_label
     * @return UniTensor
     */
    UniTensor truncate(const cytnx_int64 &bond_idx, const cytnx_uint64 &dim,
                       const bool &by_label) const {
      UniTensor out = this->clone();
      out.truncate_(bond_idx, dim, by_label);
      return out;
    }
    UniTensor truncate(const std::string &bond_idx, const cytnx_uint64 &dim) const {
      UniTensor out = this->clone();
      out.truncate_(bond_idx, dim);
      return out;
    }
    UniTensor truncate(const cytnx_int64 &bond_idx, const cytnx_uint64 &dim) const {
      UniTensor out = this->clone();
      out.truncate_(bond_idx, dim);
      return out;
    }

    /**
    @brief get the q-indices on each leg for the [bidx]-th block 
    @param bidx the bidx-th block in current block list. 
    @return 
        [vector]

    */
    const std::vector<cytnx_uint64>& get_qindices(const cytnx_uint64 &bidx) const{
        return this->_impl->get_qindices(bidx);
    }
    /**
    @brief get the q-indices on each leg for the [bidx]-th block 
    @param bidx the bidx-th block in current block list. 
    @return 
        [vector]

    */
    std::vector<cytnx_uint64>& get_qindices(const cytnx_uint64 &bidx){
        return this->_impl->get_qindices(bidx);
    }

    

    /**
    @brief get the q-indices on each leg for all the blocks 
    @return 
        [2d vector]

    */
    const vec2d<cytnx_uint64> & get_itoi() const{
        return this->_impl->get_itoi();
    }
    vec2d<cytnx_uint64> & get_itoi(){
        return this->_impl->get_itoi();
    }



    /// @cond
    void _Load(std::fstream &f);
    void _Save(std::fstream &f) const;
    /// @endcond

  };  // class UniTensor

  ///@cond
  std::ostream &operator<<(std::ostream &os, const UniTensor &in);
  ///@endcond

  /**
  @brief Contract two UniTensor by tracing the ranks with common labels.
  @param inL the Tensor #1
  @param inR the Tensor #2
  @param cacheL if the inL should be contiguous align after calling
  @param cacheR if the inR should be contiguous align after calling
  @return
      [UniTensor]

  @see cytnx::UniTensor::contract

  */
  UniTensor Contract(const UniTensor &inL, const UniTensor &inR, const bool &cacheL = false,
                     const bool &cacheR = false);

  /**
  @brief Contract multiple UniTensor by tracing the ranks with common labels with pairwise
  operation.
  @param TNs the Tensors.
  @param order desired contraction order.
  @param optimal wheather to find the optimal contraction order automatically.
  @return
      [UniTensor]

  See also \link cytnx::UniTensor::contract UniTensor.contract \endlink

  */
  UniTensor Contracts(const std::vector<UniTensor> &TNs, const std::string &order, const bool &optimal);

  /// @cond
  void _resolve_CT(std::vector<UniTensor> &TNlist);
  template <class... T>
  void _resolve_CT(std::vector<UniTensor> &TNlist, const UniTensor &in, const T &...args) {
    TNlist.push_back(in);
    _resolve_CT(TNlist, args...);
  }
  /// @endcond

  /**
  @brief Contract multiple UniTensor by tracing the ranks with common labels with pairwise
  operation.
  @param in the Tensors.
  @param args the Tensors.
  @return
      [UniTensor]

  See also \link cytnx::UniTensor::contract UniTensor.contract \endlink

  */
  template <class... T>
  UniTensor Contracts(const UniTensor &in, const T &...args, const std::string &order, const bool &optimal) {
    std::vector<UniTensor> TNlist;
    _resolve_CT(TNlist, in, args...);
    return Contracts(TNlist, order, optimal);
  }

}  // namespace cytnx
#endif
