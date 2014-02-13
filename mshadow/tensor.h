#ifndef MSHADOW_TENSOR_H
#define MSHADOW_TENSOR_H
/*!
 * \file tensor.h
 * \brief header file of tensor data structure and functions
 *        covention: this lib requires explicit memory allocation and de-allocation
 *                   all the data structure CTensor1D, GTensor1D are like handles(pointers),
 *                   no memory allocation is happening during calculation
 * \author Bing Hsu, Tianqi Chen
 */
#include "tensor_base.h"
#include "tensor_expr.h"

namespace mshadow {
    /*!
     * \brief shape of a tensor
     *       IMPORTANT NOTE: this shape is different from numpy.shape
     *       shape[0] gives the lowest dimension, shape[dimension-1] gives the highest dimension
     *       shape[k] corresponds to k-th dimension of tensor
     * \tparam dimension dimension of tensor
     */
    template<int dimension>
    struct Shape {
    public:
        /*! \brief maximum dimension of tensor */
        const static int zMaxShape = dimension;
        const static int zSubShape = dimension - 1;
    public:
        /*! \brief default constructor, do nothing */
        MSHADOW_XINLINE Shape(void) {}
        MSHADOW_XINLINE Shape( const Shape<dimension> &s ){
            #pragma unroll
            for( int i = 0; i < zMaxShape; i ++ ){
                this->shape_[i] = s[i];
            }
            this->stride_ = s.stride_;
        }
        /*!
         * \brief get corresponding index
         * \param idx dimension index
         * \return the corresponding dimension size
         */
        MSHADOW_XINLINE index_t& operator[](index_t idx) {
            return shape_[ idx ];
        }
        /*!
         * \brief get corresponding index
         * \param idx dimension index
         * \return the corresponding dimension size
         */
        MSHADOW_XINLINE const index_t& operator[](index_t idx) const {
            return shape_[ idx ];
        }
        /*! \return stride */
        MSHADOW_XINLINE const index_t& stride(void) const {
            return stride_;
        }
        /*! \return whether two shape equals */
        MSHADOW_XINLINE bool operator==(const Shape<zMaxShape> &s) const {
            #pragma unroll
            for (int i = 0; i < zMaxShape; i++) {
                if (s.shape_[i] != this->shape_[i]) return false;
            }
            return true;
        }
        /*!
         * flatten the higher dimension to second dimension, return a 2D shape
         * \return the flat 2d shape
         */
        MSHADOW_XINLINE Shape<2> FlatTo2D(void) const {
            Shape<2> s;
            s.stride_ = this->stride_;
            s.shape_[ 0 ] = this->shape_[ 0 ];
            index_t ymax = 1;

            #pragma unroll
            for (int i = 1; i < zMaxShape; ++i) {
                ymax *= this->shape_[ i ];
            }
            s.shape_[1] = ymax;
            return s;
        }
        /*! \return number of valid elements */
        MSHADOW_XINLINE size_t Size(void) {
            size_t memsz = this->shape_[ 0 ];
            #pragma unroll
            for (int i = 1; i < zMaxShape; ++i) {
                memsz *= this->shape_[ i ];
            }
            return memsz;
        }
        /*! \return memory size, including the aligned x dimension */
        MSHADOW_XINLINE size_t MSize(void) const {
            size_t memsz = this->stride_;
            #pragma unroll
            for (int i = 1; i < zMaxShape; ++i) {
                memsz *= this->shape_[ i ];
            }
            return memsz;
        }
        /*!
         * \brief get subshape
         * \return subshape
         */
        MSHADOW_XINLINE Shape<zSubShape> SubShape(void) const {
            Shape<zSubShape> s;
            s.stride_ = this->stride_;
            // for cuda
            #pragma unroll
            for (int i = 0; i < zSubShape; ++i) {
                s.shape_[ i ] = this->shape_[ i ];
            }
            return s;
        }

    public:
        /*! \brief storing the dimension information */
        index_t shape_[ zMaxShape ];
        /*!
         * \brief storing the stride information in x dimension
         *    this is used to deal with pitch allocation in gpu or sse(align x dimension to 64bit) for efficiency
         */
        index_t stride_;
    };    
    // useful construction functions to generate shape    
    /*! 
     * \brief construct a one dimension shape 
     * \param s0 size of dimension 0
     * \return the shape construction
     */
    MSHADOW_XINLINE Shape<1> Shape1( index_t s0 ){
        Shape<1> s; s[0] = s0;
        return s;
    }
    /*! 
     * \brief construct a two dimension shape 
     * \param s1 size of dimension 1
     * \param s0 size of dimension 0
     * \return the shape construction
     */
    MSHADOW_XINLINE Shape<2> Shape2( index_t s1, index_t s0 ){
        Shape<2> s; s[0] = s0; s[1] = s1;
        return s;
    }
    /*! 
     * \brief construct a three dimension shape 
     * \param s2 size of dimension 2
     * \param s1 size of dimension 1
     * \param s0 size of dimension 0
     * \return the shape construction
     */
    MSHADOW_XINLINE Shape<3> Shape3( index_t s2, index_t s1, index_t s0 ){
        Shape<3> s; s[0] = s0; s[1] = s1; s[2] = s2;
        return s;
    }
    /*! 
     * \brief construct a four dimension shape 
     * \param s3 size of dimension 3
     * \param s2 size of dimension 2
     * \param s1 size of dimension 1
     * \param s0 size of dimension 0
     * \return the shape construction
     */
    MSHADOW_XINLINE Shape<4> Shape4( index_t s3, index_t s2, index_t s1, index_t s0 ){
        Shape<4> s; s[0] = s0; s[1] = s1; s[2] = s2; s[3] = s3;
        return s;
    }
}; // namespace mshadow

namespace mshadow {
    // simple device name
    struct cpu {
        const static bool kDevCPU = true;
    };
    struct gpu {
        const static bool kDevCPU = false;
    };

    // more compact template
    /*!
     * \brief general tensor
     * \tparam Device which device the tensor is on
     * \tparam dimension dimension of the tensor
     */
    template<typename Device, int dimension>
    struct Tensor: public expr::ContainerExp< Tensor<Device,dimension> >{
    public:
        /*! \brief whether current type lies in cpu */
        const static bool kDevCPU = Device::kDevCPU;
        /*! \brief dimension of subtype */
        const static int  kSubdim = dimension - 1;

    public:
        /*! \brief pointer to the data */
        real_t *dptr;
        /*! \brief shape of the tensor */
        Shape<dimension> shape;
    public:
        /*! \brief default constructor */
        MSHADOW_XINLINE Tensor(void) {}
        /*! \brief constructor from shape  */
        MSHADOW_XINLINE Tensor(const Shape<dimension> &shape): shape(shape) {}
        /*! \brief constructor from data pointer and shape  */
        MSHADOW_XINLINE Tensor(real_t *dptr, const Shape<dimension> &shape): dptr((real_t*)dptr), shape(shape) {}
        /*!
         * \brief flatten the tensor to 2 dimension, collapse the higher dimensions together
         * \return tensor after flatten
         */
        MSHADOW_XINLINE Tensor<Device, 2> FlatTo2D(void) const {
            return Tensor<Device, 2>(reinterpret_cast<real_t*> \
                                     (dptr), shape.FlatTo2D());
        }
        /*!
         * \brief get a element of dimension - 1
         * \param idx index
         * \return the result tensor
         */
        MSHADOW_XINLINE Tensor<Device, kSubdim> operator[](index_t idx) const {
            Shape<kSubdim> s = shape.SubShape();
            return Tensor<Device, kSubdim>(reinterpret_cast<real_t*> \
                                           (dptr) + s.MSize() * idx, s);
        }
        /*!
         * \brief slice the tensor
         * \return tensor after slice
         */
        MSHADOW_XINLINE Tensor<Device, dimension> Slice(index_t begin, index_t end) const {
            Shape<dimension> s = this->shape;
            s[ dimension - 1 ] = end - begin;
            return Tensor<Device, dimension>(reinterpret_cast<real_t*>\
                                             (dptr) + s.SubShape().MSize() * begin, s);
        }
    public:
        // functions to fit expression template
        inline Tensor<Device,dimension>& operator=( double s ){
            return this->__assign( s );
        }
        template<typename E>
        inline Tensor<Device,dimension>& operator=( const expr::Exp<E,expr::type::kMapper> &exp ){
            return this->__assign( exp );
        }
        template<typename E>
        inline Tensor<Device,dimension>& operator=( const expr::Exp<E,expr::type::kComplex> &exp ){
            return this->__assign( exp );
        }
    };

    /*!
     * \brief respecialized class Tensor1D,thei is due to different implementation in operator[]
     * \tparam Device device type
     */
    template<typename Device>
    struct Tensor<Device, 1>: public expr::ContainerExp< Tensor<Device,1> >{
    public:
        real_t *dptr;
        Shape<1> shape;
    public:
        MSHADOW_XINLINE Tensor(void) {}
        MSHADOW_XINLINE Tensor(real_t *dptr, Shape<1> shape) :dptr(dptr), shape(shape) {}
        MSHADOW_XINLINE Tensor<Device, 2> FlatTo2D(void) const {
            return Tensor<Device, 2>(reinterpret_cast<real_t*> \
                                     (dptr), shape.FlatTo2D());
        }
        MSHADOW_XINLINE Tensor<Device, 1> Slice(index_t begin, index_t end) const {
            Shape<1> s;
            s[0] = s.stride_ = end  - begin;
            return Tensor<Device, 1>(reinterpret_cast<real_t*> \
                                     (dptr) + begin, s);
        }
        MSHADOW_XINLINE real_t &operator[](index_t idx) { return dptr[ idx ]; }
        MSHADOW_XINLINE const real_t &operator[](index_t idx)const { return dptr[ idx ]; }
    public:
        // functions to fit expression template
        inline Tensor<Device,1>& operator=( double s ){
            return this->__assign( s );
        }
        template<typename E>
        inline Tensor<Device,1>& operator=( const expr::Exp<E,expr::type::kMapper> &exp ){
            return this->__assign( exp );
        }
        template<typename E>
        inline Tensor<Device,1>& operator=( const expr::Exp<E,expr::type::kComplex> &exp ){
            return this->__assign( exp );
        }
    };
}; // namespace mshadow

namespace mshadow {
    typedef Tensor<cpu, 1> CTensor1D;
    typedef Tensor<cpu, 2> CTensor2D;
    typedef Tensor<cpu, 3> CTensor3D;
    typedef Tensor<cpu, 4> CTensor4D;

    typedef Tensor<gpu, 1> GTensor1D;
    typedef Tensor<gpu, 2> GTensor2D;
    typedef Tensor<gpu, 3> GTensor3D;
    typedef Tensor<gpu, 4> GTensor4D;
}; // namespace mshadow


// add unroll loops for the shape
namespace mshadow {
    // function declarations
    /*!
     * \brief CPU/CPU: allocate space for CTensor, according to the shape in the obj
     *        this function is responsible to set the stride_ in each obj.shape
     * \tparam dim specify the dim of tensor
     * \param obj the tensor object, with shape specified
     */
    template<int dim>
    inline void AllocSpace(Tensor<cpu,dim> &obj);
    template<int dim>
    inline void AllocSpace(Tensor<gpu,dim> &obj);

    /*!
     * \brief CPU/GPU: free the space of tensor
     * \tparam dim specify the dim of tensor
     * \param obj the tensor object
     */
    template<int dim>
    inline void FreeSpace(Tensor<cpu,dim> &obj);
    template<int dim>
    inline void FreeSpace(Tensor<gpu,dim> &obj);

    /*!
     * \brief copy data from one tensor to another, with same shape
     * \tparam dim specify the dim of tensor
     * \param obj the tensor object, with shape specified
     */
    template<int dim>
    inline void Copy(Tensor<cpu,dim> dst, const Tensor<cpu,dim> &src );
    template<int dim>
    inline void Copy(Tensor<cpu,dim> dst, const Tensor<gpu,dim> &src );
    template<int dim>
    inline void Copy(Tensor<gpu,dim> dst, const Tensor<cpu,dim> &src );
    template<int dim>
    inline void Copy(Tensor<gpu,dim> dst, const Tensor<gpu,dim> &src );
    
    namespace expr{
        /*!
         * \brief execution plan hat can be used to carry out calculation for specific expression
         * \tparam ExpType type of expression
         */
        template<typename ExpType>
        class Plan;        
    };    
    /*!
     * \brief CPU/GPU: map a expression plan to a tensor
     * \tparam Saver specify storage method [st]
     * \tparam E specifies the expression type, not need to specify this parameter during usage
     * \tparam dim dim of the tensor, during usage, there is no need to specify this parameter
     * \param dst destination
     * \param plan expression plan of expression
     * \sa namespace mshadow:sv, mshadow::op, mshadow::expr
     */
    template<typename Saver, typename E, int dim>
    inline void MapPlan(Tensor<cpu,dim> dst, const expr::Plan<E> &plan );
    template<typename Saver, typename E, int dim>
    inline void MapPlan(Tensor<gpu,dim> dst, const expr::Plan<E> &plan );

    /*!
     * \brief CPU/GPU: map a expression to a tensor, this function calls MapPlan
     * \tparam Saver specify storage method [st]
     * \tparam Device cpu or gpu
     * \tparam dim dim of the tensor, during usage, there is no need to specify this parameter
     * \tparam E specifies the expression type, not need to specify this parameter during usage
     * \param dst destination
     * \param exp expression
     * \sa namespace mshadow:sv, mshadow::op, mshadow::expr
     */
    template<typename Saver, typename Device, int dim, typename E, int etype>
    inline void MapExp(Tensor<Device,dim> dst, const expr::Exp<E,etype> &exp );

}; // namespace mshadow

namespace mshadow{
    // the following function is name dependent, have different name for CPU and GPU
    /*!
     * \brief CPU: short cut to allocate and initialize a CTensor
     * \tparam dim specify the dim of tensor
     * \param shape shape of the tensor
     * \return the allocated tensor
     */
    template<int dim>
    inline Tensor<cpu,dim> NewCTensor(const Shape<dim> &shape, real_t initv);

    /*!
     * \brief GPU: short cut to allocate and initialize a GTensor
     * \tparam dim specify the dim of tensor
     * \param shape shape of the tensor
     * \return the allocated tensor
     */
    template<int dim>
    inline Tensor<gpu,dim> NewGTensor(const Shape<dim> &shape, real_t initv);
    
}; // namespace mshadow

#include "tensor_cpu-inl.hpp"
#include "tensor_gpu-inl.hpp"
#include "tensor_expr_engine-inl.hpp"

#endif // TENSOR_H