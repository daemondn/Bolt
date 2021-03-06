/***************************************************************************
*   Copyright 2012 Advanced Micro Devices, Inc.
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*       http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.

***************************************************************************/

#if !defined( BOLT_BTBB_SCAN_BY_KEY_INL )
#define BOLT_BTBB_SCAN_BY_KEY_INL
#pragma once

namespace bolt
{
	namespace btbb
	{


	  template <typename InputIterator1, typename InputIterator2, typename OutputIterator,
				 typename BinaryFunction, typename BinaryPredicate,typename T>
	  struct ScanKey_tbb{
		  typedef typename std::iterator_traits< OutputIterator >::value_type oType;
		  oType sum;
		  oType start;
		  InputIterator1& first_key;
		  InputIterator2& first_value;
		  OutputIterator& result;
      unsigned int numElements;
		  const BinaryFunction binary_op;
		  const BinaryPredicate binary_pred;
		  const bool inclusive;
		  bool flag, pre_flag, next_flag;
		  public:
		  ScanKey_tbb() : sum(0) {}
		  ScanKey_tbb( InputIterator1&  _first,
			InputIterator2& first_val,
			OutputIterator& _result,
      unsigned int _numElements,
			const BinaryFunction &_opr,
			const BinaryPredicate &_pred,
			const bool& _incl,
			const oType &init) : first_key(_first), first_value(first_val), result(_result), numElements(_numElements), binary_op(_opr), binary_pred(_pred),
							 inclusive(_incl), start(init), flag(FALSE), pre_flag(TRUE),next_flag(FALSE){}
		  oType get_sum() const {return sum;}
		  template<typename Tag>
		  void operator()( const tbb::blocked_range<unsigned int>& r, Tag ) {
			  oType temp = sum, temp1;
			  next_flag = flag = FALSE;
        unsigned int i;
			  for( i=r.begin(); i<r.end(); ++i ) {
				 if( Tag::is_final_scan() ) {
					 if(!inclusive){
						  if( i==0){
                temp1 = *(first_value + i);
							 *(result + i) = start;
							 temp = binary_op(start, temp1);
						  }
						  else if(binary_pred(*(first_key+i), *(first_key +i- 1))){
                temp1 = *(first_value + i);
							 *(result + i) = temp;
							 temp = binary_op(temp, temp1);
						  }
						  else{
                temp1 = *(first_value + i);
							 *(result + i) = start;
							 temp = binary_op(start, temp1);
                flag = TRUE; 
						  }
						  continue;
					 }
					 else if(i == 0 ){
						temp = *(first_value+i);
					 }
					 else if(binary_pred(*(first_key+i), *(first_key +i- 1))) {
						temp = binary_op(temp, *(first_value+i));
					 }
					 else{
						temp = *(first_value+i);
             flag = TRUE; 
					 }
					 *(result + i) = temp;
				 }
				 else if(pre_flag){
				   temp = *(first_value+i);
				   pre_flag = FALSE;
				 }
				 else if(binary_pred(*(first_key+i), *(first_key +i - 1)))
					 temp = binary_op(temp, *(first_value+i));
				 else if (!inclusive){
					 temp = binary_op(start, *(first_value+i));
            flag = TRUE; 
				 }
				 else {
					 temp = *(first_value+i);
            flag = TRUE; 
				 }
			 }
       if(i<numElements && !binary_pred(*(first_key+i-1), *(first_key +i ))){
          next_flag = TRUE;     // this will check the key change at boundaries
       }
			 sum = temp;
		  }
		  ScanKey_tbb( ScanKey_tbb& b, tbb::split):first_key(b.first_key),result(b.result),first_value(b.first_value),
												   numElements(b.numElements),inclusive(b.inclusive),start(b.start),pre_flag(TRUE){}
		  void reverse_join( ScanKey_tbb& a ) {
			if(!a.next_flag && !flag)
				sum = binary_op(a.sum,sum);
		  }
		  void assign( ScanKey_tbb& b ) {
			 sum = b.sum;
		  }
   };

template<typename T>
struct equal_to
{
	bool operator()(const T &lhs, const T &rhs) const  {return lhs == rhs;}
};

template<typename T>
struct plus
{
	T operator()(const T &lhs, const T &rhs) const {return lhs + rhs;}
};

template<
	typename InputIterator1,
	typename InputIterator2,
	typename OutputIterator>
OutputIterator
inclusive_scan_by_key(
	InputIterator1  first1,
	InputIterator1  last1,
	InputIterator2  first2,
	OutputIterator  result)
	{
		unsigned int numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );
		typedef typename std::iterator_traits< InputIterator2 >::value_type vType;
		typedef std::iterator_traits<InputIterator1>::value_type kType;
		typedef std::iterator_traits<OutputIterator>::value_type oType;
		equal_to<kType> binary_pred;
		plus<oType> binary_funct;

		tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
		ScanKey_tbb<InputIterator1, InputIterator2, OutputIterator, plus<oType>, equal_to<kType>,vType> tbbkey_scan((InputIterator1 &)first1,
			(InputIterator2&) first2,(OutputIterator &)result, numElements, binary_funct, binary_pred, true, vType());
		tbb::parallel_scan( tbb::blocked_range<unsigned int>(  0, static_cast< unsigned int >( std::distance( first1, last1 ))), tbbkey_scan, tbb::auto_partitioner());
		return result + numElements;


	}

template<
	typename InputIterator1,
	typename InputIterator2,
	typename OutputIterator,
	typename BinaryPredicate>
OutputIterator
inclusive_scan_by_key(
	InputIterator1  first1,
	InputIterator1  last1,
	InputIterator2  first2,
	OutputIterator  result,
	BinaryPredicate binary_pred)
	{
		unsigned int numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );
		typedef typename std::iterator_traits< InputIterator2 >::value_type vType;
		typedef std::iterator_traits<OutputIterator>::value_type oType;
		plus<oType> binary_funct;

		tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
		ScanKey_tbb<InputIterator1, InputIterator2, OutputIterator, plus<oType>, BinaryPredicate,vType> tbbkey_scan((InputIterator1 &)first1,
			(InputIterator2&) first2,(OutputIterator &)result, numElements, binary_funct, binary_pred, true, vType());
		tbb::parallel_scan( tbb::blocked_range<unsigned int>(  0, static_cast< unsigned int >( std::distance( first1, last1 ))), tbbkey_scan, tbb::auto_partitioner());
		return result + numElements;

	}





template<
	typename InputIterator1,
	typename InputIterator2,
	typename OutputIterator,
	typename BinaryPredicate,
	typename BinaryFunction>
OutputIterator
inclusive_scan_by_key(
	InputIterator1  first1,
	InputIterator1  last1,
	InputIterator2  first2,
	OutputIterator  result,
	BinaryPredicate binary_pred,
	BinaryFunction  binary_funct)
	{
		unsigned int numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );
		typedef typename std::iterator_traits< InputIterator2 >::value_type vType;

		tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
		ScanKey_tbb<InputIterator1, InputIterator2, OutputIterator, BinaryFunction, BinaryPredicate,vType> tbbkey_scan((InputIterator1 &)first1,
			(InputIterator2&) first2,(OutputIterator &)result, numElements, binary_funct, binary_pred, true, vType());
		tbb::parallel_scan( tbb::blocked_range<unsigned int>(  0, static_cast< unsigned int >( std::distance( first1, last1 ))), tbbkey_scan, tbb::auto_partitioner());
		return result + numElements;

	}



template<
	typename InputIterator1,
	typename InputIterator2,
	typename OutputIterator>
OutputIterator
exclusive_scan_by_key(
	InputIterator1  first1,
	InputIterator1  last1,
	InputIterator2  first2,
	OutputIterator  result)
	{
		unsigned int numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );
		typedef typename std::iterator_traits< InputIterator2 >::value_type vType;
		typedef std::iterator_traits<InputIterator1>::value_type kType;
		typedef std::iterator_traits<OutputIterator>::value_type oType;
		equal_to<kType> binary_pred;
		plus<oType> binary_funct;

		tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
		ScanKey_tbb<InputIterator1, InputIterator2, OutputIterator, plus<oType>, equal_to<kType>,vType> tbbkey_scan((InputIterator1 &)first1,
			(InputIterator2&) first2,(OutputIterator &)result, numElements, binary_funct, binary_pred, false, vType());
		tbb::parallel_scan( tbb::blocked_range<unsigned int>(  0, static_cast< unsigned int >( std::distance( first1, last1 ))), tbbkey_scan, tbb::auto_partitioner());
		return result + numElements;

	}



template<
	typename InputIterator1,
	typename InputIterator2,
	typename OutputIterator,
	typename T>
OutputIterator
exclusive_scan_by_key(
	InputIterator1  first1,
	InputIterator1  last1,
	InputIterator2  first2,
	OutputIterator  result,
	T               init)
	{
		unsigned int numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );
		typedef std::iterator_traits<InputIterator1>::value_type kType;
		typedef std::iterator_traits<OutputIterator>::value_type oType;
		equal_to<kType> binary_pred;
		plus<oType> binary_funct;

		tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
		ScanKey_tbb<InputIterator1, InputIterator2, OutputIterator, BinaryFunction, BinaryPredicate,T> tbbkey_scan((InputIterator1 &)first1,
			(InputIterator2&) first2,(OutputIterator &)result, numElements, plus<oType> , equal_to<kType>, false, init);
		tbb::parallel_scan( tbb::blocked_range<unsigned int>(  0, static_cast< unsigned int >( std::distance( first1, last1 ))), tbbkey_scan, tbb::auto_partitioner());
		return result + numElements;
	}




template<
	typename InputIterator1,
	typename InputIterator2,
	typename OutputIterator,
	typename T,
	typename BinaryPredicate>
OutputIterator
exclusive_scan_by_key(
	InputIterator1  first1,
	InputIterator1  last1,
	InputIterator2  first2,
	OutputIterator  result,
	T               init,
	BinaryPredicate binary_pred)
	{
		unsigned int numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );
		typedef std::iterator_traits<OutputIterator>::value_type oType;
		plus<oType> binary_funct;

		tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
		ScanKey_tbb<InputIterator1, InputIterator2, OutputIterator, plus<oType>, BinaryPredicate,T> tbbkey_scan((InputIterator1 &)first1,
			(InputIterator2&) first2,(OutputIterator &)result, numElements, binary_funct, binary_pred, false, init);
		tbb::parallel_scan( tbb::blocked_range<unsigned int>(  0, static_cast< unsigned int >( std::distance( first1, last1 ))), tbbkey_scan, tbb::auto_partitioner());
		return result + numElements;
	}



template<
	typename InputIterator1,
	typename InputIterator2,
	typename OutputIterator,
	typename T,
	typename BinaryPredicate,
	typename BinaryFunction>
OutputIterator
exclusive_scan_by_key(
	InputIterator1  first1,
	InputIterator1  last1,
	InputIterator2  first2,
	OutputIterator  result,
	T               init,
	BinaryPredicate binary_pred,
	BinaryFunction  binary_funct)
	{
		unsigned int numElements = static_cast< unsigned int >( std::distance( first1, last1 ) );

		tbb::task_scheduler_init initialize(tbb::task_scheduler_init::automatic);
		ScanKey_tbb<InputIterator1, InputIterator2, OutputIterator, BinaryFunction, BinaryPredicate,T> tbbkey_scan((InputIterator1 &)first1,
			(InputIterator2&) first2,(OutputIterator &)result, numElements, binary_funct, binary_pred, false, init);
		tbb::parallel_scan( tbb::blocked_range<unsigned int>(  0, static_cast< unsigned int >( std::distance( first1, last1 ))), tbbkey_scan, tbb::auto_partitioner());
		return result + numElements;

	}

	}
}


#endif
