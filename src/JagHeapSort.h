#ifndef _jag_heapsort_h_
#define _jag_heapsort_h_

#define hp_get_parent( i, parent) \
    if ( i == 0 ) parent= DNULLKEY; else parent = (i-1)/2; 

#define hp_get_left(len, i , left ) \
    if ( 2*i+1 < len ) left=2*i+1; else left=DNULLKEY;

#define hp_get_right(len, i , right ) \
    if ( 2*i+2 < len ) right=2*i+2; else right=DNULLKEY;



////////// JagHeapData
template < class PairType>
class JagHeapData
{
    public: 
        JagHeapData&  operator= ( const JagHeapData &d2 ) {
            keyval = d2.keyval;
            col = d2.col;
            return *this;
        }

        int ge (const JagHeapData<PairType>  &d2 ) const { 
            return ( keyval.ge( d2.keyval ) );
        }       

        int gt (const JagHeapData<PairType>  &d2 ) const { 
            return ( keyval.gt( d2.keyval  ) );
        }       

        int le (const JagHeapData<PairType>  &d2 ) const { 
            return ( keyval.le ( d2.keyval ) );
        }       

        int lt (const JagHeapData<PairType>  &d2 ) const { 
            return ( keyval.lt( d2.keyval ) );
        }       

		void pointKey( const PairType &pair ) {
			keyval = pair;
		}

    public: 
        PairType      keyval; 
        int           col;    
};


///////////// JagHeapContext
class JagHeapContext
{
	public:
		JagHeapContext() 
		{
			reset();
		}

		void reset()
		{
  			fronti = 0;
   			mincol = -1;
   			prevx = -1;
   			prevy = -1;
   			first = 1;
			heap_len = 0;
		}

  		int fronti;
   		int mincol;
   		int prevx;
   		int prevy;
   		int first; 
		int heap_len;

};


////////// JagHeapSort
template <class PairType>
class JagHeapSort
{
	public:
		int  	_heapPopMin( JagHeapData<PairType> A[], bool moveLast = true );
		int  	_heapBuildMinHeap( JagHeapData<PairType> A[], int hplen );
		int 	_heapMinHeapify( JagHeapData<PairType> a[], int i ) ;
		int 	_heapInsertKeyMin( JagHeapData<PairType> A[],  const JagHeapData<PairType> &key);
		void 	_heapInsertKeyMinAtTop( JagHeapData<PairType> A[],  const JagHeapData<PairType> &key);

		int  	_heapPopMax( JagHeapData<PairType> A[], bool moveLasr = true );
		int  	_heapBuildMaxHeap( JagHeapData<PairType> A[], int hplen );
		int 	_heapMaxHeapify( JagHeapData<PairType> a[], int i ) ;
		int 	_heapInsertKeyMax( JagHeapData<PairType> A[],  const JagHeapData<PairType> &key);
		int 	_heapInsertKeyMaxAtTop( JagHeapData<PairType> A[],  const JagHeapData<PairType> &key);

		bool    _decreaseKeyMin( JagHeapData<PairType> A[], const PairType &value,  int i );
		bool    _increaseKeyMax( JagHeapData<PairType> A[], const PairType &value,  int i );


		int 	_printHeap( JagHeapData<PairType> A[] );

		void    resetContext() { _heapContext.reset(); }


	public:
		JagHeapContext  _heapContext;

};


// bubble down i-th element
template <class PairType>
int JagHeapSort<PairType>::_heapMinHeapify( JagHeapData<PairType> a[], int i ) 
{
	int  smallest = -1;
	JagHeapData<PairType> t;
	int l, r;

	// printf("c4020 start heapify i=%d\n", i );
	while ( 1 )
	{
        hp_get_left( this->_heapContext.heap_len, i, l);
        hp_get_right( this->_heapContext.heap_len, i, r);
    
        if ( l != DNULLKEY && l < this->_heapContext.heap_len && a[l].lt( a[i] ) )
            smallest = l;
        else smallest = i;
    
        if ( r != DNULLKEY && r < this->_heapContext.heap_len  &&  a[r].lt( a[smallest] ) )
        {
            smallest = r;
        }
		// printf("c4021 heapify i=%d small %d\n", i, smallest );
    
    	// i  300
    	// smalles 120   smallest 220
        if ( smallest != i )
        {
            // swap a[i] and a[smallest]
    		t = a[i];
    		a[i] = a[smallest]; 
    		a[smallest] = t;

			i = smallest;
			// printf("c4021 heapify i = small %d\n", i );
    
    		// _heapMinHeapify(a, smallest );
        }
		else {
			break;
		}
	}
    
    return 1;
}


   
template <class PairType>
int  JagHeapSort<PairType>::_heapBuildMinHeap( JagHeapData<PairType> A[], int hplen )
{
  	for ( int i= (hplen-1)/2; i>=0; --i )
	{
      	_heapMinHeapify(A, i );
	}

	return 1;
}




template <class PairType>
int JagHeapSort<PairType>::_heapPopMin( JagHeapData<PairType> A[], bool moveLast )
{
	if ( this->_heapContext.heap_len<1) {
		return -1;
	}

	int min = A[0].col;
	// abaxcout << " 1 inpopmin: A[0] = " << min << " is returned" << abaxendl;

	////////// do not do this yet, let insertmin add to A[0]
	if ( moveLast ) {
		A[0] = A[ this->_heapContext.heap_len -1 ];
		this->_heapContext.heap_len --;
		_heapMinHeapify( A, 0 ) ;
	}

	// this->_count ++;

	// abaxcout << " 2 inpopmin: A[0] = " << min.keyval << " is returned" << abaxendl;

	return min;

}

template <class PairType>
int JagHeapSort<PairType>::_heapInsertKeyMin( JagHeapData<PairType> A[],  const JagHeapData<PairType> &key)
{
	JagHeapData<PairType>  t;
	int 	i, parent;

    this->_heapContext.heap_len = this->_heapContext.heap_len +1;
	A[ this->_heapContext.heap_len-1] = key;

	i = this->_heapContext.heap_len-1;
	hp_get_parent(i, parent );
	if ( parent == DNULLKEY ) {
		return 0;
	}

	if ( A[i].ge( A[parent] ) ) {
		return 0;
	} 

	// bubble up
	while (i>0 && A[i].lt( A[parent] ) )
	{
	    // swap A[i] and A[parent(i)]
		t = A[i];
		A[i] = A[parent];
		A[parent] = t;

		i = parent;
		hp_get_parent(i, parent);
		if ( parent == DNULLKEY ) {
		    break;
		}
	}

	return 1;
}

template <class PairType>
bool JagHeapSort<PairType>::_decreaseKeyMin( JagHeapData<PairType> A[], const PairType &value,  int i )
{
	JagHeapData<PairType>  t;
	JagHeapData<PairType>  smallerValue;
	int 	parent;

	smallerValue.kyeval = value;

	if ( A[i].le( smallerValue )  ) {
		return 0;
	}

	hp_get_parent(i, parent );
	if ( parent == DNULLKEY ) {
		return 0;
	}

	A[i] = smallerValue;
	if ( A[i].ge( A[parent] ) ) {
		return 1;
	} 

	// bubble up
	while (i>0 && A[i].lt( A[parent] ) )
	{
	    // swap A[i] and A[parent(i)]
		t = A[i];
		A[i] = A[parent];
		A[parent] = t;

		i = parent;
		hp_get_parent(i, parent);
		if ( parent == DNULLKEY ) {
		    break;
		}
	}

	return 1;
}



template <class PairType>
void JagHeapSort<PairType>::_heapInsertKeyMinAtTop( JagHeapData<PairType> A[],  const JagHeapData<PairType> &key)
{
	A[0] = key;
	_heapMinHeapify( A, 0 ) ;
}



// bubble down i-th element
template <class PairType>
int JagHeapSort<PairType>::_heapMaxHeapify( JagHeapData<PairType> a[], int i ) 
{
	int  big;
	JagHeapData<PairType> t;
	int l, r;

	while ( 1 )
	{
        hp_get_left( this->_heapContext.heap_len, i, l);
        hp_get_right( this->_heapContext.heap_len, i, r);
    
        if ( l != DNULLKEY && l < this->_heapContext.heap_len && a[l].gt( a[i] ) )
    	{
            big = l;
    	}
        else 
    	{
    		big = i;
    	}
    
        if ( r != DNULLKEY && r < this->_heapContext.heap_len  &&  a[r].gt( a[big] ) )
        {
            big = r;
        }
    
    	// i: 100
    	// big: 400    big: 320
        if ( big != i )
        {
            // swap a[i] and a[big]
    		t = a[i];
    		a[i] = a[big]; 
    		a[big] = t;

			i = big;
    
    		// _heapMaxHeapify(a, big );
        }
		else
		{
			break;
		}
	}

    return 1;
}



template <class PairType>
int  JagHeapSort<PairType>::_heapBuildMaxHeap( JagHeapData<PairType> A[], int hplen )
{
  	for ( int i= (hplen-1)/2; i>=0; i--)
	{
      	_heapMaxHeapify(A, i );
	}

	return 1;
}


template <class PairType>
int JagHeapSort<PairType>::_heapInsertKeyMax( JagHeapData<PairType> A[],  const JagHeapData<PairType> &key)
{
	JagHeapData<PairType>  t;
	int 	i, parent;

    this->_heapContext.heap_len = this->_heapContext.heap_len +1;
	A[ this->_heapContext.heap_len-1] = key;

	i = this->_heapContext.heap_len-1;
	hp_get_parent(i, parent );
	if ( parent == DNULLKEY ) {
		return 0;
	}

	// parent
	// i
	if ( A[i].le( A[parent] ) ) {
		return 0;
	} 

	// bubble up
	while (i>0 && A[i].gt( A[parent] ) )
	{
	    // swap A[i] and A[parent(i)]
		t = A[i];
		A[i] = A[parent];
		A[parent] = t;

		i = parent;

		hp_get_parent(i, parent);
		if ( parent == DNULLKEY ) {
		    break;
		}
	}

	return 1;
}

template <class PairType>
bool JagHeapSort<PairType>::_increaseKeyMax( JagHeapData<PairType> A[], const PairType &value,  int i )
{
	JagHeapData<PairType>  t;
	JagHeapData<PairType>  greaterValue;
	int 	parent;

	greaterValue.keyval = value;

	if ( A[i].ge( greaterValue)  ) {
		return 0;
	}

	hp_get_parent(i, parent );
	if ( parent == DNULLKEY ) {
		return 0;
	}

	A[i] = greaterValue;
	if ( A[i].le( A[parent] ) ) {
		return 1;
	} 

	// bubble up
	while (i>0 && A[i].gt( A[parent] ) )
	{
	    // swap A[i] and A[parent(i)]
		t = A[i];
		A[i] = A[parent];
		A[parent] = t;

		i = parent;
		hp_get_parent(i, parent);
		if ( parent == DNULLKEY ) {
		    break;
		}
	}

	return 1;
}

template <class PairType>
int JagHeapSort<PairType>::_heapInsertKeyMaxAtTop( JagHeapData<PairType> A[],  const JagHeapData<PairType> &key)
{
	A[0] = key; 
	_heapMaxHeapify( A, 0 ) ;
	return 1;
}

template <class PairType>
int JagHeapSort<PairType>::_heapPopMax( JagHeapData<PairType> A[], bool moveLast )
{
	if ( this->_heapContext.heap_len<1) {
		return -1;
	}

	int max = A[0].col;

	// abaxcout << " 1 inpopmax: A[0] = " << min.keyval << " is returned" << abaxendl;

	if ( moveLast ) {
		A[0] = A[ this->_heapContext.heap_len -1 ];
		this->_heapContext.heap_len --;

		// push A[0] down if it is small
		_heapMaxHeapify( A, 0 ) ;
	}

	// this->_count ++;

	// abaxcout << " 2 inpopmax: A[0] = " << min.keyval << " is returned" << abaxendl;

	return max;
}

template <class PairType>
int JagHeapSort<PairType>::_printHeap( JagHeapData<PairType> A[] )
{
    int i =0;
    // printf("heap: ");
    for ( i = 0; i < _heapContext.heap_len; i ++)
    {
        // printf("%d:%d(%d) ", i, A[i].data, A[i].col);
        // abaxcout << i << ":" << A[i].keyval << "(" << A[i].col << ") ";
    }
    // abaxcout << abaxendl;
	return 0;
}


#endif

